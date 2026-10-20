//! The menu's mosaic data: the `CAFD` cube flight paths (res 37) and the
//! per-language lit-cell tables (res 179..184).
//!
//! Inside Game Play, Options and Calendar, words and digits are drawn as a
//! grid of small cubes ("mosaic"). Each screen has a fixed set of cubes that
//! fly in along Hermite paths; a glyph table says which of them are shown.
//!
//! * `CAFD`: `magic, u32 version (5), u32 keys, u32 sets`. `keys` (relative
//!   to the file) is an array of 0x24-byte keys: `vec3 pos, vec3 tangent
//!   in, vec3 tangent out`. `sets` is a table of 8-byte entries `u16 cubes,
//!   u16 keysPerCube, u32 offset` (relative to the entry) to `cubes` paths
//!   of 8 bytes: `u8 start, u8 end, u16 pad, u32 offset` (relative to the
//!   path) to `keysPerCube` u16 key indices. The table has as many entries
//!   as the first entry's offset leaves room for (five in PAL 1.0: Calendar,
//!   Game Play, Options, and two 74-cube side columns that fly in and out).
//! * Lit-cell tables: an array of `u16 count, u16 offset` (offset relative
//!   to the entry) to `count` u16 cube indices; the first offset bounds the
//!   array. Entry meanings are in [`glyph`].

use super::read::{be16, be32, be8, bef32};
use super::{Error, Result};
use alloc::vec::Vec;

/// One key: position and the Hermite tangents on each side, per unit time.
#[derive(Clone, Copy, Debug, Default)]
pub struct Key {
    pub pos: [f32; 3],
    pub tan_in: [f32; 3],
    pub tan_out: [f32; 3],
}

/// One cube's flight: it starts at `0.4 * (1 - start/255)` of the screen's
/// progress and lands at `0.6 + 0.4 * (1 - end/255)`.
#[derive(Clone, Debug)]
pub struct Path {
    pub start: u8,
    pub end: u8,
    pub keys: Vec<u16>,
}

/// All cube paths of one screen.
#[derive(Clone, Debug)]
pub struct Set {
    pub keys_per_cube: u16,
    pub paths: Vec<Path>,
}

/// Parsed `CAFD`.
#[derive(Clone, Debug)]
pub struct Paths {
    pub keys: Vec<Key>,
    pub sets: Vec<Set>,
}

/// Set ids in [`Paths::sets`].
pub mod set {
    pub const CLOCK: usize = 0;
    pub const GAME: usize = 1;
    pub const OPTION: usize = 2;
    /// Side columns flying in (Options, on entering a row).
    pub const SIDE_IN: usize = 3;
    /// The same columns flying out.
    pub const SIDE_OUT: usize = 4;
}

fn vec3(b: &[u8], off: usize) -> Result<[f32; 3]> {
    Ok([bef32(b, off)?, bef32(b, off + 4)?, bef32(b, off + 8)?])
}

impl Paths {
    pub fn parse(b: &[u8]) -> Result<Paths> {
        if b.get(0..4) != Some(b"CAFD") {
            return Err(Error::BadMagic(0));
        }
        let keys_at = be32(b, 8)? as usize;
        let sets_at = be32(b, 12)? as usize;
        if keys_at > sets_at {
            return Err(Error::BadValue(8));
        }
        let nkeys = (sets_at - keys_at) / 0x24;
        let mut keys = Vec::with_capacity(nkeys);
        for i in 0..nkeys {
            let o = keys_at + i * 0x24;
            keys.push(Key { pos: vec3(b, o)?, tan_in: vec3(b, o + 12)?, tan_out: vec3(b, o + 24)? });
        }
        let nsets = (be32(b, sets_at + 4)? / 8) as usize;
        if nsets == 0 || nsets > 64 {
            return Err(Error::BadValue(sets_at + 4));
        }
        let mut sets = Vec::with_capacity(nsets);
        for s in 0..nsets {
            let e = sets_at + s * 8;
            let cubes = be16(b, e)? as usize;
            let kpc = be16(b, e + 2)?;
            if kpc < 2 {
                return Err(Error::BadValue(e + 2));
            }
            let first = e + be32(b, e + 4)? as usize;
            let mut paths = Vec::with_capacity(cubes);
            for c in 0..cubes {
                let p = first + c * 8;
                let list = p + be32(b, p + 4)? as usize;
                let mut ks = Vec::with_capacity(kpc as usize);
                for k in 0..kpc as usize {
                    let i = be16(b, list + k * 2)?;
                    if i as usize >= nkeys {
                        return Err(Error::BadValue(list + k * 2));
                    }
                    ks.push(i);
                }
                paths.push(Path { start: be8(b, p)?, end: be8(b, p + 1)?, keys: ks });
            }
            sets.push(Set { keys_per_cube: kpc, paths });
        }
        Ok(Paths { keys, sets })
    }

    /// Fx_EvalShape: where cube `cube` of `set` is at progress `t` (0..1).
    pub fn eval(&self, set: usize, cube: usize, t: f32) -> [f32; 3] {
        let Some(p) = self.sets.get(set).and_then(|s| s.paths.get(cube)) else { return [0.0; 3] };
        let n = p.keys.len();
        let seg = 1.0 / (n - 1) as f32;
        let t0 = 0.4 * (1.0 - p.start as f32 / 255.0);
        let t1 = 0.6 + 0.4 * (1.0 - p.end as f32 / 255.0);
        let u = ((t.clamp(0.0, 1.0) - t0) / (t1 - t0)).clamp(0.0, 1.0);
        let i = ((u / seg) as usize).min(n - 2);
        let a = &self.keys[p.keys[i] as usize];
        let b = &self.keys[p.keys[i + 1] as usize];
        let (s0, s1) = (i as f32 * seg, (i + 1) as f32 * seg);
        core::array::from_fn(|k| hermite(u, s0, a.pos[k], a.tan_out[k], s1, b.pos[k], b.tan_in[k]))
    }

    /// Where every cube of `set` ends up.
    pub fn landing(&self, set: usize, cube: usize) -> [f32; 3] {
        self.eval(set, cube, 1.0)
    }
}

/// Cubic Hermite with tangents per unit time (BS2's `Hermite`).
fn hermite(t: f32, t0: f32, p0: f32, m0: f32, t1: f32, p1: f32, m1: f32) -> f32 {
    let d = t - t0;
    let inv = 1.0 / (t1 - t0);
    let s2 = d * d * inv * inv;
    let s3 = s2 * d * inv;
    let d3t2 = d * d * d * inv * inv;
    let d2t = d * d * inv;
    m1 * (d3t2 - d2t) + m0 * (d + d3t2 - 2.0 * d2t) + p0 * (1.0 + 2.0 * s3 - 3.0 * s2) + p1 * (3.0 * s2 - 2.0 * s3)
}

/// One language's lit-cell table: entry `i` lists cube indices.
#[derive(Clone, Debug)]
pub struct Glyphs(pub Vec<Vec<u16>>);

impl Glyphs {
    pub fn parse(b: &[u8]) -> Result<Glyphs> {
        let n = (be16(b, 2)? / 4) as usize;
        if n == 0 {
            return Err(Error::BadValue(2));
        }
        let mut out = Vec::with_capacity(n);
        for i in 0..n {
            let e = i * 4;
            let c = be16(b, e)? as usize;
            let o = e + be16(b, e + 2)? as usize;
            let mut l = Vec::with_capacity(c);
            for k in 0..c {
                l.push(be16(b, o + k * 2)?);
            }
            out.push(l);
        }
        Ok(Glyphs(out))
    }

    pub fn get(&self, i: usize) -> &[u16] {
        self.0.get(i).map(|v| v.as_slice()).unwrap_or(&[])
    }

    /// Cubes of digit `d` (0..9) placed in a 28-cell digit slot entry.
    pub fn digit(&self, slot: usize, d: u8) -> impl Iterator<Item = u16> + '_ {
        let s = self.get(slot);
        self.get(glyph::DIGIT0 + (d as usize % 10)).iter().filter_map(move |&c| s.get(c as usize).copied())
    }
}

/// Entry ids in a lit-cell table (the same in all six PAL languages; only
/// the cells of the words differ). Calendar and Options entries index cubes
/// of their [`set`]; digits index the 28 cells (4 wide, 7 high, bottom row
/// first) of a digit slot.
pub mod glyph {
    // Calendar (set::CLOCK; the time row is cubes 0..183, the date row 184..554).
    pub const TIME_ROW: usize = 0;
    /// Both slashes and both colons.
    pub const SEPARATORS: usize = 1;
    pub const SLASH: [usize; 2] = [2, 3];
    pub const COLON: [usize; 2] = [4, 5];
    pub const DATE_ROW: usize = 6;
    /// Date fields in screen order (PAL: day, month, year): the whole field,
    /// then its digit slots.
    pub const DAY: usize = 7;
    pub const DAY_DIGITS: [usize; 2] = [8, 9];
    pub const MONTH: usize = 16;
    pub const MONTH_DIGITS: [usize; 2] = [17, 18];
    pub const YEAR: usize = 22;
    pub const YEAR_DIGITS: [usize; 4] = [23, 24, 25, 26];
    pub const HOUR: usize = 10;
    pub const HOUR_DIGITS: [usize; 2] = [11, 12];
    pub const MINUTE: usize = 13;
    pub const MINUTE_DIGITS: [usize; 2] = [14, 15];
    pub const SECOND: usize = 19;
    pub const SECOND_DIGITS: [usize; 2] = [20, 21];
    // Game Play (set::GAME).
    pub const NO_DISC: usize = 27;
    pub const PRESS_START: usize = 28;
    pub const UNKNOWN_DISC: usize = 29;
    /// Digit shapes 0..9 (28-cell masks).
    pub const DIGIT0: usize = 30;
    // Options (set::OPTION).
    /// Screen position: the diamond with both arrows (drawn on its own,
    /// not lit through this table), then the arrow shown for a negative or
    /// positive offset. An offset of 0 shows neither arrow.
    pub const OFFSET_DIAMOND: usize = 40;
    pub const OFFSET_MINUS: usize = 41;
    pub const OFFSET_PLUS: usize = 42;
    /// The language row's centre dash.
    pub const LANG_DASH: usize = 43;
    pub const MONO: usize = 44;
    /// The diamond and both offset digits.
    pub const OFFSET_ROW: usize = 45;
    pub const OFFSET_DIGITS: [usize; 2] = [46, 47];
    pub const SOUND_ARROWS: [usize; 2] = [48, 49];
    pub const OFFSET_ARROWS: [usize; 2] = [50, 51];
    pub const LANG_ARROWS: [usize; 2] = [52, 53];
    /// The whole sound band.
    pub const SOUND_ROW: usize = 54;
    pub const STEREO: usize = 55;
}
