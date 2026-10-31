//! Mosaic word wipes (`menu_fx_wipe.c`, PAL 1.0 BS2 0x81336968..0x813376A0):
//! the Game Play message and the Options sound word are drawn from copies;
//! when the word changes, the old word's cubes fly out one way and the new
//! one's fly in from the other.

use super::{anim as e, FxCube, State, P};
use crate::bs2::mtx::to_s16;
use crate::bs2::rnd::rand;
use crate::ipl::fx::Glyphs;
use alloc::vec;
use alloc::vec::Vec;

/// `FX_WIPE_CUBES`
const N: usize = 1430;

pub struct Wipe {
    cube: Vec<FxCube>,
    alpha: Vec<u8>,
    dist: Vec<i16>,
    dir: [f32; 3],
    frame: i16,
    from: i16,
    to: i16,
    count: i16,
}

impl Wipe {
    pub fn new() -> Wipe {
        Wipe { cube: vec![FxCube::default(); N], alpha: vec![0; N], dist: vec![0; N], dir: [0.0; 3], frame: 0, from: 0, to: 0, count: 0 }
    }

    /// fn_81336968, fn_8133697C, fn_81336970
    pub fn out(&self) -> (&[FxCube], &[u8], usize) {
        (&self.cube, &self.alpha, self.count.max(0) as usize)
    }

    /// fn_81336988: finish the wipe at once.
    pub fn finish(&mut self) {
        self.frame = P.d4;
    }

    /// fn_81337530: wipe progress, 0..1.
    fn t(&self) -> f32 {
        self.frame as f32 / P.d4 as f32
    }

    /// The old word out, then the new one in; returns the copies made.
    fn copy(&mut self, src: &[FxCube], g: &Glyphs, old: usize, new: usize) {
        self.alpha.fill(0);
        let t = self.t();
        let n1 = g.count(old) as usize;
        for i in 0..n1.min(N) {
            let s = src.get(g.at(old, i) as usize).copied().unwrap_or_default();
            let c = &mut self.cube[i];
            for j in 0..3 {
                c.pos[j] = s.pos[j] + -self.dir[j] * self.dist[i] as f32 * t;
            }
            c.rot = s.rot;
            c.scale = s.scale;
            self.alpha[i] = (255.0 * (1.0 - t)) as u8;
        }
        let n2 = g.count(new) as usize;
        for i in 0..n2 {
            let k = n1 + i;
            if k >= N {
                break;
            }
            let s = src.get(g.at(new, i) as usize).copied().unwrap_or_default();
            let c = &mut self.cube[k];
            for j in 0..3 {
                c.pos[j] = s.pos[j] + self.dir[j] * self.dist[k] as f32 * (1.0 - t);
            }
            c.rot = s.rot;
            c.scale = s.scale;
            self.alpha[k] = (255.0 * t) as u8;
        }
    }

    fn step(&mut self) {
        if self.frame < P.d4 {
            self.frame += 1;
        }
    }

    /// fn_813369B0: the Options sound word (mono or stereo) while sound is
    /// being edited.
    pub fn options(&mut self, src: &[FxCube], g: &Glyphs, cur: &State, prev: &State) {
        if cur.v[2] == 1 && prev.v[2] == 1 {
            self.count = N as i16;
        } else {
            self.from = cur.v[5];
            self.to = cur.v[5];
            self.count = 0;
            return;
        }
        let word = |k: i16| if k == 1 { e::STEREO } else { e::MONO };
        self.copy(src, g, word(self.from), word(self.to));
        let dir = if cur.v[5] != 0 { [-1.0, 0.0, 0.0] } else { [1.0, 0.0, 0.0] };
        self.start(dir, prev.v[5], cur.v[5]);
        self.step();
    }

    /// fn_81336F9C: the Game Play message.
    pub fn game(&mut self, src: &[FxCube], g: &Glyphs, cur: &State, prev: &State) {
        if cur.screen == 3 && prev.screen == 3 {
            self.count = N as i16;
        } else {
            self.from = cur.v[1];
            self.to = cur.v[1];
            self.count = 0;
            return;
        }
        let word = |k: i16| match k {
            1 => e::NO_DISC,
            2 => e::UNKNOWN_DISC,
            _ => e::PRESS_START,
        };
        self.copy(src, g, word(self.from), word(self.to));
        self.start([-1.0, 0.0, 0.0], prev.v[1], cur.v[1]);
        self.step();
    }

    /// fn_8133758C: start a wipe from word `from` to word `to` along `dir`.
    fn start(&mut self, dir: [f32; 3], from: i16, to: i16) {
        if from != to {
            self.dir = dir;
            for d in self.dist.iter_mut() {
                *d = to_s16(P.cc * (rand() % 600 + 200) as f32 / 10.0);
            }
            self.from = from;
            self.frame = 0;
            self.to = to;
        }
    }
}
