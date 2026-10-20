//! Bounds-checked big-endian reads.

use super::{Error, Result};

pub fn bytes(b: &[u8], off: usize, len: usize) -> Result<&[u8]> {
    let end = off.checked_add(len).ok_or(Error::Truncated(off))?;
    b.get(off..end).ok_or(Error::Truncated(off))
}

pub fn be8(b: &[u8], off: usize) -> Result<u8> {
    b.get(off).copied().ok_or(Error::Truncated(off))
}

pub fn be16(b: &[u8], off: usize) -> Result<u16> {
    let s = bytes(b, off, 2)?;
    Ok(u16::from_be_bytes([s[0], s[1]]))
}

pub fn bei16(b: &[u8], off: usize) -> Result<i16> {
    be16(b, off).map(|v| v as i16)
}

pub fn be32(b: &[u8], off: usize) -> Result<u32> {
    let s = bytes(b, off, 4)?;
    Ok(u32::from_be_bytes([s[0], s[1], s[2], s[3]]))
}

pub fn bef32(b: &[u8], off: usize) -> Result<f32> {
    be32(b, off).map(f32::from_bits)
}

/// `base + rel` with overflow turned into an error.
pub fn at(base: usize, rel: u32) -> Result<usize> {
    base.checked_add(rel as usize).ok_or(Error::BadValue(base))
}
