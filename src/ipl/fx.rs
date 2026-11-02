//! The menu's mosaic data: the `CAFD` cube flight paths (res 37) and the
//! per-language lit-cell tables (res 179..184), kept raw: BS2 reads both
//! straight from the resource, and so does [`crate::bs2::fx`].
//!
//! Inside Game Play, Options and Calendar, words and digits are drawn as a
//! grid of small cubes ("mosaic"). Each shape has a fixed set of cubes that
//! fly in along Hermite paths; a glyph table says which of them are shown.
//!
//! * `CAFD`: `magic, u32 version (5), u32 keys, u32 shapes`. `keys`
//!   (relative to the file) is an array of 0x24-byte keys: `vec3 pos, vec3
//!   tangent in, vec3 tangent out`. `shapes` is a table of 8-byte entries
//!   `u16 cubes, u16 keysPerCube, s32 offset` (relative to the entry) to
//!   `cubes` paths of 8 bytes: `u8 start, u8 end, u16 pad, s32 offset`
//!   (relative to the path) to `keysPerCube` u16 key indices. PAL 1.0 has
//!   five shapes: Calendar, Game Play, Options, and two 74-cube side
//!   columns that fly in and out.
//! * Lit-cell tables: an array of `u16 count, u16 offset` (offset relative
//!   to the entry) to `count` u16 cube indices; the first offset bounds the
//!   array.

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

/// `CAFD` as `Fx_SetShapes` keeps it: the file and its two tables.
#[derive(Clone, Debug)]
pub struct Shapes {
    data: Vec<u8>,
    keys: usize,
    shapes: usize,
}

fn vec3(b: &[u8], off: usize) -> [f32; 3] {
    [bef32(b, off).unwrap_or(0.0), bef32(b, off + 4).unwrap_or(0.0), bef32(b, off + 8).unwrap_or(0.0)]
}

/// `base + rel` for a signed offset; out-of-range reads come back as 0.
fn at(base: usize, rel: u32) -> usize {
    base.wrapping_add(rel as i32 as isize as usize)
}

impl Shapes {
    pub fn parse(b: &[u8]) -> Result<Shapes> {
        if b.get(0..4) != Some(b"CAFD") {
            return Err(Error::BadMagic(0));
        }
        let keys = be32(b, 8)? as usize;
        let shapes = be32(b, 12)? as usize;
        be32(b, shapes + 4)?;
        Ok(Shapes { data: b.to_vec(), keys, shapes })
    }

    fn entry(&self, n: usize) -> usize {
        self.shapes + n * 8
    }

    /// `FxShape.count` (fn_81329B24).
    pub fn count(&self, n: usize) -> u16 {
        be16(&self.data, self.entry(n)).unwrap_or(0)
    }

    /// `FxShape.nKeys`.
    pub fn n_keys(&self, n: usize) -> u16 {
        be16(&self.data, self.entry(n) + 2).unwrap_or(0)
    }

    fn cube(&self, n: usize, i: usize) -> usize {
        let e = self.entry(n);
        at(e, be32(&self.data, e + 4).unwrap_or(0)) + i * 8
    }

    /// Cube `i` of shape `n`: (start, end).
    pub fn times(&self, n: usize, i: usize) -> (u8, u8) {
        let c = self.cube(n, i);
        (be8(&self.data, c).unwrap_or(0), be8(&self.data, c + 1).unwrap_or(0))
    }

    /// Entry `k` of the key list of cube `i` of shape `n`, read past the
    /// list's end as BS2 does.
    pub fn key_index(&self, n: usize, i: usize, k: usize) -> u16 {
        let c = self.cube(n, i);
        be16(&self.data, at(c, be32(&self.data, c + 4).unwrap_or(0)) + k * 2).unwrap_or(0)
    }

    pub fn key(&self, idx: u16) -> Key {
        let o = self.keys + idx as usize * 0x24;
        Key { pos: vec3(&self.data, o), tan_in: vec3(&self.data, o + 12), tan_out: vec3(&self.data, o + 24) }
    }
}

/// One language's lit-cell table, kept raw (`FxGlyphs`).
#[derive(Clone, Debug)]
pub struct Glyphs(Vec<u8>);

impl Glyphs {
    pub fn parse(b: &[u8]) -> Result<Glyphs> {
        if be16(b, 2)? < 4 {
            return Err(Error::BadValue(2));
        }
        Ok(Glyphs(b.to_vec()))
    }

    /// The cell count of entry `e`.
    pub fn count(&self, e: usize) -> u16 {
        be16(&self.0, e * 4).unwrap_or(0)
    }

    /// Cell `i` of entry `e` (`FX_GLYPH_DATA(g, e)[i]`), read past the
    /// entry's end as BS2 does.
    pub fn at(&self, e: usize, i: usize) -> u16 {
        let o = e * 4 + be16(&self.0, e * 4 + 2).unwrap_or(0) as usize;
        be16(&self.0, o + i * 2).unwrap_or(0)
    }

    /// The cells of entry `e`.
    pub fn list(&self, e: usize) -> impl Iterator<Item = u16> + '_ {
        (0..self.count(e) as usize).map(move |i| self.at(e, i))
    }
}
