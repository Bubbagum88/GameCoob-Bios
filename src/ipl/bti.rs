//! BTI textures: a 0x20-byte header followed (at offsets relative to the
//! header) by an optional palette and the image data. TEX1 sections of J3D
//! models use the same header.

use super::gx::{self, Texture};
use super::read::{at, be16, be32, be8, bytes};
use super::{Error, Result};
use alloc::string::String;

/// The fields of a BTI header that matter for decoding and sampling.
#[derive(Clone, Copy, Debug)]
pub struct Header {
    pub format: u8,
    pub width: u16,
    pub height: u16,
    pub wrap_s: u8,
    pub wrap_t: u8,
    pub palette_format: u8,
    pub palette_count: u16,
    /// Relative to the header.
    pub palette_offset: u32,
    pub min_filter: u8,
    pub mag_filter: u8,
    pub mip_count: u8,
    /// Relative to the header.
    pub data_offset: u32,
}

/// Parse the header at `off`.
pub fn header(buf: &[u8], off: usize) -> Result<Header> {
    bytes(buf, off, 0x20)?;
    Ok(Header {
        format: be8(buf, off)?,
        width: be16(buf, off + 2)?,
        height: be16(buf, off + 4)?,
        wrap_s: be8(buf, off + 6)?,
        wrap_t: be8(buf, off + 7)?,
        palette_format: be8(buf, off + 9)?,
        palette_count: be16(buf, off + 0x0A)?,
        palette_offset: be32(buf, off + 0x0C)?,
        min_filter: be8(buf, off + 0x14)?,
        mag_filter: be8(buf, off + 0x15)?,
        mip_count: be8(buf, off + 0x18)?,
        data_offset: be32(buf, off + 0x1C)?,
    })
}

/// Decode the texture whose header is at `off` in `buf`.
pub fn decode_at(buf: &[u8], off: usize, name: String) -> Result<Texture> {
    let h = header(buf, off)?;
    let (w, ht) = (h.width as usize, h.height as usize);
    if w == 0 || ht == 0 || w > 1024 || ht > 1024 {
        return Err(Error::BadValue(off + 2));
    }
    let size = gx::data_size(h.format, w, ht).ok_or(Error::Unsupported(h.format as u32))?;
    let data = bytes(buf, at(off, h.data_offset)?, size)?;
    let palette = if gx::is_paletted(h.format) {
        let p = bytes(buf, at(off, h.palette_offset)?, h.palette_count as usize * 2)?;
        Some(gx::decode_palette(p, h.palette_format, h.palette_count as usize)?)
    } else {
        None
    };
    let rgba = gx::decode(data, h.format, w, ht, palette.as_deref())?;
    Ok(Texture {
        name,
        format: h.format,
        width: h.width,
        height: h.height,
        wrap_s: h.wrap_s,
        wrap_t: h.wrap_t,
        min_filter: h.min_filter,
        mag_filter: h.mag_filter,
        mip_count: h.mip_count,
        rgba,
    })
}

/// Decode a standalone BTI resource (e.g. IPL resource 0x65, I8 64x64).
pub fn parse(buf: &[u8]) -> Result<Texture> {
    decode_at(buf, 0, String::new())
}
