//! The IPL's 2D menu data: `GLH0` screen layouts, `STH0` string tables,
//! `TXH0` texture banks and the boot ROM's ANSI font.
//!
//! Formats as BS2 reads them (all big-endian, positions in 12.4 fixed point):
//!
//! * `GLH0`: header `magic, u32 pics, u32 texts, u32 frames, u16 nPics,
//!   u16 nTexts, u16 nFrames`. A picture pane (0x10 bytes) is
//!   `tag, x, y, w, h, u16 tex, u8 fit, u8 flags` (centre and size; flags
//!   bit 0 mirrors s, bit 1 mirrors t, bit 2 turns the image 90 degrees).
//!   A text pane (0x24 bytes) is `tag, x, y, w, h, rgba top, rgba bottom,
//!   u8 hAlign, u8 vAlign, s16 spacing, s16 line, u16 size, u16 bgPane,
//!   u16 maxLen, u32 default string (relative to the pane)`. A frame (0x30
//!   bytes) is a 9-slice box: `tag, x, y, w, h, u16 cornerW, u16 cornerH,
//!   u16 innerW, u16 innerH, u16 tex[4] (TL, TR, BL, BR), u8 flips, pad,
//!   rgba[4] (TL, TR, BL, BR)`.
//! * `STH0`: `magic, u16 count, pad`, then `count` entries of `u16 pane,
//!   u16 length, u32 offset` (relative to the entry), Windows-1252 text.
//! * `TXH0`: `magic, u16 count`, then BTI headers from 0x20, 0x20 apart.
//! * The ANSI font is the SDK `OSFontHeader` format, Yay0-packed at ROM
//!   0x1FCF00, with 2-bit glyph sheets in I4 tile order.

use super::gx::{self, Texture};
use super::read::{be16, be32, be8, bytes};
use super::{bti, yay0, Error, Result};
use alloc::string::String;
use alloc::vec::Vec;

/// A FourCC pane tag.
pub const fn tag(s: &[u8; 4]) -> u32 {
    u32::from_be_bytes(*s)
}

fn fx(v: u16) -> f32 {
    v as f32 / 16.0
}

#[derive(Clone, Copy, Debug)]
pub struct Pic {
    pub tag: u32,
    pub x: f32,
    pub y: f32,
    pub w: f32,
    pub h: f32,
    pub tex: u16,
    pub flags: u8,
}

#[derive(Clone, Debug)]
pub struct Text {
    pub tag: u32,
    pub x: f32,
    pub y: f32,
    pub w: f32,
    pub h: f32,
    pub top: [u8; 4],
    pub bottom: [u8; 4],
    /// 0 centre, 1 right, 2 left.
    pub h_align: u8,
    /// 0 centre, 1 bottom, 2 top.
    pub v_align: u8,
    pub spacing: f32,
    pub line: f32,
    pub size: f32,
    pub default: Vec<u8>,
}

#[derive(Clone, Copy, Debug)]
pub struct Frame {
    pub tag: u32,
    pub x: f32,
    pub y: f32,
    pub inner_w: f32,
    pub inner_h: f32,
    /// TL, TR, BL, BR.
    pub tex: [u16; 4],
    pub flips: u8,
    pub colors: [[u8; 4]; 4],
}

#[derive(Clone, Debug, Default)]
pub struct Layout {
    pub pics: Vec<Pic>,
    pub texts: Vec<Text>,
    pub frames: Vec<Frame>,
}

impl Layout {
    pub fn parse(b: &[u8]) -> Result<Layout> {
        if bytes(b, 0, 4)? != b"GLH0" {
            return Err(Error::BadMagic(0));
        }
        let (po, to, fo) = (be32(b, 4)? as usize, be32(b, 8)? as usize, be32(b, 0xC)? as usize);
        let (pc, tc, fc) = (be16(b, 0x10)? as usize, be16(b, 0x12)? as usize, be16(b, 0x14)? as usize);
        let mut l = Layout::default();
        for i in 0..pc {
            let o = po + i * 0x10;
            l.pics.push(Pic {
                tag: be32(b, o)?,
                x: fx(be16(b, o + 4)?),
                y: fx(be16(b, o + 6)?),
                w: fx(be16(b, o + 8)?),
                h: fx(be16(b, o + 10)?),
                tex: be16(b, o + 12)?,
                flags: be8(b, o + 15)?,
            });
        }
        for i in 0..tc {
            let o = to + i * 0x24;
            let rgba = |p: usize| -> Result<[u8; 4]> { Ok(bytes(b, p, 4)?.try_into().unwrap_or([255; 4])) };
            let s = o + be32(b, o + 0x20)? as usize;
            let max = be16(b, o + 0x1E)? as usize;
            let default = b.get(s..).unwrap_or(&[]).iter().take(max).take_while(|&&c| c != 0).copied().collect();
            l.texts.push(Text {
                tag: be32(b, o)?,
                x: fx(be16(b, o + 4)?),
                y: fx(be16(b, o + 6)?),
                w: fx(be16(b, o + 8)?),
                h: fx(be16(b, o + 10)?),
                top: rgba(o + 0xC)?,
                bottom: rgba(o + 0x10)?,
                h_align: be8(b, o + 0x14)? & 0x7F,
                v_align: be8(b, o + 0x15)?,
                spacing: be16(b, o + 0x16)? as i16 as f32,
                line: be16(b, o + 0x18)? as i16 as f32,
                size: be16(b, o + 0x1A)? as f32,
                default,
            });
        }
        for i in 0..fc {
            let o = fo + i * 0x30;
            let mut colors = [[0u8; 4]; 4];
            for (k, c) in colors.iter_mut().enumerate() {
                c.copy_from_slice(bytes(b, o + 0x20 + k * 4, 4)?);
            }
            l.frames.push(Frame {
                tag: be32(b, o)?,
                x: fx(be16(b, o + 4)?),
                y: fx(be16(b, o + 6)?),
                inner_w: fx(be16(b, o + 0x10)?),
                inner_h: fx(be16(b, o + 0x12)?),
                tex: [be16(b, o + 0x14)?, be16(b, o + 0x16)?, be16(b, o + 0x18)?, be16(b, o + 0x1A)?],
                flips: be8(b, o + 0x1C)?,
                colors,
            });
        }
        Ok(l)
    }

    pub fn pic(&self, t: u32) -> impl Iterator<Item = &Pic> {
        self.pics.iter().filter(move |p| p.tag == t)
    }

    pub fn text(&self, t: u32) -> Option<&Text> {
        self.texts.iter().find(|p| p.tag == t)
    }

    pub fn frame(&self, t: u32) -> impl Iterator<Item = &Frame> {
        self.frames.iter().filter(move |p| p.tag == t)
    }
}

/// One `STH0` string: the text pane it belongs to and its bytes.
#[derive(Clone, Debug, Default)]
pub struct Str {
    pub pane: u16,
    pub text: Vec<u8>,
}

#[derive(Clone, Debug, Default)]
pub struct Strings(pub Vec<Str>);

impl Strings {
    pub fn parse(b: &[u8]) -> Result<Strings> {
        if bytes(b, 0, 4)? != b"STH0" {
            return Err(Error::BadMagic(0));
        }
        let n = be16(b, 4)? as usize;
        let mut v = Vec::with_capacity(n);
        for i in 0..n {
            let e = 8 + i * 8;
            let len = be16(b, e + 2)? as usize;
            let off = e + be32(b, e + 4)? as usize;
            v.push(Str { pane: be16(b, e)?, text: bytes(b, off, len)?.iter().copied().take_while(|&c| c != 0).collect() });
        }
        Ok(Strings(v))
    }

    pub fn get(&self, i: usize) -> Option<&Str> {
        self.0.get(i)
    }
}

/// Every texture of a `TXH0` bank.
pub fn texture_bank(b: &[u8]) -> Result<Vec<Texture>> {
    if bytes(b, 0, 4)? != b"TXH0" {
        return Err(Error::BadMagic(0));
    }
    let n = be16(b, 4)? as usize;
    (0..n).map(|i| bti::decode_at(b, 0x20 + i * 0x20, String::new())).collect()
}

/// ROM offset of the Windows-1252 font (stored unscrambled).
pub const ANSI_FONT_ROM: usize = 0x1F_CF00;

/// The ROM font: glyph cells on 8-bit coverage sheets.
pub struct Font {
    pub first: u16,
    pub last: u16,
    pub ascent: u16,
    pub cell_w: u16,
    pub cell_h: u16,
    pub cols: u16,
    pub rows: u16,
    pub sheet_w: u16,
    pub sheet_h: u16,
    pub widths: Vec<u8>,
    /// Coverage 0..255 per pixel, one sheet after another.
    pub sheets: Vec<Vec<u8>>,
}

impl Font {
    pub fn parse_rom(rom: &[u8]) -> Result<Font> {
        let f = yay0::decompress(rom.get(ANSI_FONT_ROM..).ok_or(Error::Truncated(ANSI_FONT_ROM))?)?;
        Font::parse(&f)
    }

    pub fn parse(f: &[u8]) -> Result<Font> {
        let first = be16(f, 2)?;
        let last = be16(f, 4)?;
        let sheet_size = be32(f, 0x14)? as usize;
        if be16(f, 0x18)? != 0 || sheet_size == 0 {
            return Err(Error::Unsupported(be16(f, 0x18)? as u32));
        }
        let (sw, sh) = (be16(f, 0x1E)?, be16(f, 0x20)?);
        let wt = be16(f, 0x22)? as usize;
        let img = be32(f, 0x24)? as usize;
        let full = be32(f, 0x28)? as usize;
        let lut = [be8(f, 0x2C)?, be8(f, 0x2D)?, be8(f, 0x2E)?, be8(f, 0x2F)?];
        let src = bytes(f, img, full / 2)?;
        // 2 bits per pixel -> I4 in the same tile order.
        let mut i4 = Vec::with_capacity(src.len() * 2);
        for &b in src {
            i4.push((lut[(b >> 6) as usize] & 0xF0) | (lut[((b >> 4) & 3) as usize] & 0x0F));
            i4.push((lut[((b >> 2) & 3) as usize] & 0xF0) | (lut[(b & 3) as usize] & 0x0F));
        }
        let mut sheets = Vec::new();
        for chunk in i4.chunks_exact(sheet_size) {
            let rgba = gx::decode(chunk, gx::I4, sw as usize, sh as usize, None)?;
            sheets.push(rgba.chunks_exact(4).map(|p| p[3]).collect());
        }
        let n = (last - first + 1) as usize;
        Ok(Font {
            first,
            last,
            ascent: be16(f, 8)?,
            cell_w: be16(f, 0x10)?,
            cell_h: be16(f, 0x12)?,
            cols: be16(f, 0x1A)?,
            rows: be16(f, 0x1C)?,
            sheet_w: sw,
            sheet_h: sh,
            widths: bytes(f, wt, n)?.to_vec(),
            sheets,
        })
    }

    /// Glyph index for a Windows-1252 byte.
    pub fn glyph(&self, c: u8) -> Option<usize> {
        let c = c as u16;
        (c >= self.first && c <= self.last).then(|| (c - self.first) as usize)
    }

    pub fn advance(&self, c: u8) -> u8 {
        self.glyph(c).and_then(|g| self.widths.get(g).copied()).unwrap_or(0)
    }

    /// (sheet, x, y) of a glyph's cell.
    pub fn cell(&self, g: usize) -> (usize, u16, u16) {
        let per = (self.cols * self.rows) as usize;
        let k = g % per;
        (g / per, (k % self.cols as usize) as u16 * self.cell_w, (k / self.cols as usize) as u16 * self.cell_h)
    }
}
