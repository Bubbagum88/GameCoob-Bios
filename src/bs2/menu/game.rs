//! Game Play (BS2 `game.c`, 0x813275CC..0x81327EE4): the disc banner and a
//! status line that follows the drive, behind the [`DiscDrive`] trait.
//!
//! On the console START boots the disc. Here START on a ready disc opens
//! the PC's firmware boot list instead (BootNext), so the PC can still
//! start something.

use super::backend::{DiscDrive, DiscState};
use super::{draw_backdrop, fit, with_a, Action, Fade, Fader, MenuAssets, Pad, St, SysInfo, WHITE};
use super::super::ui::{cp1252, Ui};
use crate::ipl::glh::tag;
use alloc::format;
use alloc::vec::Vec;

/// Status lines: indexes into the game strings (res 22..27).
const MSG_PRESS_START: u16 = 0;
const MSG_ERROR: u16 = 1;
const MSG_CANT_READ: u16 = 2;
const MSG_INSERT_DISC: u16 = 3;
const MSG_READING: u16 = 4;

/// Rows of the boot list shown at once.
const LIST_ROWS: usize = 5;

pub struct Game {
    /// [0] banner, then the status lines in `IDS` order.
    faders: [Fader; 6],
    /// START was pressed: the boot list is up.
    starting: bool,
    fatal: bool,
    sel: usize,
    /// The boot list's fader.
    list: Fader,
}

/// String index of each status fader 1..5.
const IDS: [u16; 5] = [MSG_PRESS_START, MSG_INSERT_DISC, MSG_ERROR, MSG_READING, MSG_CANT_READ];

impl Game {
    pub fn new() -> Game {
        Game { faders: [Fader::default(); 6], starting: false, fatal: false, sel: 0, list: Fader::default() }
    }

    /// Game_Enter: every fader starts from zero.
    pub fn enter(&mut self) {
        for f in self.faders.iter_mut() {
            f.reset();
        }
        self.starting = false;
        self.sel = 0;
        self.list.reset();
    }

    /// What the mosaic shows (Game_FxMessage): 0 ready, 1 no disc, 2 other.
    pub fn fx_message(drive: &dyn DiscDrive) -> u8 {
        match drive.state() {
            DiscState::Ready => 0,
            DiscState::NoDisc => 1,
            _ => 2,
        }
    }

    pub fn starting(&self) -> bool {
        self.starting
    }

    pub fn fatal(&self) -> bool {
        self.fatal
    }

    /// Game_Update. `ready` is true once the zoom, icons and mosaic have
    /// settled (the console waits for all three before taking input).
    pub fn update(&mut self, p: &Pad, drive: &dyn DiscDrive, info: &SysInfo, ready: bool) -> (St, Option<Action>) {
        let mut next = St::Game;
        let mut action = None;
        if self.starting {
            let n = info.entries.len();
            if p.up && self.sel > 0 {
                self.sel -= 1;
            }
            if p.down && self.sel + 1 < n {
                self.sel += 1;
            }
            if (p.a || p.start) && n > 0 {
                action = Some(Action::Boot(info.entries[self.sel.min(n - 1)].0));
            }
            if p.b || drive.state() != DiscState::Ready {
                self.starting = false;
            }
        } else if ready && !self.fatal {
            if p.b {
                next = St::GameFace;
            }
            if p.start && drive.state() == DiscState::Ready {
                self.starting = true;
                self.sel = 0;
            }
        }
        self.update_messages(drive);
        self.list.step_if(self.starting);
        self.faders[0].step_if(drive.banner().is_some());
        (next, action)
    }

    /// Game_UpdateMessages: fade the status lines with the drive state.
    fn update_messages(&mut self, drive: &dyn DiscDrive) {
        let st = if self.fatal { DiscState::Fatal } else { drive.state() };
        let f = &mut self.faders;
        f[1].step_if(st == DiscState::Ready && !self.starting);
        f[2].step_if(matches!(st, DiscState::NoDisc | DiscState::CoverOpen));
        f[5].step_if(matches!(st, DiscState::Retry | DiscState::Wrong));
        f[3].step_if(st == DiscState::Fatal);
        match st {
            DiscState::Ready => f[4].step(Fade::Out),
            DiscState::Fatal => {
                self.starting = false;
                self.fatal = true;
                f[4].step(Fade::Out);
            }
            DiscState::NoDisc | DiscState::CoverOpen | DiscState::Wrong | DiscState::Retry => {
                self.starting = false;
                f[4].step(Fade::Out);
            }
            DiscState::Reading => f[4].step(Fade::In),
        }
    }

    /// Game_Draw: backdrop, banner, then the status lines (the mosaic is
    /// drawn by the caller between the two).
    pub fn draw<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, drive: &'a dyn DiscDrive, info: &SysInfo, lang: usize, al: [f32; 3]) {
        draw_backdrop(ui, ma, al[1]);
        self.draw_banner(ui, ma, drive, al[2]);
        let g = &ma.game;
        let s = &ma.game_str[lang];
        for (k, &id) in IDS.iter().enumerate() {
            let a = self.faders[k + 1].alpha() as f32 / 255.0 * al[2];
            if let Some(st) = s.get(id as usize) {
                ui.string(&ma.font, g, &ma.bank, st, with_a(WHITE, a), 0.0, 0.0);
            }
        }
        self.draw_list(ui, ma, info, al[2]);
    }

    fn draw_banner<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, drive: &'a dyn DiscDrive, alpha: f32) {
        let a = self.faders[0].alpha() as f32 / 255.0 * alpha * (1.0 - self.list.alpha() as f32 / 255.0);
        let Some(b) = drive.banner() else { return };
        if a <= 0.0 {
            return;
        }
        let g = &ma.game;
        let c = with_a(WHITE, a);
        for p in g.pic(tag(b"bana")) {
            ui.pic_tex(p, &b.image, c);
        }
        for (t, s) in [(b"titl", &b.long_name), (b"makr", &b.long_maker), (b"info", &b.description)] {
            if let Some(p) = g.text(tag(t)) {
                ui.text(&ma.font, p, &cp1252(s), c, 0.0, 0.0);
            }
        }
    }

    /// The firmware boot list, in the status box once START is pressed.
    fn draw_list<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, info: &SysInfo, alpha: f32) {
        let a = self.list.alpha() as f32 / 255.0 * alpha;
        if a <= 0.0 {
            return;
        }
        let Some(box_) = ma.game.frame(tag(b"mes1")).next() else { return };
        let n = info.entries.len();
        let line = 25.0;
        let h = LIST_ROWS as f32 * line + 8.0;
        let (bx, bw) = (box_.x - box_.inner_w / 2.0, box_.inner_w);
        let by = box_.y - h / 2.0;
        ui.frame_rect(box_, &ma.bank, with_a(WHITE, a), bx, by, bw, h);
        let first = self.sel.saturating_sub(LIST_ROWS - 1).min(n.saturating_sub(LIST_ROWS));
        for (i, (num, desc)) in info.entries.iter().enumerate().skip(first).take(LIST_ROWS) {
            let y = by + 4.0 + (i - first) as f32 * line;
            let sel = i == self.sel;
            if sel {
                ui.fill(bx - 6.0, y, bw + 12.0, line - 2.0, [1.0, 1.0, 1.0, 0.3 * a]);
            }
            let text: Vec<u8> = fit(&ma.font, &cp1252(&format!("{num:04X}  {desc}")), bw - 20.0, 20.0);
            let c = if sel { WHITE } else { [0.85, 0.8, 0.95, 0.85] };
            ui.label(&ma.font, bx + 6.0, y + 1.0, 20.0, &text, with_a(c, a), 2);
        }
        if n == 0 {
            ui.label(&ma.font, box_.x, by + h / 2.0 - 10.0, 20.0, &cp1252("No firmware boot entries."), with_a(WHITE, a), 0);
        }
    }
}
