//! Single save files: a directory entry plus the save's blocks.
//!
//! | format | from | layout |
//! |--------|------|--------|
//! | `.gci` | Dolphin, Swiss, GCMM, MemCard PRO GC | entry (0x40), blocks |
//! | `.gcs` | GameShark / Interact "GameSaves" | 0x150 header: `GCSAVE` at 0, entry at 0x110 (its block count always 1), blocks |
//! | `.sav` | Action Replay / MaxDrive (Datel) | 0xC0 header: `DATELGC_SAVE` at 0, entry at 0x80 with byte pairs swapped, blocks |
//!
//! The header length is the file length modulo the 8 KB block, so the three
//! are told apart by size first and checked by magic second.

use super::{BLOCK, DENTRY, Dentry, Error, Result};
use alloc::string::String;
use alloc::vec::Vec;

const GCS_MAGIC: &[u8] = b"GCSAVE";
const GCS_HEADER: usize = 0x150;
const GCS_DENTRY: usize = 0x110;
const SAV_MAGIC: &[u8] = b"DATELGC_SAVE";
const SAV_HEADER: usize = 0xC0;
const SAV_DENTRY: usize = 0x80;
/// Entry bytes the Datel format stores swapped in pairs.
const SAV_SWAPPED: [usize; 11] = [0x06, 0x2C, 0x2E, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E];

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum SaveFormat {
    Gci,
    Gcs,
    Sav,
}

impl SaveFormat {
    pub const ALL: [SaveFormat; 3] = [SaveFormat::Gci, SaveFormat::Gcs, SaveFormat::Sav];

    /// Lower-case extension without the dot.
    pub fn ext(self) -> &'static str {
        match self {
            SaveFormat::Gci => "gci",
            SaveFormat::Gcs => "gcs",
            SaveFormat::Sav => "sav",
        }
    }

    pub fn from_ext(ext: &str) -> Option<SaveFormat> {
        SaveFormat::ALL.into_iter().find(|f| f.ext().eq_ignore_ascii_case(ext))
    }

    /// What the explorer calls it.
    pub fn label(self) -> &'static str {
        match self {
            SaveFormat::Gci => "GCI (Dolphin)",
            SaveFormat::Gcs => "GCS (GameShark)",
            SaveFormat::Sav => "SAV (Action Replay)",
        }
    }

    fn header_len(self) -> usize {
        match self {
            SaveFormat::Gci => DENTRY,
            SaveFormat::Gcs => GCS_HEADER,
            SaveFormat::Sav => SAV_HEADER,
        }
    }
}

/// A save outside any card. `data` is a whole number of blocks; the
/// entry's first-block field is whatever the card it came from had and is
/// replaced on import.
#[derive(Clone, PartialEq, Eq, Debug)]
pub struct SaveFile {
    pub dentry: Dentry,
    pub data: Vec<u8>,
}

fn swap_pairs(d: &mut [u8; DENTRY]) {
    for p in SAV_SWAPPED {
        d.swap(p, p + 1);
    }
}

impl SaveFile {
    /// Which format `bytes` is in, from its length and magic.
    pub fn detect(bytes: &[u8]) -> Option<SaveFormat> {
        let f = SaveFormat::ALL.into_iter().find(|f| bytes.len() % BLOCK == f.header_len() % BLOCK && bytes.len() > f.header_len())?;
        let ok = match f {
            SaveFormat::Gci => true,
            SaveFormat::Gcs => bytes.starts_with(GCS_MAGIC),
            SaveFormat::Sav => bytes.starts_with(SAV_MAGIC),
        };
        ok.then_some(f)
    }

    /// Read a save file of any of the three formats.
    pub fn parse(bytes: &[u8]) -> Result<(SaveFile, SaveFormat)> {
        let f = SaveFile::detect(bytes).ok_or(Error::NotASave)?;
        let (at, hdr) = match f {
            SaveFormat::Gci => (0, DENTRY),
            SaveFormat::Gcs => (GCS_DENTRY, GCS_HEADER),
            SaveFormat::Sav => (SAV_DENTRY, SAV_HEADER),
        };
        let mut dentry = Dentry::from_slice(&bytes[at..]);
        if f == SaveFormat::Sav {
            swap_pairs(&mut dentry.0);
        }
        let data = &bytes[hdr..];
        let blocks = data.len() / BLOCK;
        if f == SaveFormat::Gcs {
            // GameSaves keeps the real count in a separate .gsv file.
            dentry.set_block_count(blocks as u16);
        } else if dentry.block_count() as usize != blocks {
            return Err(Error::Truncated);
        }
        if dentry.is_free() || blocks == 0 || blocks > u16::MAX as usize {
            return Err(Error::NotASave);
        }
        Ok((SaveFile { dentry, data: data.to_vec() }, f))
    }

    /// The file's bytes in `f`.
    pub fn to_bytes(&self, f: SaveFormat) -> Vec<u8> {
        let mut out = Vec::with_capacity(f.header_len() + self.data.len());
        match f {
            SaveFormat::Gci => out.extend_from_slice(&self.dentry.0),
            SaveFormat::Gcs => {
                out.resize(GCS_HEADER, 0);
                out[..GCS_MAGIC.len()].copy_from_slice(GCS_MAGIC);
                let mut d = self.dentry;
                d.set_block_count(1);
                out[GCS_DENTRY..GCS_DENTRY + DENTRY].copy_from_slice(&d.0);
            }
            SaveFormat::Sav => {
                out.resize(SAV_HEADER, 0);
                out[..SAV_MAGIC.len()].copy_from_slice(SAV_MAGIC);
                let mut d = self.dentry.0;
                swap_pairs(&mut d);
                out[SAV_DENTRY..SAV_DENTRY + DENTRY].copy_from_slice(&d);
            }
        }
        out.extend_from_slice(&self.data);
        out
    }

    /// The name Dolphin gives the save in a GCI folder (without `.gci`):
    /// `MAKER-GAME-filename`, with characters FAT and NTFS refuse written
    /// as `-XX` hex escapes (`%` would read as an escape in some tools).
    pub fn dolphin_name(&self) -> String {
        let d = &self.dentry.0;
        let mut raw = Vec::new();
        raw.extend_from_slice(&d[4..6]);
        raw.push(b'-');
        raw.extend_from_slice(&d[0..4]);
        raw.push(b'-');
        raw.extend_from_slice(self.dentry.file_name());
        escape_file_name(&raw)
    }

    /// The two comment lines (title, description), raw bytes up to NUL.
    pub fn comments(&self) -> Option<(&[u8], &[u8])> {
        let o = self.dentry.comment_offset() as usize;
        let c = self.data.get(o..o.checked_add(0x40)?)?;
        let cut = |s: &'_ [u8]| -> usize { s.iter().position(|&b| b == 0).unwrap_or(s.len()) };
        let (a, b) = c.split_at(0x20);
        Some((&a[..cut(a)], &b[..cut(b)]))
    }
}

/// Make `raw` safe as a FAT/NTFS file name. Characters the file systems
/// refuse, control bytes and bytes above 0x7E become `-XX` (two hex
/// digits). Dolphin escapes the same characters, with its own scheme, so
/// names may differ from Dolphin's for saves with odd names; Dolphin reads
/// a GCI folder by content, not by name, so both work.
pub fn escape_file_name(raw: &[u8]) -> String {
    let mut s = String::with_capacity(raw.len());
    for &c in raw {
        let bad = c < 0x20 || c > 0x7E || b"\"*/:<>?\\|".contains(&c);
        if bad {
            const HEX: &[u8; 16] = b"0123456789ABCDEF";
            s.push('-');
            s.push(HEX[(c >> 4) as usize] as char);
            s.push(HEX[(c & 15) as usize] as char);
        } else {
            s.push(c as char);
        }
    }
    s
}
