//! Runtime access to the user's own GameCube IPL dump (`ipl.bin`).
//!
//! Nothing from the ROM is ever stored in this repository: everything here is
//! code that turns the dump into renderable data at boot.
//!
//! * [`Ipl::parse`] checks the dump, descrambles it and finds the resource
//!   archive that BS2 loads its models, textures and strings from.
//! * [`Ipl::resource`] returns one archive entry, Yay0-decompressed.
//! * [`j3d::Model::parse`] turns a J3D model (`J3D1bmd1` in every known IPL,
//!   `J3D2bmd2/3` also accepted) into joints, triangle lists, materials and
//!   RGBA8 textures.
//! * [`bti::parse`] decodes a standalone BTI texture resource.
//!
//! All parsers are panic-free on malformed input and report an [`Error`].
#![allow(dead_code)]

pub mod bti;
pub mod gx;
pub mod j3d;
pub mod yay0;
mod read;

use alloc::borrow::Cow;
use alloc::vec::Vec;

/// Size of a GameCube IPL ROM dump.
pub const ROM_SIZE: usize = 2 * 1024 * 1024;
/// First scrambled byte of the ROM.
pub const SCRAMBLE_START: usize = 0x100;
/// End of the scrambled region (the fonts after it are stored in the clear).
pub const SCRAMBLE_END: usize = 0x1A_FF00;
/// ROM offset that BS1 copies to [`BS2_RAM`].
pub const BS2_ROM: usize = 0x820;
/// RAM address BS2 runs at.
pub const BS2_RAM: u32 = 0x8130_0000;
/// Where the resource archive sits in the PAL 1.0 dump (RAM 0x81381820).
pub const PAL10_ARCHIVE_ROM: usize = 0x8_2040;

/// Why parsing failed.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    /// The dump is not 2 MiB.
    BadSize(usize),
    /// No resource archive was found in the descrambled ROM.
    NoArchive,
    /// A resource id past the end of the archive.
    NoSuchResource(u16),
    /// Data ended before a structure it describes (offset that was needed).
    Truncated(usize),
    /// A magic number or header did not match (offset of the header).
    BadMagic(usize),
    /// A value outside what the format allows (offset of the value).
    BadValue(usize),
    /// A valid but unsupported feature (e.g. a GX texture format id).
    Unsupported(u32),
}

pub type Result<T> = core::result::Result<T, Error>;

/// A descrambled IPL dump with its resource archive located.
pub struct Ipl {
    rom: Vec<u8>,
    archive: usize,
    offsets: Vec<u32>,
}

impl Ipl {
    /// Check the size, descramble ROM 0x100..0x1AFF00 and locate the
    /// resource archive (known PAL 1.0 location first, then a scan).
    pub fn parse(rom: &[u8]) -> Result<Ipl> {
        if rom.len() != ROM_SIZE {
            return Err(Error::BadSize(rom.len()));
        }
        let mut rom = rom.to_vec();
        descramble(&mut rom[SCRAMBLE_START..SCRAMBLE_END]);
        let archive = if archive_at(&rom, PAL10_ARCHIVE_ROM).is_some() {
            PAL10_ARCHIVE_ROM
        } else {
            find_archive(&rom).ok_or(Error::NoArchive)?
        };
        let offsets = archive_at(&rom, archive).ok_or(Error::NoArchive)?;
        Ok(Ipl { rom, archive, offsets })
    }

    /// The whole descrambled ROM.
    pub fn rom(&self) -> &[u8] {
        &self.rom
    }

    /// The descrambled BS2 image: index 0 is RAM [`BS2_RAM`] (ROM 0x820).
    pub fn bs2(&self) -> &[u8] {
        &self.rom[BS2_ROM..SCRAMBLE_END]
    }

    /// Translate a BS2 RAM address (0x813xxxxx) to a ROM offset.
    pub fn ram_to_rom(addr: u32) -> Option<usize> {
        let off = addr.checked_sub(BS2_RAM)? as usize + BS2_ROM;
        (off < SCRAMBLE_END).then_some(off)
    }

    /// ROM offset of the resource archive (its u32 entry count).
    pub fn archive_offset(&self) -> usize {
        self.archive
    }

    /// Number of entries in the resource archive.
    pub fn resource_count(&self) -> usize {
        self.offsets.len()
    }

    /// ROM offset and stored (possibly compressed) length of entry `id`.
    pub fn resource_span(&self, id: u16) -> Option<(usize, usize)> {
        let i = id as usize;
        let start = self.archive + *self.offsets.get(i)? as usize;
        // Entries are in ascending order; the last one runs to the end of the
        // scrambled region (its own header bounds it further).
        let end = match self.offsets.get(i + 1) {
            Some(&o) => self.archive + o as usize,
            None => SCRAMBLE_END,
        };
        Some((start, end.saturating_sub(start)))
    }

    /// Entry `id`'s bytes, Yay0-decompressed when stored compressed. An
    /// uncompressed entry is borrowed (it may include alignment padding).
    pub fn resource(&self, id: u16) -> Result<Cow<'_, [u8]>> {
        let (start, len) = self.resource_span(id).ok_or(Error::NoSuchResource(id))?;
        let raw = &self.rom[start..start + len];
        if yay0::is_yay0(raw) {
            Ok(Cow::Owned(yay0::decompress(raw)?))
        } else {
            Ok(Cow::Borrowed(raw))
        }
    }
}

/// XOR `data` (ROM 0x100..0x1AFF00) with the IPL keystream: three 16-bit
/// LFSRs, as documented by segher and used by Dolphin and MAME.
pub fn descramble(data: &mut [u8]) {
    let (mut t, mut u, mut v): (u32, u32, u32) = (0x2953, 0xD9C2, 0x3FF1);
    let mut x: u32 = 1;
    for byte in data.iter_mut() {
        let mut acc: u32 = 0;
        for _ in 0..8 {
            let t0 = t & 1;
            let t1 = (t >> 1) & 1;
            let u0 = u & 1;
            let u1 = (u >> 1) & 1;
            let v0 = v & 1;
            x ^= t1 ^ v0;
            x ^= u0 | u1;
            x ^= (t0 ^ u1 ^ v0) & (t0 ^ u0);
            // Branch-free LFSR steps: v steps when t0 == u0, u when t0 == 0.
            let step_v = 0u32.wrapping_sub(1 ^ (t0 ^ u0));
            v = (v & !step_v) | (((v >> 1) ^ (0xB3D0 & 0u32.wrapping_sub(v0))) & step_v);
            let step_u = 0u32.wrapping_sub(1 ^ t0);
            u = (u & !step_u) | (((u >> 1) ^ (0xFB10 & 0u32.wrapping_sub(u0))) & step_u);
            t = (t >> 1) ^ (0xA740 & 0u32.wrapping_sub(t0));
            acc = (acc << 1) | (x & 1);
        }
        *byte ^= acc as u8;
    }
}

/// Validate a resource archive header at `off`: a u32 count in 100..400
/// followed by strictly increasing u32 offsets (relative to `off`) that all
/// land inside the scrambled region, the first entry being an `STH0` table.
fn archive_at(rom: &[u8], off: usize) -> Option<Vec<u32>> {
    let count = read::be32(rom, off).ok()? as usize;
    if !(100..400).contains(&count) {
        return None;
    }
    let table_end = off + 4 + count * 4;
    if table_end > SCRAMBLE_END {
        return None;
    }
    let mut offsets = Vec::with_capacity(count);
    let mut prev = 0u32;
    for i in 0..count {
        let o = read::be32(rom, off + 4 + i * 4).ok()?;
        if o <= prev || off + o as usize >= SCRAMBLE_END || (i == 0 && (o as usize) < 4 + count * 4) {
            return None;
        }
        offsets.push(o);
        prev = o;
    }
    let first = off + offsets[0] as usize;
    (rom.get(first..first + 4)? == b"STH0").then_some(offsets)
}

/// Scan a descrambled ROM (BS2 region, 4-byte aligned) for the resource
/// archive; [`Ipl::parse`] falls back to this for unknown IPL versions.
pub fn find_archive(rom: &[u8]) -> Option<usize> {
    (BS2_ROM..SCRAMBLE_END - 8).step_by(4).find(|&off| {
        // Cheap pre-check before the full validation.
        matches!(read::be32(rom, off), Ok(c) if (100..400).contains(&c))
            && archive_at(rom, off).is_some()
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn keystream_prefix() {
        // First keystream bytes, computed with the reference Python port.
        let mut z = [0u8; 8];
        descramble(&mut z);
        assert_eq!(z, [0x89, 0x7e, 0x47, 0x7f, 0xf4, 0x42, 0x3f, 0xe2]);
    }

    #[test]
    fn ram_mapping() {
        assert_eq!(Ipl::ram_to_rom(0x8138_1820), Some(PAL10_ARCHIVE_ROM));
        assert_eq!(Ipl::ram_to_rom(0x8000_0000), None);
    }
}
