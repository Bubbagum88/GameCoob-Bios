//! The PC's file systems as the Card Files explorer sees them: the volumes
//! the firmware can read (FAT on UEFI) and their folders and files.
//!
//! The menu only talks to [`Volumes`]; the UEFI app implements it over
//! SimpleFileSystem (`src/fs.rs`) and the host tools over folders.
//! Paths are absolute within a volume, `\`-separated, `\` being the root.

use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;

/// One readable volume.
#[derive(Clone, Debug)]
pub struct Volume {
    /// Stays the same for the same volume across boots (on UEFI, the
    /// device path as text).
    pub id: String,
    /// The volume label, if it has one.
    pub label: String,
    /// Bytes.
    pub size: u64,
    pub free: u64,
    /// The volume GameCoob booted from.
    pub boot: bool,
}

/// One folder entry.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Entry {
    pub name: String,
    pub dir: bool,
    pub size: u64,
}

pub trait Volumes {
    /// The volumes, the boot volume first. Re-read on each call (a USB
    /// stick may have come or gone).
    fn volumes(&mut self) -> Vec<Volume>;
    /// The entries of folder `path` on volume `id`, without `.` and `..`.
    fn list(&mut self, id: &str, path: &str) -> Result<Vec<Entry>, String>;
    fn read(&mut self, id: &str, path: &str) -> Result<Vec<u8>, String>;
    /// Create or replace a file.
    fn write(&mut self, id: &str, path: &str, bytes: &[u8]) -> Result<(), String>;
    /// Create a folder (its parent must exist); fine if it exists.
    fn mkdir(&mut self, id: &str, path: &str) -> Result<(), String>;
    /// The card image last left in `slot` (a [`handle`]): `None` if never
    /// set, `Some("")` if the slot was left empty. Kept across boots.
    fn slot_memory(&mut self, _slot: usize) -> Option<String> {
        None
    }
    /// Remember `handle` (or `""`, empty) for `slot`.
    fn set_slot_memory(&mut self, _slot: usize, _handle: &str) -> Result<(), String> {
        Ok(())
    }
}

/// A file's handle for the card backend: volume id and path in one string.
pub fn handle(id: &str, path: &str) -> String {
    format!("{id}|{path}")
}

/// Undo [`handle`].
pub fn split_handle(h: &str) -> (&str, &str) {
    h.rsplit_once('|').unwrap_or(("", h))
}

/// `dir` and `name` joined.
pub fn join(dir: &str, name: &str) -> String {
    if dir.ends_with('\\') { format!("{dir}{name}") } else { format!("{dir}\\{name}") }
}

/// The last part of `path`.
pub fn file_name(path: &str) -> &str {
    path.rsplit('\\').next().unwrap_or(path)
}

/// Lower-case extension of `name`, without the dot.
pub fn ext(name: &str) -> String {
    name.rsplit_once('.').map(|(_, e)| e.to_ascii_lowercase()).unwrap_or_default()
}

/// A name in `dir` not taken by `taken`: `stem.ext`, else `stem-2.ext`...
pub fn free_name(taken: &[Entry], stem: &str, ext: &str) -> String {
    let has = |n: &str| taken.iter().any(|e| e.name.eq_ignore_ascii_case(n));
    let first = format!("{stem}.{ext}");
    if !has(&first) {
        return first;
    }
    (2..).map(|i| format!("{stem}-{i}.{ext}")).find(|n| !has(n)).unwrap()
}

/// Bytes as the explorer shows them: `512 KB`, `16 MB`, `7.5 GB`.
pub fn size_text(b: u64) -> String {
    const K: u64 = 1024;
    if b < K * K {
        format!("{} KB", b.div_ceil(K))
    } else if b < K * K * K {
        format!("{} MB", b / (K * K))
    } else {
        let tenths = b * 10 / (K * K * K);
        format!("{}.{} GB", tenths / 10, tenths % 10)
    }
}
