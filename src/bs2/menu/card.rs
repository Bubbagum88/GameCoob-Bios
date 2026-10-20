//! Memory Card (BS2 `memcard.c` 0x813157B8..0x8131BC00 and its block
//! renderer 0x8131ECC4..0x81321714): two slots side by side, every save a
//! small glass block with its icon, the banner and comment of the chosen
//! save, and Move / Copy / Erase / Format with BS2's dialogs, messages and
//! block animations. The cards come from [`MemCards`]: fake ones in RAM
//! for now.
//!
//! UI states (`sCardUiState`): 1 browse, 2 operation menu, 3 Yes/No,
//! 4 format question, 5 message. Input sets the *next* state, which takes
//! effect on the following frame, like on the console. Sound effects are
//! noted as `SE n` comments; there is no sound yet.

use super::backend::{CardError, CardOp, MemCards, Save, SlotState, DIR_ENTRIES};
use super::super::mtx::{self, Mtx, SinTable};
use super::super::raster::Prim;
use super::super::render::{self, DrawOpts, Prepared, View};
use super::super::shader::FastTex;
use super::super::ui::{self, Rgba, Ui, UiFont};
use super::{rgb, with_a, BackdropLight, Fade, Fader, MenuAssets, Pad, St, WHITE};
use crate::ipl::glh::{tag, Strings, Text};
use alloc::vec;
use alloc::vec::Vec;

// ---------------------------------------------------------------------
// Constants (CardParams_SetDefaults and the DOL's tables)
// ---------------------------------------------------------------------

const N: usize = DIR_ENTRIES;
/// Block zoom: resting and chosen.
const ZOOM_MIN: f32 = 1.3;
const ZOOM_MAX: f32 = 2.0;
const ZOOM_LEN: i16 = 6;
/// Frames a block takes to reach its grid cell.
const MOVE_LEN: i16 = 40;
/// Light height over the grid.
const LIGHT_Z: f32 = 320.0;
/// Dialog padding.
const PAD: f32 = 8.0;
/// Header dimming by the cursor fader (param 0x6E).
const HDR_DIM: i32 = 128;
/// Dim factor of a not-allowed Move / Copy line (param 0x5C).
const OP_DIM: i32 = 70;
const GLOW: [u8; 3] = [240, 230, 0];

/// UI errors (`Card_MapError`).
const BUSY: i32 = -0x15;
const UNUSABLE: i32 = -0x16;
const NO_CARD: i32 = -0x17;
const EXISTS: i32 = -0x18;
const NO_SPACE: i32 = -0x19;
const NOT_ALLOWED: i32 = -0x1A;
const WRONG_TYPE: i32 = -0x1B;
const FAILED: i32 = -0x1C;

/// `sCardOpKind`.
const K_NONE: u8 = 0;
const K_FORMAT: u8 = 1;
const K_COPY: u8 = 2;
const K_MOVE: u8 = 3;
const K_ERASE: u8 = 4;
const K_COPY_DONE: u8 = 5;
const K_MOVE_DONE: u8 = 6;
const K_ERASE_DONE: u8 = 7;
const K_FORMAT_DONE: u8 = 8;

/// `sCardOp`: the operation menu's lines.
const OP_MOVE: i16 = 0;
const OP_COPY: i16 = 1;
const OP_ERASE: i16 = 2;

/// Message strings (card strings res 30).
const MSG_COUNT: usize = 47;
/// Faders `Card_Enter` resets.
const ENTER_RESET: [usize; 30] = [19, 20, 21, 22, 23, 24, 27, 28, 29, 30, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 31, 32, 33, 34, 35, 36, 43, 44, 45, 46];
/// Messages `CardDraw_Messages` draws.
const RESULT_MSGS: [usize; 26] = [19, 20, 21, 22, 23, 24, 27, 28, 29, 30, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 31, 32, 43, 44, 45, 46];

/// Sector sizes a card can have, and the block colours for each
/// (TEVREG0): chosen save, save, chosen free entry, free entry.
const SIZES: [u32; 6] = [0x2000, 0x4000, 0x8000, 0x10000, 0x20000, 0x40000];
const COLORS: [[[i16; 3]; 4]; 6] = [
    [[100, 150, 255], [20, 50, 200], [30, 80, 100], [0, 20, 60]],
    [[100, 255, 110], [0, 105, 70], [50, 120, 80], [0, 37, 0]],
    [[200, 255, 75], [80, 120, 0], [90, 110, 0], [28, 34, 0]],
    [[255, 180, 50], [160, 100, 0], [100, 80, 0], [50, 30, 0]],
    [[255, 110, 100], [160, 45, 50], [150, 60, 50], [50, 20, 0]],
    [[200, 80, 190], [90, 30, 170], [100, 40, 90], [45, 0, 60]],
];
/// The models' own TEVREG0 (res 88, res 87).
const FILE_TEV0: [i16; 4] = [0, 50, 200, 255];
const EMPTY_TEV0: [i16; 4] = [0, 20, 60, 255];

/// The browser's block models, recoloured per sector size and variant.
pub struct Models {
    /// res 88: `[size][0 chosen, 1 other]`.
    file: Vec<[Prepared; 2]>,
    /// res 87: `[size][0 chosen, 1 other]`.
    empty: Vec<[Prepared; 2]>,
}

impl Models {
    pub fn new(file: Prepared, empty: Prepared) -> Models {
        let tint = |p: &Prepared, old: [i16; 4], c: [i16; 3]| p.with_tev0(old, [c[0], c[1], c[2], 255]);
        Models {
            file: COLORS.iter().map(|c| [tint(&file, FILE_TEV0, c[0]), tint(&file, FILE_TEV0, c[1])]).collect(),
            empty: COLORS.iter().map(|c| [tint(&empty, EMPTY_TEV0, c[2]), tint(&empty, EMPTY_TEV0, c[3])]).collect(),
        }
    }
}

fn size_index(bytes: u32) -> usize {
    SIZES.iter().position(|&s| s == bytes).unwrap_or(0)
}

fn map_error(e: CardError) -> i32 {
    match e {
        CardError::NoPermission => NOT_ALLOWED,
        CardError::NoCard => NO_CARD,
        CardError::WrongDevice => UNUSABLE,
        CardError::Exists => EXISTS,
        CardError::NoSpace => NO_SPACE,
        CardError::WrongType => WRONG_TYPE,
        CardError::Failed => FAILED,
    }
}

// ---------------------------------------------------------------------
// Blocks
// ---------------------------------------------------------------------

/// One directory entry's block (0x5C bytes on the console).
#[derive(Clone, Copy)]
struct Tile {
    /// Frames to wait before moving, and the frame of the move.
    start: i16,
    frame: i16,
    len: i16,
    alpha: i16,
    /// Fading in (true) or out.
    on: bool,
    /// Zoom timer and its length.
    zt: i16,
    zlen: i16,
    zoom: f32,
    zoom_from: f32,
    zoom_to: f32,
    pos: [f32; 2],
    vel: [f32; 2],
    from: [f32; 2],
    from_v: [f32; 2],
    to: [f32; 2],
}

impl Default for Tile {
    fn default() -> Tile {
        Tile {
            start: 0,
            frame: 0,
            len: MOVE_LEN,
            alpha: 0,
            on: false,
            zt: 0,
            zlen: ZOOM_LEN,
            zoom: ZOOM_MIN,
            zoom_from: ZOOM_MIN,
            zoom_to: ZOOM_MAX,
            pos: [292.0, 224.0],
            vel: [0.0; 2],
            from: [292.0, 224.0],
            from_v: [0.0; 2],
            to: [292.0, 224.0],
        }
    }
}

impl Tile {
    /// One step of the Hermite move (VecHermite clamps outside the range).
    fn advance(&mut self) {
        let new = if self.frame >= self.start {
            let f = (self.frame as f32).min(self.len as f32);
            [0, 1].map(|k| mtx::hermite(f, self.start as f32, self.from[k], self.from_v[k], self.len as f32, self.to[k], 0.0))
        } else {
            self.from
        };
        self.vel = [new[0] - self.pos[0], new[1] - self.pos[1]];
        self.pos = new;
        self.frame = (self.frame + 1).min(self.len);
    }

    /// Start a new move to `to` from where the block is now.
    fn retarget(&mut self) {
        self.start = 0;
        self.frame = 0;
        self.len = MOVE_LEN;
        self.from = self.pos;
        self.from_v = self.vel;
    }

    fn settled(&self) -> bool {
        self.frame == self.len
    }

    /// The block's model matrix: its spot, a wobble that grows with the
    /// zoom, and the zoom.
    fn mtx(&self, s: &SinTable, c: i32) -> Mtx {
        let k = (self.zoom - ZOOM_MIN) / (ZOOM_MAX - ZOOM_MIN);
        let t = [
            self.pos[0] - 292.0 + 10.0 * k * s.sin(-0x4000 + 35 * c),
            224.0 - self.pos[1] - 5.0 * k * s.sin(70 * c),
            2.0 * k,
        ];
        let rx = (350.0 * k * s.cos(70 * c)) as i32;
        let ry = (1000.0 * k * s.cos(-15000 + 35 * c)) as i32;
        let rz = (1000.0 * k * s.cos(35 * c)) as i32;
        mtx::concat(&mtx::rot_trans(s, rx, ry, rz, t), &mtx::scale(self.zoom, self.zoom, self.zoom))
    }
}

/// A burst piece of an erased block.
#[derive(Clone, Copy, Default)]
struct Spark {
    frame: i16,
    len: i16,
    delay: i16,
    angle: i16,
}

impl Spark {
    /// Progress 0..1 while it flies, `None` before and after.
    fn t(&self) -> Option<f32> {
        let d = self.frame - self.delay;
        (self.frame > self.delay && d <= self.len).then(|| d as f32 / self.len as f32)
    }
}

/// The block that leaves its cell while Copy, Move or Erase runs.
#[derive(Clone, Copy, Default)]
struct Fly {
    slot: usize,
    file: usize,
    /// Grid cell (col + row * 4) it left.
    cell: usize,
    kind: u8,
    t: Tile,
}

// ---------------------------------------------------------------------
// The slot dialog fader (0x20 bytes: f0, f4, ripple)
// ---------------------------------------------------------------------

#[derive(Clone, Copy, Default)]
struct DialogFader {
    /// Box in (0..20).
    f0: i16,
    /// Collapse to the chosen line and the Yes/No box (0..30).
    f4: i16,
    ripple: u16,
}

impl DialogFader {
    const F0: i16 = 20;
    const F4: i16 = 30;
    /// f4 thresholds: lines collapse between A and C, Yes/No shows after C.
    const A: i16 = 10;
    const C: i16 = 20;

    /// `CardUi_StepSlotFader`: 0 op menu, 1 away, 2 Yes/No.
    fn step(&mut self, mode: u8) {
        match mode {
            0 => {
                self.f0 = (self.f0 + 1).min(Self::F0);
                self.f4 = (self.f4 - 1).max(0);
            }
            2 => {
                self.f0 = (self.f0 + 1).min(Self::F0);
                self.f4 = (self.f4 + 1).min(Self::F4);
            }
            _ => {
                self.f0 = (self.f0 - 1).max(0);
                self.f4 = (self.f4 - 1).max(0);
            }
        }
        self.ripple = self.ripple.wrapping_add(5);
    }
}

// ---------------------------------------------------------------------
// State
// ---------------------------------------------------------------------

pub struct Card {
    ui: u8,
    ui_next: u8,
    op: i16,
    op_next: i16,
    /// 0 Yes, 1 No.
    yes_no: u8,
    slot: usize,
    col: [i8; 2],
    row: [i8; 2],
    scroll: [i8; 2],
    max_scroll: [i8; 2],
    /// Format question per slot: bit 1 second question, bit 0 "No".
    fmt: [u8; 2],
    fmt_next: [u8; 2],
    /// The player declined to format this card (slot code 7 / 9).
    asked: [bool; 2],
    /// The card thread's "dirty" flag, from [`MemCards::version`].
    dirty: [bool; 2],
    seen: [u32; 2],
    op_kind: u8,
    op_err: i32,
    op_slot: usize,
    busy_frames: u32,
    /// Directory index at each grid position.
    sorted: [Vec<usize>; 2],
    /// Per directory index.
    tiles: [Vec<Tile>; 2],
    icon_t: [Vec<i16>; 2],
    icon_dir: [Vec<i8>; 2],
    /// Grid cells ia00..ia15 / ib00..ib15 (layout units).
    cells: [[[f32; 2]; 16]; 2],
    msg: [Fader; MSG_COUNT],
    ready_f: [Fader; 2],
    copy_f: [Fader; 2],
    move_f: [Fader; 2],
    up_f: [Fader; 2],
    down_f: [Fader; 2],
    cursor_f: [Fader; 2],
    no_cursor_f: Fader,
    dialog: [DialogFader; 2],
    /// Arrow bob 0..15 and header pulse 0..30, each bouncing.
    bob: (i16, i16),
    pulse: (i16, i16),
    /// Wobble clock of the chosen block (+7 a frame).
    clock: i16,
    entering: bool,
    fly: Fly,
    flying: bool,
    erasing: bool,
    bursting: bool,
    sparks: [Spark; 12],
    rng: u32,
}

impl Card {
    /// Card_Init: the grid cells come from the layout.
    pub fn new(ma: &MenuAssets, cards: &dyn MemCards) -> Card {
        let mut cells = [[[0.0; 2]; 16]; 2];
        for (s, c) in cells.iter_mut().enumerate() {
            for (i, cell) in c.iter_mut().enumerate() {
                let t = [b'i', b'a' + s as u8, b'0' + (i / 10) as u8, b'0' + (i % 10) as u8];
                if let Some(p) = ma.card.pic(tag(&t)).next() {
                    *cell = [p.x, p.y];
                }
            }
        }
        let cursor = Fader { length: 10, ..Default::default() };
        let mut c = Card {
            ui: 1,
            ui_next: 1,
            op: OP_ERASE,
            op_next: OP_ERASE,
            yes_no: 1,
            slot: 0,
            col: [0; 2],
            row: [0; 2],
            scroll: [0; 2],
            max_scroll: [0; 2],
            fmt: [1; 2],
            fmt_next: [1; 2],
            asked: [false; 2],
            dirty: [false; 2],
            seen: [cards.version(0), cards.version(1)],
            op_kind: K_NONE,
            op_err: 0,
            op_slot: 0,
            busy_frames: 0,
            sorted: [(0..N).collect(), (0..N).collect()],
            tiles: [vec![Tile::default(); N], vec![Tile::default(); N]],
            icon_t: [vec![0; N], vec![0; N]],
            icon_dir: [vec![1; N], vec![1; N]],
            cells,
            msg: [Fader::default(); MSG_COUNT],
            ready_f: [Fader::default(); 2],
            copy_f: [Fader::default(); 2],
            move_f: [Fader::default(); 2],
            up_f: [Fader::default(); 2],
            down_f: [Fader::default(); 2],
            cursor_f: [cursor; 2],
            no_cursor_f: cursor,
            dialog: [DialogFader::default(); 2],
            bob: (0, 1),
            pulse: (0, 1),
            clock: 0,
            entering: false,
            fly: Fly::default(),
            flying: false,
            erasing: false,
            bursting: false,
            sparks: [Spark::default(); 12],
            rng: 0x2D9B_1C47,
        };
        for s in 0..2 {
            c.sort(cards, s);
        }
        c
    }

    fn rand(&mut self) -> i32 {
        self.rng = self.rng.wrapping_mul(1_103_515_245).wrapping_add(12345);
        ((self.rng >> 16) & 0x7FFF) as i32
    }

    /// While browsing the guide reads Select, Finish, Confirm.
    pub fn browsing(&self) -> bool {
        self.ui == 1
    }

    /// BS2's slot code: 0 empty, 3 mounting, 4 ready, 5 not a card,
    /// 6 / 7 needs formatting (7 once declined), 8 / 9 other region,
    /// 10 other trouble.
    fn code(&self, cards: &dyn MemCards, s: usize) -> u8 {
        match cards.state(s) {
            SlotState::Empty => 0,
            SlotState::Mounting => 3,
            SlotState::Ready => 4,
            SlotState::WrongDevice => 5,
            SlotState::Broken => 6 + self.asked[s] as u8,
            SlotState::Encoding => 8 + self.asked[s] as u8,
            SlotState::Other => 10,
        }
    }

    fn ready(&self, cards: &dyn MemCards, s: usize) -> bool {
        self.code(cards, s) == 4
    }

    fn cell_index(&self, s: usize) -> usize {
        (self.col[s] as i32 + self.row[s] as i32 * 4).clamp(0, 15) as usize
    }

    fn grid_index(&self, s: usize) -> usize {
        (self.col[s] as i32 + (self.row[s] as i32 + self.scroll[s] as i32) * 4).clamp(0, N as i32 - 1) as usize
    }

    /// `Card_GetSelectedFile` for slot `s`.
    fn selected(&self, s: usize) -> usize {
        self.sorted[s][self.grid_index(s)]
    }

    fn save(cards: &dyn MemCards, s: usize, file: usize) -> Option<&Save> {
        cards.files(s).get(file).and_then(|f| f.as_ref())
    }

    fn calc_max_scroll(cards: &dyn MemCards) -> impl Fn(usize) -> i8 + '_ {
        move |s| {
            let used = N as i32 - cards.free_files(s) as i32;
            let mut m = used / 4 - 3;
            if used % 4 == 0 {
                m -= 1;
            }
            m.max(0) as i8
        }
    }

    /// Card_SortFiles: saves by modification time, free entries after them.
    fn sort(&mut self, cards: &dyn MemCards, s: usize) {
        let files = cards.files(s);
        let mut keys: Vec<(u32, usize)> = (0..N)
            .map(|i| match files.get(i).and_then(|f| f.as_ref()) {
                Some(f) => (f.modified, i),
                None => ((0xFFi32 - i as i32 - 0x100) as u32, i),
            })
            .collect();
        keys.sort();
        self.sorted[s] = keys.into_iter().map(|(_, i)| i).collect();
        if cards.state(s) == SlotState::Ready {
            self.max_scroll[s] = Self::calc_max_scroll(cards)(s);
        }
        if self.max_scroll[s] < self.scroll[s] {
            self.scroll[s] = self.max_scroll[s];
        }
    }

    /// Card_CheckCopy / Card_CheckMove. As on the console, "the other
    /// slot" for the readiness test is the one opposite the cursor.
    fn check(&self, cards: &dyn MemCards, s: usize, file: usize, mv: bool) -> Result<(), i32> {
        let o = self.slot ^ 1;
        if !self.ready(cards, o) {
            return Err(match self.code(cards, o) {
                0 => NO_CARD,
                5 | 7 | 9 => UNUSABLE,
                _ => FAILED,
            });
        }
        let Some(save) = Self::save(cards, s, file) else { return Err(NOT_ALLOWED) };
        let other = s ^ 1;
        if (mv && save.no_move) || (!mv && save.no_copy) {
            Err(NOT_ALLOWED)
        } else if cards.sector_size(s) != cards.sector_size(other) {
            Err(WRONG_TYPE)
        } else if cards.free_blocks(other) < save.blocks || cards.free_files(other) == 0 {
            Err(NO_SPACE)
        } else if cards.files(other).iter().flatten().any(|f| f.same_as(save)) {
            Err(EXISTS)
        } else {
            Ok(())
        }
    }

    // -----------------------------------------------------------------
    // Entering (Card_Enter, CardUi_Reset, CardIcon_ResetFiles)
    // -----------------------------------------------------------------

    pub fn enter(&mut self) {
        for i in ENTER_RESET {
            self.msg[i].reset();
        }
        self.rebuild_grid();
        for s in 0..2 {
            for i in 0..N {
                let r = self.rand();
                let start = (self.rand() & 15) as i16;
                let (sn, cs) = sin_cos(r);
                let t = &mut self.tiles[s][i];
                t.pos = [292.0, 224.0];
                t.vel = [350.0 / 16.0 * sn, 350.0 / 16.0 * cs];
                t.from = t.pos;
                t.from_v = t.vel;
                t.alpha = 0;
                t.frame = 0;
                t.start = start;
                t.len = start + 33;
            }
        }
        self.entering = true;
    }

    /// fn_8132056C: send every block to its cell (or just off the grid,
    /// fading, when scrolled away).
    fn rebuild_grid(&mut self) {
        for s in 0..2 {
            let first = self.scroll[s] as usize * 4;
            for k in 0..N {
                let idx = self.sorted[s][k];
                let cells = self.cells[s];
                let t = &mut self.tiles[s][idx];
                t.retarget();
                if (first..=first + 15).contains(&k) {
                    t.on = true;
                    t.to = cells[k - first];
                } else {
                    t.on = false;
                    if k < first {
                        let c = cells[k & 3];
                        t.to = [c[0], c[1] - 32.0];
                    } else {
                        let c = cells[12 + (k & 3)];
                        t.to = [c[0], c[1] + 32.0];
                    }
                }
            }
        }
    }

    /// fn_81320920: a slot with no usable card fades its blocks out, then
    /// gathers them in the middle of its grid for next time.
    fn hide_slot(&mut self, s: usize) {
        let c = self.cells[s];
        let mid = [(c[5][0] + c[6][0]) / 2.0, (c[5][1] + c[9][1]) / 2.0];
        let tiles = &mut self.tiles[s];
        for t in tiles.iter_mut() {
            t.on = false;
        }
        if tiles.iter().all(|t| t.alpha == 0) {
            for t in tiles.iter_mut() {
                t.pos = mid;
                t.vel = [0.0; 2];
                t.from = mid;
                t.from_v = [0.0; 2];
                t.frame = 0;
            }
        }
    }

    /// fn_81320ABC: every block is home and the screen has faded in. Also
    /// commits each block's place as the start of its next move.
    fn settled(&mut self, cards: &dyn MemCards, fade_done: bool) -> bool {
        if !fade_done || self.erasing {
            return false;
        }
        let ready = [self.ready(cards, 0), self.ready(cards, 1)];
        if (0..2).any(|s| ready[s] && !self.tiles[s].iter().all(Tile::settled)) {
            return false;
        }
        for (tiles, _) in self.tiles.iter_mut().zip(ready).filter(|(_, r)| *r) {
            for t in tiles.iter_mut() {
                t.from = t.pos;
                t.from_v = t.vel;
            }
        }
        true
    }

    // -----------------------------------------------------------------
    // Block animation (fn_81320094 dispatch, fn_8132030C land,
    // fn_813204B0 cancel, fn_8131F8F8 update)
    // -----------------------------------------------------------------

    fn dispatch(&mut self, kind: u8) {
        let s = self.slot;
        let file = self.selected(s);
        let src = &mut self.tiles[s][file];
        let mut t = *src;
        src.alpha = 0;
        src.on = false;
        self.fly = Fly { slot: s, file, cell: self.cell_index(s), kind, t };
        if kind == K_ERASE {
            self.fly.t.zt = 20;
            self.fly.t.zlen = 20;
            self.fly.t.zoom_from = 0.0;
            self.erasing = true;
            self.bursting = false;
        } else {
            let (sn, cs) = sin_cos(self.rand());
            t.from = t.pos;
            t.from_v = t.vel;
            t.to = [292.0 + 40.0 * sn, 200.0 + 40.0 * cs];
            t.start = 0;
            t.frame = 0;
            t.len = MOVE_LEN;
            self.fly.t = t;
        }
        self.flying = true;
    }

    fn land(&mut self, cards: &dyn MemCards) {
        if !matches!(self.fly.kind, K_COPY | K_MOVE | K_ERASE) {
            return;
        }
        self.flying = false;
        let f = self.fly;
        if f.kind != K_ERASE {
            if let Some(dst) = cards.dest_file() {
                let d = &mut self.tiles[f.slot ^ 1][dst];
                d.pos = f.t.pos;
                d.vel = if f.cell < 8 { [0.0, 150.0 / 16.0] } else { [0.0, -150.0 / 16.0] };
                d.zoom = d.zoom_to;
                d.zt = d.zlen;
                d.alpha = 255;
                d.on = true;
                d.len = 60;
            }
            if f.kind == K_COPY {
                self.tiles[f.slot][f.file].pos = f.t.pos;
            }
        }
        if f.kind != K_COPY {
            let s = &mut self.tiles[f.slot][f.file];
            s.alpha = 0;
            s.on = false;
        }
    }

    fn cancel(&mut self) {
        if !self.flying {
            return;
        }
        self.flying = false;
        self.erasing = false;
        self.bursting = false;
        let f = self.fly;
        let t = &mut self.tiles[f.slot][f.file];
        t.pos = f.t.pos;
        t.from = f.t.pos;
        t.frame = 0;
        t.len = MOVE_LEN;
        t.alpha = 255;
        t.on = true;
    }

    fn update_tiles(&mut self, cards: &dyn MemCards, fade_done: bool) {
        if self.erasing {
            let f = &mut self.fly.t;
            f.zoom = mtx::hermite(f.zt as f32, 0.0, f.zoom_from, 0.0, f.zlen as f32, f.zoom_to, 0.0);
            f.zt -= 1;
            let shrunk = f.zt < 0;
            f.zt = f.zt.clamp(0, f.zlen);
            // The burst waits for the card (land() clears `flying`).
            if !self.flying && shrunk {
                if !self.bursting {
                    for i in 0..12 {
                        let delay = (self.rand() & 3) as i16;
                        let angle = self.rand() as i16;
                        self.sparks[i] = Spark { frame: 0, len: 10, delay, angle };
                    }
                }
                self.bursting = true;
                let mut done = 0;
                for sp in self.sparks.iter_mut() {
                    sp.frame += 1;
                    if sp.frame > sp.delay && sp.frame - sp.delay > sp.len {
                        sp.frame = sp.len + sp.delay;
                        done += 1;
                    }
                }
                if done == 12 {
                    self.bursting = false;
                    self.erasing = false;
                }
            }
        } else {
            let step = if self.entering { 25 } else { 8 };
            let sel = (self.slot, self.selected(self.slot));
            for s in 0..2 {
                for (i, t) in self.tiles[s].iter_mut().enumerate() {
                    if t.frame >= t.start {
                        t.alpha += if t.on { step } else { -step };
                    }
                    t.alpha = t.alpha.clamp(0, 255);
                    t.advance();
                    t.zoom = mtx::hermite(t.zt as f32, 0.0, t.zoom_from, 0.0, t.zlen as f32, t.zoom_to, 0.0);
                    t.zt += if (s, i) == sel && !self.entering { 1 } else { -1 };
                    t.zt = t.zt.clamp(0, t.zlen);
                }
            }
            if self.flying {
                let f = &mut self.fly.t;
                f.advance();
                if f.frame >= f.len - 5 {
                    f.from = f.pos;
                    f.from_v = f.vel;
                    // Orbit through the point mirrored about (292, 200).
                    f.to = [584.0 - f.to[0], 400.0 - f.to[1]];
                    f.frame = 0;
                    f.len = 180;
                }
            }
        }
        if self.entering && self.settled(cards, fade_done) {
            for t in self.tiles.iter_mut().flatten() {
                t.start = 0;
                t.len = MOVE_LEN;
                t.frame = MOVE_LEN;
            }
            self.entering = false;
        }
        self.clock = self.clock.wrapping_add(7);
    }

    // -----------------------------------------------------------------
    // Card_Update
    // -----------------------------------------------------------------

    /// One frame. `zoom_full`: the cube has zoomed all the way in;
    /// `fade_done`: the screen's three faders are past 172.
    pub fn update(&mut self, p: &Pad, cards: &mut dyn MemCards, zoom_full: bool, fade_done: bool) -> St {
        self.update_faders(cards);
        self.update_icon_anim(cards);
        let cur = self.slot;
        let mode = if self.ready(cards, cur) && self.ui == 2 {
            0
        } else if self.ready(cards, cur) && self.ui == 3 {
            2
        } else {
            1
        };
        if mode == 1 {
            self.dialog[0].step(1);
            self.dialog[1].step(1);
        } else {
            self.dialog[cur].step(mode);
            self.dialog[cur ^ 1].step(1);
        }
        self.update_result_msg();
        bounce(&mut self.bob, 15);
        self.update_cursor(cards);
        self.ui_update(p, cards, zoom_full, fade_done)
    }

    /// CardUi_Update.
    fn ui_update(&mut self, p: &Pad, cards: &mut dyn MemCards, zoom_full: bool, fade_done: bool) -> St {
        self.ui = self.ui_next;
        self.op = self.op_next;
        self.fmt = self.fmt_next;
        for s in 0..2 {
            let v = cards.version(s);
            if v != self.seen[s] {
                self.seen[s] = v;
                self.dirty[s] = true;
            }
        }
        self.poll_slot(cards, 0);
        self.poll_slot(cards, 1);
        // The blocks wait for the cube to finish zooming in.
        if !(self.entering && !zoom_full) {
            self.update_tiles(cards, fade_done);
        }
        match self.ui {
            1 => self.browse(p, cards, zoom_full, fade_done),
            2 => {
                self.op_menu(p, cards);
                St::Card
            }
            3 => {
                self.confirm(p, cards);
                St::Card
            }
            4 => {
                self.format(p, cards, fade_done);
                St::Card
            }
            5 => {
                self.message(p, cards, fade_done);
                St::Card
            }
            _ => St::Card,
        }
    }

    /// CardUi_Browse_Enter.
    fn browse_enter(&mut self, cards: &dyn MemCards, force: bool) {
        if self.ui == 5 && !force {
            return;
        }
        let o = self.slot ^ 1;
        if matches!(self.code(cards, o), 6 | 8) {
            self.ui = 4;
            self.ui_next = 4;
            self.slot = o;
            return;
        }
        self.ui = 1;
        self.ui_next = 1;
        if !self.ready(cards, self.slot) && self.ready(cards, o) {
            self.slot = o;
        }
    }

    /// CardUi_PollSlot.
    fn poll_slot(&mut self, cards: &dyn MemCards, s: usize) {
        if self.dirty[s] {
            match self.code(cards, s) {
                4 => self.on_slot_refresh(cards, s),
                6 | 8 => {
                    self.slot = s;
                    self.ui = 4;
                    self.ui_next = 4;
                    self.fmt_next[s] = 1;
                    self.dirty[s] = false;
                }
                _ => {}
            }
            // A card pulled while its message shows.
            if self.op_err == 0 && cards.state(s) == SlotState::Empty && self.ui == 5 {
                self.browse_enter(cards, true);
            }
        }
        self.poll_op_result(cards);
        if !self.ready(cards, s) {
            self.hide_slot(s);
        }
        match self.code(cards, self.slot) {
            0 => {
                self.browse_enter(cards, false);
                if self.ready(cards, self.slot ^ 1) {
                    self.slot ^= 1;
                }
            }
            4 | 6 | 8 => {}
            _ => self.browse_enter(cards, false),
        }
    }

    /// CardUi_OnSlotRefresh: re-sort, and land or put back a flying block.
    fn on_slot_refresh(&mut self, cards: &dyn MemCards, s: usize) {
        self.dirty[s] = false;
        self.max_scroll[s] = Self::calc_max_scroll(cards)(s);
        self.sort(cards, s);
        if matches!(self.op_kind, K_COPY | K_MOVE | K_ERASE) {
            if self.op_err == BUSY {
                self.op_err = self.result(cards);
            }
            if self.op_err == 0 {
                self.land(cards);
                self.op_kind = match self.op_kind {
                    // SE 20 (from A) / SE 21 (from B)
                    K_MOVE => K_MOVE_DONE,
                    K_ERASE => K_ERASE_DONE,
                    // SE 19
                    _ => K_COPY_DONE,
                };
            } else {
                self.cancel();
            }
        }
        self.rebuild_grid();
    }

    fn result(&self, cards: &dyn MemCards) -> i32 {
        match cards.result() {
            None => BUSY,
            Some(Ok(())) => 0,
            Some(Err(e)) => map_error(e),
        }
    }

    /// CardUi_PollOpResult.
    fn poll_op_result(&mut self, cards: &dyn MemCards) {
        if matches!(self.op_kind, K_COPY | K_MOVE | K_ERASE) {
            if self.op_err == BUSY {
                // SE 17 (slot A) / SE 18 (slot B) at frame 10.
                self.busy_frames = (self.busy_frames + 1).min(20);
                self.op_err = self.result(cards);
            }
            if self.op_err != BUSY && self.op_err != 0 {
                self.op_kind = match self.op_kind {
                    K_MOVE => K_MOVE_DONE,
                    K_ERASE => K_ERASE_DONE,
                    _ => K_COPY_DONE,
                };
                self.cancel();
                // SE 13
            }
        } else if self.op_kind == K_FORMAT {
            if self.op_err == BUSY {
                self.op_err = self.result(cards);
            }
            if self.op_err != BUSY {
                // SE 16 on success, else SE 13.
                self.op_kind = K_FORMAT_DONE;
            }
        }
    }

    /// CardUi_Browse.
    fn browse(&mut self, p: &Pad, cards: &dyn MemCards, zoom_full: bool, fade_done: bool) -> St {
        if self.entering {
            return St::Card;
        }
        let s = self.slot;
        let o = s ^ 1;
        if self.ready(cards, s) {
            self.max_scroll[s] = Self::calc_max_scroll(cards)(s);
            let at = |col: i8, row: i8, scroll: i8| col as i32 + (row as i32 + scroll as i32) * 4;
            if p.up {
                self.row[s] -= 1;
                if self.row[s] < 0 {
                    self.row[s] = 0;
                    if self.scroll[s] > 0 {
                        self.scroll[s] -= 1;
                        self.rebuild_grid();
                    }
                }
            }
            if p.down {
                self.row[s] += 1;
                if at(self.col[s], self.row[s], self.scroll[s]) >= N as i32 {
                    self.row[s] -= 1;
                } else if self.row[s] > 3 {
                    self.row[s] = 3;
                    if self.scroll[s] < self.max_scroll[s] {
                        self.scroll[s] += 1;
                        self.rebuild_grid();
                    }
                }
            }
            if p.left {
                if s == 0 {
                    if self.col[0] > 0 {
                        self.col[0] -= 1;
                    }
                } else {
                    self.col[1] -= 1;
                    if self.col[1] < 0 {
                        self.col[1] = 0;
                        if self.ready(cards, o) && at(self.col[o], self.row[s], self.scroll[o]) < N as i32 {
                            self.slot = o;
                            self.row[o] = self.row[s];
                            self.col[o] = 3;
                        }
                    }
                }
            }
            if p.right {
                let s = self.slot;
                if s == 1 {
                    if self.col[1] < 3 {
                        self.col[1] += 1;
                        if at(self.col[1], self.row[1], self.scroll[1]) >= N as i32 {
                            self.col[1] -= 1;
                        }
                    }
                } else {
                    self.col[0] += 1;
                    if at(self.col[0], self.row[0], self.scroll[0]) >= N as i32 {
                        self.col[0] -= 1;
                    } else if self.col[0] > 3 {
                        self.col[0] = 3;
                        if self.ready(cards, 1) {
                            self.slot = 1;
                            self.row[1] = self.row[0];
                            self.col[1] = 0;
                        }
                    }
                }
            }
            let s = self.slot;
            let used = Self::save(cards, s, self.selected(s)).is_some();
            if p.a && used && self.settled(cards, fade_done) {
                // SE 8
                self.ui_next = 2;
            }
        }
        if p.b && zoom_full && self.settled(cards, fade_done) {
            // SE 6. Leaving asks again about cards it was told to leave alone.
            for k in 0..2 {
                if self.asked[k] {
                    self.asked[k] = false;
                    self.dirty[k] = true;
                }
            }
            return St::CardFace;
        }
        St::Card
    }

    /// CardUi_OpMenu.
    fn op_menu(&mut self, p: &Pad, cards: &dyn MemCards) {
        if p.up {
            // SE 10
            self.op_next = if self.op_next == 0 { 2 } else { self.op_next - 1 };
        }
        if p.down {
            self.op_next = if self.op_next >= 2 { 0 } else { self.op_next + 1 };
        }
        if p.a && self.op_next == self.op {
            let s = self.slot;
            let file = self.selected(s);
            self.op_slot = s;
            self.yes_no = 1;
            match self.op {
                OP_ERASE => self.ui_next = 3,
                op => match self.check(cards, s, file, op == OP_MOVE) {
                    Ok(()) => self.ui_next = 3,
                    Err(e) => {
                        // SE 13
                        self.op_kind = if op == OP_MOVE { K_MOVE_DONE } else { K_COPY_DONE };
                        self.op_err = e;
                        self.ui_next = 5;
                    }
                },
            }
        }
        if p.b {
            // SE 7
            self.browse_enter(cards, false);
        }
    }

    /// CardUi_Confirm.
    fn confirm(&mut self, p: &Pad, cards: &mut dyn MemCards) {
        if p.up {
            self.yes_no = 0;
        }
        if p.down {
            self.yes_no = 1;
        }
        if p.a {
            if self.yes_no == 0 {
                let s = self.slot;
                let file = self.selected(s);
                self.op_slot = s;
                let other_ready = self.ready(&*cards, s ^ 1);
                match self.op {
                    OP_ERASE => {
                        self.op_kind = K_ERASE;
                        self.dispatch(K_ERASE);
                        cards.start(CardOp::Erase, s, file);
                        self.busy_frames = 0;
                        self.op_err = BUSY;
                    }
                    op => {
                        let (kind, cop) = if op == OP_COPY { (K_COPY, CardOp::Copy) } else { (K_MOVE, CardOp::Move) };
                        self.op_kind = kind;
                        if other_ready {
                            self.dispatch(kind);
                            cards.start(cop, s, file);
                            self.op_err = BUSY;
                            self.busy_frames = 0;
                        } else {
                            self.op_err = NO_CARD;
                        }
                    }
                }
                self.ui_next = 5;
            } else {
                self.ui_next = 2;
            }
        }
        if p.b {
            self.ui_next = 2;
        }
        if !self.ready(&*cards, self.slot ^ 1) && (0..2).contains(&self.op) {
            self.ui_next = 2;
        }
    }

    /// CardUi_Format.
    fn format(&mut self, p: &Pad, cards: &mut dyn MemCards, fade_done: bool) {
        let s = self.slot;
        let shown = self.msg[39 + s].alpha() > 172 || self.msg[41 + s].alpha() > 172;
        if !(self.settled(&*cards, fade_done) && shown) {
            return;
        }
        if p.up {
            self.fmt_next[s] &= !1;
        }
        if p.down {
            self.fmt_next[s] |= 1;
        }
        let decline = |c: &mut Card, cards: &dyn MemCards| {
            c.asked[s] = true;
            c.fmt_next[s] = 1;
            c.browse_enter(cards, false);
        };
        if p.a {
            match self.fmt[s] & 3 {
                0 => self.fmt_next[s] = 3,
                2 => {
                    cards.start(CardOp::Format, s, 0);
                    self.ui_next = 5;
                    self.op_slot = s;
                    self.op_kind = K_FORMAT;
                    self.op_err = BUSY;
                }
                3 => decline(self, &*cards),
                _ => {}
            }
        }
        if p.b {
            decline(self, &*cards);
        }
    }

    /// CardUi_Message.
    fn message(&mut self, p: &Pad, cards: &dyn MemCards, fade_done: bool) {
        if !self.settled(cards, fade_done) || cards.result().is_none() {
            return;
        }
        // SE 16 (copy) / SE 15 (erase) once, when the operation ran.
        self.busy_frames = 0;
        if p.a || p.b {
            self.op_kind = K_NONE;
            self.yes_no = 1;
            self.op_err = 0;
            self.browse_enter(cards, true);
        }
    }

    // -----------------------------------------------------------------
    // Faders
    // -----------------------------------------------------------------

    /// CardUi_UpdateFaders.
    fn update_faders(&mut self, cards: &dyn MemCards) {
        for s in 0..2 {
            let c = self.code(cards, s);
            if c == 4 {
                self.ready_f[s].step(Fade::In);
                self.update_op_faders(cards, 0);
                self.update_op_faders(cards, 1);
            } else {
                self.ready_f[s].step(Fade::Out);
            }
            let normal = self.ui != 5;
            let other_format = !matches!(self.op_kind, K_FORMAT | K_FORMAT_DONE) || self.op_slot == s;
            if normal || !other_format {
                self.msg[33 + s].step_if(c == 0);
                self.msg[35 + s].step_if(matches!(c, 5 | 10 | 7 | 9));
                if matches!(c, 0 | 5 | 10 | 7 | 9) {
                    self.icon_t[s].iter_mut().for_each(|t| *t = 0);
                }
                self.format_msg(cards, s, !normal);
            } else {
                for i in [33, 35, 39, 41] {
                    self.msg[i + s].step(Fade::Out);
                }
            }
            if self.ui == 1 {
                self.up_f[s].step_if(self.scroll[s] > 0);
                self.down_f[s].step_if(self.max_scroll[s] > self.scroll[s]);
            } else {
                self.up_f[s].step(Fade::Out);
                self.down_f[s].step(Fade::Out);
            }
        }
    }

    /// CardUi_UpdateOpFaders: Move and Copy dim when they can't be done.
    fn update_op_faders(&mut self, cards: &dyn MemCards, s: usize) {
        let file = self.selected(s);
        self.copy_f[s].step_if(self.check(cards, s, file, false).is_ok());
        self.move_f[s].step_if(self.check(cards, s, file, true).is_ok());
    }

    /// CardUi_UpdateFormatMsg.
    fn format_msg(&mut self, cards: &dyn MemCards, s: usize, force: bool) {
        let here = if s == self.slot { Fade::In } else { Fade::Half };
        let (q1, q2) = (39 + s, 41 + s);
        if matches!(self.code(cards, s), 6 | 8) && (self.ui == 4 || force) {
            if self.fmt[s] & 2 == 0 {
                self.msg[q1].step(here);
                self.msg[q2].step(Fade::Out);
            } else {
                self.msg[q1].step(Fade::Out);
                self.msg[q2].step(here);
            }
        } else {
            self.msg[q1].step(Fade::Out);
            self.msg[q2].step(Fade::Out);
        }
    }

    /// CardUi_UpdateResultMsg (+ CopyResultMsg, MoveResultMsg).
    fn update_result_msg(&mut self) {
        let e = self.op_err;
        let k = self.op_kind;
        let show = self.ui == 5 && e != BUSY;
        let os = self.op_slot;
        for (base, kinds) in [(19usize, [K_COPY, K_COPY_DONE]), (7, [K_MOVE, K_MOVE_DONE])] {
            let on = show && kinds.contains(&k);
            let map = [(base, 0), (base + 1, NOT_ALLOWED), (base + 3 - os, NO_CARD), (base + 5 - os, UNUSABLE), (base + 8, WRONG_TYPE), (base + 9, EXISTS), (base + 10, NO_SPACE), (base + 11, FAILED)];
            // Indices the map doesn't pick fade out too.
            for i in base..base + 12 {
                if !map.iter().any(|&(m, _)| m == i) {
                    self.msg[i].step(Fade::Out);
                }
            }
            for (i, err) in map {
                self.msg[i].step_if(on && e == err);
            }
        }
        let erase = show && matches!(k, K_ERASE | K_ERASE_DONE);
        self.msg[31].step_if(erase && e == 0);
        self.msg[32].step_if(erase && e != 0);
        let fmt = show && matches!(k, K_FORMAT | K_FORMAT_DONE);
        self.msg[43 + os].step_if(fmt && e == 0);
        self.msg[45 + os].step_if(fmt && e != 0);
    }

    /// CardUi_UpdateCursor: the spot over the slot with the cursor.
    fn update_cursor(&mut self, cards: &dyn MemCards) {
        bounce(&mut self.pulse, 30);
        let mut any = false;
        for s in 0..2 {
            let on = s == self.slot && self.ready(cards, s);
            self.cursor_f[s].step_if(on);
            any |= on;
        }
        self.no_cursor_f.step_if(!any);
    }

    /// CardUi_UpdateIconAnim: the visible saves' icons play.
    fn update_icon_anim(&mut self, cards: &dyn MemCards) {
        for s in 0..2 {
            let first = self.scroll[s] as usize * 4;
            for k in first..(first + 16).min(N) {
                let i = self.sorted[s][k];
                let Some(save) = Self::save(cards, s, i) else { continue };
                if save.icons.is_empty() {
                    continue;
                }
                let total: i16 = save.icons.iter().map(|f| f.1 as i16).sum();
                let (t, d) = (&mut self.icon_t[s][i], &mut self.icon_dir[s][i]);
                *t += *d as i16;
                if *t >= total {
                    if save.ping_pong {
                        *t = total - save.icons.last().map_or(0, |f| f.1 as i16) - 1;
                        *d = -1;
                    } else {
                        *t = 0;
                        *d = 1;
                    }
                }
                if *t < 0 {
                    *t = if save.ping_pong { save.icons[0].1 as i16 } else { 0 };
                    *d = 1;
                }
            }
        }
    }

    fn icon<'c>(&self, save: &'c Save, s: usize, i: usize) -> Option<&'c FastTex> {
        let mut acc = 0i16;
        let t = self.icon_t[s][i];
        for (tex, ticks) in save.icons.iter() {
            acc += *ticks as i16;
            if t < acc {
                return Some(tex);
            }
        }
        save.icons.last().map(|f| &*f.0)
    }

    // -----------------------------------------------------------------
    // Drawing (Card_Draw)
    // -----------------------------------------------------------------

    /// Card_Draw with the screen's three fader alphas (fx, backdrop, text).
    #[allow(clippy::too_many_arguments)]
    pub fn draw<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, cards: &'a dyn MemCards, lang: usize, al: [f32; 3], s: &SinTable) {
        let g = &ma.card;
        let mid = |k: usize| {
            let c = self.cells[k];
            [(c[5][0] + c[6][0]) / 2.0, (c[5][1] + c[9][1]) / 2.0]
        };
        let lights = [
            BackdropLight::slot(mid(0), LIGHT_Z, self.cursor_f[0].alpha() as f32 / 255.0),
            BackdropLight::slot(mid(1), LIGHT_Z, self.cursor_f[1].alpha() as f32 / 255.0),
            BackdropLight::main(self.no_cursor_f.alpha() as f32 / 255.0),
        ];
        super::draw_backdrop_lit(ui, ma, al[1], &lights);
        for f in g.frame(tag(b"mes6")) {
            ui.frame(f, &ma.bank, with_a(WHITE, al[2]));
        }
        let cur = self.slot;
        let light = mid(cur);
        let mut v = View::intro(0.0, false);
        v.vp = ui.view.vp;
        v.light = mtx::apply(&v.view, [light[0] - 292.0, 224.0 - light[1], LIGHT_Z]);
        for k in [cur, cur ^ 1] {
            self.draw_blocks(ui.list, &v, ma, cards, k, al[0], s);
        }
        let st = &ma.card_str[lang];
        for k in [cur ^ 1, cur] {
            self.draw_header(ui, ma, cards, st, k, al[2]);
            for i in [33, 35] {
                self.draw_msg(ui, ma, st, i + k, al[2]);
            }
            for q in [39, 41] {
                self.draw_format(ui, ma, st, q, k, al[2]);
            }
        }
        for k in 0..2 {
            self.draw_dialog(ui, ma, st, k, al[2]);
        }
        for i in RESULT_MSGS {
            self.draw_msg(ui, ma, st, i, al[2]);
        }
    }

    /// fn_81321378: one slot's blocks; the chosen one and the flying one last.
    #[allow(clippy::too_many_arguments)]
    fn draw_blocks<'a>(&'a self, list: &mut Vec<Prim<'a>>, v: &View, ma: &'a MenuAssets, cards: &'a dyn MemCards, k: usize, a: f32, s: &SinTable) {
        if a <= 0.0 {
            return;
        }
        let m = &ma.card_models;
        let size = size_index(cards.sector_size(k));
        let c = self.clock as i32;
        let alpha = |t: &Tile| (a * t.alpha as f32) as u8;
        let chosen = (k == self.slot && !self.entering).then(|| self.selected(k));
        for (i, t) in self.tiles[k].iter().enumerate() {
            let al = alpha(t);
            if al == 0 {
                continue;
            }
            match Self::save(cards, k, i) {
                Some(save) => {
                    if (self.flying && k == self.fly.slot && i == self.fly.file) || chosen == Some(i) {
                        continue;
                    }
                    draw_block(list, &m.file[size][1], &t.mtx(s, c), v, al, self.icon(save, k, i));
                }
                None => {
                    let var = if chosen == Some(i) { 0 } else { 1 };
                    for d in 0..2 {
                        render::draw_model(list, &m.empty[size][var], &t.mtx(s, c), v, &DrawOpts { alpha: al, only: Some(d..d + 1), ..Default::default() });
                    }
                }
            }
        }
        if self.flying && !self.erasing && k == self.fly.slot {
            if let Some(save) = Self::save(cards, k, self.fly.file) {
                let t = &self.fly.t;
                draw_block(list, &m.file[size][1], &t.mtx(s, c), v, alpha(t), self.icon(save, k, self.fly.file));
            }
        } else if let Some((i, save)) = chosen.filter(|_| !self.flying).and_then(|i| Some((i, Self::save(cards, k, i)?))) {
            let t = &self.tiles[k][i];
            draw_block(list, &m.file[size][0], &t.mtx(s, c), v, alpha(t), self.icon(save, k, i));
        }
        if self.erasing && k == self.fly.slot {
            let t = &self.fly.t;
            let p = &m.file[size][1];
            if !self.bursting {
                if t.zoom > 0.0 {
                    draw_block(list, p, &t.mtx(s, c), v, alpha(t), None);
                }
            } else {
                for sp in self.sparks.iter() {
                    let Some(f) = sp.t() else { continue };
                    let al = (a * 255.0 * (1.0 - f)) as u8;
                    if al == 0 {
                        continue;
                    }
                    let r = 48.0 * (0.25 + 0.75 * f);
                    let off = [r * s.sin(sp.angle as i32), r * s.cos(sp.angle as i32)];
                    let at = mtx::trans(t.pos[0] - 292.0 + off[0], 224.0 - t.pos[1] + off[1], 0.0);
                    draw_block(list, p, &mtx::concat(&at, &mtx::scale(0.3, 0.3, 0.3)), v, al, None);
                }
            }
        }
    }

    /// CardDraw_FileBanner + CardDraw_SlotHeader + CardDraw_FileInfo.
    fn draw_header<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, cards: &'a dyn MemCards, st: &'a Strings, k: usize, a: f32) {
        let g = &ma.card;
        let bank = &ma.bank[..];
        let ready = self.ready_f[k].alpha() as f32 / 255.0;
        let ar = a * ready;
        let cursor = self.cursor_f[k].alpha();
        let hdr = ar * ((255 - HDR_DIM) + HDR_DIM * cursor / 255) as f32 / 255.0;
        let sl = b'a' + k as u8;
        let white = with_a(WHITE, hdr);
        super::pics(ui, g, &[b't', b'e', b'x', sl], bank, white);
        if let Some(p) = g.pic(tag(&[b'f', b'n', b'm', sl])).next() {
            draw_number(ui, &ma.card_bar, p.x, p.y, cards.free_blocks(k) as u32, white);
        }
        for f in g.frame(tag(&[b'f', b'f', b'r', sl])) {
            ui.frame(f, bank, white);
        }
        if let Some(t) = st.get(k) {
            ui.string(&ma.font, g, bank, t, white, 0.0, 0.0);
        }
        // Scroll arrows, bobbing.
        let bob = mtx::hermite(self.bob.0 as f32, 0.0, 0.0, 0.0, 15.0, 128.0, 0.0) / 16.0;
        let ups = [(b'u', &self.up_f[k], bob), (b'd', &self.down_f[k], -bob)];
        for (d, f, dy) in ups {
            let c = with_a(WHITE, ar * f.alpha() as f32 / 255.0);
            if c[3] <= 0.0 {
                continue;
            }
            for p in g.pic(tag(&[b'a', b'r', sl, d])) {
                if let Some(t) = bank.get(p.tex as usize) {
                    ui.pic(p, t, c, 0.0, dy);
                }
            }
        }
        if k != self.slot || !self.ready(cards, k) {
            return;
        }
        let file = self.selected(k);
        let Some(save) = Self::save(cards, k, file) else { return };
        let c = with_a(WHITE, ar);
        match &save.banner {
            Some(b) => {
                for p in g.pic(tag(b"bana")) {
                    ui.pic_tex(p, b, c);
                }
            }
            None => {
                if let (Some(p), Some(icon)) = (g.pic(tag(b"ic00")).next(), self.icon(save, k, file)) {
                    ui.pic_tex(p, icon, c);
                }
            }
        }
        for (t, s) in [(b"titl", &save.title), (b"info", &save.comment)] {
            if let Some(p) = g.text(tag(t)) {
                let mut line = ui::cp1252(s);
                line.truncate(31);
                ui.text(&ma.font, p, &line, c, 0.0, 0.0);
            }
        }
        for f in g.frame(tag(b"frmc")) {
            ui.frame(f, bank, c);
        }
        if let Some(p) = g.pic(tag(b"nmc1")).next() {
            draw_number(ui, &ma.card_bar, p.x, p.y, save.blocks as u32, c);
        }
    }

    /// Fader_DrawString: message `i` at its fader's alpha.
    fn draw_msg<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, st: &'a Strings, i: usize, a: f32) {
        let k = a * self.msg[i].alpha() as f32 / 255.0;
        if k <= 0.0 {
            return;
        }
        if let Some(t) = st.get(i) {
            ui.string(&ma.font, &ma.card, &ma.bank, t, with_a(WHITE, k), 0.0, 0.0);
        }
    }

    /// CardDraw_OpMenu: a format question with its Yes / No box below.
    fn draw_format<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, st: &'a Strings, q: usize, k: usize, a: f32) {
        let i = q + k;
        let f = &self.msg[i];
        let ca = a * f.alpha() as f32 / 255.0;
        if ca <= 0.0 {
            return;
        }
        let g = &ma.card;
        let font = &ma.font;
        let (Some(s), Some(yes), Some(no)) = (st.get(i), st.get(5), st.get(6)) else { return };
        let Some(p) = g.texts.get(s.pane as usize) else { return };
        let (_, mh) = font.measure(p, &s.text, p.size);
        let (wy, _) = font.measure(p, &yes.text, p.size);
        let (wn, yh) = font.measure(p, &no.text, p.size);
        let yw = wy.max(wn);
        let line = if p.line > 0.0 { p.line.max(p.size) } else { p.size };
        let h_msg = mh + 2.0 * PAD;
        let h_yn = yh + line + 2.0 * PAD;
        let total = h_msg + h_yn + 8.0;
        ui.string(font, g, &ma.bank, s, with_a(WHITE, ca), 0.0, -total / 2.0 + h_msg / 2.0);
        // Yes / No once the question is half in.
        let half = f.length / 2;
        if f.frame <= f.delay + half {
            return;
        }
        let cy = a * ((f.frame - f.delay - half) as f32 * 2.0 / f.length as f32).min(1.0);
        let (x, y) = (p.x - (yw / 2.0 + PAD), total / 2.0 + p.y - h_yn);
        let (w, h) = (yw + 2.0 * PAD, h_yn);
        if let Some(bg) = p.bg.and_then(|b| g.frames.get(b as usize)) {
            ui.frame_rect(bg, &ma.bank, with_a(WHITE, cy), x, y, w, h);
        }
        ripple(ui, x, y, w, h, (f.frame as u32 * 5) as u8, cy);
        let fmt = self.fmt[k];
        let mut lp = centred(p, p.x, y + PAD + yh / 2.0, yw);
        for (n, t) in [yes, no].into_iter().enumerate() {
            let lit = if q == 39 {
                (fmt & 2 == 2 && n == 0) || (fmt & 2 != 2 && n == (fmt & 1) as usize)
            } else {
                n == (fmt & 1) as usize
            };
            let lit = lit && k == self.slot;
            text_line(ui, font, &lp, &t.text, with_a(WHITE, cy), lit.then_some(self.glow(cy)));
            lp.y += line;
        }
    }

    fn glow(&self, a: f32) -> Rgba {
        with_a(rgb(GLOW[0], GLOW[1], GLOW[2]), a * (0.5 + 0.5 * self.pulse.0 as f32 / 30.0))
    }

    /// CardDraw_Dialog: Move / Copy / Erase beside the chosen block, then
    /// Yes / No under the chosen line.
    fn draw_dialog<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, st: &'a Strings, k: usize, a: f32) {
        let d = self.dialog[k];
        if d.f0 == 0 {
            return;
        }
        let g = &ma.card;
        let font = &ma.font;
        let (Some(l0), Some(box_)) = (st.get(2), g.frame(tag(b"mes0")).next()) else { return };
        let Some(p) = g.texts.get(l0.pane as usize) else { return };
        let lines = [2usize, 3, 4].map(|i| st.get(i));
        let w = lines.iter().flatten().map(|t| font.measure(p, &t.text, p.size).0).fold(0.0, f32::max);
        let h1 = p.size;
        let line = if p.line > 0.0 { p.line.max(p.size) } else { p.size };
        let full = h1 + 2.0 * line;
        let gap = if d.f4 < DialogFader::A {
            2.0 * line
        } else if d.f4 < DialogFader::C {
            (DialogFader::C - d.f4) as f32 * 2.0 * line / (DialogFader::C - DialogFader::A) as f32
        } else {
            0.0
        };
        let h_menu = h1 + gap;
        let h_yn = h1 + line;
        let (col, row) = (self.col[k], self.row[k]);
        let [cx, cy] = self.cells[k][self.cell_index(k)];
        let mut slide = 40.0 * (DialogFader::F0 - d.f0) as f32 / DialogFader::F0 as f32;
        let x = if col < 2 { cx + 40.0 } else { cx - (w + 2.0 * PAD + 40.0) };
        let mut y0 = cy;
        if row >= 2 {
            slide = -slide;
            y0 = cy - (full + PAD / 2.0);
        }
        let top = y0 + slide;
        let ca = a * d.f0 as f32 / DialogFader::F0 as f32;
        ui.frame_rect(box_, &ma.bank, with_a(WHITE, ca), x, top, w + 2.0 * PAD, h_menu + 2.0 * PAD);
        // The three lines, collapsing onto the first.
        let fade = if d.f4 < DialogFader::A { ca * (DialogFader::A - d.f4) as f32 / DialogFader::A as f32 } else { 0.0 };
        let allowed = [self.move_f[k].alpha() / 2, self.copy_f[k].alpha() / 2];
        let mut lp = centred(p, x + PAD + w / 2.0, top + PAD + h1 / 2.0, w);
        for (n, t) in lines.iter().enumerate() {
            let Some(t) = t else { continue };
            let dim = if n < 2 { (OP_DIM + allowed[n]) as f32 / (OP_DIM + 127) as f32 } else { 1.0 };
            let base = [dim, dim, dim, 1.0];
            if self.op == n as i16 {
                let glow = (self.ui == 2).then(|| self.glow(ca));
                text_line(ui, font, &lp, &t.text, with_a(base, ca), glow);
            } else {
                text_line(ui, font, &lp, &t.text, with_a(base, fade), None);
            }
            lp.y += gap / 2.0;
        }
        let r = (d.ripple & 0xFF) as u8;
        ripple(ui, x, top, w + 2.0 * PAD, h_menu + 2.0 * PAD, r, ca * (DialogFader::F4 - d.f4) as f32 / DialogFader::F4 as f32);
        if d.f4 < DialogFader::C {
            return;
        }
        let span = (DialogFader::F4 - DialogFader::C) as f32;
        let slide2 = 20.0 * (DialogFader::F4 - d.f4) as f32 / span;
        let cyn = a * (d.f4 - DialogFader::C) as f32 / span;
        let ytop = y0 + h_menu + 2.0 * PAD + slide2 + 8.0;
        ui.frame_rect(box_, &ma.bank, with_a(WHITE, cyn), x, ytop, w + 2.0 * PAD, h_yn + 2.0 * PAD);
        ripple(ui, x, ytop, w + 2.0 * PAD, h_yn + 2.0 * PAD, r, cyn);
        let mut lp = centred(p, x + PAD + w / 2.0, y0 + PAD + h1 + 2.0 * PAD + slide2 + 8.0 + h1 / 2.0, w);
        for (n, i) in [5usize, 6].into_iter().enumerate() {
            if let Some(t) = st.get(i) {
                let glow = (self.yes_no as usize == n).then(|| self.glow(cyn));
                text_line(ui, font, &lp, &t.text, with_a(WHITE, cyn), glow);
            }
            lp.y += line;
        }
    }
}

/// A two-way counter bouncing between 0 and `max`.
fn bounce(v: &mut (i16, i16), max: i16) {
    v.0 += v.1;
    if v.0 < 0 {
        v.0 = 0;
        v.1 = 1;
    }
    if v.0 > max {
        v.0 = max;
        v.1 = -1;
    }
}

/// The three packets of a save block: back faces, front faces, then the
/// icon on its face (BS2 draws packets 0 and 2, then 1 with the icon).
fn draw_block<'a>(list: &mut Vec<Prim<'a>>, p: &'a Prepared, m: &Mtx, v: &View, alpha: u8, icon: Option<&'a FastTex>) {
    if alpha == 0 {
        return;
    }
    for d in [0usize, 2] {
        render::draw_model(list, p, m, v, &DrawOpts { alpha, only: Some(d..d + 1), ..Default::default() });
    }
    if let Some(tex) = icon {
        render::draw_model_tex(list, p, m, v, &DrawOpts { alpha, only: Some(1..2), ..Default::default() }, tex);
    }
}

/// CardDraw_LoadTexture: a number from the 110 x 14 digit strip (res 90),
/// digit centres 12 units apart around `x`.
fn draw_number<'a>(ui: &mut Ui<'a, '_>, strip: &'a FastTex, x: f32, y: f32, n: u32, c: Rgba) {
    if c[3] <= 0.0 {
        return;
    }
    let digits: Vec<u32> = alloc::format!("{n}").bytes().map(|b| (b - b'0') as u32).collect();
    let (sw, sh) = strip.size();
    let x0 = x - (digits.len() as f32 - 1.0) * 6.0;
    for (i, d) in digits.into_iter().enumerate() {
        let cx = x0 + i as f32 * 12.0;
        let (u0, u1) = (d as f32 * 10.0 / sw, (d as f32 + 1.0) * 10.0 / sw);
        ui.rect(cx - 5.0, y - sh / 2.0, 10.0, sh, [[u0, 0.0], [u1, 0.0], [u1, 1.0], [u0, 1.0]], c, Some(strip));
    }
}

/// CardDraw_SetScissor: the silver outline that ripples out of a box.
fn ripple(ui: &mut Ui<'_, '_>, x: f32, y: f32, w: f32, h: f32, r: u8, a: f32) {
    let k = a * ((256 - r as i32) * 255 / 256).min(255) as f32 / 255.0;
    if k <= 0.0 {
        return;
    }
    let o = (r as f32 + 16.0) / 16.0;
    let (x0, y0, x1, y1) = (x - o, y - o, x + w + o, y + h + o);
    let c = with_a(rgb(0xC0, 0xC0, 0xC0), k);
    let lw = 3.0;
    ui.fill(x0 - lw / 2.0, y0 - lw / 2.0, x1 - x0 + lw, lw, c);
    ui.fill(x0 - lw / 2.0, y1 - lw / 2.0, x1 - x0 + lw, lw, c);
    ui.fill(x0 - lw / 2.0, y0 + lw / 2.0, lw, y1 - y0 - lw, c);
    ui.fill(x1 - lw / 2.0, y0 + lw / 2.0, lw, y1 - y0 - lw, c);
}

/// A copy of text pane `p` holding one line centred on (x, y), `w` wide.
fn centred(p: &Text, x: f32, y: f32, w: f32) -> Text {
    Text { x, y, w, h: p.size, v_align: 0, ..p.clone() }
}

/// fn_81308DB8 / fn_813090A4: a line of text, optionally over a soft glow
/// (25 copies offset -4..4 units).
fn text_line<'a>(ui: &mut Ui<'a, '_>, font: &'a UiFont, p: &Text, s: &[u8], c: Rgba, glow: Option<Rgba>) {
    if let Some(gc) = glow {
        let ga = with_a(gc, 1.0 / 6.0);
        for dy in -2..=2 {
            for dx in -2..=2 {
                ui.text(font, p, s, ga, dx as f32 * 2.0, dy as f32 * 2.0);
            }
        }
    }
    ui.text(font, p, s, c, 0.0, 0.0);
}

/// SinS / CosS of a 16-bit angle.
fn sin_cos(a: i32) -> (f32, f32) {
    let r = a as i16 as f32 * core::f32::consts::TAU / 65536.0;
    (libm::sinf(r), libm::cosf(r))
}
