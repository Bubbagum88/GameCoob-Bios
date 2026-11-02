//! Mosaic sway ("sparks", `menu_fx_spark.c`, PAL 1.0 BS2
//! 0x8133370C..0x81334C08): once a screen has come in, each word turns
//! about its centre and drifts on an eased path, looping every `fxC8`
//! frames. The offsets are kept per cube; fn_8132BD00 adds them weighted by
//! each cube's sway value.

use super::{anim as e, centre, deg, rot_about, FxCube, FX_MAX, P};
use crate::bs2::gbase::hermite;
use crate::ipl::fx::Glyphs;
use alloc::vec;
use alloc::vec::Vec;

pub struct Spark {
    /// `sFxSpark`: per-cube offset and angles.
    pub cube: Vec<FxCube>,
    frame: u32,
    /// `sFxSparkCount`: counted to 100, never read.
    #[allow(dead_code)]
    count: u32,
}

/// fn_81334BA8 (and the two inlined wraps of fn_8133498C): `x mod y`.
fn wrap(x: f32, y: f32) -> f32 {
    if libm::fabs(y as f64) > libm::fabs(x as f64) {
        return x;
    }
    x - y * ((x / y) as i64) as f32
}

/// fn_81334B40: ease `lo` to `hi` over 0..`mid`, back to `lo` over `mid`..1.
pub fn ease(mid: f32, t: f32, lo: f32, hi: f32) -> f32 {
    if t < mid {
        hermite(t, 0.0, lo, 0.0, mid, hi, 0.0)
    } else {
        hermite(t, mid, hi, 0.0, 1.0, lo, 0.0)
    }
}

/// fn_8133498C: drift and spin angles at time `t` (0..1).
fn flight(t: f32) -> (f32, f32, i16, i16) {
    let px = wrap(P.c4 / 2.0 + t, 1.0);
    let py = wrap(0.25 + 2.0 * wrap(t, 1.0), 1.0);
    let pr = wrap(0.25 + t, 1.0);
    let dx = ease(P.c4, px, -P.b4, P.b4);
    let dy = ease(0.5, py, -P.b8, P.b8);
    let rx = deg(ease(0.5, pr, -P.bc, P.bc));
    let ry = deg(ease(0.5, pr, -P.c0, P.c0));
    (dx, dy, rx, ry)
}

impl Spark {
    pub fn new() -> Spark {
        Spark { cube: vec![FxCube::default(); FX_MAX], frame: 0, count: 0 }
    }

    /// fn_8133370C
    pub fn reset(&mut self) {
        self.frame = 0;
        self.count = 0;
        self.cube.fill(FxCube::default());
    }

    fn t(&self) -> f32 {
        self.frame as f32 / P.c8 as f32
    }

    /// fn_8133374C: Game Play drifts all its cubes and turns them as one.
    pub fn game(&mut self, cubes: &mut [FxCube], n: u32) {
        let (dx, dy, rx, ry) = flight(self.t());
        let n = (n as usize).min(cubes.len());
        let c = centre(cubes, 0..n, n as u32);
        for i in 0..n {
            cubes[i].pos[0] += dx;
            cubes[i].pos[1] += dy;
            let t = rot_about([0, rx, ry], &cubes[i], c);
            self.cube[i].pos = t.pos;
            self.cube[i].rot = t.rot;
        }
        self.step();
    }

    /// SPIN_LIST: turn one word about its centre and drift it.
    fn spin(&mut self, cubes: &[FxCube], g: &Glyphs, entry: usize, f: (f32, f32, i16, i16)) {
        let (dx, dy, rx, ry) = f;
        let c = centre(cubes, g.list(entry).map(|i| i as usize), g.count(entry) as u32);
        for i in g.list(entry) {
            let i = i as usize;
            if i >= FX_MAX {
                continue;
            }
            let t = rot_about([0, rx, ry], &cubes.get(i).copied().unwrap_or_default(), c);
            let s = &mut self.cube[i];
            s.pos = t.pos;
            s.rot = t.rot;
            s.pos[0] += dx;
            s.pos[1] += dy;
        }
    }

    /// Fx_EmitSparks (0x81333898): the Options words.
    pub fn options(&mut self, cubes: &[FxCube], g: &Glyphs) {
        let f = flight(self.t());
        for entry in [e::OFFSET_DIAMOND, e::OFFSET_DIGITS[0], e::OFFSET_DIGITS[1], e::SOUND_ROW, e::LANG_LIST] {
            self.spin(cubes, g, entry, f);
        }
        self.step();
    }

    /// fn_81333C7C: the Calendar separators and digits.
    pub fn clock(&mut self, cubes: &[FxCube], g: &Glyphs) {
        let f = flight(self.t());
        let lists = e::SEPARATOR.iter().chain(&e::YEAR_DIGITS).chain(&e::MONTH_DIGITS).chain(&e::DAY_DIGITS);
        for &entry in lists.chain(&e::HOUR_DIGITS).chain(&e::MINUTE_DIGITS).chain(&e::SECOND_DIGITS) {
            self.spin(cubes, g, entry, f);
        }
        self.step();
    }

    /// fn_81334928
    fn step(&mut self) {
        self.frame += 1;
        self.count += 1;
        if self.frame >= P.c8 as u32 {
            self.frame = 0;
        }
        if self.count >= 100 {
            self.count = 0;
        }
    }
}
