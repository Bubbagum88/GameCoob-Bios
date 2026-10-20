//! What the Game Play and Memory Card screens show, behind two small traits
//! so a real backend (an optical drive, a folder of saves...) can replace
//! the stand-ins later. For now both are fakes that live in RAM: nothing
//! here touches the PC's disks.
//!
//! Every name, picture and save below is made up for GameCoob.

use super::super::shader::FastTex;
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;

// ---------------------------------------------------------------------
// Disc drive
// ---------------------------------------------------------------------

/// The drive states the Game Play screen reacts to (BS2's DVD states).
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum DiscState {
    /// A bootable disc is in and readable.
    Ready,
    /// Spinning up or reading.
    Reading,
    NoDisc,
    CoverOpen,
    /// Not a disc this system can start.
    Wrong,
    /// A read failed; the drive will try again.
    Retry,
    /// The drive gave up.
    Fatal,
}

/// A disc's banner: the 96 x 32 picture and its texts.
pub struct Banner {
    pub image: FastTex,
    pub short_name: String,
    pub short_maker: String,
    pub long_name: String,
    pub long_maker: String,
    pub description: String,
}

pub trait DiscDrive {
    /// Called once per menu frame.
    fn update(&mut self);
    fn state(&self) -> DiscState;
    fn banner(&self) -> Option<&Banner>;
    /// Open or close the tray (a stand-in for a real eject button).
    fn toggle_tray(&mut self) {}
}

/// A drive that starts empty; toggling the tray inserts or removes one
/// made-up disc, which spins up for a moment before it is ready.
pub struct FakeDrive {
    state: DiscState,
    inserted: bool,
    timer: u32,
    banner: Banner,
}

/// Frames the fake tray stays open and the fake disc takes to spin up.
const TRAY_FRAMES: u32 = 30;
const SPIN_UP_FRAMES: u32 = 100;

impl FakeDrive {
    pub fn new() -> FakeDrive {
        FakeDrive {
            state: DiscState::NoDisc,
            inserted: false,
            timer: 0,
            banner: Banner {
                image: FastTex::from_rgba(96, 32, &disc_banner_art()),
                short_name: "Lantern Valley".into(),
                short_maker: "GameCoob".into(),
                long_name: "Lantern Valley".into(),
                long_maker: "GameCoob sample disc".into(),
                description: "A pretend disc. Press START to pick\nwhat this PC boots instead.".into(),
            },
        }
    }
}

impl Default for FakeDrive {
    fn default() -> Self {
        FakeDrive::new()
    }
}

impl DiscDrive for FakeDrive {
    fn update(&mut self) {
        if self.timer > 0 {
            self.timer -= 1;
            if self.timer == 0 {
                self.state = match self.state {
                    DiscState::CoverOpen if self.inserted => {
                        self.timer = SPIN_UP_FRAMES;
                        DiscState::Reading
                    }
                    DiscState::CoverOpen => DiscState::NoDisc,
                    DiscState::Reading => DiscState::Ready,
                    s => s,
                };
            }
        }
    }

    fn state(&self) -> DiscState {
        self.state
    }

    fn banner(&self) -> Option<&Banner> {
        (self.state == DiscState::Ready).then_some(&self.banner)
    }

    fn toggle_tray(&mut self) {
        self.inserted = !self.inserted;
        self.state = DiscState::CoverOpen;
        self.timer = TRAY_FRAMES;
    }
}

// ---------------------------------------------------------------------
// Memory cards
// ---------------------------------------------------------------------

/// Directory entries per card, as on the console.
pub const DIR_ENTRIES: usize = 127;

/// A slot's state (BS2's card-slot states).
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum SlotState {
    Empty,
    Mounting,
    Ready,
    /// Something that isn't a memory card.
    WrongDevice,
    /// A card that must be formatted.
    Broken,
    /// A card formatted for another region.
    Encoding,
    Other,
}

/// Why an operation failed, as the menu words it.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum CardError {
    /// The save may not be moved or copied.
    NoPermission,
    /// No usable card in the other slot.
    NoCard,
    /// The other slot holds something unusable.
    WrongDevice,
    /// The other card holds this save already.
    Exists,
    /// The other card is full.
    NoSpace,
    /// The two cards are different kinds.
    WrongType,
    Failed,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum CardOp {
    Copy,
    Move,
    Erase,
    Format,
}

/// One save file.
#[derive(Clone)]
pub struct Save {
    pub game: [u8; 4],
    pub maker: [u8; 2],
    pub file_name: String,
    /// The two comment lines: title and description.
    pub title: String,
    pub comment: String,
    pub blocks: u16,
    /// Seconds since 2000, for sorting.
    pub modified: u32,
    pub banner: Option<FastTexRc>,
    /// Icon frames and how many 1/60 s ticks each one shows.
    pub icons: Vec<(FastTexRc, u8)>,
    /// Play the icon forwards then backwards.
    pub ping_pong: bool,
    pub no_copy: bool,
    pub no_move: bool,
}

/// Shared picture data (saves are cloned by Copy).
pub type FastTexRc = alloc::rc::Rc<FastTex>;

impl Save {
    /// Same save, as the console decides: game, maker and file name.
    pub fn same_as(&self, o: &Save) -> bool {
        self.game == o.game && self.maker == o.maker && self.file_name == o.file_name
    }
}

pub trait MemCards {
    /// Called once per menu frame; finishes operations over time.
    fn update(&mut self);
    fn state(&self, slot: usize) -> SlotState;
    /// Bytes per block (`CARDGetSectorSize`); the browser colours a card's
    /// blocks by it. Every standard card has 8 KB blocks.
    fn sector_size(&self, _slot: usize) -> u32 {
        0x2000
    }
    fn total_blocks(&self, slot: usize) -> u16;
    fn free_blocks(&self, slot: usize) -> u16;
    /// The card's directory: `DIR_ENTRIES` entries, `None` where free.
    fn files(&self, slot: usize) -> &[Option<Save>];
    /// Free directory entries.
    fn free_files(&self, slot: usize) -> usize {
        self.files(slot).iter().filter(|f| f.is_none()).count()
    }
    /// Bumped whenever the slot's state or directory changes (a card goes
    /// in or out, an operation finishes): the card thread's "dirty" flag.
    fn version(&self, slot: usize) -> u32;
    /// Start an operation on `file` of `slot` (Copy and Move go to the
    /// other slot). The result comes from [`MemCards::result`].
    fn start(&mut self, op: CardOp, slot: usize, file: usize);
    /// `None` while an operation runs, then its outcome.
    fn result(&self) -> Option<Result<(), CardError>>;
    /// The directory entry the last Copy or Move wrote on the other card.
    fn dest_file(&self) -> Option<usize>;
    /// Insert or pull the card in `slot` (a stand-in for the real thing).
    fn cycle(&mut self, _slot: usize) {}
}

struct FakeCard {
    state: SlotState,
    total: u16,
    files: Vec<Option<Save>>,
    version: u32,
    /// Frames left until an inserted card has mounted.
    mounting: u32,
    /// What the card becomes once mounted.
    mounts_as: SlotState,
}

impl FakeCard {
    fn used(&self) -> u16 {
        self.files.iter().flatten().map(|s| s.blocks).sum()
    }
}

/// Two 251-block cards in RAM with made-up saves. Each slot cycles
/// through: card with saves, no card, unformatted card, something that is
/// not a card, and back.
pub struct FakeCards {
    cards: [FakeCard; 2],
    busy: Option<(CardOp, usize, usize, u32)>,
    result: Option<Result<(), CardError>>,
    dest: Option<usize>,
    clock: u32,
}

/// Frames a fake operation takes.
const OP_FRAMES: u32 = 90;
/// Frames an inserted fake card takes to mount.
const MOUNT_FRAMES: u32 = 20;
const FAKE_BLOCKS: u16 = 251;

impl FakeCards {
    pub fn new() -> FakeCards {
        let card = || FakeCard { state: SlotState::Ready, total: FAKE_BLOCKS, files: Vec::new(), version: 1, mounting: 0, mounts_as: SlotState::Ready };
        let mut c = FakeCards { cards: [card(), card()], busy: None, result: Some(Ok(())), dest: None, clock: 0 };
        c.fill(0);
        c.fill(1);
        c
    }

    fn fill(&mut self, slot: usize) {
        let mut files = vec![None; DIR_ENTRIES];
        for (i, s) in fake_saves(slot).into_iter().enumerate() {
            files[i] = Some(s);
        }
        self.cards[slot].files = files;
    }

    fn free(&self, slot: usize) -> u16 {
        let c = &self.cards[slot];
        c.total.saturating_sub(c.used())
    }

    fn run(&mut self, op: CardOp, slot: usize, file: usize) -> Result<(), CardError> {
        let other = slot ^ 1;
        if op == CardOp::Format {
            self.cards[slot].state = SlotState::Ready;
            self.cards[slot].files = vec![None; DIR_ENTRIES];
            return Ok(());
        }
        let save = self.cards[slot].files.get(file).cloned().flatten().ok_or(CardError::Failed)?;
        if op == CardOp::Erase {
            self.cards[slot].files[file] = None;
            return Ok(());
        }
        if (op == CardOp::Copy && save.no_copy) || (op == CardOp::Move && save.no_move) {
            return Err(CardError::NoPermission);
        }
        match self.cards[other].state {
            SlotState::Ready => {}
            SlotState::Empty => return Err(CardError::NoCard),
            _ => return Err(CardError::WrongDevice),
        }
        if self.cards[other].files.iter().flatten().any(|s| s.same_as(&save)) {
            return Err(CardError::Exists);
        }
        let free_entry = self.cards[other].files.iter().position(|f| f.is_none());
        let Some(dst) = free_entry.filter(|_| self.free(other) >= save.blocks) else {
            return Err(CardError::NoSpace);
        };
        let mut copy = save;
        copy.modified = copy.modified.max(self.clock);
        self.cards[other].files[dst] = Some(copy);
        self.dest = Some(dst);
        if op == CardOp::Move {
            self.cards[slot].files[file] = None;
        }
        Ok(())
    }
}

impl Default for FakeCards {
    fn default() -> Self {
        FakeCards::new()
    }
}

impl MemCards for FakeCards {
    fn update(&mut self) {
        self.clock += 1;
        if let Some((op, slot, file, t)) = self.busy {
            if t > 1 {
                self.busy = Some((op, slot, file, t - 1));
            } else {
                self.busy = None;
                self.result = Some(self.run(op, slot, file));
                // The card thread re-reads both directories after an operation.
                for c in self.cards.iter_mut() {
                    c.version += 1;
                }
            }
        }
        for slot in 0..2 {
            let c = &mut self.cards[slot];
            if c.mounting > 0 {
                c.mounting -= 1;
                if c.mounting == 0 {
                    c.state = c.mounts_as;
                    c.version += 1;
                    if c.state == SlotState::Ready {
                        self.fill(slot);
                    }
                }
            }
        }
    }

    fn state(&self, slot: usize) -> SlotState {
        self.cards[slot & 1].state
    }

    fn total_blocks(&self, slot: usize) -> u16 {
        self.cards[slot & 1].total
    }

    fn free_blocks(&self, slot: usize) -> u16 {
        self.free(slot & 1)
    }

    fn files(&self, slot: usize) -> &[Option<Save>] {
        &self.cards[slot & 1].files
    }

    fn version(&self, slot: usize) -> u32 {
        self.cards[slot & 1].version
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

    fn cycle(&mut self, slot: usize) {
        let slot = slot & 1;
        if self.busy.is_some() || self.cards[slot].mounting > 0 {
            return;
        }
        let c = &mut self.cards[slot];
        c.version += 1;
        let insert = |c: &mut FakeCard, as_: SlotState| {
            c.state = SlotState::Mounting;
            c.mounting = MOUNT_FRAMES;
            c.mounts_as = as_;
        };
        match c.state {
            SlotState::Ready => {
                c.state = SlotState::Empty;
                c.files = vec![None; DIR_ENTRIES];
            }
            SlotState::Empty => insert(c, SlotState::Broken),
            // Swap the unformatted card for something that is not a card.
            SlotState::Broken => c.state = SlotState::WrongDevice,
            _ => insert(c, SlotState::Ready),
        }
    }
}

// ---------------------------------------------------------------------
// Made-up content
// ---------------------------------------------------------------------

struct Art {
    game: &'static [u8; 4],
    title: &'static str,
    comment: &'static str,
    blocks: u16,
    shape: Shape,
    hue: f32,
    frames: u8,
    /// Has a banner (otherwise the browser shows the icon).
    banner: bool,
    /// May not be copied or moved.
    locked: bool,
}

const fn art(game: &'static [u8; 4], title: &'static str, comment: &'static str, blocks: u16, shape: Shape, hue: f32, frames: u8) -> Art {
    Art { game, title, comment, blocks, shape, hue, frames, banner: true, locked: false }
}

#[derive(Clone, Copy)]
enum Shape {
    Ring,
    Stripes,
    Star,
    Diamond,
    Moon,
}

const SLOT_A: [Art; 18] = [
    art(b"GCR0", "Coob Racer", "Lap records", 3, Shape::Ring, 0.3, 4),
    art(b"GPP0", "Puzzle Pals", "Chapter 4 - Clocktower", 2, Shape::Stripes, 2.0, 2),
    art(b"GSR0", "Starfield Ranger", "Sector 7, 3 lives", 5, Shape::Star, 4.0, 3),
    art(b"GGD0", "Garden Days", "Spring, year 2", 2, Shape::Diamond, 1.2, 1),
    Art { locked: true, ..art(b"GTK0", "Tournament Kart", "League ghost data (locked)", 4, Shape::Ring, 5.5, 2) },
    Art { banner: false, ..art(b"GMM0", "Marble Maze", "Best times", 1, Shape::Moon, 0.9, 3) },
    art(b"GFF0", "Fjord Fishing", "Lake log, 41 fish", 3, Shape::Stripes, 3.5, 2),
    art(b"GHH0", "Haunted Hallway", "Floor 3", 2, Shape::Moon, 4.6, 2),
    art(b"GSK0", "Sky Kite", "Windy hills", 1, Shape::Diamond, 2.6, 1),
    art(b"GBO0", "Bowling Night", "Lane 4, 212 points", 2, Shape::Ring, 1.7, 2),
    art(b"GCC0", "Comet Chaser", "Act II", 6, Shape::Star, 3.0, 3),
    art(b"GTT0", "Tidepool Tales", "Shell collection", 2, Shape::Stripes, 0.6, 2),
    art(b"GDR0", "Desert Rally", "Stage 5", 4, Shape::Diamond, 5.0, 1),
    art(b"GPN0", "Penguin Post", "Day 18", 1, Shape::Moon, 2.2, 2),
    art(b"GVV0", "Volcano Valley", "Base camp", 3, Shape::Star, 0.1, 3),
    art(b"GRB0", "Robot Band", "Setlist 2", 2, Shape::Ring, 3.9, 4),
    art(b"GLL0", "Lily Lake", "Pond 6", 1, Shape::Diamond, 2.9, 1),
    art(b"GOW0", "Owl Watch", "Night survey", 2, Shape::Moon, 4.3, 2),
];

const SLOT_B: [Art; 2] = [
    art(b"GNL0", "Night Lighthouse", "Keeper's log, night 12", 4, Shape::Moon, 3.1, 2),
    art(b"GBB0", "Block Builder", "Island map", 1, Shape::Stripes, 5.0, 1),
];

fn fake_saves(slot: usize) -> Vec<Save> {
    let arts: &[Art] = if slot == 0 { &SLOT_A } else { &SLOT_B };
    arts.iter()
        .enumerate()
        .map(|(i, a)| Save {
            game: *a.game,
            maker: *b"GC",
            file_name: String::from("save"),
            title: a.title.into(),
            comment: a.comment.into(),
            blocks: a.blocks,
            modified: 1000 + i as u32 * 10 + slot as u32,
            banner: a.banner.then(|| alloc::rc::Rc::new(FastTex::from_rgba(96, 32, &save_banner_art(a.hue)))),
            icons: (0..a.frames).map(|f| (alloc::rc::Rc::new(FastTex::from_rgba(32, 32, &icon_art(a.shape, a.hue, f))), 8)).collect(),
            ping_pong: false,
            no_copy: a.locked,
            no_move: a.locked,
        })
        .collect()
}

fn hue_rgb(h: f32, k: f32) -> [f32; 3] {
    [0.0f32, 2.1, 4.2].map(|o| (0.5 + 0.5 * libm::cosf(h + o)) * k)
}

fn px(c: [f32; 3], a: f32) -> [u8; 4] {
    let b = |v: f32| (v.clamp(0.0, 1.0) * 255.0) as u8;
    [b(c[0]), b(c[1]), b(c[2]), b(a)]
}

fn icon_art(shape: Shape, hue: f32, frame: u8) -> Vec<u8> {
    let f = frame as f32;
    let (c1, c2) = (hue_rgb(hue, 1.0), hue_rgb(hue, 0.35));
    let mut out = Vec::with_capacity(32 * 32 * 4);
    for y in 0..32 {
        for x in 0..32 {
            let (dx, dy) = (x as f32 - 15.5, y as f32 - 15.5);
            let r = libm::sqrtf(dx * dx + dy * dy);
            let t = libm::atan2f(dy, dx);
            let p = match shape {
                Shape::Ring if r > 9.0 && r < 14.0 => px(hue_rgb(hue, 0.6 + 0.4 * libm::sinf(3.0 * t + f * 0.8)), 1.0),
                Shape::Ring if r < 5.0 => [255, 255, 255, 255],
                Shape::Stripes if dx.abs() < 13.0 && dy.abs() < 13.0 => {
                    px(if ((x + y + frame as i32 * 2) / 4) % 2 == 1 { c1 } else { c2 }, 1.0)
                }
                Shape::Star if r < 9.0 + 5.0 * libm::cosf(5.0 * t + f * 0.6) => {
                    px(c1.map(|v| v + 0.25 * (1.0 - r / 14.0)), 1.0)
                }
                Shape::Diamond if dx.abs() + dy.abs() < 14.0 => {
                    let k = 1.0 - (dx.abs() + dy.abs()) / 14.0;
                    px([0, 1, 2].map(|i| c2[i] + (c1[i] - c2[i]) * k), 1.0)
                }
                Shape::Moon if r < 13.0 && {
                    let (ex, ey) = (dx - 5.0 + f * 2.0, dy + 3.0);
                    ex * ex + ey * ey > 100.0
                } =>
                {
                    px(c1.map(|v| v * 0.6 + 0.4), 1.0)
                }
                _ => [0, 0, 0, 0],
            };
            out.extend_from_slice(&p);
        }
    }
    out
}

fn save_banner_art(hue: f32) -> Vec<u8> {
    let c = hue_rgb(hue, 1.0);
    let mut out = Vec::with_capacity(96 * 32 * 4);
    for y in 0..32 {
        for x in 0..96 {
            let k = 0.3 + 0.7 * x as f32 / 95.0;
            let mut v = c.map(|v| v * k);
            if (x / 8 + y / 8) % 2 == 0 && y > 20 {
                v = v.map(|v| v * 0.7);
            }
            if (10..=14).contains(&y) {
                v = [1.0; 3];
            }
            out.extend_from_slice(&px(v, 1.0));
        }
    }
    out
}

/// The fake disc's banner: a valley at dusk with a row of lanterns.
fn disc_banner_art() -> Vec<u8> {
    let mut out = Vec::with_capacity(96 * 32 * 4);
    for y in 0..32 {
        for x in 0..96 {
            let (fx, fy) = (x as f32, y as f32);
            let sky = [0.10 + 0.5 * fy / 32.0, 0.08 + 0.2 * fy / 32.0, 0.30 - 0.1 * fy / 32.0];
            let hill = 22.0 + 4.0 * libm::sinf(fx * 0.09) + 2.0 * libm::sinf(fx * 0.23 + 1.0);
            let mut c = if fy > hill { [0.05, 0.15 + 0.1 * (fy - hill) / 10.0, 0.08] } else { sky };
            for k in 0..5 {
                let (lx, ly) = (10.0 + k as f32 * 19.0, 12.0 + 3.0 * libm::sinf(k as f32 * 1.7));
                let d = (fx - lx) * (fx - lx) + (fy - ly) * (fy - ly) * 1.6;
                let glow = libm::expf(-d / 9.0);
                c = [c[0] + glow, c[1] + 0.7 * glow, c[2] + 0.2 * glow];
            }
            out.extend_from_slice(&px(c, 1.0));
        }
    }
    out
}
