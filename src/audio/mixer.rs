//! The software mixer that stands in for the GameCube DSP (`jdsp` microcode).
//!
//! The console mixes at 32028.5 Hz and its output then has to be resampled
//! again for a PC sound card. Here every voice is resampled once, straight
//! from its wave's own rate to the output rate (48 kHz by default), with
//! 4-point cubic interpolation, and mixed in f32, so nothing is lost twice
//! and nothing clips until the final soft limiter.

use super::data::Wave;
use alloc::vec::Vec;

/// Output sample rate the sound card is set to.
pub const RATE: u32 = 48_000;

/// Position fraction bits (32.32 fixed point sample positions).
const FRAC: u32 = 32;

/// One playing wave.
#[derive(Clone, Copy)]
struct Voice {
    wave: usize,
    /// Position in the wave, 32.32 fixed point.
    pos: u64,
    /// Position step per output sample, 32.32 fixed point.
    step: u64,
    /// Left and right gain.
    vol: [f32; 2],
    /// Caller's handle, to stop or retune it later.
    id: u32,
}

/// A handle to a started voice.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct VoiceId(u32);

pub struct Mixer {
    rate: u32,
    voices: Vec<Voice>,
    next_id: u32,
    /// Overall gain before the limiter.
    pub master: f32,
    /// Scratch mix bus, stereo f32.
    bus: Vec<[f32; 2]>,
}

impl Mixer {
    pub fn new(rate: u32) -> Mixer {
        Mixer { rate, voices: Vec::new(), next_id: 1, master: 1.0, bus: Vec::new() }
    }

    pub fn rate(&self) -> u32 {
        self.rate
    }

    /// Start `waves[wave]` at `pitch` times its own rate with left/right
    /// gain `vol`.
    pub fn play(&mut self, waves: &[Wave], wave: usize, pitch: f32, vol: [f32; 2]) -> Option<VoiceId> {
        let w = waves.get(wave)?;
        let id = self.next_id;
        self.next_id = self.next_id.wrapping_add(1).max(1);
        let step = step(w.rate * pitch, self.rate);
        self.voices.push(Voice { wave, pos: 0, step, vol, id });
        Some(VoiceId(id))
    }

    /// Stop a voice now. Unknown or finished voices are ignored.
    pub fn stop(&mut self, v: VoiceId) {
        self.voices.retain(|x| x.id != v.0);
    }

    pub fn stop_all(&mut self) {
        self.voices.clear();
    }

    /// Voices still playing.
    pub fn active(&self) -> usize {
        self.voices.len()
    }

    /// Mix `out.len() / 2` stereo frames into `out` (interleaved i16).
    pub fn render(&mut self, waves: &[Wave], out: &mut [i16]) {
        let n = out.len() / 2;
        self.bus.clear();
        self.bus.resize(n, [0.0; 2]);
        self.voices.retain_mut(|v| mix_voice(v, &waves[v.wave], &mut self.bus));
        let g = self.master;
        for (o, s) in out.chunks_exact_mut(2).zip(&self.bus) {
            o[0] = to_i16(s[0] * g);
            o[1] = to_i16(s[1] * g);
        }
    }
}

fn step(from: f32, to: u32) -> u64 {
    ((from as f64 / to as f64) * (1u64 << FRAC) as f64) as u64
}

/// Sample `i` of a wave, following its loop; silence past the end.
#[inline]
fn at(w: &Wave, i: u64) -> f32 {
    let len = w.pcm.len() as u64;
    let i = match w.looped {
        Some((s, e)) if i >= e as u64 => s as u64 + (i - s as u64) % (e - s) as u64,
        _ => i,
    };
    if i < len { w.pcm[i as usize] as f32 * (1.0 / 32768.0) } else { 0.0 }
}

/// Add one voice into `bus`; false once it has played to its end.
fn mix_voice(v: &mut Voice, w: &Wave, bus: &mut [[f32; 2]]) -> bool {
    let end = match w.looped {
        Some(_) => u64::MAX,
        None => w.pcm.len() as u64,
    };
    for s in bus.iter_mut() {
        let i = v.pos >> FRAC;
        if i >= end {
            return false;
        }
        let t = (v.pos & ((1 << FRAC) - 1)) as f32 * (1.0 / (1u64 << FRAC) as f32);
        let p0 = if i == 0 { 0.0 } else { at(w, i - 1) };
        let (p1, p2, p3) = (at(w, i), at(w, i + 1), at(w, i + 2));
        // Catmull-Rom.
        let x = p1
            + 0.5 * t * (p2 - p0 + t * (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3 + t * (3.0 * (p1 - p2) + p3 - p0)));
        s[0] += x * v.vol[0];
        s[1] += x * v.vol[1];
        v.pos += v.step;
    }
    true
}

/// Soft limiter: linear up to 0.9, then a smooth knee to full scale.
#[inline]
fn to_i16(x: f32) -> i16 {
    let a = x.abs();
    let y = if a <= 0.9 { a } else { 0.9 + 0.1 * (1.0 - 1.0 / (1.0 + (a - 0.9) * 10.0)) };
    (y.copysign(x) * 32767.0) as i16
}
