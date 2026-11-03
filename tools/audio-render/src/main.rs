//! audio-render: run the app's IPL sound code (src/audio) on the host.
//!
//! Usage: audio-render <ipl.bin> <out-dir> [id | ambient | hold | spinout | bank ...]
//!
//! Finds the sound archive in your own dump, prints its wave table and
//! writes `sound_<id>.wav` (48 kHz stereo) for each `Sound_Play` id given
//! (default: the three boot jingles 0, 1, 2). Nothing from the dump is
//! written anywhere else.

extern crate alloc;

#[path = "../../../src/ipl/mod.rs"]
#[allow(dead_code)]
mod ipl;

#[path = "../../../src/audio/mod.rs"]
#[allow(dead_code)]
mod audio;

use audio::{Engine, data::SoundArchive, mixer::RATE};
use std::io::Write;

fn wav(path: &std::path::Path, pcm: &[i16]) -> std::io::Result<()> {
    let mut f = std::io::BufWriter::new(std::fs::File::create(path)?);
    let data = (pcm.len() * 2) as u32;
    f.write_all(b"RIFF")?;
    f.write_all(&(36 + data).to_le_bytes())?;
    f.write_all(b"WAVEfmt ")?;
    f.write_all(&16u32.to_le_bytes())?;
    f.write_all(&1u16.to_le_bytes())?;
    f.write_all(&2u16.to_le_bytes())?;
    f.write_all(&RATE.to_le_bytes())?;
    f.write_all(&(RATE * 4).to_le_bytes())?;
    f.write_all(&4u16.to_le_bytes())?;
    f.write_all(&16u16.to_le_bytes())?;
    f.write_all(b"data")?;
    f.write_all(&data.to_le_bytes())?;
    for s in pcm {
        f.write_all(&s.to_le_bytes())?;
    }
    Ok(())
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 3 {
        eprintln!("usage: audio-render <ipl.bin> <out-dir> [id...]");
        std::process::exit(2);
    }
    let rom = std::fs::read(&args[1]).expect("read ipl.bin");
    let ipl = ipl::Ipl::parse(&rom).expect("not a usable IPL dump");
    let arc = SoundArchive::find(ipl.bs2()).expect("no sound archive in this dump");
    println!("sound archive at BS2 +0x{:X}: bank {} bytes, {} waves", arc.offset, arc.bank.len(), arc.waves.len());
    for (i, w) in arc.waves.iter().enumerate() {
        println!(
            "  wave {i:2}: {:5} Hz, key {:3}, {:6} samples ({:.2} s){}",
            w.rate,
            w.base_key,
            w.pcm.len(),
            w.pcm.len() as f32 / w.rate,
            w.looped.map_or(String::new(), |(s, e)| format!(", loop {s}..{e}"))
        );
    }
    if args.get(3).map(String::as_str) == Some("bank") {
        dump_bank(&arc);
        return;
    }
    let names: Vec<String> = if args.len() > 3 { args[3..].to_vec() } else { vec!["0".into(), "1".into(), "2".into()] };
    let out = std::path::Path::new(&args[2]);
    std::fs::create_dir_all(out).expect("out dir");
    let mut eng = Engine::new(arc, RATE);
    for name in names {
        eng.reset();
        eng.silence();
        let mut r = Rec { eng: &mut eng, pcm: Vec::new() };
        match name.as_str() {
            // The menu's background loop for a minute, then its fade out.
            "ambient" => {
                r.eng.start_ambient(0);
                r.secs(60.0);
                r.eng.stop_ambient();
                r.secs(4.0);
            }
            // Holding A through the jingle: the spin speeds up, the jingle
            // fades and each turn past the front whooshes (Scene_UpdateSpin
            // calls Sound_UpdateSpin(0, speed) once per turn).
            "hold" => {
                r.eng.sound_play(0);
                r.secs(1.0);
                for turn in 1..=10 {
                    r.eng.update_spin(0, turn as f32 / 10.0);
                    r.secs(0.6 - turn as f32 * 0.04);
                }
                r.until_quiet(10.0);
            }
            // The spin-out: Sound_Play(23) once per turn.
            "spinout" => {
                for _ in 0..21 {
                    r.eng.sound_play(23);
                    r.secs(0.15);
                }
                r.until_quiet(5.0);
            }
            id => {
                r.eng.sound_play(id.parse().expect("sound id or scenario"));
                r.until_quiet(20.0);
            }
        }
        let pcm = r.pcm;
        let p = out.join(format!("sound_{name}.wav"));
        wav(&p, &pcm).expect("write wav");
        let peak = pcm.iter().map(|s| s.unsigned_abs()).max().unwrap_or(0);
        println!("{}: {:.2} s, peak {peak}", p.display(), pcm.len() as f32 / 2.0 / RATE as f32);
    }
}

/// Renders the engine into a growing buffer.
struct Rec<'a> {
    eng: &'a mut Engine,
    pcm: Vec<i16>,
}

impl Rec<'_> {
    fn secs(&mut self, s: f32) {
        let n = (s * RATE as f32) as usize;
        let at = self.pcm.len();
        self.pcm.resize(at + 2 * n, 0);
        self.eng.render(&mut self.pcm[at..]);
    }

    /// Until no voice sounds (plus the echo's tail), at most `max` s.
    fn until_quiet(&mut self, max: f32) {
        let start = self.pcm.len();
        while self.pcm.len() - start < (2.0 * max * RATE as f32) as usize {
            self.secs(0.1);
            if self.eng.active() == 0 {
                self.secs(1.0);
                break;
            }
        }
    }
}

fn dump_bank(arc: &SoundArchive) {
    println!("wave ids: {:?}", arc.ids);
    let bank = audio::bank::Bank::parse(&arc.bank).expect("bank");
    let osc = |o: &Option<audio::bank::Osc>| match o {
        Some(o) => format!("mode {} rate {} w {} v {} atk {:?} rel {:?}", o.mode, o.rate, o.width, o.vertex, o.attack.as_deref(), o.release.as_deref()),
        None => "-".into(),
    };
    for (i, inst) in bank.inst.iter().enumerate() {
        let Some(inst) = inst else { continue };
        println!("inst {i}: flag {:x} vol {} pitch {}", inst.flag, inst.vol, inst.pitch);
        println!("  osc0 {}\n  osc1 {}", osc(&inst.osc[0]), osc(&inst.osc[1]));
        for r in inst.rand.iter().flatten() {
            println!("  rand id {} value {} range {}", r.id, r.value, r.range);
        }
        for s in inst.sense.iter().flatten() {
            println!("  sense id {} kind {} thr {} {}..{}", s.id, s.kind, s.threshold, s.min, s.max);
        }
        for k in &inst.keys {
            for v in &k.vmaps {
                println!("  key<={} vel<={} wave id {} (index {:?}) vol {} pitch {}", k.base_key, v.base_vel, v.wave, arc.wave_by_id(v.wave), v.vol, v.pitch);
            }
        }
    }
}
