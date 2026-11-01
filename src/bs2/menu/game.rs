//! Game Play (`game.c` and `game_params.c`, PAL 1.0 BS2
//! 0x813275CC..0x81327EE4 and 0x81315724..0x813157B8): the disc banner and
//! a status line that follows the drive, behind the [`DiscDrive`] trait.
//!
//! GameCoob: on the console START boots the disc (`sStarting`, then
//! Disc_StartGame). Here START on a ready disc opens the PC's firmware
//! boot list instead (BootNext): while it is up, up/down pick an entry,
//! A or START boots it and B closes it. "Press START" fades out under the
//! list. Sound and the ambient loop are left out.

use super::backend::{DiscDrive, DiscState};
use super::{draw_backdrop, fit, with_a, Action, Fade, Fader, MenuAssets, Pad, St, SysInfo, Work, WHITE};
use super::super::glh as bglh;
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

/// String index of each status fader 1..5 (Game_Init's `id`s).
const IDS: [u16; 5] = [MSG_PRESS_START, MSG_INSERT_DISC, MSG_ERROR, MSG_READING, MSG_CANT_READ];

/// Rows of the boot list shown at once.
const LIST_ROWS: usize = 5;

/// What Game Play reads outside itself.
pub struct Ctx {
    /// GameIcon_IsSettled.
    pub icon_settled: bool,
    /// Fx_IsReady(0).
    pub fx_ready: bool,
    /// `zoom == 0x7FFF || inputAlways`.
    pub zoomed_in: bool,
}

pub struct Game {
    /// `sGameFader`: [0] banner, then the status lines in `IDS` order.
    faders: [Fader; 6],
    /// `sStarting`: START was pressed; here the boot list is up.
    starting: bool,
    /// `sFatal`.
    fatal: bool,
    sel: usize,
    /// The boot list's fader.
    list: Fader,
}

impl Game {
    /// Game_Init (0x813275F4).
    pub fn new() -> Game {
        Game { faders: [Fader::default(); 6], starting: false, fatal: false, sel: 0, list: Fader::default() }
    }

    /// Game_Enter (0x81327A10), less FxGame_Update (the menu runs it):
    /// every fader starts from zero. GameCoob: the boot list closes.
    pub fn enter(&mut self) {
        for f in self.faders.iter_mut().skip(1) {
            f.reset();
        }
        self.faders[0].reset();
        self.starting = false;
        self.sel = 0;
        self.list.reset();
    }

    /// Game_FxMessage: what the mosaic shows, 0 ready, 1 no disc, 2 other.
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

    /// Game_IsFatal (0x81327AB4).
    pub fn fatal(&self) -> bool {
        self.fatal
    }

    /// Game_HandleInput (0x813276AC): B leaves, START on a ready disc
    /// starts.
    fn handle_input(&mut self, p: &Pad, c: &Ctx, drive: &dyn DiscDrive) -> St {
        let mut next = St::Game;
        if !c.icon_settled || !c.fx_ready || self.fatal {
            return St::Game;
        }
        // Disc_GetFadeAlpha is 0 here: Disc_FadeOutStart keeps it parked.
        if p.b && c.zoomed_in {
            next = St::GameFace;
        }
        if p.start && drive.state() == DiscState::Ready {
            self.starting = true;
            self.sel = 0;
        }
        next
    }

    /// GameCoob: the boot list, in Disc_StartGame's place.
    fn update_list(&mut self, p: &Pad, drive: &dyn DiscDrive, info: &SysInfo) -> Option<Action> {
        let n = info.entries.len();
        let mut action = None;
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
        action
    }

    /// Game_Update (0x81327920), less FxGame_Update (the menu's
    /// `update_fx` runs it after). Returns the next menu state.
    pub fn update(&mut self, p: &Pad, c: &Ctx, drive: &dyn DiscDrive, info: &SysInfo) -> (St, Option<Action>) {
        let mut next = St::Game;
        let mut action = None;
        if self.starting {
            action = self.update_list(p, drive, info);
        } else {
            next = self.handle_input(p, c, drive);
        }
        self.update_messages(drive);
        self.list.step_if(self.starting);
        self.faders[0].step_if(drive.banner().is_some());
        (next, action)
    }

    /// Game_UpdateMessages (0x813277A8): fade the status lines with the
    /// drive state.
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

    /// Game_Draw (0x81327CCC) up to the mosaic: the backdrop and the
    /// banner. `al` is the fx, backdrop and text faders (0..255).
    pub fn draw_back<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, w: &Work, drive: &'a dyn DiscDrive, al: [i32; 3]) {
        draw_backdrop(ui, ma, w, al[1] as f32 / 255.0);
        self.draw_banner(ui, ma, drive, al[2]);
    }

    /// Game_Draw after the mosaic: the status lines (Fader_DrawString on
    /// faders 1, 2, 4 and 5; the fatal line 3 is the scene's), then the
    /// GameCoob boot list.
    pub fn draw_text<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, info: &SysInfo, lang: usize, text_alpha: i32) {
        let g = &ma.game;
        let s = &ma.game_str[lang];
        for k in [1usize, 2, 4, 5] {
            let (a, slide) = self.faders[k].alpha_slide();
            let a = text_alpha * a / 255;
            if a == 0 {
                continue;
            }
            if let Some(st) = s.get(IDS[k - 1] as usize) {
                ui.string(&ma.font, g, &ma.bank, st, with_a(WHITE, a as f32 / 255.0), 0.0, slide as f32 / 16.0);
            }
        }
        self.draw_list(ui, ma, info, text_alpha as f32 / 255.0);
    }

    /// Game_DrawBanner (0x81327ABC): the banner image ("bana") and its
    /// name, maker and description (the backend picks the BNR2 language).
    fn draw_banner<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, drive: &'a dyn DiscDrive, alpha: i32) {
        let a = self.faders[0].alpha();
        if a == 0 {
            return;
        }
        let Some(b) = drive.banner() else { return };
        // GameCoob: the banner gives way to the boot list.
        let a = alpha * a / 255 * (255 - self.list.alpha()) / 255;
        let c = [255, 255, 255, a as u8];
        let g = &ma.game;
        for p in g.pic(tag(b"bana")) {
            ui.pic_tex(p, &b.image, with_a(WHITE, a as f32 / 255.0));
        }
        ui.load();
        for (t, s, n, line) in [(b"titl", &b.long_name, 0x40, true), (b"makr", &b.long_maker, 0x40, true), (b"info", &b.description, 0x80, false)] {
            let s = cp1252(s);
            if line {
                bglh::draw_banner_line(ui.gx, &ma.font, tag(t), g, &ma.bank, c, &s, n);
            } else {
                bglh::draw_banner_box(ui.gx, &ma.font, tag(t), g, &ma.bank, c, &s, n);
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
