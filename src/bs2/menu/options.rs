//! The Options submenu (`options.c` and `option_params.c`, PAL 1.0 BS2
//! 0x81327EE4..0x81328CA4 and 0x81321730..0x813217F0): sound mode, screen
//! position and language.
//!
//! Sub-states (`sOptSub`):
//!
//! | sub | what                                                          |
//! |-----|---------------------------------------------------------------|
//! | 0   | up/down picks a row, A enters it, B zooms back out            |
//! | 1   | sound: left mono, right stereo; A keeps it, B restores it     |
//! | 2   | screen position; A keeps it, B restores it                    |
//! | 3   | language: left/right browse (the guide follows), A sets it    |
//!
//! The rows themselves are drawn by the mosaic ([`crate::bs2::fx`]); this
//! unit draws the titles and the language list.
//!
//! GameCoob: the screen-position row edits the firmware boot timeout
//! (0..[`MAX_TIMEOUT`] seconds) and its title says so; the sound mode is
//! kept in RAM (no audio yet). Sound_Play calls are left out.

use super::{Action, Fade, Fader, MenuAssets, Pad, St};
use crate::bs2::ui::{cp1252, Ui};
use crate::ipl::glh::{tag, Layout};
use alloc::format;
use alloc::string::String;

/// `sOptSub` values.
pub const OPT_MENU: i16 = 0;
pub const OPT_SOUND: i16 = 1;
pub const OPT_OFFSET: i16 = 2;
pub const OPT_LANG: i16 = 3;

/// Rows (`sOptRow`).
const ROW_SOUND: i16 = 0;
const ROW_OFFSET: i16 = 1;
const ROW_LANG: i16 = 2;

/// GameCoob: the longest firmware boot timeout the offset row sets.
pub const MAX_TIMEOUT: u16 = 30;

const LANGS: usize = 6;

/// `OptionParams` (OptionParams_SetDefaults, 0x81321778): the parts this
/// unit reads.
struct Params {
    text_color: [u8; 3],
    item_ofs: [f32; 3],
    lang_ofs: f32,
}

const P: Params = Params { text_color: [0, 255, 120], item_ofs: [-15.0, -4.0, 6.0], lang_ofs: 8.0 };

/// What Options reads and changes outside itself.
pub struct Ctx<'a> {
    /// Fx_IsReady(0).
    pub fx_ready: bool,
    /// `zoom == 0x7FFF || inputAlways`.
    pub zoomed_in: bool,
    /// Lang_Get / Lang_Set.
    pub lang: &'a mut usize,
    /// Menu_SetGuideLang.
    pub guide_lang: &'a mut usize,
    /// The firmware boot timeout (the SRAM display offset's place).
    pub timeout: &'a mut u16,
    /// A note for the menu to show.
    pub note: &'a mut Option<String>,
}

pub struct Options {
    /// `sOptPrevSub`: the sub-state at the start of the frame.
    pub prev_sub: i16,
    sub: i16,
    pub row: i16,
    /// `sOptOffset`: here the boot timeout being edited.
    pub offset: i16,
    /// `sOptSound`: 0 mono, 1 stereo.
    pub sound: i16,
    /// OSGetSoundMode: kept in RAM.
    sound_mode: i16,
    /// `sOptLang`: the language being browsed.
    pub lang: usize,
    lang_name: [Fader; LANGS],
    lang_text: [Fader; LANGS],
    /// `sOptLangY`: y of the `lan1`..`lan6` anchors.
    pub lang_y: [i16; LANGS],
}

impl Options {
    /// Option_Init (0x81327F08).
    pub fn new(layout: &Layout, lang: usize, timeout: u16) -> Options {
        let mut lang_y = [0i16; LANGS];
        for (i, y) in lang_y.iter_mut().enumerate() {
            for p in layout.pic(tag(&[b'l', b'a', b'n', b'1' + i as u8])) {
                *y = libm::floorf(p.y) as i16;
            }
        }
        Options {
            prev_sub: OPT_MENU,
            sub: OPT_MENU,
            row: ROW_SOUND,
            offset: timeout as i16,
            sound: 1,
            sound_mode: 1,
            lang,
            lang_name: [Fader::default(); LANGS],
            lang_text: [Fader::default(); LANGS],
            lang_y,
        }
    }

    /// GameCoob: Options opens on its top row, at the menu.
    pub fn enter(&mut self) {
        self.sub = OPT_MENU;
        self.prev_sub = OPT_MENU;
        self.row = ROW_SOUND;
    }

    /// Option_UpdateMenu (0x81328104).
    fn update_menu(&mut self, p: &Pad, c: &mut Ctx) -> St {
        if !c.fx_ready {
            return St::Option;
        }
        if p.up && self.row > 0 {
            self.row -= 1;
        }
        if p.down && self.row < 2 {
            self.row += 1;
        }
        if p.a {
            match self.row {
                ROW_SOUND => {
                    self.sound = self.sound_mode;
                    self.sub = OPT_SOUND;
                }
                ROW_OFFSET => {
                    self.sub = OPT_OFFSET;
                    self.offset = *c.timeout as i16;
                }
                ROW_LANG => {
                    self.sub = OPT_LANG;
                    self.lang = *c.lang;
                }
                _ => {}
            }
        }
        if p.b && c.zoomed_in {
            return St::OptionFace;
        }
        St::Option
    }

    /// Option_UpdateSound (0x8132827C).
    fn update_sound(&mut self, p: &Pad) {
        if p.left {
            self.sound = 0;
        }
        if p.right {
            self.sound = 1;
        }
        if p.a {
            // OSSetSoundMode, Sound_SetOutputMode
            self.sound_mode = self.sound;
            self.sub = OPT_MENU;
        }
        if p.b {
            self.sound = self.sound_mode;
            self.sub = OPT_MENU;
        }
    }

    /// Option_UpdateOffset (0x813283A0), on the boot timeout.
    fn update_offset(&mut self, p: &Pad, c: &mut Ctx) -> Option<Action> {
        let mut action = None;
        if p.left {
            self.offset -= 1;
            if self.offset < 0 {
                self.offset = 0;
            }
        }
        if p.right {
            self.offset += 1;
            if self.offset > MAX_TIMEOUT as i16 {
                self.offset = MAX_TIMEOUT as i16;
            }
        }
        if p.a {
            // The SRAM write.
            let t = self.offset as u16;
            if t != *c.timeout {
                *c.timeout = t;
                *c.note = Some(format!("Firmware boot timeout: {} s", t));
                action = Some(Action::SetTimeout(t));
            }
            self.sub = OPT_MENU;
        }
        if p.b {
            self.offset = *c.timeout as i16;
            self.sub = OPT_MENU;
        }
        action
    }

    /// Option_UpdateLang (0x81328510).
    fn update_lang(&mut self, p: &Pad, c: &mut Ctx) {
        if p.left {
            self.lang = self.lang.saturating_sub(1);
            *c.guide_lang = self.lang;
        }
        if p.right {
            self.lang = (self.lang + 1).min(LANGS - 1);
            *c.guide_lang = self.lang;
        }
        if p.a {
            // Lang_Set, then Option_LoadLangRes.
            *c.lang = self.lang;
            self.sub = OPT_MENU;
        }
        if p.b {
            self.lang = *c.lang;
            *c.guide_lang = self.lang;
            self.sub = OPT_MENU;
        }
    }

    /// Option_Update (0x81328648), less FxOption_Update (the menu's
    /// `update_fx` runs it after). Returns the next menu state.
    pub fn update(&mut self, p: &Pad, c: &mut Ctx) -> (St, Option<Action>) {
        let mut next = St::Option;
        let mut action = None;
        self.prev_sub = self.sub;
        match self.prev_sub {
            OPT_MENU => next = self.update_menu(p, c),
            OPT_SOUND => self.update_sound(p),
            OPT_OFFSET => action = self.update_offset(p, c),
            OPT_LANG => self.update_lang(p, c),
            _ => {}
        }
        // Opt_StepFaders
        for i in 0..LANGS {
            self.lang_name[i].step(if i == self.lang { Fade::In } else { Fade::Half });
            self.lang_text[i].step(if i == self.lang { Fade::In } else { Fade::Out });
        }
        (next, action)
    }

    /// Option_IsTop (0x813287D4).
    pub fn is_top(&self) -> bool {
        self.prev_sub == OPT_MENU
    }

    /// Option_DrawText (0x81328814): the titles, cross-faded between
    /// languages, and the language list. `alpha` is the text fader.
    pub fn draw_text<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, alpha: i32) {
        let g = &ma.option;
        let tc = P.text_color;
        let rgb = |a: f32| [tc[0] as f32 / 255.0, tc[1] as f32 / 255.0, tc[2] as f32 / 255.0, a];
        // GameCoob: its own prompt and the boot timeout's title, in English
        // only, so they are not cross-faded.
        let full = alpha as f32 / 255.0;
        if let Some(p) = g.text(tag(b"mesg")) {
            ui.text(&ma.font, p, &cp1252("Choose GameCoob boot settings."), [1.0, 1.0, 1.0, full], 0.0, 0.0);
        }
        if let Some(p) = ma.option_str[0].get(2).and_then(|st| g.texts.get(st.pane as usize)) {
            ui.text(&ma.font, p, &cp1252("Boot Timeout"), rgb(full), 0.0, P.item_ofs[1]);
        }
        for i in 0..LANGS {
            let a = (alpha * self.lang_text[i].alpha() / 255) as f32 / 255.0;
            if a <= 0.0 {
                continue;
            }
            for k in [1usize, 3] {
                if let Some(st) = ma.option_str[i].get(k) {
                    ui.string(&ma.font, g, &ma.bank, st, rgb(a), 0.0, P.item_ofs[k - 1]);
                }
            }
        }
        let dy = P.lang_ofs;
        for i in 0..LANGS {
            let a = (alpha * self.lang_name[i].alpha() / 255) as f32 / 255.0;
            for p in g.pic(tag(&[b'l', b'a', b'n', b'1' + i as u8])) {
                if let Some(tex) = ma.bank.get(p.tex as usize) {
                    ui.pic(p, tex, [1.0, 1.0, 1.0, a], 0.0, dy);
                }
            }
        }
        // The separators between the names, half transparent.
        let a = ((alpha << 7) / 255) as f32 / 255.0;
        for i in 0..5u8 {
            for p in g.pic(tag(&[b's', b'l', b'a', b'1' + i])) {
                if let Some(tex) = ma.bank.get(p.tex as usize) {
                    ui.pic(p, tex, [1.0, 1.0, 1.0, a], 0.0, dy);
                }
            }
        }
    }
}
