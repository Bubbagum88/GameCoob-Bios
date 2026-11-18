//! UEFI configuration strings (spec "Configuration String Syntax"), the
//! text the config routing protocol speaks: `GUID=..&NAME=..&PATH=..`
//! (the header naming a variable store and its driver), then blocks of
//! `OFFSET=..&WIDTH=..&VALUE=..`.
//!
//! GUID and PATH are their bytes in memory order, two hex digits each;
//! NAME is UTF-16, four hex digits per character; OFFSET and WIDTH are
//! plain hex numbers; VALUE is the block's bytes as one little-endian
//! number, so last byte first.

use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;
use core::fmt::Write;

/// One `<ConfigResp>`: a store's header and its blocks.
#[derive(Clone, Debug, Default)]
pub struct Record {
    pub guid: [u8; 16],
    pub name: String,
    /// `GUID=..&NAME=..&PATH=..`, exactly as the firmware wrote it, for
    /// writing back to the same driver.
    pub hdr: String,
    /// An `ALTCFG` (default values) record, not the current values.
    pub alt: bool,
    pub blocks: Vec<(usize, Vec<u8>)>,
}

fn hex_bytes(s: &str) -> Vec<u8> {
    s.as_bytes()
        .chunks(2)
        .filter_map(|c| core::str::from_utf8(c).ok().and_then(|c| u8::from_str_radix(c, 16).ok()))
        .collect()
}

fn hex_num(s: &str) -> Option<usize> {
    usize::from_str_radix(s, 16).ok()
}

fn hex_name(s: &str) -> String {
    let units = s.as_bytes().chunks(4).filter_map(|c| core::str::from_utf8(c).ok().and_then(|c| u16::from_str_radix(c, 16).ok()));
    char::decode_utf16(units.take_while(|&u| u != 0)).map(|c| c.unwrap_or('?')).collect()
}

/// Split a `<MultiConfigResp>` (what ExportConfig returns) into records.
/// Damaged blocks are dropped; the rest still count.
pub fn parse(s: &str) -> Vec<Record> {
    let mut out: Vec<Record> = Vec::new();
    let mut cur: Option<Record> = None;
    // the header's parts so far, and the last OFFSET and WIDTH
    let (mut off, mut width): (Option<usize>, Option<usize>) = (None, None);
    let mut hdr_open = false;
    for kv in s.trim_end_matches('\0').split('&') {
        let (k, v) = kv.split_once('=').unwrap_or((kv, ""));
        match k {
            "GUID" => {
                if let Some(r) = cur.take() {
                    out.push(r);
                }
                let mut r = Record::default();
                let g = hex_bytes(v);
                if g.len() == 16 {
                    r.guid.copy_from_slice(&g);
                }
                r.hdr = format!("GUID={v}");
                hdr_open = true;
                cur = Some(r);
                (off, width) = (None, None);
            }
            "NAME" | "PATH" if hdr_open => {
                if let Some(r) = cur.as_mut() {
                    if k == "NAME" {
                        r.name = hex_name(v);
                    }
                    let _ = write!(r.hdr, "&{k}={v}");
                    if k == "PATH" {
                        hdr_open = false;
                    }
                }
            }
            "ALTCFG" => {
                hdr_open = false;
                if let Some(r) = cur.as_mut() {
                    r.alt = true;
                }
            }
            "OFFSET" => {
                hdr_open = false;
                (off, width) = (hex_num(v), None);
            }
            "WIDTH" => width = hex_num(v),
            "VALUE" => {
                if let (Some(r), Some(o), Some(w)) = (cur.as_mut(), off, width) {
                    let mut b = hex_bytes(v);
                    b.reverse();
                    b.resize(w, 0);
                    r.blocks.push((o, b));
                }
                (off, width) = (None, None);
            }
            _ => {}
        }
    }
    if let Some(r) = cur.take() {
        out.push(r);
    }
    out
}

/// `&OFFSET=..&WIDTH=..&VALUE=..` for `bytes` at `off`.
pub fn block(off: usize, bytes: &[u8]) -> String {
    let mut s = format!("&OFFSET={off:04x}&WIDTH={:04x}&VALUE=", bytes.len());
    for b in bytes.iter().rev() {
        let _ = write!(s, "{b:02x}");
    }
    s
}

/// A header for a store the export did not list (no PATH: the routing
/// protocol then can't find its driver, so this is only for display).
pub fn guid_hex(g: &[u8; 16]) -> String {
    let mut s = String::new();
    for b in g {
        let _ = write!(s, "{b:02x}");
    }
    s
}

/// The ranges of bytes that differ between `a` and `b` (same length), as
/// (offset, length), neighbours merged.
pub fn diff_runs(a: &[u8], b: &[u8]) -> Vec<(usize, usize)> {
    let mut out: Vec<(usize, usize)> = Vec::new();
    for i in 0..a.len().min(b.len()) {
        if a[i] != b[i] {
            match out.last_mut() {
                Some((o, n)) if *o + *n == i => *n += 1,
                _ => out.push((i, 1)),
            }
        }
    }
    out
}
