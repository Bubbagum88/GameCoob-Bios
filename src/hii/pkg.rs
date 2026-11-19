//! HII package lists as `EFI_HII_DATABASE_PROTOCOL.ExportPackageLists`
//! hands them out: a run of package lists, each a GUID, a length and its
//! packages. Only the form (IFR) and string packages matter here.

use alloc::collections::BTreeMap;
use alloc::string::String;
use alloc::vec::Vec;

const PKG_FORMS: u8 = 0x02;
const PKG_STRINGS: u8 = 0x04;
const PKG_END: u8 = 0xDF;

/// One package list: its form packages (IFR, header stripped) and the
/// strings of the language picked for it.
pub struct PackageList {
    pub guid: [u8; 16],
    pub forms: Vec<Vec<u8>>,
    pub strings: Strings,
}

/// String id to text, for one language.
#[derive(Default)]
pub struct Strings {
    pub lang: String,
    map: BTreeMap<u16, String>,
}

impl Strings {
    pub fn get(&self, id: u16) -> &str {
        self.map.get(&id).map_or("", |s| s.as_str())
    }
}

pub fn u16le(b: &[u8], at: usize) -> u16 {
    b.get(at..at + 2).map_or(0, |s| u16::from_le_bytes([s[0], s[1]]))
}

pub fn u32le(b: &[u8], at: usize) -> u32 {
    b.get(at..at + 4).map_or(0, |s| u32::from_le_bytes([s[0], s[1], s[2], s[3]]))
}

pub fn guid_at(b: &[u8], at: usize) -> [u8; 16] {
    let mut g = [0; 16];
    if let Some(s) = b.get(at..at + 16) {
        g.copy_from_slice(s);
    }
    g
}

/// Split an export into package lists. Bad lengths end the walk, keeping
/// what was read so far.
pub fn parse(mut b: &[u8]) -> Vec<PackageList> {
    let mut out = Vec::new();
    while b.len() >= 20 {
        let guid = guid_at(b, 0);
        let len = u32le(b, 16) as usize;
        if len < 20 || len > b.len() {
            break;
        }
        out.push(list(guid, &b[20..len]));
        b = &b[len..];
    }
    out
}

fn list(guid: [u8; 16], mut b: &[u8]) -> PackageList {
    let mut forms = Vec::new();
    // (language, strings) per string package
    let mut langs: Vec<(String, BTreeMap<u16, String>)> = Vec::new();
    while b.len() >= 4 {
        let hdr = u32le(b, 0);
        let (len, ty) = ((hdr & 0x00FF_FFFF) as usize, (hdr >> 24) as u8);
        if len < 4 || len > b.len() || ty == PKG_END {
            break;
        }
        let p = &b[..len];
        match ty {
            PKG_FORMS => forms.push(p[4..].to_vec()),
            PKG_STRINGS => {
                if let Some(s) = string_package(p) {
                    langs.push(s);
                }
            }
            _ => {}
        }
        b = &b[len..];
    }
    let pick = langs
        .iter()
        .position(|(l, _)| l.eq_ignore_ascii_case("en-US"))
        .or_else(|| langs.iter().position(|(l, _)| l.starts_with("en")))
        .or_else(|| langs.iter().position(|(l, _)| !l.starts_with("x-")))
        .or(if langs.is_empty() { None } else { Some(0) });
    let strings = match pick {
        Some(i) => {
            let (lang, map) = langs.swap_remove(i);
            Strings { lang, map }
        }
        None => Strings::default(),
    };
    PackageList { guid, forms, strings }
}

/// EFI_HII_STRING_PACKAGE_HDR and its string blocks.
fn string_package(p: &[u8]) -> Option<(String, BTreeMap<u16, String>)> {
    let hdr_size = u32le(p, 4) as usize;
    let info = u32le(p, 8) as usize;
    // Header(4) HdrSize(4) StringInfoOffset(4) LanguageWindow(32) LanguageName(2) Language
    let lang_bytes = p.get(46..hdr_size.min(p.len()))?;
    let lang: String = lang_bytes.iter().take_while(|&&c| c != 0).map(|&c| c as char).collect();
    let mut map = BTreeMap::new();
    let mut id: u16 = 1;
    let mut at = info;
    let ucs2 = |at: usize| -> (String, usize) {
        let mut s = String::new();
        let mut i = at;
        let mut units = Vec::new();
        while i + 1 < p.len() {
            let c = u16le(p, i);
            i += 2;
            if c == 0 {
                break;
            }
            units.push(c);
        }
        for c in char::decode_utf16(units.iter().copied()) {
            s.push(c.unwrap_or('?'));
        }
        (s, i)
    };
    let scsu = |at: usize| -> (String, usize) {
        let mut i = at;
        let mut s = String::new();
        while i < p.len() {
            let c = p[i];
            i += 1;
            if c == 0 {
                break;
            }
            s.push(if c < 0x80 { c as char } else { '?' });
        }
        (s, i)
    };
    while at < p.len() {
        let ty = p[at];
        match ty {
            0x00 => break,
            // SCSU, SCSU_FONT
            0x10 | 0x11 => {
                let (s, next) = scsu(at + 1 + usize::from(ty == 0x11));
                map.insert(id, s);
                id = id.wrapping_add(1);
                at = next;
            }
            // STRINGS_SCSU, STRINGS_SCSU_FONT
            0x12 | 0x13 => {
                let f = usize::from(ty == 0x13);
                let n = u16le(p, at + 1 + f);
                at += 3 + f;
                for _ in 0..n {
                    let (s, next) = scsu(at);
                    map.insert(id, s);
                    id = id.wrapping_add(1);
                    at = next;
                }
            }
            // UCS2, UCS2_FONT
            0x14 | 0x15 => {
                let (s, next) = ucs2(at + 1 + usize::from(ty == 0x15));
                map.insert(id, s);
                id = id.wrapping_add(1);
                at = next;
            }
            // STRINGS_UCS2, STRINGS_UCS2_FONT
            0x16 | 0x17 => {
                let f = usize::from(ty == 0x17);
                let n = u16le(p, at + 1 + f);
                at += 3 + f;
                for _ in 0..n {
                    let (s, next) = ucs2(at);
                    map.insert(id, s);
                    id = id.wrapping_add(1);
                    at = next;
                }
            }
            // DUPLICATE: this id is a copy of an earlier one
            0x20 => {
                let of = u16le(p, at + 1);
                let s = map.get(&of).cloned().unwrap_or_default();
                map.insert(id, s);
                id = id.wrapping_add(1);
                at += 3;
            }
            // SKIP2, SKIP1
            0x21 => {
                id = id.wrapping_add(u16le(p, at + 1));
                at += 3;
            }
            0x22 => {
                id = id.wrapping_add(u16::from(p.get(at + 1).copied().unwrap_or(0)));
                at += 2;
            }
            // EXT1, EXT2, EXT4 (fonts and such): skip by their length
            0x30 => at += usize::from(p.get(at + 2).copied().unwrap_or(0)).max(3),
            0x31 => at += usize::from(u16le(p, at + 2)).max(4),
            0x32 => at += (u32le(p, at + 2) as usize).max(6),
            _ => break,
        }
    }
    Some((lang, map))
}
