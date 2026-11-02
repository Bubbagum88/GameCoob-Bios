//! Real memory cards for the Memory Card screen: card images (`.raw`,
//! `.mcp`, `.gcp`) the user loads from a disk, one per slot, read and
//! changed through [`crate::card`]. This replaces BS2's card thread
//! (`memcard_card.c`) the way [`super::backend::FakeCards`] does, but on
//! the user's own data.
//!
//! Nothing here reads or writes files: the driver inserts an image's bytes
//! with [`ImageCards::insert`] and, after an operation changed a card,
//! takes the new bytes from [`ImageCards::take_changed`] to write back.

use super::backend::{CardError, CardOp, FastTexRc, MemCards, Save, SlotState, DIR_ENTRIES};
use super::super::shader::FastTex;
use crate::card::icon::{BANNER_H, BANNER_W, ICON_H, ICON_W};
use crate::card::{self, Card, Encoding, PERM_NO_COPY, PERM_NO_MOVE};
use alloc::rc::Rc;
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;

/// Frames an operation takes, so the screen's flights and bursts play out
/// as they do on the console (same as the fake cards).
const OP_FRAMES: u32 = 90;
/// Frames an inserted card takes to mount.
const MOUNT_FRAMES: u32 = 20;

/// What a slot holds.
enum Media {
    Empty,
    /// A card image that opened.
    Card(Card),
    /// A file that isn't a usable card: its size in megabits when it has
    /// a card's size (formatting can then fix it).
    Bad(Option<u16>),
}

struct Slot {
    media: Media,
    /// Where the image came from (shown by the explorer, used for writing
    /// back). Opaque here.
    name: String,
    files: Vec<Option<Save>>,
    version: u32,
    mounting: u32,
    /// The image changed since the driver last took it.
    changed: bool,
}

impl Slot {
    fn new() -> Slot {
        Slot { media: Media::Empty, name: String::new(), files: vec![None; DIR_ENTRIES], version: 1, mounting: 0, changed: false }
    }

    fn card(&self) -> Option<&Card> {
        match (&self.media, self.mounting) {
            (Media::Card(c), 0) => Some(c),
            _ => None,
        }
    }

    /// Re-read the directory into `files`.
    fn refresh(&mut self) {
        self.files = vec![None; DIR_ENTRIES];
        if let Media::Card(c) = &self.media {
            let enc = c.encoding();
            for (i, _) in c.saves() {
                // A save with a broken chain still shows, without pictures,
                // so it can be erased.
                self.files[i] = Some(match c.read(i) {
                    Ok(s) => to_save(&s, enc),
                    Err(_) => to_save(&card::SaveFile { dentry: c.entry(i), data: Vec::new() }, enc),
                });
            }
        }
        self.version += 1;
    }
}

/// Card text to a `String` the menu's font code turns back into bytes:
/// Windows-1252 bytes map one to one onto U+0000..U+00FF (as
/// [`super::super::ui::cp1252`] expects); Shift JIS keeps ASCII only for
/// now.
pub fn card_text(b: &[u8], enc: Encoding) -> String {
    b.iter()
        .map(|&c| match enc {
            Encoding::Ansi => c as char,
            Encoding::ShiftJis if c < 0x80 => c as char,
            Encoding::ShiftJis => '?',
        })
        .collect()
}

fn to_save(s: &card::SaveFile, enc: Encoding) -> Save {
    let d = &s.dentry;
    let (title, comment) = s.comments().unwrap_or((&[], &[]));
    let tex = |w, h, rgba: &[u8]| -> FastTexRc { Rc::new(FastTex::from_rgba(w, h, rgba)) };
    Save {
        game: d.game(),
        maker: d.maker(),
        file_name: card_text(d.file_name(), enc),
        title: card_text(title, enc),
        comment: card_text(comment, enc),
        blocks: d.block_count(),
        modified: d.modified(),
        banner: s.banner().map(|b| tex(BANNER_W, BANNER_H, &b)),
        icons: s.icon_frames().into_iter().map(|f| (tex(ICON_W, ICON_H, &f.rgba), f.ticks)).collect(),
        ping_pong: d.ping_pong(),
        no_copy: d.permissions() & PERM_NO_COPY != 0,
        no_move: d.permissions() & PERM_NO_MOVE != 0,
    }
}

/// Two slots of card images.
pub struct ImageCards {
    slots: [Slot; 2],
    /// A card pulled with the slot key, put back by the next press.
    stash: [Option<(String, Vec<u8>)>; 2],
    busy: Option<(CardOp, usize, usize, u32)>,
    result: Option<Result<(), CardError>>,
    dest: Option<usize>,
}

impl Default for ImageCards {
    fn default() -> Self {
        ImageCards::new()
    }
}

impl ImageCards {
    pub fn new() -> ImageCards {
        ImageCards { slots: [Slot::new(), Slot::new()], stash: [None, None], busy: None, result: Some(Ok(())), dest: None }
    }

    /// Put an image in `slot`, replacing what was there. `name` is the
    /// driver's handle for the file. Images that don't open still go in:
    /// the screen then says the card is damaged and offers to format it,
    /// as the console does.
    pub fn insert(&mut self, slot: usize, name: String, bytes: Vec<u8>) {
        self.stash[slot & 1] = None;
        let s = &mut self.slots[slot & 1];
        let len = bytes.len();
        s.media = match Card::open(bytes) {
            Ok(c) => Media::Card(c),
            Err(_) => Media::Bad(card::SIZES_MBIT.iter().copied().find(|&m| card::image_len(m) == len)),
        };
        s.name = name;
        s.mounting = MOUNT_FRAMES;
        s.changed = false;
        s.files = vec![None; DIR_ENTRIES];
        s.version += 1;
    }

    /// Pull the card from `slot`. Returns the image's handle and its bytes
    /// if it changed and wasn't taken yet.
    pub fn eject(&mut self, slot: usize) -> Option<(String, Vec<u8>)> {
        let out = self.changed_bytes(slot);
        self.stash[slot & 1] = None;
        let s = &mut self.slots[slot & 1];
        s.media = Media::Empty;
        s.name.clear();
        s.mounting = 0;
        s.files = vec![None; DIR_ENTRIES];
        s.version += 1;
        out
    }

    /// The handle of the image in `slot`, if any.
    pub fn name(&self, slot: usize) -> Option<&str> {
        let s = &self.slots[slot & 1];
        (!matches!(s.media, Media::Empty)).then_some(s.name.as_str())
    }

    /// The image in `slot` as it now is, for the explorer's export.
    pub fn card(&self, slot: usize) -> Option<&Card> {
        self.slots[slot & 1].card()
    }

    /// Change the card in `slot` outside the Memory Card screen (the
    /// explorer's import and erase). The directory is re-read after.
    pub fn edit<T>(&mut self, slot: usize, f: impl FnOnce(&mut Card) -> T) -> Option<T> {
        if self.busy.is_some() {
            return None;
        }
        let s = &mut self.slots[slot & 1];
        if s.mounting > 0 {
            return None;
        }
        let Media::Card(c) = &mut s.media else { return None };
        let out = f(c);
        s.changed = true;
        s.refresh();
        Some(out)
    }

    /// If the card in `slot` changed, its handle and new bytes (once).
    fn changed_bytes(&mut self, slot: usize) -> Option<(String, Vec<u8>)> {
        let s = &mut self.slots[slot & 1];
        if !s.changed {
            return None;
        }
        s.changed = false;
        match &s.media {
            Media::Card(c) => Some((s.name.clone(), c.as_bytes().to_vec())),
            _ => None,
        }
    }

    fn run(&mut self, op: CardOp, slot: usize, file: usize) -> Result<(), CardError> {
        let other = slot ^ 1;
        if op == CardOp::Format {
            let s = &mut self.slots[slot];
            let (mbit, enc, ticks) = match &s.media {
                Media::Card(c) => (c.size_mbit(), c.encoding(), c.serial().1),
                Media::Bad(Some(m)) => (*m, Encoding::Ansi, 0),
                _ => return Err(CardError::Failed),
            };
            // Keep the format time so the serial stays the card's own.
            s.media = Media::Card(Card::format(mbit, enc, ticks).map_err(|_| CardError::Failed)?);
            s.changed = true;
            return Ok(());
        }
        let src = self.slots[slot].card().ok_or(CardError::Failed)?;
        let save = src.read(file).map_err(|_| CardError::Failed)?;
        let enc = src.encoding();
        if op == CardOp::Erase {
            let Media::Card(c) = &mut self.slots[slot].media else { return Err(CardError::Failed) };
            c.delete(file).map_err(|_| CardError::Failed)?;
            self.slots[slot].changed = true;
            return Ok(());
        }
        let perm = save.dentry.permissions();
        if (op == CardOp::Copy && perm & PERM_NO_COPY != 0) || (op == CardOp::Move && perm & PERM_NO_MOVE != 0) {
            return Err(CardError::NoPermission);
        }
        let dst = match &mut self.slots[other].media {
            Media::Empty => return Err(CardError::NoCard),
            Media::Bad(..) => return Err(CardError::WrongDevice),
            Media::Card(c) => c,
        };
        if dst.encoding() != enc {
            return Err(CardError::WrongType);
        }
        let at = dst.import(&save).map_err(|e| match e {
            card::Error::Exists => CardError::Exists,
            card::Error::NoSpace | card::Error::NoEntry => CardError::NoSpace,
            _ => CardError::Failed,
        })?;
        self.slots[other].changed = true;
        self.dest = Some(at);
        if op == CardOp::Move {
            if let Media::Card(c) = &mut self.slots[slot].media {
                c.delete(file).map_err(|_| CardError::Failed)?;
            }
            self.slots[slot].changed = true;
        }
        Ok(())
    }
}

impl MemCards for ImageCards {
    fn update(&mut self) {
        if let Some((op, slot, file, t)) = self.busy {
            if t > 1 {
                self.busy = Some((op, slot, file, t - 1));
            } else {
                self.busy = None;
                self.result = Some(self.run(op, slot, file));
                for s in self.slots.iter_mut() {
                    s.refresh();
                }
            }
        }
        for s in self.slots.iter_mut() {
            if s.mounting > 0 {
                s.mounting -= 1;
                if s.mounting == 0 {
                    s.refresh();
                }
            }
        }
    }

    fn state(&self, slot: usize) -> SlotState {
        let s = &self.slots[slot & 1];
        match &s.media {
            Media::Empty => SlotState::Empty,
            _ if s.mounting > 0 => SlotState::Mounting,
            Media::Card(_) => SlotState::Ready,
            // A file of a card's size that doesn't read as a card: the
            // console's "damaged, format?" case. Anything else isn't a card.
            Media::Bad(Some(_)) => SlotState::Broken,
            Media::Bad(None) => SlotState::WrongDevice,
        }
    }

    fn total_blocks(&self, slot: usize) -> u16 {
        self.slots[slot & 1].card().map_or(0, |c| c.user_blocks())
    }

    fn free_blocks(&self, slot: usize) -> u16 {
        self.slots[slot & 1].card().map_or(0, |c| c.free_blocks())
    }

    fn files(&self, slot: usize) -> &[Option<Save>] {
        &self.slots[slot & 1].files
    }

    fn version(&self, slot: usize) -> u32 {
        self.slots[slot & 1].version
    }

    fn start(&mut self, op: CardOp, slot: usize, file: usize) {
        self.result = None;
        self.dest = None;
        self.busy = Some((op, slot & 1, file, if op == CardOp::Erase { OP_FRAMES / 2 } else { OP_FRAMES }));
    }

    fn result(&self) -> Option<Result<(), CardError>> {
        self.result
    }

    fn dest_file(&self) -> Option<usize> {
        self.dest
    }

    /// Pull the card out (keeping it aside) or put the same one back in.
    fn cycle(&mut self, slot: usize) {
        let slot = slot & 1;
        if self.busy.is_some() || self.slots[slot].mounting > 0 {
            return;
        }
        match self.stash[slot].take() {
            Some((name, bytes)) if matches!(self.slots[slot].media, Media::Empty) => {
                let changed = self.slots[slot].changed;
                self.insert(slot, name, bytes);
                self.slots[slot].changed = changed;
            }
            _ => {
                let name = self.slots[slot].name.clone();
                let bytes = match &self.slots[slot].media {
                    Media::Card(c) => Some(c.as_bytes().to_vec()),
                    _ => None,
                };
                let changed = self.slots[slot].changed;
                if let Some(b) = bytes {
                    // Keep unsaved changes pending: eject hands them out
                    // only through take_changed, so leave the flag set.
                    self.stash[slot] = Some((name, b));
                    let s = &mut self.slots[slot];
                    s.media = Media::Empty;
                    s.files = vec![None; DIR_ENTRIES];
                    s.version += 1;
                    s.changed = changed;
                }
            }
        }
    }

    fn images(&mut self) -> Option<&mut ImageCards> {
        Some(self)
    }

    fn take_changed(&mut self, slot: usize) -> Option<(String, Vec<u8>)> {
        let s = &mut self.slots[slot & 1];
        if s.changed && matches!(s.media, Media::Empty) {
            // Pulled with changes: write what was pulled.
            s.changed = false;
            return self.stash[slot & 1].clone();
        }
        self.changed_bytes(slot)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn blank(mbit: u16) -> Vec<u8> {
        Card::format(mbit, Encoding::Ansi, 99).unwrap().into_bytes()
    }

    fn settle(c: &mut ImageCards) {
        for _ in 0..OP_FRAMES + MOUNT_FRAMES {
            c.update();
        }
    }

    fn one_block_save(name: &str) -> card::SaveFile {
        let mut d = [0u8; card::DENTRY];
        d[0..6].copy_from_slice(b"GCOBGC");
        d[8..8 + name.len()].copy_from_slice(name.as_bytes());
        d[0x38..0x3A].copy_from_slice(&1u16.to_be_bytes());
        card::SaveFile { dentry: card::Dentry(d), data: vec![7; card::BLOCK] }
    }

    #[test]
    fn copy_move_erase_format() {
        let mut c = ImageCards::new();
        c.insert(0, "a".into(), blank(4));
        c.insert(1, "b".into(), blank(16));
        assert_eq!(c.state(0), SlotState::Mounting);
        settle(&mut c);
        assert_eq!((c.total_blocks(0), c.total_blocks(1)), (59, 251));
        c.edit(0, |k| k.import(&one_block_save("x")).unwrap()).unwrap();
        assert!(c.files(0)[0].is_some());
        assert_eq!(c.take_changed(0).map(|t| t.0), Some("a".into()));
        assert!(c.take_changed(0).is_none());

        c.start(CardOp::Copy, 0, 0);
        settle(&mut c);
        assert_eq!(c.result(), Some(Ok(())));
        assert_eq!(c.free_blocks(1), 250);
        c.start(CardOp::Copy, 0, 0);
        settle(&mut c);
        assert_eq!(c.result(), Some(Err(CardError::Exists)));

        c.start(CardOp::Erase, 1, 0);
        settle(&mut c);
        c.start(CardOp::Move, 0, 0);
        settle(&mut c);
        assert_eq!(c.result(), Some(Ok(())));
        assert!(c.files(0)[0].is_none() && c.files(1)[0].is_some());
        let (_, bytes) = c.take_changed(1).unwrap();
        assert_eq!(Card::open(bytes).unwrap().saves().count(), 1);

        c.start(CardOp::Format, 1, 0);
        settle(&mut c);
        assert_eq!(c.free_blocks(1), 251);
    }

    #[test]
    fn pull_and_reinsert() {
        let mut c = ImageCards::new();
        c.insert(0, "a".into(), blank(4));
        settle(&mut c);
        c.edit(0, |k| k.import(&one_block_save("x")).unwrap()).unwrap();
        c.cycle(0);
        assert_eq!(c.state(0), SlotState::Empty);
        assert_eq!(c.take_changed(0).map(|t| t.0), Some("a".into()));
        c.cycle(0);
        settle(&mut c);
        assert_eq!(c.state(0), SlotState::Ready);
        assert!(c.files(0)[0].is_some());
        assert!(c.take_changed(0).is_none());
    }

    #[test]
    fn bad_images() {
        let mut c = ImageCards::new();
        let mut b = blank(4);
        b[0] ^= 1; // header checksum
        c.insert(0, "a".into(), b);
        c.insert(1, "b".into(), vec![0; 100]);
        settle(&mut c);
        assert_eq!(c.state(0), SlotState::Broken);
        assert_eq!(c.state(1), SlotState::WrongDevice);
        c.start(CardOp::Format, 0, 0);
        settle(&mut c);
        assert_eq!(c.state(0), SlotState::Ready);
        assert_eq!(c.total_blocks(0), 59);
        assert!(c.eject(0).is_some());
        assert_eq!(c.state(0), SlotState::Empty);
    }
}
