//! The software mixer that stands in for the GameCube DSP (the IPL's
//! JAudio microcode; facts about its mixing from Dolphin's notes on that
//! microcode).
//!
//! The DSP mixes every voice into buses at 32028.5 Hz, once per 80-sample
//! subframe: front left (bus 1), front right (2), and the inputs of two
//! echo lines (3, 4) that feed back into themselves through an 8-tap FIR
//! and play into left and right. Here every voice is resampled once,
//! straight from its wave's own rate to the output rate (48 kHz by
//! default) with 4-point cubic interpolation, and the buses are f32, so
//! nothing is lost twice and nothing clips until the final soft limiter.

use super::data::Wave;
use super::voice::{DAC_RATE, SUBFRAME, Voice};
use alloc::vec;
use alloc::vec::Vec;

/// Output sample rate the sound card is set to.
pub const RATE: u32 = 48_000;

/// Position fraction bits (32.32 fixed point sample positions).
pub const FRAC: u32 = 32;

/// Voice bus levels are 1.15 volumes the DSP applies as `>> 16`, and its
/// output volume is 4.0 (`Jac_SetMixerLevel(0.8, 4.0)`, 4.12 fixed point).
const LEVEL_GAIN: f32 = 4.0 / 65536.0;

/// The buses a voice can reach (`connect_table`), by their output here.
#[derive(Clone, Copy, PartialEq)]
pub enum Bus {
    Left,
    Right,
    Fx(usize),
    None,
}

/// Where bus number `n` (a routing's upper byte) goes. 0x0FA0 (bus 9) is a
/// mixing destination only the ambient's lead voice uses, paired with the
/// left bus: it plays on the right here.
pub fn bus(n: u16) -> Bus {
    match n {
        1 => Bus::Left,
        2 | 9 => Bus::Right,
        3 => Bus::Fx(0),
        4 => Bus::Fx(1),
        _ => Bus::None,
    }
}

/// One echo line (`sFxConfig`, `DFX_SetFxLine`): a delay of `size`
/// subframes whose output is filtered, played into the dry buses, and fed
/// back into the delay together with what the voices send.
pub struct FxLine {
    ring: Vec<f32>,
    pos: usize,
    hist: [f32; 8],
    coef: [f32; 8],
    /// Gain into left and right.
    out: [f32; 2],
}

impl FxLine {
    fn new(rate: u32, size: u32, coef: [i16; 8], out: [f32; 2]) -> FxLine {
        let n = ((size as f32 * SUBFRAME * rate as f32 / DAC_RATE) as usize).max(1);
        FxLine { ring: vec![0.0; n], pos: 0, hist: [0.0; 8], coef: coef.map(|c| c as f32 / 32768.0), out }
    }

    fn clear(&mut self) {
        self.ring.fill(0.0);
        self.hist = [0.0; 8];
    }

    /// One sample: `send` in, (left, right) out.
    #[inline]
    fn step(&mut self, send: f32) -> (f32, f32) {
        let d = self.ring[self.pos];
        // The filter reads the 8 delayed samples before this one.
        let mut y = 0.0;
        for j in 0..8 {
            y += self.hist[j] * self.coef[j];
        }
        self.hist.copy_within(1.., 0);
        self.hist[7] = d;
        self.ring[self.pos] = y + send;
        self.pos += 1;
        if self.pos == self.ring.len() {
            self.pos = 0;
        }
        (y * self.out[0], y * self.out[1])
    }
}

/// The IPL's two echo lines (the third mixes into a buffer that is never
/// played, the fourth is off).
pub fn fx_lines(rate: u32) -> [FxLine; 2] {
    const FIR: [i16; 8] = [0, 0, 0, 500, 1000, 4000, 7000, 8000];
    let v = 0x6000 as f32 / 32768.0;
    [FxLine::new(rate, 50, FIR, [v, 0.0]), FxLine::new(rate, 50, FIR, [0.0, v])]
}

pub fn clear_fx(fx: &mut [FxLine; 2]) {
    for f in fx {
        f.clear();
    }
}

/// Mix `n` frames of one subframe (`from` of `len` frames into it) into
/// the buses: `dry` stereo and `send` per echo line.
pub fn mix_voice(v: &mut Voice, w: &Wave, out_rate: u32, from: usize, len: usize, dry: &mut [[f32; 2]], send: &mut [[f32; 2]]) {
    let step = ((v.ratio as f64 * DAC_RATE as f64 / out_rate as f64) * (1u64 << FRAC) as f64) as u64;
    let end = match w.looped {
        Some(_) => u64::MAX,
        None => w.pcm.len() as u64,
    };
    let silent = v.lv0.iter().chain(v.lv1.iter()).all(|&l| l == 0.0);
    let inv = 1.0 / len as f32;
    let routes = v.routing.map(|r| bus(r >> 8));
    for (k, (d, s)) in dry.iter_mut().zip(send.iter_mut()).enumerate() {
        let i = v.pos >> FRAC;
        if i >= end {
            v.done = true;
            return;
        }
        if !silent {
            let t = (v.pos & ((1 << FRAC) - 1)) as f32 * (1.0 / (1u64 << FRAC) as f32);
            let p0 = if i == 0 { 0.0 } else { at(w, i - 1) };
            let (p1, p2, p3) = (at(w, i), at(w, i + 1), at(w, i + 2));
            // Catmull-Rom.
            let x = p1 + 0.5 * t * (p2 - p0 + t * (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3 + t * (3.0 * (p1 - p2) + p3 - p0)));
            let f = (from + k) as f32 * inv;
            for b in 0..4 {
                let g = v.lv0[b] + (v.lv1[b] - v.lv0[b]) * f;
                if g == 0.0 {
                    continue;
                }
                let y = x * g * LEVEL_GAIN;
                match routes[b] {
                    Bus::Left => d[0] += y,
                    Bus::Right => d[1] += y,
                    Bus::Fx(n) => s[n] += y,
                    Bus::None => {}
                }
            }
        }
        v.pos += step;
    }
}

/// Run the echo lines over the sends and add their output to `dry`.
pub fn mix_fx(fx: &mut [FxLine; 2], dry: &mut [[f32; 2]], send: &[[f32; 2]]) {
    for (d, s) in dry.iter_mut().zip(send) {
        for (n, f) in fx.iter_mut().enumerate() {
            let (l, r) = f.step(s[n]);
            d[0] += l;
            d[1] += r;
        }
    }
}

/// Sample `i` of a wave, following its loop; silence past the end.
#[inline]
pub fn at(w: &Wave, i: u64) -> f32 {
    let len = w.pcm.len() as u64;
    let i = match w.looped {
        Some((s, e)) if i >= e as u64 => s as u64 + (i - s as u64) % (e - s) as u64,
        _ => i,
    };
    if i < len { w.pcm[i as usize] as f32 * (1.0 / 32768.0) } else { 0.0 }
}

/// Soft limiter: linear up to 0.9, then a smooth knee to full scale.
#[inline]
pub fn to_i16(x: f32) -> i16 {
    let a = x.abs();
    let y = if a <= 0.9 { a } else { 0.9 + 0.1 * (1.0 - 1.0 / (1.0 + (a - 0.9) * 10.0)) };
    (y.copysign(x) * 32767.0) as i16
}
