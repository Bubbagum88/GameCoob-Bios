//! Memory Card (BS2 `memcard.c` 0x813157B8..0x8131BC00; the blocks are
//! [`super::card_blocks`]): two slots side by side, every save a
//! small glass block with its icon, the banner and comment of the chosen
//! save, and Move / Copy / Erase / Format with BS2's dialogs, messages and
//! block animations. The cards come from [`MemCards`]: fake ones in RAM
//! for now.
//!
//! UI states (`sCardUiState`): 1 browse, 2 operation menu, 3 Yes/No,
//! 4 format question, 5 message. Input sets the *next* state, which takes
//! effect on the following frame, like on the console.

use super::backend::{CardError, CardOp, MemCards, Save, SlotState, DIR_ENTRIES};
use super::card_blocks::{Blocks, Sel, SlotInfo};
use crate::bs2::gbase;
use super::super::render::View;
use super::super::shader::FastTex;
use super::super::ui::{self, Rgba, Ui, UiFont};
use super::{rgb, with_a, Fade, Fader, MenuAssets, Pad, St, Work, WHITE};
use crate::bs2::sound::{self, se};
use crate::ipl::glh::{tag, Strings, Text};
use alloc::vec;
use alloc::vec::Vec;

// ---------------------------------------------------------------------
// Constants (CardParams_SetDefaults and the DOL's tables)
// ---------------------------------------------------------------------

const N: usize = DIR_ENTRIES;
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
    /// The blocks (`memcard_draw.c`).
    blocks: Blocks,
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
        // sCardAnchorX / Y: the cells in 12.4.
        let grid_x = cells.map(|c| c.map(|v| (v[0] * 16.0) as i16));
        let grid_y = cells.map(|c| c.map(|v| (v[1] * 16.0) as i16));
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
            blocks: Blocks::new(grid_x, grid_y),
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
        };
        for s in 0..2 {
            c.sort(cards, s);
        }
        c
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

    /// Card_Enter, CardUi_Reset (the grid) and CardIcon_ResetFiles.
    pub fn enter(&mut self) {
        for i in ENTER_RESET {
            self.msg[i].reset();
        }
        self.rebuild_grid();
        self.blocks.reset_files();
    }

    /// fn_8132056C on the current order and scroll.
    fn rebuild_grid(&mut self) {
        self.blocks.arrange(&self.sorted, self.scroll);
    }

    /// fn_81320ABC with this frame's card states.
    fn settled(&mut self, cards: &dyn MemCards, fade_done: bool) -> bool {
        let ready = [self.ready(cards, 0), self.ready(cards, 1)];
        self.blocks.settled(fade_done, ready)
    }

    /// The cursor, for the blocks.
    fn sel(&self) -> Sel {
        Sel { slot: self.slot, file: self.selected(self.slot) }
    }

    /// fn_81320094 on the chosen save.
    fn dispatch(&mut self, kind: u8) {
        let s = self.slot;
        let (cell, file) = (self.cell_index(s), self.selected(s));
        self.blocks.start(s, cell as u16, file, kind as u16);
    }

    // -----------------------------------------------------------------
    // Card_Update
    // -----------------------------------------------------------------

    /// One frame. `zoom_full`: the cube has zoomed all the way in;
    /// `fade_done`: the screen's three faders are past 172.
    /// `w` and `icon_settled` drive the blocks (CardIcon_IsSettled).
    #[allow(clippy::too_many_arguments)]
    pub fn update(&mut self, p: &Pad, cards: &mut dyn MemCards, w: &Work, icon_settled: bool, zoom_full: bool, fade_done: bool) -> St {
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
        self.ui_update(p, cards, w, icon_settled, zoom_full, fade_done)
    }

    /// CardUi_Update.
    #[allow(clippy::too_many_arguments)]
    fn ui_update(&mut self, p: &Pad, cards: &mut dyn MemCards, w: &Work, icon_settled: bool, zoom_full: bool, fade_done: bool) -> St {
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
        // fn_8131F8F8
        let sel = self.sel();
        let ready = [self.ready(cards, 0), self.ready(cards, 1)];
        self.blocks.update(w, icon_settled, &sel, fade_done, ready);
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
            self.blocks.hide_slot(s);
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
                self.blocks.land(cards.dest_file().unwrap_or(0));
                self.op_kind = match self.op_kind {
                    K_MOVE => {
                        sound::play(if self.op_slot == 0 { se::CARD_MOVED_FROM_A } else { se::CARD_MOVED_FROM_B });
                        K_MOVE_DONE
                    }
                    K_ERASE => K_ERASE_DONE,
                    _ => {
                        sound::play(se::CARD_COPIED);
                        K_COPY_DONE
                    }
                };
            } else {
                self.blocks.cancel();
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
                if matches!(self.op_kind, K_COPY | K_MOVE) && self.busy_frames == 10 {
                    sound::play(if self.op_slot == 0 { se::CARD_SEND_A } else { se::CARD_SEND_B });
                }
                self.busy_frames = (self.busy_frames + 1).min(20);
                self.op_err = self.result(cards);
            }
            if self.op_err != BUSY && self.op_err != 0 {
                self.op_kind = match self.op_kind {
                    K_MOVE => K_MOVE_DONE,
                    K_ERASE => K_ERASE_DONE,
                    _ => K_COPY_DONE,
                };
                self.blocks.cancel();
                sound::play(se::BUZZER);
            }
        } else if self.op_kind == K_FORMAT {
            if self.op_err == BUSY {
                self.op_err = self.result(cards);
            }
            if self.op_err != BUSY {
                sound::play(if self.op_err == 0 { se::CARD_DONE } else { se::BUZZER });
                self.op_kind = K_FORMAT_DONE;
            }
        }
    }

    /// CardUi_Browse.
    fn browse(&mut self, p: &Pad, cards: &dyn MemCards, zoom_full: bool, fade_done: bool) -> St {
        if !self.blocks.entered() {
            return St::Card;
        }
        let s = self.slot;
        let o = s ^ 1;
        if self.ready(cards, s) {
            self.max_scroll[s] = Self::calc_max_scroll(cards)(s);
            let at = |col: i8, row: i8, scroll: i8| col as i32 + (row as i32 + scroll as i32) * 4;
            // A step plays the cursor; a blocked one the buzzer.
            let mv = || sound::play(se::CURSOR);
            let buzz = || {
                if !p.repeat {
                    sound::play(se::BUZZER);
                }
            };
            if p.up {
                self.row[s] -= 1;
                if self.row[s] < 0 {
                    self.row[s] = 0;
                    if self.scroll[s] > 0 {
                        self.scroll[s] -= 1;
                        self.rebuild_grid();
                        mv();
                    } else {
                        buzz();
                    }
                } else {
                    mv();
                }
            }
            if p.down {
                self.row[s] += 1;
                if at(self.col[s], self.row[s], self.scroll[s]) >= N as i32 {
                    buzz();
                    self.row[s] -= 1;
                } else if self.row[s] > 3 {
                    self.row[s] = 3;
                    if self.scroll[s] < self.max_scroll[s] {
                        self.scroll[s] += 1;
                        self.rebuild_grid();
                        mv();
                    } else {
                        buzz();
                    }
                } else {
                    mv();
                }
            }
            if p.left {
                if s == 0 {
                    if self.col[0] > 0 {
                        mv();
                        self.col[0] -= 1;
                    } else {
                        buzz();
                    }
                } else {
                    self.col[1] -= 1;
                    if self.col[1] < 0 {
                        self.col[1] = 0;
                        if self.ready(cards, o) {
                            if at(self.col[o], self.row[s], self.scroll[o]) >= N as i32 {
                                buzz();
                            } else {
                                self.slot = o;
                                self.row[o] = self.row[s];
                                self.col[o] = 3;
                                mv();
                            }
                        } else {
                            buzz();
                        }
                    } else {
                        mv();
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
                            buzz();
                        } else {
                            mv();
                        }
                    } else {
                        buzz();
                    }
                } else {
                    self.col[0] += 1;
                    if at(self.col[0], self.row[0], self.scroll[0]) >= N as i32 {
                        self.col[0] -= 1;
                        buzz();
                    } else if self.col[0] > 3 {
                        self.col[0] = 3;
                        if self.ready(cards, 1) {
                            self.slot = 1;
                            self.row[1] = self.row[0];
                            self.col[1] = 0;
                            mv();
                        } else {
                            buzz();
                        }
                    } else {
                        mv();
                    }
                }
            }
            let s = self.slot;
            let used = Self::save(cards, s, self.selected(s)).is_some();
            if p.a && used && self.settled(cards, fade_done) {
                self.ui_next = 2;
                sound::play(se::DECIDE);
            }
        }
        if p.b && zoom_full && self.settled(cards, fade_done) {
            sound::play(se::EXIT);
            // Leaving asks again about cards it was told to leave alone.
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
            sound::play(se::CURSOR_H);
            self.op_next = if self.op_next == 0 { 2 } else { self.op_next - 1 };
        }
        if p.down {
            sound::play(se::CURSOR_H);
            self.op_next = if self.op_next >= 2 { 0 } else { self.op_next + 1 };
        }
        if p.a && self.op_next == self.op {
            let s = self.slot;
            let file = self.selected(s);
            self.op_slot = s;
            self.yes_no = 1;
            match self.op {
                OP_ERASE => {
                    sound::play(se::DECIDE);
                    self.ui_next = 3;
                }
                op => match self.check(cards, s, file, op == OP_MOVE) {
                    Ok(()) => {
                        sound::play(se::DECIDE);
                        self.ui_next = 3;
                    }
                    Err(e) => {
                        sound::play(se::BUZZER);
                        self.op_kind = if op == OP_MOVE { K_MOVE_DONE } else { K_COPY_DONE };
                        self.op_err = e;
                        self.ui_next = 5;
                    }
                },
            }
        }
        if p.b {
            sound::play(se::CANCEL);
            self.browse_enter(cards, false);
        }
    }

    /// CardUi_Confirm.
    fn confirm(&mut self, p: &Pad, cards: &mut dyn MemCards) {
        if p.up {
            if self.yes_no != 0 {
                sound::play(se::CHANGE);
            }
            self.yes_no = 0;
        }
        if p.down {
            if self.yes_no != 1 {
                sound::play(se::CHANGE);
            }
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
                        sound::play(se::DECIDE);
                        self.dispatch(K_ERASE);
                        cards.start(CardOp::Erase, s, file);
                        self.busy_frames = 0;
                        self.op_err = BUSY;
                    }
                    op => {
                        let (kind, cop) = if op == OP_COPY { (K_COPY, CardOp::Copy) } else { (K_MOVE, CardOp::Move) };
                        self.op_kind = kind;
                        if other_ready {
                            sound::play(se::DECIDE);
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
                sound::play(se::CANCEL);
                self.ui_next = 2;
            }
        }
        if p.b {
            sound::play(se::CANCEL);
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
        for (pressed, v) in [(p.up, self.fmt_next[s] & !1), (p.down, self.fmt_next[s] | 1)] {
            if pressed {
                if v != self.fmt_next[s] {
                    sound::play(se::CURSOR);
                }
                self.fmt_next[s] = v;
            }
        }
        let decline = |c: &mut Card, cards: &dyn MemCards| {
            sound::play(se::CANCEL);
            c.asked[s] = true;
            c.fmt_next[s] = 1;
            c.browse_enter(cards, false);
        };
        if p.a {
            match self.fmt[s] & 3 {
                0 => {
                    self.fmt_next[s] = 3;
                    sound::play(se::DECIDE);
                }
                2 => {
                    sound::play(se::DECIDE);
                    cards.start(CardOp::Format, s, 0);
                    self.ui_next = 5;
                    self.op_slot = s;
                    self.op_kind = K_FORMAT;
                    self.op_err = BUSY;
                }
                _ => decline(self, &*cards),
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
        // The result sound, once, when an operation ran.
        if self.busy_frames != 0 && self.op_err == 0 {
            match self.op_kind {
                K_COPY | K_COPY_DONE => sound::play(se::CARD_DONE),
                K_ERASE | K_ERASE_DONE => sound::play(se::CARD_ERASED),
                _ => {}
            }
        }
        self.busy_frames = 0;
        if p.a || p.b {
            self.op_kind = K_NONE;
            self.yes_no = 1;
            self.op_err = 0;
            self.browse_enter(cards, true);
            sound::play(se::CANCEL);
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

    /// CardUi_CopyResultMsg / CardUi_MoveResultMsg: the result lines of
    /// a Copy (from 19) or a Move (from 7).
    fn xfer_result_msg(&mut self, base: usize, show: bool) {
        let (e, os) = (self.op_err, self.op_slot);
        let map = [(base, 0), (base + 1, NOT_ALLOWED), (base + 3 - os, NO_CARD), (base + 5 - os, UNUSABLE), (base + 8, WRONG_TYPE), (base + 9, EXISTS), (base + 10, NO_SPACE), (base + 11, FAILED)];
        if show {
            for (i, err) in map {
                self.msg[i].step_if(e == err);
            }
        } else {
            for i in (base..base + 6).chain(base + 8..base + 12) {
                self.msg[i].step(Fade::Out);
            }
        }
    }

    /// CardUi_UpdateResultMsg. While the operation runs the lines hold.
    fn update_result_msg(&mut self) {
        let (e, k, os) = (self.op_err, self.op_kind, self.op_slot);
        if self.ui == 5 {
            if e != BUSY {
                if matches!(k, K_COPY | K_COPY_DONE) {
                    self.xfer_result_msg(19, true);
                }
                if matches!(k, K_MOVE | K_MOVE_DONE) {
                    self.xfer_result_msg(7, true);
                }
                if matches!(k, K_ERASE | K_ERASE_DONE) {
                    self.msg[31].step_if(e == 0);
                    self.msg[32].step_if(e != 0);
                }
                if matches!(k, K_FORMAT | K_FORMAT_DONE) {
                    self.msg[43 + os].step_if(e == 0);
                    self.msg[45 + os].step_if(e != 0);
                }
            }
        } else {
            self.xfer_result_msg(19, false);
            self.xfer_result_msg(7, false);
            for i in [31, 32, 43 + os, 45 + os] {
                self.msg[i].step(Fade::Out);
            }
        }
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
    pub fn draw<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, w: &Work, cards: &'a dyn MemCards, lang: usize, al: [f32; 3]) {
        let g = &ma.card;
        let mid = |k: usize| {
            let c = self.cells[k];
            [(c[5][0] + c[6][0]) / 2.0, (c[5][1] + c[9][1]) / 2.0]
        };
        let spot = |k: usize| [mid(k)[0], mid(k)[1], LIGHT_Z];
        let a = |f: &Fader| f.alpha() as u8;
        super::draw_backdrop_spots(ui, ma, w, al[1], [spot(0), spot(1)], [a(&self.cursor_f[0]), a(&self.cursor_f[1])], a(&self.no_cursor_f));
        for f in g.frame(tag(b"mes6")) {
            ui.frame(f, &ma.bank, with_a(WHITE, al[2]));
        }
        let cur = self.slot;
        let mut v = View::intro(0.0, false);
        v.vp = ui.view.vp;
        // fn_813216F4, cursor slot first.
        let fx_alpha = (al[0] * 255.0 + 0.5) as u8;
        let sel = self.sel();
        let used = [0, 1].map(|k| move |i: usize| Self::save(cards, k, i).is_some());
        let icon = [0, 1].map(|k| move |i: usize| Self::save(cards, k, i).and_then(|save| self.icon(save, k, i)));
        let info = [0, 1].map(|k| SlotInfo { used: &used[k], icon: &icon[k], sector_size: cards.sector_size(k) });
        for k in [cur, cur ^ 1] {
            self.blocks.draw_slot(ui.gx, &v, &ma.card_models, k, fx_alpha, &sel, &info);
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
        let bob = gbase::hermite(self.bob.0 as f32, 0.0, 0.0, 0.0, 15.0, 128.0, 0.0) / 16.0;
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
