//! What is on a disc, read through any byte reader: a GameCube disc (its
//! game code, title and the banner from `opening.bnr`), a PC disc
//! (ISO 9660 label and publisher, and whether El Torito lets UEFI boot it)
//! or something else.
//!
//! Shared by the boot app's optical drive backend and, later, the helper.
//! Nothing here touches a device: the caller gives a [`ReadAt`].

use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;

/// GameCube disc magic at 0x1C, and the Wii one at 0x18.
pub const GC_MAGIC: u32 = 0xC233_9F3D;
pub const WII_MAGIC: u32 = 0x5D1C_9EA3;

/// Banner picture size (RGB5A3, 4x4 tiles).
pub const BANNER_W: usize = 96;
pub const BANNER_H: usize = 32;
/// The FST is read only up to this size; real ones are well under it.
const MAX_FST: usize = 4 << 20;
const SECTOR: u64 = 2048;

/// Random access to a disc's bytes. False when the read failed.
pub trait ReadAt {
    fn read_at(&mut self, offset: u64, buf: &mut [u8]) -> bool;
}

impl ReadAt for &[u8] {
    fn read_at(&mut self, offset: u64, buf: &mut [u8]) -> bool {
        let Ok(o) = usize::try_from(offset) else { return false };
        match self.get(o..o + buf.len()) {
            Some(s) => {
                buf.copy_from_slice(s);
                true
            }
            None => false,
        }
    }
}

/// A banner's texts in one language.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct BannerText {
    pub short_name: String,
    pub short_maker: String,
    pub long_name: String,
    pub long_maker: String,
    pub description: String,
}

/// `opening.bnr`: the 96 x 32 picture as RGBA and the texts. BNR1 has
/// one language; BNR2 (PAL) six: English, German, French, Spanish,
/// Italian, Dutch.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Banner {
    pub rgba: Vec<u8>,
    pub texts: Vec<BannerText>,
}

impl Banner {
    /// The texts for menu language `lang` (the IPL's order: English,
    /// German, French, Spanish, Italian, Dutch), or the first ones.
    pub fn text(&self, lang: usize) -> Option<&BannerText> {
        self.texts.get(lang).or(self.texts.first())
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Disc {
    GameCube {
        /// Game code and maker code, e.g. `GALE01`.
        code: String,
        /// The header's title (empty when it isn't printable ASCII).
        title: String,
        banner: Option<Banner>,
    },
    Wii {
        code: String,
        title: String,
    },
    /// An ISO 9660 disc.
    Pc {
        label: String,
        publisher: String,
        /// El Torito has a UEFI boot image, so the firmware can start it.
        efi_boot: bool,
        /// El Torito has a BIOS (x86) boot image only.
        bios_boot: bool,
    },
    Unknown,
}

fn be32(b: &[u8], o: usize) -> u32 {
    u32::from_be_bytes([b[o], b[o + 1], b[o + 2], b[o + 3]])
}

fn le32(b: &[u8], o: usize) -> u32 {
    u32::from_le_bytes([b[o], b[o + 1], b[o + 2], b[o + 3]])
}

/// Bytes up to the first 0, as text: Windows-1252 bytes taken as
/// Latin-1 (the menu's font is Windows-1252, and turns them back).
pub fn latin1(b: &[u8]) -> String {
    let b = &b[..b.iter().position(|&c| c == 0).unwrap_or(b.len())];
    b.iter().map(|&c| c as char).collect::<String>().trim().into()
}

/// Identify the disc behind `r`. `None` when even the first sectors
/// can't be read.
pub fn identify(r: &mut impl ReadAt) -> Option<Disc> {
    let mut head = vec![0u8; 0x440];
    if !r.read_at(0, &mut head) {
        return None;
    }
    let printable = |b: &[u8]| {
        let t = &b[..b.iter().position(|&c| c == 0).unwrap_or(b.len())];
        if t.iter().all(|&c| (0x20..0x7F).contains(&c)) { latin1(t) } else { String::new() }
    };
    let code_ok = head[..6].iter().all(|c| c.is_ascii_alphanumeric());
    if be32(&head, 0x1C) == GC_MAGIC && code_ok {
        let code = latin1(&head[..6]);
        let title = printable(&head[0x20..0x400]);
        let banner = read_banner(r, &head);
        return Some(Disc::GameCube { code, title, banner });
    }
    if be32(&head, 0x18) == WII_MAGIC && code_ok {
        return Some(Disc::Wii { code: latin1(&head[..6]), title: printable(&head[0x20..0x60]) });
    }
    let mut pvd = vec![0u8; SECTOR as usize];
    if !r.read_at(16 * SECTOR, &mut pvd) || pvd[0] != 1 || &pvd[1..6] != b"CD001" {
        return Some(Disc::Unknown);
    }
    let label = latin1(&pvd[40..72]);
    let publisher = latin1(&pvd[318..446]);
    let (efi_boot, bios_boot) = el_torito(r);
    Some(Disc::Pc { label, publisher, efi_boot, bios_boot })
}

/// The El Torito boot catalog's platforms: (has UEFI, has x86 BIOS).
fn el_torito(r: &mut impl ReadAt) -> (bool, bool) {
    let mut br = vec![0u8; SECTOR as usize];
    if !r.read_at(17 * SECTOR, &mut br) || br[0] != 0 || &br[1..6] != b"CD001" || !br[7..].starts_with(b"EL TORITO SPECIFICATION") {
        return (false, false);
    }
    let mut cat = vec![0u8; SECTOR as usize];
    if !r.read_at(le32(&br, 0x47) as u64 * SECTOR, &mut cat) || cat[0] != 1 || cat[0x1E] != 0x55 || cat[0x1F] != 0xAA {
        return (false, false);
    }
    // The validation entry names the default entry's platform; each
    // section header (0x90, 0x91 = last) names its entries' platform.
    let (mut efi, mut bios) = (false, false);
    let mut platform = cat[1];
    for e in cat.chunks_exact(32) {
        match e[0] {
            0x01 => platform = e[1],
            0x90 | 0x91 => platform = e[1],
            0x88 => match platform {
                0xEF => efi = true,
                0 => bios = true,
                _ => {}
            },
            0x00 => {}
            _ => {}
        }
    }
    (efi, bios)
}

/// `opening.bnr` from the root of the disc's file system table.
fn read_banner(r: &mut impl ReadAt, head: &[u8]) -> Option<Banner> {
    let (fst_off, fst_len) = (be32(head, 0x424) as u64, be32(head, 0x428) as usize);
    if fst_len < 12 || fst_len > MAX_FST {
        return None;
    }
    let mut fst = vec![0u8; fst_len];
    if !r.read_at(fst_off, &mut fst) {
        return None;
    }
    let n = be32(&fst, 8) as usize;
    let names = n.checked_mul(12).filter(|&s| s <= fst.len())?;
    let name = |e: usize| {
        let o = names + (be32(&fst, e * 12) & 0xFF_FFFF) as usize;
        let s = fst.get(o..)?;
        Some(&s[..s.iter().position(|&c| c == 0).unwrap_or(s.len())])
    };
    let mut i = 1;
    while i < n {
        let dir = fst[i * 12] != 0;
        if !dir && name(i).is_some_and(|s| s.eq_ignore_ascii_case(b"opening.bnr")) {
            let (off, len) = (be32(&fst, i * 12 + 4) as u64, be32(&fst, i * 12 + 8) as usize);
            if !(0x1820 + 0x140..=0x1820 + 6 * 0x140).contains(&len) {
                return None;
            }
            let mut bnr = vec![0u8; len];
            return if r.read_at(off, &mut bnr) { parse_banner(&bnr) } else { None };
        }
        // A folder's third word is the index past its contents.
        i = if dir { (be32(&fst, i * 12 + 8) as usize).max(i + 1) } else { i + 1 };
    }
    None
}

/// A BNR1 or BNR2 file.
pub fn parse_banner(b: &[u8]) -> Option<Banner> {
    let langs = match b.get(..4)? {
        b"BNR1" => 1,
        b"BNR2" => 6,
        _ => return None,
    };
    if b.len() < 0x1820 + langs * 0x140 {
        return None;
    }
    let rgba = rgb5a3(&b[0x20..0x1820], BANNER_W, BANNER_H);
    let texts = (0..langs)
        .map(|k| {
            let t = &b[0x1820 + k * 0x140..];
            BannerText {
                short_name: latin1(&t[..0x20]),
                short_maker: latin1(&t[0x20..0x40]),
                long_name: latin1(&t[0x40..0x80]),
                long_maker: latin1(&t[0x80..0xC0]),
                description: latin1(&t[0xC0..0x140]),
            }
        })
        .collect();
    Some(Banner { rgba, texts })
}

/// Decode a GX RGB5A3 texture (4 x 4 texel tiles, big-endian texels).
pub fn rgb5a3(src: &[u8], w: usize, h: usize) -> Vec<u8> {
    let mut out = vec![0u8; w * h * 4];
    let mut s = src.chunks_exact(2).map(|p| u16::from_be_bytes([p[0], p[1]]));
    for ty in (0..h).step_by(4) {
        for tx in (0..w).step_by(4) {
            for y in ty..ty + 4 {
                for x in tx..tx + 4 {
                    let v = s.next().unwrap_or(0);
                    let px = if v & 0x8000 != 0 {
                        let c5 = |c: u16| ((c << 3) | (c >> 2)) as u8;
                        [c5((v >> 10) & 31), c5((v >> 5) & 31), c5(v & 31), 255]
                    } else {
                        let c4 = |c: u16| (c * 0x11) as u8;
                        let a = (v >> 12) & 7;
                        [c4((v >> 8) & 15), c4((v >> 4) & 15), c4(v & 15), ((a << 5) | (a << 2) | (a >> 1)) as u8]
                    };
                    if x < w && y < h {
                        out[(y * w + x) * 4..][..4].copy_from_slice(&px);
                    }
                }
            }
        }
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A made-up GameCube disc with a root folder, a sub folder and a
    /// BNR1 banner.
    pub fn gc_disc() -> Vec<u8> {
        let mut d = vec![0u8; 0x10000];
        d[..6].copy_from_slice(b"GTST01");
        d[0x1C..0x20].copy_from_slice(&GC_MAGIC.to_be_bytes());
        d[0x20..0x29].copy_from_slice(b"Test Game");
        // FST at 0x2000: root (4 entries), "sys" folder holding "a.bin",
        // then opening.bnr at 0x4000.
        let fst = 0x2000;
        let put = |d: &mut Vec<u8>, o: usize, v: u32| d[o..o + 4].copy_from_slice(&v.to_be_bytes());
        put(&mut d, fst, 0x0100_0000);
        put(&mut d, fst + 8, 4);
        put(&mut d, fst + 12, 0x0100_0000); // "sys", parent 0, next 3
        put(&mut d, fst + 20, 3);
        put(&mut d, fst + 24, 4); // "a.bin"
        put(&mut d, fst + 28, 0x100);
        put(&mut d, fst + 32, 4);
        put(&mut d, fst + 36, 10); // "opening.bnr"
        put(&mut d, fst + 40, 0x4000);
        put(&mut d, fst + 44, 0x1960);
        d[fst + 48..fst + 48 + 22].copy_from_slice(b"sys\0a.bin\0opening.bnr\0");
        put(&mut d, 0x424, fst as u32);
        put(&mut d, 0x428, 70);
        let b = 0x4000;
        d[b..b + 4].copy_from_slice(b"BNR1");
        // First texel opaque red, second half-transparent.
        d[b + 0x20..b + 0x22].copy_from_slice(&0xFC00u16.to_be_bytes());
        d[b + 0x22..b + 0x24].copy_from_slice(&0x4F00u16.to_be_bytes());
        d[b + 0x1820..b + 0x1824].copy_from_slice(b"Test");
        d[b + 0x1860..b + 0x186A].copy_from_slice(b"Test Game\xe9");
        d[b + 0x18A0..b + 0x18A8].copy_from_slice(b"Made Up!");
        d[b + 0x18E0..b + 0x18E5].copy_from_slice(b"Hello");
        d
    }

    #[test]
    fn gamecube_disc_and_banner() {
        let d = gc_disc();
        let Some(Disc::GameCube { code, title, banner }) = identify(&mut d.as_slice()) else { panic!() };
        assert_eq!((code.as_str(), title.as_str()), ("GTST01", "Test Game"));
        let b = banner.unwrap();
        assert_eq!(b.texts.len(), 1);
        let t = b.text(3).unwrap();
        assert_eq!(t.short_name, "Test");
        assert_eq!(t.long_name, "Test Game\u{e9}");
        assert_eq!(t.long_maker, "Made Up!");
        assert_eq!(t.description, "Hello");
        assert_eq!(&b.rgba[..4], &[255, 0, 0, 255]);
        assert_eq!(&b.rgba[4..8], &[255, 0, 0, 146]);
        assert_eq!(b.rgba.len(), BANNER_W * BANNER_H * 4);
    }

    #[test]
    fn gamecube_disc_without_banner() {
        let mut d = gc_disc();
        d[0x4000..0x4004].copy_from_slice(b"XXXX");
        assert!(matches!(identify(&mut d.as_slice()), Some(Disc::GameCube { banner: None, .. })));
        // A broken FST size doesn't read past the end.
        d[0x428..0x42C].copy_from_slice(&0xFFFF_FFFFu32.to_be_bytes());
        assert!(matches!(identify(&mut d.as_slice()), Some(Disc::GameCube { banner: None, .. })));
    }

    fn iso(platforms: &[u8]) -> Vec<u8> {
        let mut d = vec![0u8; 20 * 2048];
        let p = 16 * 2048;
        d[p] = 1;
        d[p + 1..p + 6].copy_from_slice(b"CD001");
        d[p + 40..p + 72].copy_from_slice(b"ARCH_202611                     ");
        d[p + 318..p + 330].copy_from_slice(b"ARCH LINUX  ");
        let b = 17 * 2048;
        d[b + 1..b + 6].copy_from_slice(b"CD001");
        d[b + 7..b + 30].copy_from_slice(b"EL TORITO SPECIFICATION");
        d[b + 0x47..b + 0x4B].copy_from_slice(&18u32.to_le_bytes());
        let c = 18 * 2048;
        d[c] = 1;
        d[c + 1] = platforms[0];
        d[c + 0x1E] = 0x55;
        d[c + 0x1F] = 0xAA;
        d[c + 32] = 0x88;
        for (k, &pl) in platforms[1..].iter().enumerate() {
            let e = c + 64 + k * 64;
            d[e] = if k + 2 == platforms.len() { 0x91 } else { 0x90 };
            d[e + 1] = pl;
            d[e + 32] = 0x88;
        }
        d
    }

    #[test]
    fn pc_discs() {
        let want = |efi, bios| Some(Disc::Pc { label: "ARCH_202611".into(), publisher: "ARCH LINUX".into(), efi_boot: efi, bios_boot: bios });
        assert_eq!(identify(&mut iso(&[0, 0xEF]).as_slice()), want(true, true));
        assert_eq!(identify(&mut iso(&[0]).as_slice()), want(false, true));
        assert_eq!(identify(&mut iso(&[0xEF]).as_slice()), want(true, false));
        let mut plain = iso(&[0]);
        plain[17 * 2048 + 7] = b'X';
        assert_eq!(identify(&mut plain.as_slice()), want(false, false));
        assert_eq!(identify(&mut vec![0u8; 40 * 2048].as_slice()), Some(Disc::Unknown));
        assert_eq!(identify(&mut [0u8; 16].as_slice()), None);
    }
}
