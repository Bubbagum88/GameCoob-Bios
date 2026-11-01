//! GameCube memory cards and save files, as the PC sees them: raw card
//! images (`.raw`, `.mcp`, `.gcp`: the card's flash, byte for byte) and
//! single saves (`.gci`, `.gcs`, `.sav`, see [`save`]).
//!
//! A card is a row of 8 KB blocks. Block 0 is the header, blocks 1-2 are
//! two copies of the directory and blocks 3-4 two copies of the block
//! allocation table (BAT); saves start at block 5. Each directory and BAT
//! copy carries an update counter, and the copy with the higher one (as an
//! `s16`, the way the console compares them) is the live one. Every change
//! here writes the new state, with the counter bumped, to both copies.
//!
//! Nothing in this module touches a disk or the screen: it turns bytes into
//! bytes, so the explorer, the card backend behind the Memory Card screen
//! and the host tool (`tools/card-tool`) share it.

pub mod icon;
pub mod save;

#[cfg(test)]
mod tests;

use alloc::vec;
use alloc::vec::Vec;
use core::fmt;

pub use save::{SaveFile, SaveFormat};

/// Bytes per block.
pub const BLOCK: usize = 0x2000;
/// Header, two directories and two BATs.
pub const SYSTEM_BLOCKS: u16 = 5;
/// Directory entries per card.
pub const DIR_ENTRIES: usize = 127;
/// Bytes per directory entry.
pub const DENTRY: usize = 0x40;
/// Blocks per megabit of card.
pub const BLOCKS_PER_MBIT: usize = 16;
/// Card sizes in megabits: the 59, 123, 251, 507, 1019 and 2043 block cards.
pub const SIZES_MBIT: [u16; 6] = [4, 8, 16, 32, 64, 128];
/// BAT entries (blocks 5..0x1000).
const BAT_ENTRIES: usize = 0xFFB;

/// What went wrong reading or changing a card or save.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Error {
    /// Not the size of any memory card (bytes).
    BadSize(usize),
    /// The header's checksums are wrong: the console asks to format.
    BadHeader,
    /// The header gives a different size than the image has.
    SizeMismatch,
    /// Neither directory copy has good checksums.
    BadDirectory,
    /// Neither BAT copy has good checksums.
    BadBat,
    /// A save's block chain leaves the card, loops or ends early.
    BadChain(usize),
    /// Not a save file of any known format.
    NotASave,
    /// A save's data is shorter than its directory entry says.
    Truncated,
    /// The card holds a save with the same game, maker and file name.
    Exists,
    /// Not enough free blocks.
    NoSpace,
    /// All 127 directory entries are used.
    NoEntry,
    /// No save at that directory index.
    NoFile,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::BadSize(n) => write!(f, "not a memory card size ({n} bytes)"),
            Error::BadHeader => f.write_str("card header is damaged"),
            Error::SizeMismatch => f.write_str("card header gives another size"),
            Error::BadDirectory => f.write_str("card directory is damaged"),
            Error::BadBat => f.write_str("card block table is damaged"),
            Error::BadChain(i) => write!(f, "save {i} has a broken block chain"),
            Error::NotASave => f.write_str("not a GCI, GCS or SAV save"),
            Error::Truncated => f.write_str("save data is cut short"),
            Error::Exists => f.write_str("the card holds this save already"),
            Error::NoSpace => f.write_str("not enough free blocks"),
            Error::NoEntry => f.write_str("no free directory entry"),
            Error::NoFile => f.write_str("no save there"),
        }
    }
}

pub type Result<T> = core::result::Result<T, Error>;

fn be16(b: &[u8], o: usize) -> u16 {
    u16::from_be_bytes([b[o], b[o + 1]])
}

fn be32(b: &[u8], o: usize) -> u32 {
    u32::from_be_bytes([b[o], b[o + 1], b[o + 2], b[o + 3]])
}

fn put16(b: &mut [u8], o: usize, v: u16) {
    b[o..o + 2].copy_from_slice(&v.to_be_bytes());
}

/// The card's two checksums over `data`: the sum of its big-endian words
/// and the sum of their complements, each 0xFFFF turned into 0.
pub fn checksums(data: &[u8]) -> (u16, u16) {
    let (mut sum, mut inv) = (0u16, 0u16);
    for w in data.chunks_exact(2) {
        let v = u16::from_be_bytes([w[0], w[1]]);
        sum = sum.wrapping_add(v);
        inv = inv.wrapping_add(!v);
    }
    (if sum == 0xFFFF { 0 } else { sum }, if inv == 0xFFFF { 0 } else { inv })
}

/// Text encoding of a card's names and comments.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Encoding {
    /// Windows-1252: cards formatted on US and PAL consoles.
    Ansi,
    /// Shift JIS: cards formatted on Japanese consoles.
    ShiftJis,
}

// ---------------------------------------------------------------------
// Directory entries
// ---------------------------------------------------------------------

/// Banner formats (low two bits of the flags byte).
pub const BANNER_NONE: u8 = 0;
pub const BANNER_CI8: u8 = 1;
pub const BANNER_RGB5A3: u8 = 2;
/// Icon formats (two bits per frame).
pub const ICON_NONE: u8 = 0;
pub const ICON_CI8_SHARED: u8 = 1;
pub const ICON_RGB5A3: u8 = 2;
pub const ICON_CI8_UNIQUE: u8 = 3;
/// Permission bits.
pub const PERM_PUBLIC: u8 = 0x04;
pub const PERM_NO_COPY: u8 = 0x08;
pub const PERM_NO_MOVE: u8 = 0x10;

/// One 0x40-byte directory entry, kept as the bytes the card holds.
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Dentry(pub [u8; DENTRY]);

impl fmt::Debug for Dentry {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Dentry({}-{}-{:?})", ascii(&self.0[4..6]), ascii(&self.0[0..4]), ascii(self.file_name()))
    }
}

fn ascii(b: &[u8]) -> alloc::string::String {
    b.iter().map(|&c| if (0x20..0x7F).contains(&c) { c as char } else { '.' }).collect()
}

impl Dentry {
    /// A free entry: all 0xFF.
    pub const FREE: Dentry = Dentry([0xFF; DENTRY]);

    pub fn from_slice(b: &[u8]) -> Dentry {
        let mut d = [0; DENTRY];
        d.copy_from_slice(&b[..DENTRY]);
        Dentry(d)
    }

    /// Free entries start with an 0xFF game code byte.
    pub fn is_free(&self) -> bool {
        self.0[0] == 0xFF
    }

    pub fn game(&self) -> [u8; 4] {
        [self.0[0], self.0[1], self.0[2], self.0[3]]
    }

    pub fn maker(&self) -> [u8; 2] {
        [self.0[4], self.0[5]]
    }

    /// Low two bits: banner format. Bit 2: icon plays forwards then back.
    pub fn flags(&self) -> u8 {
        self.0[7]
    }

    pub fn banner_format(&self) -> u8 {
        self.0[7] & 3
    }

    pub fn ping_pong(&self) -> bool {
        self.0[7] & 4 != 0
    }

    /// The file name, up to its first NUL.
    pub fn file_name(&self) -> &[u8] {
        let n = &self.0[8..0x28];
        &n[..n.iter().position(|&c| c == 0).unwrap_or(n.len())]
    }

    /// Seconds since 2000-01-01.
    pub fn modified(&self) -> u32 {
        be32(&self.0, 0x28)
    }

    /// Where the banner and icons start in the save's data.
    pub fn image_offset(&self) -> u32 {
        be32(&self.0, 0x2C)
    }

    pub fn icon_formats(&self) -> u16 {
        be16(&self.0, 0x30)
    }

    pub fn anim_speeds(&self) -> u16 {
        be16(&self.0, 0x32)
    }

    pub fn permissions(&self) -> u8 {
        self.0[0x34]
    }

    pub fn copy_count(&self) -> u8 {
        self.0[0x35]
    }

    pub fn first_block(&self) -> u16 {
        be16(&self.0, 0x36)
    }

    pub fn block_count(&self) -> u16 {
        be16(&self.0, 0x38)
    }

    /// Where the two 32-byte comment lines start in the save's data.
    pub fn comment_offset(&self) -> u32 {
        be32(&self.0, 0x3C)
    }

    pub fn set_first_block(&mut self, b: u16) {
        put16(&mut self.0, 0x36, b);
    }

    pub fn set_block_count(&mut self, n: u16) {
        put16(&mut self.0, 0x38, n);
    }

    /// The console's test for "the same save": game, maker and file name
    /// (compared up to the name's first NUL, as Dolphin does too).
    pub fn same_save(&self, o: &Dentry) -> bool {
        self.0[0..6] == o.0[0..6] && self.file_name() == o.file_name()
    }
}

// ---------------------------------------------------------------------
// Card images
// ---------------------------------------------------------------------

/// A whole card image in memory.
#[derive(Clone)]
pub struct Card {
    img: Vec<u8>,
    /// Live directory and BAT blocks (1 or 2, 3 or 4).
    dir: usize,
    bat: usize,
}

/// Image length of a card of `mbit` megabits.
pub const fn image_len(mbit: u16) -> usize {
    mbit as usize * BLOCKS_PER_MBIT * BLOCK
}

/// The update counter as the console compares it.
fn counter(b: &[u8], o: usize) -> i16 {
    be16(b, o) as i16
}

impl Card {
    /// Check an image read from a file. The header, one good directory and
    /// one good BAT are needed; save chains are checked when a save is read.
    pub fn open(img: Vec<u8>) -> Result<Card> {
        let mbit = SIZES_MBIT.iter().copied().find(|&m| image_len(m) == img.len()).ok_or(Error::BadSize(img.len()))?;
        let h = &img[..BLOCK];
        if checksums(&h[..0x1FC]) != (be16(h, 0x1FC), be16(h, 0x1FE)) {
            return Err(Error::BadHeader);
        }
        if be16(h, 0x22) != mbit {
            return Err(Error::SizeMismatch);
        }
        let dir_ok = |b: usize| {
            let d = &img[b * BLOCK..(b + 1) * BLOCK];
            checksums(&d[..0x1FFC]) == (be16(d, 0x1FFC), be16(d, 0x1FFE))
        };
        let bat_ok = |b: usize| {
            let d = &img[b * BLOCK..(b + 1) * BLOCK];
            checksums(&d[4..]) == (be16(d, 0), be16(d, 2))
        };
        let pick = |a: usize, b: usize, ok: &dyn Fn(usize) -> bool, ctr: usize, err: Error| match (ok(a), ok(b)) {
            (true, true) => Ok(if counter(&img, a * BLOCK + ctr) >= counter(&img, b * BLOCK + ctr) { a } else { b }),
            (true, false) => Ok(a),
            (false, true) => Ok(b),
            _ => Err(err),
        };
        let dir = pick(1, 2, &dir_ok, 0x1FFA, Error::BadDirectory)?;
        let bat = pick(3, 4, &bat_ok, 4, Error::BadBat)?;
        Ok(Card { img, dir, bat })
    }

    /// A freshly formatted card, as `CARDFormat` lays it out (with a zero
    /// flash ID). `ticks` is the format time in console ticks and seeds the
    /// card's serial.
    pub fn format(mbit: u16, enc: Encoding, ticks: u64) -> Result<Card> {
        if !SIZES_MBIT.contains(&mbit) {
            return Err(Error::BadSize(image_len(mbit)));
        }
        let mut img = vec![0xFFu8; image_len(mbit)];
        let h = &mut img[..BLOCK];
        let mut rand = ticks;
        for i in 0..12 {
            rand = rand.wrapping_mul(0x41C6_4E6D).wrapping_add(0x3039) >> 16;
            h[i] = rand as u8;
            rand = (rand.wrapping_mul(0x41C6_4E6D).wrapping_add(0x3039) >> 16) & 0x7FFF;
        }
        h[0x0C..0x14].copy_from_slice(&ticks.to_be_bytes());
        h[0x14..0x22].fill(0); // SRAM bias, language, DTV status, device ID
        put16(h, 0x22, mbit);
        put16(h, 0x24, if enc == Encoding::ShiftJis { 1 } else { 0 });
        let (s, i) = checksums(&h[..0x1FC]);
        put16(h, 0x1FC, s);
        put16(h, 0x1FE, i);
        // Directories: all free, counter 0. BATs: nothing used.
        for b in [1, 2] {
            let d = &mut img[b * BLOCK..(b + 1) * BLOCK];
            put16(d, 0x1FFA, 0);
            seal_dir(d);
        }
        let total = (mbit as usize * BLOCKS_PER_MBIT) as u16;
        for b in [3, 4] {
            let d = &mut img[b * BLOCK..(b + 1) * BLOCK];
            d.fill(0);
            put16(d, 6, total - SYSTEM_BLOCKS);
            put16(d, 8, SYSTEM_BLOCKS - 1);
            seal_bat(d);
        }
        // Save blocks stay erased (0xFF), as flash reads when blank.
        Ok(Card { img, dir: 1, bat: 3 })
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.img
    }

    pub fn into_bytes(self) -> Vec<u8> {
        self.img
    }

    fn header(&self) -> &[u8] {
        &self.img[..BLOCK]
    }

    pub fn size_mbit(&self) -> u16 {
        be16(self.header(), 0x22)
    }

    /// All blocks, the five system blocks included.
    pub fn total_blocks(&self) -> u16 {
        (self.img.len() / BLOCK) as u16
    }

    /// Blocks saves can use (59, 123, 251...).
    pub fn user_blocks(&self) -> u16 {
        self.total_blocks() - SYSTEM_BLOCKS
    }

    pub fn free_blocks(&self) -> u16 {
        be16(self.bat_block(), 6)
    }

    pub fn encoding(&self) -> Encoding {
        if be16(self.header(), 0x24) & 1 != 0 { Encoding::ShiftJis } else { Encoding::Ansi }
    }

    /// The 12-byte serial and the format time (ticks).
    pub fn serial(&self) -> ([u8; 12], u64) {
        let h = self.header();
        let mut s = [0; 12];
        s.copy_from_slice(&h[..12]);
        (s, u64::from_be_bytes(h[0x0C..0x14].try_into().unwrap()))
    }

    fn dir_block(&self) -> &[u8] {
        &self.img[self.dir * BLOCK..(self.dir + 1) * BLOCK]
    }

    fn bat_block(&self) -> &[u8] {
        &self.img[self.bat * BLOCK..(self.bat + 1) * BLOCK]
    }

    /// Directory entry `i` (free entries included).
    pub fn entry(&self, i: usize) -> Dentry {
        Dentry::from_slice(&self.dir_block()[i * DENTRY..])
    }

    /// The used entries with their directory index.
    pub fn saves(&self) -> impl Iterator<Item = (usize, Dentry)> + '_ {
        (0..DIR_ENTRIES).map(|i| (i, self.entry(i))).filter(|(_, d)| !d.is_free())
    }

    pub fn free_entries(&self) -> usize {
        (0..DIR_ENTRIES).filter(|&i| self.entry(i).is_free()).count()
    }

    /// The next block after `b` in the BAT (0 = free, 0xFFFF = last).
    fn next(&self, b: u16) -> u16 {
        be16(self.bat_block(), 0x0A + (b - SYSTEM_BLOCKS) as usize * 2)
    }

    /// The blocks save `i` sits in, in order.
    pub fn chain(&self, i: usize) -> Result<Vec<u16>> {
        let d = self.entry(i);
        if d.is_free() {
            return Err(Error::NoFile);
        }
        let n = d.block_count() as usize;
        if n == 0 {
            return Err(Error::BadChain(i));
        }
        let mut out = Vec::with_capacity(n);
        let mut b = d.first_block();
        for k in 0..n {
            if b < SYSTEM_BLOCKS || b >= self.total_blocks() || out.contains(&b) {
                return Err(Error::BadChain(i));
            }
            out.push(b);
            let nx = self.next(b);
            if k + 1 == n {
                // The console marks the end with 0xFFFF; don't refuse a
                // save whose chain is otherwise good if it doesn't.
                break;
            }
            b = nx;
        }
        Ok(out)
    }

    /// Save `i` with its data, ready to write out as GCI, GCS or SAV.
    pub fn read(&self, i: usize) -> Result<SaveFile> {
        let blocks = self.chain(i)?;
        let mut data = Vec::with_capacity(blocks.len() * BLOCK);
        for b in blocks {
            data.extend_from_slice(&self.img[b as usize * BLOCK..(b as usize + 1) * BLOCK]);
        }
        Ok(SaveFile { dentry: self.entry(i), data })
    }

    /// A save on this card that is the same save as `d`.
    pub fn find(&self, d: &Dentry) -> Option<usize> {
        self.saves().find(|(_, e)| e.same_save(d)).map(|(i, _)| i)
    }

    /// Write `save` to the card in free blocks and a free entry (the
    /// lowest one, as `CARDCreate` picks). Returns its directory index.
    pub fn import(&mut self, save: &SaveFile) -> Result<usize> {
        let n = save.dentry.block_count();
        if n == 0 || save.data.len() < n as usize * BLOCK {
            return Err(Error::Truncated);
        }
        if self.find(&save.dentry).is_some() {
            return Err(Error::Exists);
        }
        let slot = (0..DIR_ENTRIES).find(|&i| self.entry(i).is_free()).ok_or(Error::NoEntry)?;
        if self.free_blocks() < n {
            return Err(Error::NoSpace);
        }
        let mut bat = self.bat_block().to_vec();
        let blocks = alloc_blocks(&mut bat, self.total_blocks(), n).ok_or(Error::NoSpace)?;
        for (k, &b) in blocks.iter().enumerate() {
            let at = b as usize * BLOCK;
            self.img[at..at + BLOCK].copy_from_slice(&save.data[k * BLOCK..(k + 1) * BLOCK]);
        }
        let mut d = save.dentry;
        d.set_first_block(blocks[0]);
        let mut dir = self.dir_block().to_vec();
        dir[slot * DENTRY..(slot + 1) * DENTRY].copy_from_slice(&d.0);
        self.commit(dir, bat);
        Ok(slot)
    }

    /// Erase save `i`: free its blocks and its entry.
    pub fn delete(&mut self, i: usize) -> Result<()> {
        // A save with a broken chain can still be erased: free what can be
        // followed.
        let blocks = match self.chain(i) {
            Ok(b) => b,
            Err(Error::BadChain(_)) => Vec::new(),
            Err(e) => return Err(e),
        };
        let mut bat = self.bat_block().to_vec();
        for &b in &blocks {
            put16(&mut bat, 0x0A + (b - SYSTEM_BLOCKS) as usize * 2, 0);
        }
        let free = be16(&bat, 6) + blocks.len() as u16;
        put16(&mut bat, 6, free);
        let mut dir = self.dir_block().to_vec();
        dir[i * DENTRY..(i + 1) * DENTRY].fill(0xFF);
        self.commit(dir, bat);
        Ok(())
    }

    /// Write a new directory and BAT to both copies, counters bumped.
    fn commit(&mut self, mut dir: Vec<u8>, mut bat: Vec<u8>) {
        let dc = (be16(&dir, 0x1FFA)).wrapping_add(1);
        put16(&mut dir, 0x1FFA, dc);
        seal_dir(&mut dir);
        let bc = (be16(&bat, 4)).wrapping_add(1);
        put16(&mut bat, 4, bc);
        seal_bat(&mut bat);
        for b in [1, 2] {
            self.img[b * BLOCK..(b + 1) * BLOCK].copy_from_slice(&dir);
        }
        for b in [3, 4] {
            self.img[b * BLOCK..(b + 1) * BLOCK].copy_from_slice(&bat);
        }
        (self.dir, self.bat) = (1, 3);
    }
}

fn seal_dir(d: &mut [u8]) {
    let (s, i) = checksums(&d[..0x1FFC]);
    put16(d, 0x1FFC, s);
    put16(d, 0x1FFE, i);
}

fn seal_bat(d: &mut [u8]) {
    let (s, i) = checksums(&d[4..]);
    put16(d, 0, s);
    put16(d, 2, i);
}

/// Take `n` free blocks the way `__CARDAllocBlock` does: scan on from the
/// last block handed out, wrapping to block 5, and chain them in that order.
fn alloc_blocks(bat: &mut [u8], total: u16, n: u16) -> Option<Vec<u16>> {
    let free = be16(bat, 6);
    if free < n {
        return None;
    }
    let ent = |b: u16| 0x0A + (b - SYSTEM_BLOCKS) as usize * 2;
    let mut cur = be16(bat, 8);
    let mut out: Vec<u16> = Vec::with_capacity(n as usize);
    let mut scanned = 0usize;
    while out.len() < n as usize {
        scanned += 1;
        if scanned > BAT_ENTRIES + 1 {
            return None; // the free count lied
        }
        cur = cur.wrapping_add(1);
        if cur < SYSTEM_BLOCKS || cur >= total {
            cur = SYSTEM_BLOCKS;
        }
        if be16(bat, ent(cur)) == 0 {
            if let Some(&prev) = out.last() {
                put16(bat, ent(prev), cur);
            }
            put16(bat, ent(cur), 0xFFFF);
            out.push(cur);
        }
    }
    put16(bat, 6, free - n);
    put16(bat, 8, cur);
    Some(out)
}
