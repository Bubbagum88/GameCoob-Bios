//! audio-render: run the app's IPL sound code (src/audio) on the host.
//!
//! Usage: audio-render <ipl.bin> <out-dir> [id...]
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
    let ids: Vec<u32> = if args.len() > 3 { args[3..].iter().map(|a| a.parse().expect("id")).collect() } else { vec![0, 1, 2] };
    let out = std::path::Path::new(&args[2]);
    std::fs::create_dir_all(out).expect("out dir");
    let mut eng = Engine::new(arc, RATE);
    for id in ids {
        eng.reset();
        eng.sound_play(id);
        let mut pcm = Vec::new();
        let mut block = vec![0i16; 2 * 1024];
        // Until every voice ends (or 20 s for loops).
        while eng.mixer.active() > 0 && pcm.len() < 2 * 20 * RATE as usize {
            eng.render(&mut block);
            pcm.extend_from_slice(&block);
        }
        let p = out.join(format!("sound_{id}.wav"));
        wav(&p, &pcm).expect("write wav");
        let peak = pcm.iter().map(|s| s.unsigned_abs()).max().unwrap_or(0);
        println!("{}: {:.2} s, peak {peak}", p.display(), pcm.len() as f32 / 2.0 / RATE as f32);
    }
}
