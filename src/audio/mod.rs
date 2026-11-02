//! IPL sound, rebuilt from the JAudio decomp (`decomp/src/jaudio`, map in
//! `docs/AUDIO.md`). Everything here is plain computation, so the host tool
//! `tools/audio-render` runs the same code; the sound card driver is
//! `crate::hda`.
//!
//! BS2 makes every sound through a few calls (`Sound_Play`, the spin and
//! ambient helpers, `jaudio/ipl_8135A8E0.c`) that write ports of the
//! always-running `iplrom.com` sequence. The engine runs that sequence
//! ([`jam`]), which plays the bank's instruments ([`bank`]) on voices
//! ([`voice`]) that the [`mixer`] mixes like the DSP does. All of it ticks
//! once per DSP subframe (80 samples at 32028.5 Hz, about 400 Hz), as on
//! the console.

pub mod bank;
pub mod data;
mod jam;
pub mod mixer;
#[cfg(test)]
mod tests;
pub mod voice;

use alloc::collections::VecDeque;
use alloc::vec::Vec;
use bank::Bank;
use data::SoundArchive;
use jam::{POOL, Track};
use voice::{Ctl, DAC_RATE, OscBuf, SUBFRAME, Tables, Voice};

/// DSP channels (`DSPCH`): voices beyond this steal the lowest priority one.
const DSP_CHANNELS: usize = 64;

/// Sound ids of `Sound_Play` (`decomp/include/bs2/menu.h`).
pub mod id {
    pub const FACE_SELECT: u32 = 3;
    pub const FACE_BACK: u32 = 4;
    pub const ENTER: u32 = 5;
    pub const EXIT: u32 = 6;
    pub const CANCEL: u32 = 7;
    pub const DECIDE: u32 = 8;
    pub const CHANGE: u32 = 9;
    pub const CURSOR_H: u32 = 10;
    pub const CURSOR: u32 = 11;
    pub const CONFIRM: u32 = 12;
    pub const BUZZER: u32 = 13;
    pub const GAME_START: u32 = 22;
    pub const SPIN_CYCLE: u32 = 23;
}

/// Plays the IPL's sounds into a stereo stream.
pub struct Engine {
    pub archive: SoundArchive,
    bank: Bank,
    tabs: Tables,
    tracks: Vec<Track>,
    free: VecDeque<usize>,
    registry: Vec<(u32, usize)>,
    voices: Vec<Option<Voice>>,
    next_id: u16,
    rng: (i32, i32),
    /// `JAC_SYSTEM_OUTPUT_MODE`: stereo (1) or mono (0).
    stereo: bool,
    /// `Sound_Play`'s spin counter for id 23.
    spin: u32,
    rate: u32,
    /// Output frames left in the current subframe, and its length.
    left: usize,
    len: usize,
    acc: f32,
    fx: [mixer::FxLine; 2],
    dry: Vec<[f32; 2]>,
    send: Vec<[f32; 2]>,
    /// Overall gain before the limiter.
    pub master: f32,
}

impl Engine {
    pub fn new(archive: SoundArchive, rate: u32) -> Engine {
        let bank = Bank::parse(&archive.bank).unwrap_or(Bank { inst: Vec::new() });
        let mut tracks = Vec::with_capacity(POOL + 1);
        for _ in 0..=POOL {
            tracks.push(Track::new());
        }
        let mut e = Engine {
            archive,
            bank,
            tabs: Tables::new(),
            tracks,
            free: (1..=POOL).collect(),
            registry: Vec::new(),
            voices: Vec::new(),
            next_id: 0,
            rng: (0x1000, 0x5555),
            stereo: true,
            spin: 0,
            rate,
            left: 0,
            len: 1,
            acc: 0.0,
            fx: mixer::fx_lines(rate),
            dry: Vec::new(),
            send: Vec::new(),
            master: 1.0,
        };
        e.start();
        e
    }

    /// Start the sequence and let it set its tracks up, as the IPL does
    /// long before BS2 first calls `Sound_Play`.
    fn start(&mut self) {
        self.start_root();
        for _ in 0..4 {
            self.tick();
        }
    }

    // ---- what BS2 calls (ipl_8135A8E0.c) ----

    /// `Sound_Play(id)`: 0..2 the boot jingle and its two Z easter eggs,
    /// 3..22 menu sounds, 23 the spin-out's cycling whoosh.
    pub fn sound_play(&mut self, id: u32) {
        if id < 3 {
            let t = self.track_by_id(0x21002);
            self.write_port(t, 0, id as u16);
            self.spin = 0;
        } else if id == 23 {
            self.spin += 1;
            if self.spin > 20 {
                self.spin = 0;
            }
            if self.spin > 10 {
                self.update_spin(1, (21 - self.spin) as f32 / 10.0);
            } else {
                self.update_spin(0, self.spin as f32 / 10.0);
            }
        } else if id < 23 {
            let t = self.track_by_id(0x20002);
            self.write_port(t, 0, id as u16);
        } else {
            let t = self.track_by_id(0x20001);
            self.write_port(t, 0, (id - 5) as u16);
        }
    }

    /// `Sound_UpdateSpin(mode, speed)`: holding A spins the cube; the boot
    /// jingle fades as the spin speeds up and each fast turn whooshes.
    /// Mode 1 is the spin-out's far side, where the jingle is silenced.
    pub fn update_spin(&mut self, mode: u8, speed: f32) {
        let s = libm::sqrtf(speed.min(1.0));
        let vol = if mode == 1 { 0.0 } else { 32767.0 - 32767.0 * s };
        let t = self.track_by_id(0x21002);
        self.write_port(t, 1, vol as u16);
        if s != 0.0 {
            let (a, b) = if mode == 1 {
                (2048.0 * (3.0 * (0.15 + s)), 2048.0 * (3.6 * (0.15 + s)))
            } else {
                (2048.0 * (1.6 * (0.1 + s)), 2048.0 * (0.1 + s))
            };
            let pitch = (10.0 / s).min(100.0);
            let t = self.track_by_id(0x21003);
            self.write_port(t, 3, a as u16);
            self.write_port(t, 2, b as u16);
            if s > 0.6 {
                self.write_port(t, 0, pitch as u8 as u16);
            }
        }
    }

    /// `Sound_StartAmbient(id)`: the menu's background loop.
    pub fn start_ambient(&mut self, id: u32) {
        if id == 0 {
            let t = self.track_by_id(0x21001);
            self.write_port(t, 0, 1);
        }
    }

    /// `Sound_StopAmbient`: fade the background loop out.
    pub fn stop_ambient(&mut self) {
        let t = self.track_by_id(0x21001);
        self.write_port(t, 1, 0);
    }

    /// `Sound_SetOutputMode`: 0 mono, otherwise stereo.
    pub fn set_output_mode(&mut self, mode: u32) {
        self.stereo = mode != 0;
    }

    /// `fn_8135B180`, gInit's reset: stop the sequence, let every voice
    /// die out fast, and start the sequence again.
    pub fn reset(&mut self) {
        self.close_track(0);
        self.registry.clear();
        for v in self.voices.iter_mut().flatten() {
            v.mgr = None;
            v.follow = false;
            for b in v.buf.iter_mut() {
                if b.state != 6 && b.state != 7 && b.state != 0 {
                    b.state = 6;
                }
            }
        }
        self.spin = 0;
        self.start();
    }

    /// Voices still sounding.
    pub fn active(&self) -> usize {
        self.voices.iter().flatten().count()
    }

    // ---- once per subframe (UpdateDSP) ----

    /// `UpdateDSP`: the sequence, then every voice's update.
    fn tick(&mut self) {
        self.root_callback();
        self.update_voices();
    }

    /// `UpdateDSPchannelAll` with `CommonCallbackLogicalChannel`.
    fn update_voices(&mut self) {
        for s in 0..self.voices.len() {
            let Some(v) = self.voices[s].as_mut() else { continue };
            v.lv0 = v.lv1;
            if v.fading {
                // A stolen channel: the DSP halves it until it is silent.
                v.lv1 = v.lv1.map(|l| libm::floorf(l / 2.0));
                if v.lv0.iter().all(|&l| l == 0.0) {
                    self.voices[s] = None;
                }
                continue;
            }
            if v.done {
                self.voices[s] = None;
                continue;
            }
            v.age += 1;
            v.pitch_mod = 1.0;
            v.vol_mod = 1.0;
            v.pan[1][1] = 0.5;
            v.pan[2][1] = 0.0;
            v.pan[3][1] = 0.0;
            let mut active = 0;
            let mut stop = false;
            for k in 0..2 {
                let Some(o) = v.osc[k].as_ref() else { continue };
                let x = voice::osc_to_ofs(o, &mut v.buf[k], &self.tabs.force_release);
                let mode = o.mode;
                v.do_effect_osc(mode, x);
                if v.buf[k].state == 0 {
                    stop = true;
                    break;
                }
                active += 1;
            }
            if stop {
                self.voices[s] = None;
                continue;
            }
            if active != 0 {
                let ctl = v.mgr.map(|m| self.tracks[m].ctl);
                v.update_effecter(ctl.as_ref(), self.stereo);
            }
            if v.sweep != 0 {
                v.sweep_step();
                let ctl = v.mgr.map(|m| self.tracks[m].ctl);
                v.update_effecter(ctl.as_ref(), self.stereo);
            }
            if v.countdown > 0 {
                v.countdown -= 1;
            }
            if v.countdown == 0 {
                self.release_voice(s);
                if let Some(v) = self.voices[s].as_mut() {
                    v.countdown = v.note_id;
                }
            }
        }
    }

    /// `Jesus1Shot_Update(jc, 0)`: start the release envelopes.
    fn release_voice(&mut self, s: usize) {
        let Some(v) = self.voices[s].as_mut() else { return };
        let mut any = false;
        for k in 0..2 {
            if v.osc[k].is_some() && v.buf[k].state != 6 && v.buf[k].state != 7 {
                v.buf[k].state = 4;
                any = true;
            }
        }
        if any {
            v.releasing = true;
            let p = (v.chan_prio >> 8) as u8;
            v.prio = if p == 0 { 1 } else { p };
        }
        v.note_id = -1;
    }

    /// `FlushRelease_1Shot`: cut the releasing voices of a track short.
    fn flush_release(&mut self, t: usize) {
        for v in self.voices.iter_mut().flatten() {
            if v.mgr == Some(t) && v.releasing {
                for k in 0..2 {
                    if v.osc[k].is_some() && v.buf[k].state != 6 && v.buf[k].state != 7 {
                        v.buf[k].state = 6;
                    }
                }
            }
        }
    }

    /// A free DSP channel, stealing the lowest priority, oldest voice when
    /// all are taken (`GetLowerDSPchannel` / `BreakLowerDSPchannel`).
    fn alloc_slot(&mut self, prio: u8) -> Option<usize> {
        let playing = self.voices.iter().flatten().filter(|v| !v.fading).count();
        if playing >= DSP_CHANNELS {
            let mut best: Option<(usize, u8, u32)> = None;
            for (i, v) in self.voices.iter().enumerate() {
                let Some(v) = v else { continue };
                if v.fading {
                    continue;
                }
                if best.is_none_or(|(_, p, a)| v.prio < p || (v.prio == p && v.age > a)) {
                    best = Some((i, v.prio, v.age));
                }
            }
            let (i, p, _) = best?;
            if p > prio {
                return None;
            }
            let v = self.voices[i].as_mut().unwrap();
            v.fading = true;
            v.note = 0xFF;
            v.id = 0;
        }
        match self.voices.iter().position(|v| v.is_none()) {
            Some(i) => Some(i),
            None => {
                self.voices.push(None);
                Some(self.voices.len() - 1)
            }
        }
    }

    /// `Play_1shot`: start `key` on instrument `prog` in track `t`'s
    /// manager, for `dur` subframes (-1: until note off).
    fn play_1shot(&mut self, t: usize, prog: usize, key: u8, vel: u8, dur: i32) -> Option<usize> {
        let (inst, vm) = self.bank.vmap(prog, key, vel)?;
        let wi = self.archive.wave_by_id(vm.wave)?;
        let w = &self.archive.waves[wi];
        let ctl: Ctl = self.tracks[t].ctl;
        let x = vel as f32 / 127.0;
        let base_pitch = vm.pitch * (w.rate / DAC_RATE) * inst.pitch;
        let base_vol = vm.vol * inst.vol;
        let osc = [inst.osc[0].clone(), inst.osc[1].clone()];
        let mut v = Voice {
            id: 0,
            mgr: Some(t),
            follow: true,
            velocity: vel,
            note: key,
            note_id: if dur == 0 { -1 } else { dur },
            countdown: 0,
            releasing: false,
            osc: [None, None],
            buf: [OscBuf::default(); 2],
            base_pitch,
            base_vol,
            cur_pitch: base_pitch * jam::key_pitch(key as i32 + 60 - w.base_key as i32),
            cur_vol: x * x * base_vol,
            pitch_mod: 1.0,
            vol_mod: 1.0,
            pan: [[1.0 / 3.0; 3], [0.5, 0.5, 0.0], [0.0; 3], [0.0; 3]],
            pan_calc: ctl.pan_calc,
            mgr_pitch: 1.0,
            mgr_vol: 1.0,
            target_pitch: 0.0,
            sweep: 0,
            routing: ctl.bus,
            chan_prio: ctl.prio,
            wave: wi,
            pos: 0,
            ratio: 1.0,
            lv0: [0.0; 4],
            lv1: [0.0; 4],
            prio: ctl.prio as u8,
            age: 0,
            done: false,
            fading: false,
        };
        v.countdown = v.note_id;
        // EffecterInit: the instrument's envelopes start (no instrument in
        // the IPL's bank has random or sense effects).
        for (k, o) in osc.into_iter().enumerate() {
            if let Some(o) = o {
                v.buf[k].state = 1;
                let y = voice::osc_to_ofs(&o, &mut v.buf[k], &self.tabs.force_release);
                v.do_effect_osc(o.mode, y);
                v.osc[k] = Some(o);
            }
        }
        for i in 1..3 {
            v.pan[i][0] = v.pan[i][0].clamp(0.0, 1.0);
        }
        let slot = self.alloc_slot(v.prio)?;
        self.next_id = self.next_id.wrapping_add(1).max(1);
        v.id = self.next_id;
        // PlayLogicalChannel
        v.update_effecter(Some(&ctl), self.stereo);
        v.lv0 = v.lv1;
        self.voices[slot] = Some(v);
        Some(slot)
    }

    // ---- output ----

    /// Render the next `out.len() / 2` stereo frames (interleaved i16).
    pub fn render(&mut self, out: &mut [i16]) {
        let frames = out.len() / 2;
        let mut done = 0;
        while done < frames {
            if self.left == 0 {
                self.tick();
                self.acc += SUBFRAME * self.rate as f32 / DAC_RATE;
                let n = self.acc as usize;
                self.acc -= n as f32;
                self.left = n.max(1);
                self.len = self.left;
            }
            let n = self.left.min(frames - done);
            let from = self.len - self.left;
            self.mix(&mut out[done * 2..(done + n) * 2], from);
            self.left -= n;
            done += n;
        }
    }

    fn mix(&mut self, out: &mut [i16], from: usize) {
        let n = out.len() / 2;
        self.dry.clear();
        self.dry.resize(n, [0.0; 2]);
        self.send.clear();
        self.send.resize(n, [0.0; 2]);
        for v in self.voices.iter_mut().flatten() {
            if !v.done {
                mixer::mix_voice(v, &self.archive.waves[v.wave], self.rate, from, self.len, &mut self.dry, &mut self.send);
            }
        }
        mixer::mix_fx(&mut self.fx, &mut self.dry, &self.send);
        let g = self.master;
        for (o, s) in out.chunks_exact_mut(2).zip(&self.dry) {
            o[0] = mixer::to_i16(s[0] * g);
            o[1] = mixer::to_i16(s[1] * g);
        }
    }

    /// Silence everything at once, echoes included (for tests and tools).
    pub fn silence(&mut self) {
        self.voices.clear();
        mixer::clear_fx(&mut self.fx);
    }
}
