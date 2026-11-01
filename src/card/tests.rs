//! Host tests (run from `tools/card-tool`: `cargo test`). Every card and
//! save here is made up in the test.

use super::*;
use alloc::string::ToString;

/// A made-up save of `blocks` blocks: RGB5A3 banner, a two-frame icon on
/// the shared CI8 palette, comments after the pictures.
fn sample(name: &str, blocks: u16) -> SaveFile {
    let mut d = [0xFFu8; DENTRY];
    d[0..4].copy_from_slice(b"GCOB");
    d[4..6].copy_from_slice(b"GC");
    d[7] = BANNER_RGB5A3 | 4;
    d[8..0x28].fill(0);
    d[8..8 + name.len()].copy_from_slice(name.as_bytes());
    d[0x28..0x2C].copy_from_slice(&1234u32.to_be_bytes());
    d[0x2C..0x30].copy_from_slice(&0u32.to_be_bytes());
    // Frames 0 and 1 shared CI8, speeds 1 (4 ticks) and 2 (8 ticks).
    d[0x30..0x32].copy_from_slice(&0b0101u16.to_be_bytes());
    d[0x32..0x34].copy_from_slice(&0b1001u16.to_be_bytes());
    d[0x34] = PERM_PUBLIC;
    d[0x35] = 0;
    d[0x36..0x38].copy_from_slice(&0u16.to_be_bytes());
    d[0x38..0x3A].copy_from_slice(&blocks.to_be_bytes());
    let comments = 0x1800 + 2 * 0x400 + 0x200;
    d[0x3C..0x40].copy_from_slice(&(comments as u32).to_be_bytes());
    // Pictures and comments need two blocks; a one-block save keeps the
    // first block of them only.
    let mut data = alloc::vec![0u8; blocks.max(2) as usize * BLOCK];
    // Banner: opaque pure red (RGB5A3 0x FC00) everywhere.
    for p in data[..0x1800].chunks_exact_mut(2) {
        p.copy_from_slice(&0xFC00u16.to_be_bytes());
    }
    // Frame 0 all index 1, frame 1 all index 2; palette 1 green, 2 blue.
    data[0x1800..0x1C00].fill(1);
    data[0x1C00..0x2000].fill(2);
    let pal = 0x2000;
    data[pal + 2..pal + 4].copy_from_slice(&0x83E0u16.to_be_bytes());
    data[pal + 4..pal + 6].copy_from_slice(&0x801Fu16.to_be_bytes());
    data[comments..comments + 9].copy_from_slice(b"Coob Save");
    data[comments + 0x20..comments + 0x24].copy_from_slice(b"Test");
    // Mark each block so reads can be checked.
    for (k, b) in data.chunks_exact_mut(BLOCK).enumerate() {
        b[BLOCK - 1] = k as u8 ^ name.len() as u8;
    }
    data.truncate(blocks as usize * BLOCK);
    SaveFile { dentry: Dentry(d), data }
}

#[test]
fn format_every_size() {
    for (mbit, user) in SIZES_MBIT.iter().zip([59u16, 123, 251, 507, 1019, 2043]) {
        let c = Card::format(*mbit, Encoding::Ansi, 0x1234_5678).unwrap();
        let c = Card::open(c.into_bytes()).unwrap();
        assert_eq!(c.user_blocks(), user);
        assert_eq!(c.free_blocks(), user);
        assert_eq!(c.free_entries(), DIR_ENTRIES);
        assert_eq!(c.size_mbit(), *mbit);
        assert_eq!(c.encoding(), Encoding::Ansi);
    }
    assert!(Card::format(5, Encoding::Ansi, 0).is_err());
}

#[test]
fn empty_directory_checksums() {
    // A blank directory (all 0xFF, counter 0) sums to 0xF003 / 0.
    let c = Card::format(16, Encoding::ShiftJis, 0).unwrap();
    let b = c.as_bytes();
    assert_eq!(&b[BLOCK + 0x1FFC..BLOCK + 0x2000], &[0xF0, 0x03, 0, 0]);
    assert_eq!(c.encoding(), Encoding::ShiftJis);
}

#[test]
fn rejects_bad_images() {
    assert_eq!(Card::open(alloc::vec![0; 1000]).err(), Some(Error::BadSize(1000)));
    let mut b = Card::format(4, Encoding::Ansi, 7).unwrap().into_bytes();
    b[3] ^= 1;
    assert_eq!(Card::open(b).err(), Some(Error::BadHeader));
    let mut b = Card::format(4, Encoding::Ansi, 7).unwrap().into_bytes();
    b[BLOCK + 5] ^= 1;
    b[2 * BLOCK + 5] ^= 1;
    assert_eq!(Card::open(b).err(), Some(Error::BadDirectory));
}

#[test]
fn import_read_delete() {
    let mut c = Card::format(4, Encoding::Ansi, 1).unwrap();
    let a = sample("alpha", 3);
    let b = sample("beta", 2);
    assert_eq!(c.import(&a).unwrap(), 0);
    assert_eq!(c.import(&b).unwrap(), 1);
    assert_eq!(c.chain(0).unwrap(), [5, 6, 7]);
    assert_eq!(c.chain(1).unwrap(), [8, 9]);
    assert_eq!(c.free_blocks(), 59 - 5);
    let c2 = Card::open(c.as_bytes().to_vec()).unwrap();
    let r = c2.read(0).unwrap();
    assert_eq!(r.data, a.data);
    assert_eq!(r.dentry.first_block(), 5);
    assert_eq!(c2.read(1).unwrap().data, b.data);
    assert_eq!(c.import(&a).err(), Some(Error::Exists));

    c.delete(0).unwrap();
    assert_eq!(c.free_blocks(), 59 - 2);
    assert!(c.entry(0).is_free());
    // Allocation carries on after the last block handed out, then wraps.
    let g = sample("gamma", 2);
    assert_eq!(c.import(&g).unwrap(), 0);
    assert_eq!(c.chain(0).unwrap(), [10, 11]);
    let big = sample("big", 52);
    c.import(&big).unwrap();
    assert_eq!(&c.chain(2).unwrap()[..4], &[12, 13, 14, 15]);
    assert_eq!(&c.chain(2).unwrap()[48..], &[60, 61, 62, 63]);
    let w = sample("wrap", 3);
    assert_eq!(c.import(&w).unwrap(), 3);
    assert_eq!(c.chain(3).unwrap(), [5, 6, 7]);
    assert_eq!(c.free_blocks(), 0);
    assert_eq!(c.import(&sample("full", 1)).err(), Some(Error::NoSpace));
    let c3 = Card::open(c.into_bytes()).unwrap();
    assert_eq!(c3.read(2).unwrap().data, big.data);
}

#[test]
fn directory_fills_up() {
    let mut c = Card::format(16, Encoding::Ansi, 1).unwrap();
    for i in 0..DIR_ENTRIES {
        c.import(&sample(&i.to_string(), 1)).unwrap();
    }
    assert_eq!(c.import(&sample("one more", 1)).err(), Some(Error::NoEntry));
}

#[test]
fn newer_copy_wins_and_backup_rescues() {
    let mut c = Card::format(4, Encoding::Ansi, 1).unwrap();
    c.import(&sample("alpha", 1)).unwrap();
    let mut b = c.into_bytes();
    // Make block 2 an older directory (empty, counter 0): block 1 wins.
    let blank = Card::format(4, Encoding::Ansi, 1).unwrap().into_bytes();
    b[2 * BLOCK..3 * BLOCK].copy_from_slice(&blank[2 * BLOCK..3 * BLOCK]);
    let c = Card::open(b.clone()).unwrap();
    assert_eq!(c.saves().count(), 1);
    // Damage block 1: the older copy in block 2 is used.
    b[BLOCK] ^= 0x40;
    let c = Card::open(b).unwrap();
    assert_eq!(c.saves().count(), 0);
}

#[test]
fn save_formats_round_trip() {
    let s = sample("alpha", 2);
    for f in SaveFormat::ALL {
        let bytes = s.to_bytes(f);
        assert_eq!(SaveFile::detect(&bytes), Some(f));
        let (r, rf) = SaveFile::parse(&bytes).unwrap();
        assert_eq!(rf, f);
        assert_eq!(r, s);
    }
    let gcs = s.to_bytes(SaveFormat::Gcs);
    assert_eq!(gcs.len(), 0x150 + 2 * BLOCK);
    assert_eq!(&gcs[..6], b"GCSAVE");
    assert_eq!(&gcs[0x110 + 0x38..0x110 + 0x3A], &[0, 1]);
    let sav = s.to_bytes(SaveFormat::Sav);
    assert_eq!(sav.len(), 0xC0 + 2 * BLOCK);
    assert_eq!(&sav[..12], b"DATELGC_SAVE");
    // Block count 0x0002 stored as 02 00, flags byte moved to 0x06.
    assert_eq!(&sav[0x80 + 0x38..0x80 + 0x3A], &[2, 0]);
    assert_eq!(sav[0x80 + 6], s.dentry.flags());
    assert_eq!(&sav[0x80..0x80 + 6], b"GCOBGC");
    // Wrong magic or length is refused.
    let mut bad = gcs.clone();
    bad[0] = b'X';
    assert_eq!(SaveFile::parse(&bad).err(), Some(Error::NotASave));
    assert_eq!(SaveFile::parse(&s.to_bytes(SaveFormat::Gci)[..BLOCK]).err(), Some(Error::NotASave));
}

#[test]
fn names_and_comments() {
    let s = sample("al:pha", 2);
    assert_eq!(s.dolphin_name(), "GC-GCOB-al-3Apha");
    let (t, d) = s.comments().unwrap();
    assert_eq!((t, d), (&b"Coob Save"[..], &b"Test"[..]));
}

#[test]
fn banner_and_icon() {
    let s = sample("alpha", 2);
    let b = s.banner().unwrap();
    assert_eq!(b.len(), 96 * 32 * 4);
    assert_eq!(&b[..4], &[255, 0, 0, 255]);
    let f = s.icon_frames();
    assert_eq!(f.len(), 2);
    assert_eq!((f[0].ticks, f[1].ticks), (4, 8));
    assert_eq!(&f[0].rgba[..4], &[0, 255, 0, 255]);
    assert_eq!(&f[1].rgba[..4], &[0, 0, 255, 255]);
    // A save cut short draws no icon instead of failing.
    let mut short = s.clone();
    short.data.truncate(0x1900);
    assert!(short.icon_frames().is_empty());
}
