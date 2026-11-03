//! Title and game code from a disc image's header, where the format keeps
//! it readable without decompressing:
//! - `.iso`/`.gcm`: the plain disc, header at 0.
//! - `.ciso`: a 0x8000-byte block map, then the used blocks in order; the
//!   header is at 0x8000 when block 0 is present (map byte at 8).
//! - `.rvz`/`.wia`: an uncompressed copy of the disc's first 0x80 bytes in
//!   the file's second header, at 0x58.
//!
//! Other formats (`.gcz`, `.wbfs`, `.dol`) fall back to the file name.

use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::path::Path;

/// GameCube disc magic at 0x1C, and the Wii one at 0x18.
const GC_MAGIC: u32 = 0xC233_9F3D;
const WII_MAGIC: u32 = 0x5D1C_9EA3;

#[derive(Debug, PartialEq, Eq)]
pub struct Header {
    pub code: String,
    pub title: String,
}

/// Reads the header of a disc image, or None if the format hides it or
/// the bytes don't look like a disc.
pub fn read(path: &Path) -> Option<Header> {
    let mut f = File::open(path).ok()?;
    let mut head = [0u8; 0x80];
    let n = read_at(&mut f, 0, &mut head)?;
    let at = match &head[..4] {
        b"CISO" if n >= 9 && head[8] == 1 => 0x8000,
        b"WIA\x01" | b"RVZ\x01" => 0x58,
        _ => 0,
    };
    let mut disc = [0u8; 0x60];
    if read_at(&mut f, at, &mut disc)? < disc.len() {
        return None;
    }
    parse(&disc)
}

fn read_at(f: &mut File, at: u64, buf: &mut [u8]) -> Option<usize> {
    f.seek(SeekFrom::Start(at)).ok()?;
    let mut n = 0;
    while n < buf.len() {
        match f.read(&mut buf[n..]).ok()? {
            0 => break,
            k => n += k,
        }
    }
    Some(n)
}

/// The first 0x60 bytes of a disc: game code at 0, magics at 0x18/0x1C,
/// title from 0x20 up to a 0.
pub fn parse(disc: &[u8]) -> Option<Header> {
    let be = |o: usize| u32::from_be_bytes(disc[o..o + 4].try_into().unwrap());
    if be(0x1C) != GC_MAGIC && be(0x18) != WII_MAGIC {
        return None;
    }
    let code = &disc[..6];
    if !code.iter().all(|c| c.is_ascii_alphanumeric()) {
        return None;
    }
    let raw = &disc[0x20..];
    let raw = &raw[..raw.iter().position(|&b| b == 0).unwrap_or(raw.len())];
    // Japanese titles are Shift-JIS; anything outside printable ASCII
    // leaves the title to the file name.
    let title = if raw.iter().all(|&b| (0x20..0x7F).contains(&b)) {
        String::from_utf8_lossy(raw).trim().to_string()
    } else {
        String::new()
    };
    Some(Header { code: String::from_utf8_lossy(code).into_owned(), title })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn disc(code: &[u8; 6], title: &[u8]) -> Vec<u8> {
        let mut d = vec![0u8; 0x60];
        d[..6].copy_from_slice(code);
        d[0x1C..0x20].copy_from_slice(&GC_MAGIC.to_be_bytes());
        d[0x20..0x20 + title.len()].copy_from_slice(title);
        d
    }

    #[test]
    fn plain_header() {
        let h = parse(&disc(b"GTST01", b"Test Game  ")).unwrap();
        assert_eq!(h, Header { code: "GTST01".into(), title: "Test Game".into() });
        assert!(parse(&vec![0u8; 0x60]).is_none());
        assert_eq!(parse(&disc(b"GTST01", &[0x82, 0xA0])).unwrap().title, "");
    }

    #[test]
    fn container_formats() {
        let dir = std::env::temp_dir().join(format!("gcl-disc-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let d = disc(b"GABC01", b"Made Up");
        let want = Some(Header { code: "GABC01".into(), title: "Made Up".into() });

        let iso = dir.join("a.iso");
        std::fs::write(&iso, &d).unwrap();
        assert_eq!(read(&iso), want);

        let mut ciso = vec![0u8; 0x8000];
        ciso[..4].copy_from_slice(b"CISO");
        ciso[8] = 1;
        ciso.extend_from_slice(&d);
        let p = dir.join("a.ciso");
        std::fs::write(&p, &ciso).unwrap();
        assert_eq!(read(&p), want);

        let mut rvz = vec![0u8; 0x58];
        rvz[..4].copy_from_slice(b"RVZ\x01");
        rvz.extend_from_slice(&d);
        rvz.extend_from_slice(&[0u8; 0x20]);
        let p = dir.join("a.rvz");
        std::fs::write(&p, &rvz).unwrap();
        assert_eq!(read(&p), want);

        let p = dir.join("short.iso");
        std::fs::write(&p, b"tiny").unwrap();
        assert_eq!(read(&p), None);
        std::fs::remove_dir_all(&dir).unwrap();
    }
}
