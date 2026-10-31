//! Mosaic digit flights (`menu_fx_grid.c`, PAL 1.0 BS2
//! 0x81334C08..0x81336968): while a number is being edited (the Options
//! screen position, a Calendar field), its two digit slots are drawn from
//! copies: when a digit changes, the old digit's cubes fly out one way and
//! the new one's fly in from the other.

use super::{anim as e, digits, FxCube, State, P};
use crate::bs2::mtx::to_s16;
use crate::bs2::rnd::rand;
use crate::ipl::fx::Glyphs;

const N: usize = 0x70;

type Vec3 = [f32; 3];

pub struct Grid {
    /// `sFxGridCube`: old tens, new tens, old ones, new ones.
    cube: [FxCube; N],
    alpha: [u8; N],
    dist: [i16; N],
    dir: Vec3,
    /// (ones, tens) digit flown to, from, and each flight's frame.
    to: [i16; 2],
    from: [i16; 2],
    frame: [i16; 2],
    count: i16,
}

impl Grid {
    pub fn new() -> Grid {
        Grid { cube: [FxCube::default(); N], alpha: [0; N], dist: [0; N], dir: [0.0; 3], to: [0; 2], from: [0; 2], frame: [0; 2], count: 0 }
    }

    /// fn_81334C08, fn_81334C1C, fn_81334C10
    pub fn out(&self) -> (&[FxCube], &[u8], usize) {
        (&self.cube, &self.alpha, self.count.max(0) as usize)
    }

    /// fn_81334C28: finish both flights at once.
    pub fn finish(&mut self) {
        self.frame = [P.d4; 2];
    }

    /// fn_81336594: the tens flight, 0..1.
    fn t_tens(&self) -> f32 {
        self.frame[1] as f32 / P.d4 as f32
    }

    /// fn_813365F4: the ones flight, 0..1.
    fn t_ones(&self) -> f32 {
        self.frame[0] as f32 / P.d4 as f32
    }

    /// One block of FXGRID_MOVE: `n` copies of `slot`'s cubes, at `k` of
    /// their flight along `sign * dir`, shown where digit `d` has a cell.
    #[allow(clippy::too_many_arguments)]
    fn block(&mut self, src: &[FxCube], g: &Glyphs, slot: usize, n: usize, base: usize, d: i16, sign: f32, k: f32, a: f32, far: f32) {
        let idx = e::digit(d);
        for i in 0..n {
            let s = src.get(g.at(slot, i) as usize).copied().unwrap_or_default();
            let Some(c) = self.cube.get_mut(base + i) else { break };
            let dist = self.dist.get(base + i).copied().unwrap_or(0) as f32;
            for j in 0..3 {
                c.pos[j] = s.pos[j] + sign * self.dir[j] * (dist * far) * k;
            }
            c.rot = s.rot;
            c.scale = s.scale;
            if let Some(x) = self.alpha.get_mut(base + g.at(idx, i) as usize) {
                *x = (255.0 * a) as u8;
            }
        }
    }

    /// FXGRID_MOVE: the four flights of a two-slot number (`l1` tens, `l2`
    /// ones). Only the tens flights go `fxD0` times as far.
    fn moves(&mut self, src: &[FxCube], g: &Glyphs, l1: usize, l2: usize, n: usize) {
        let (ty, tx) = (self.t_tens(), self.t_ones());
        self.block(src, g, l1, n, 0, self.from[1], -1.0, ty, 1.0 - ty, P.d0);
        self.block(src, g, l1, n, n, self.to[1], 1.0, 1.0 - ty, ty, P.d0);
        self.block(src, g, l2, n, n * 2, self.from[0], -1.0, tx, 1.0 - tx, 1.0);
        self.block(src, g, l2, n, n * 3, self.to[0], 1.0, 1.0 - tx, tx, 1.0);
    }

    fn step(&mut self) {
        for f in &mut self.frame {
            if *f < P.d4 {
                *f += 1;
            }
        }
    }

    /// fn_81334C60: the Options screen position while it is being edited.
    pub fn options(&mut self, src: &[FxCube], g: &Glyphs, cur: &State, prev: &State) {
        if cur.v[2] == 2 {
            if prev.v[2] != 2 {
                self.snap(cur.v[4]);
                self.count = 0;
                return;
            }
            self.count = N as i16;
        } else {
            self.count = 0;
            return;
        }
        let n = g.count(e::OFFSET_DIGITS[0]) as usize;
        self.alpha = [0; N];
        self.moves(src, g, e::OFFSET_DIGITS[0], e::OFFSET_DIGITS[1], n);
        self.fly([-1.0, 0.0, 0.0], prev.v[4], cur.v[4]);
        self.step();
    }

    /// fn_813355E0: the Calendar field being edited.
    pub fn clock(&mut self, src: &[FxCube], g: &Glyphs, cur: &State, prev: &State, now: [i32; 6]) {
        if cur.v[6] != 0 {
            if prev.v[6] != 0 {
                self.count = N as i16;
            } else {
                self.snap_clock(cur, now);
                self.count = 0;
                return;
            }
        } else {
            self.count = 0;
            return;
        }
        let n = g.count(e::YEAR_DIGITS[0]) as usize;
        let c = shown(now, cur);
        let p = shown(now, prev);
        // A wrap (2099 to 2000, 12 to 1, ...) flies up, every other change down.
        let wraps = |k: usize, hi: &[i16], lo: i16| (hi.contains(&c[k]) && p[k] == lo) || (c[k] == lo && hi.contains(&p[k]));
        let (k, l1, l2, wrap) = match (cur.v[6], cur.v[8], cur.v[9]) {
            (1, 0, _) => (0, e::YEAR_DIGITS[2], e::YEAR_DIGITS[3], wraps(0, &[2099], 2000)),
            (1, 1, _) => (1, e::MONTH_DIGITS[0], e::MONTH_DIGITS[1], wraps(1, &[12], 1)),
            (1, 2, _) => (2, e::DAY_DIGITS[0], e::DAY_DIGITS[1], wraps(2, &[31, 30, 29, 28], 1)),
            (2, _, 0) => (3, e::HOUR_DIGITS[0], e::HOUR_DIGITS[1], wraps(3, &[23], 0)),
            (2, _, 1) => (4, e::MINUTE_DIGITS[0], e::MINUTE_DIGITS[1], wraps(4, &[59], 0)),
            (2, _, 2) => (5, e::SECOND_DIGITS[0], e::SECOND_DIGITS[1], wraps(5, &[59], 0)),
            (1 | 2, _, _) => return,
            _ => return,
        };
        self.fly([0.0, if wrap { 1.0 } else { -1.0 }, 0.0], p[k], c[k]);
        self.alpha = [0; N];
        let field = |f: i16, base: usize| if (0..3).contains(&f) { Some(c[base + f as usize]) } else { None };
        if cur.v[8] != prev.v[8] {
            self.finish();
            if let Some(v) = field(cur.v[8], 0) {
                let d = digits(v as i32);
                self.from = [d[0], d[1]];
                self.to = self.from;
            }
        }
        if cur.v[9] != prev.v[9] {
            self.finish();
            if let Some(v) = field(cur.v[9], 3) {
                let d = digits(v as i32);
                self.from = [d[0], d[1]];
                self.to = self.from;
            }
        }
        self.moves(src, g, l1, l2, n);
        self.step();
    }

    /// fn_81336650: start a flight from `from` to `to` along `dir`.
    fn fly(&mut self, dir: Vec3, from: i16, to: i16) {
        let d = to.wrapping_sub(from);
        if from == to {
            return;
        }
        self.dir = if d > 0 { dir } else { dir.map(|v| -v) };
        for x in self.dist.iter_mut() {
            *x = to_s16(P.cc * (rand() % 600 + 200) as f32 / 10.0);
        }
        let p0 = digits(libm::fabs(from as f64) as i32);
        let p1 = digits(libm::fabs(to as f64) as i32);
        if p0[1] != p1[1] {
            self.frame[1] = 0;
            self.from[1] = p0[1];
            self.to[1] = p1[1];
        }
        if p0[0] != p1[0] {
            self.from[0] = p0[0];
            self.frame[0] = 0;
            self.to[0] = p1[0];
        }
    }

    /// fn_8133682C: snap the Calendar flight to the current field.
    fn snap_clock(&mut self, cur: &State, now: [i32; 6]) {
        let t = shown(now, cur);
        let v = match cur.v[6] {
            1 => t.get(cur.v[8] as usize),
            2 => t.get((3 + cur.v[9]) as usize),
            _ => return,
        };
        let d = digits(v.copied().unwrap_or(0) as i32);
        self.from = [d[0], d[1]];
        self.to = self.from;
    }

    /// fn_813368F0: snap the Options flight to the current offset.
    fn snap(&mut self, item: i16) {
        let d = digits(libm::fabs(item as f64) as i32);
        self.from = [d[0], d[1]];
        self.to = self.from;
    }
}

/// fn_8132A690 for a state's page and edit values.
fn shown(now: [i32; 6], s: &State) -> [i16; 6] {
    let [mut year, mut mon, mut mday, mut hour, mut min, mut sec] = now;
    let e = [s.v[10] as i32, s.v[11] as i32, s.v[12] as i32];
    if s.v[6] == 1 {
        [year, mon, mday] = e;
    } else if s.v[6] == 2 {
        [hour, min, sec] = e;
    }
    [year as i16, (mon + 1) as i16, mday as i16, hour as i16, min as i16, sec as i16]
}
