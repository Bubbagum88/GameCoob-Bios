//! The Calendar submenu (`calendar.c` and `clock_params.c`, PAL 1.0 BS2
//! 0x81328CA4..0x81329890 and 0x813217F0..0x813218C8): view and set the
//! date and time.
//!
//! Sub-states (`sClockSub`):
//!
//! | sub | what                                                          |
//! |-----|---------------------------------------------------------------|
//! | 0   | up/down picks the date or the time, A edits it, B zooms out   |
//! | 1   | year / month / day                                            |
//! | 2   | hour / minute / second (the second starts at 0)               |
//!
//! In 1 and 2 left/right move between the fields and down/up change the
//! value by -1/+1; A sets the clock, B cancels. On PAL "left" raises the
//! date field index, so the date reads day / month / year.
//!
//! The digits are drawn by the mosaic ([`crate::bs2::fx`]); this unit
//! draws the title and the row labels.
//!
//! GameCoob: Clock_Commit sets the firmware clock ([`Action::SetTime`])
//! instead of the SRAM counter bias, and the title says so. Sound_Play
//! calls are left out, and so is `sClockPulse`, which nothing reads.

use super::{Action, MenuAssets, Pad, St};
use crate::bs2::ui::{cp1252, Ui};
use crate::ipl::glh::tag;
use alloc::format;
use alloc::string::String;

/// `sClockSub` values.
pub const CLOCK_MENU: i16 = 0;
pub const CLOCK_DATE: i16 = 1;
pub const CLOCK_TIME: i16 = 2;

/// Rows (`sClockRow`).
const ROW_DATE: i16 = 0;
const ROW_TIME: i16 = 1;

/// `ClockParams.textColor` (ClockParams_SetDefaults, 0x81321838).
const TEXT_COLOR: [u8; 3] = [255, 255, 0];

const DAYS_IN_MONTH: [i16; 12] = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
const DAYS_IN_MONTH_LEAP: [i16; 12] = [31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];

/// Clock_FieldStep: the way "left" moves the date field, -1 for every PAL
/// language (codes 0 and 2 give 1).
const FIELD_STEP: i16 = -1;

/// What Calendar reads and changes outside itself.
pub struct Ctx<'a> {
    /// Fx_IsReady(0).
    pub fx_ready: bool,
    /// `zoom == 0x7FFF || inputAlways`.
    pub zoomed_in: bool,
    /// OSGetTime as calendar time: year, month 1..12, day, hour, minute,
    /// second.
    pub now: &'a mut [i32; 6],
    /// A note for the menu to show.
    pub note: &'a mut Option<String>,
}

pub struct Calendar {
    /// `sClockShownSub`: the sub-state at the start of the frame.
    pub shown_sub: i16,
    sub: i16,
    pub row: i16,
    /// `sDateField`: 0 year, 1 month, 2 day.
    pub date_field: i16,
    /// `sTimeField`: 0 hour, 1 minute, 2 second.
    pub time_field: i16,
    /// `gClockEdit.value`: year, month 0..11, day; or hour, minute,
    /// second.
    pub value: [i16; 3],
}

impl Calendar {
    /// Clock_Init (0x81328DFC).
    pub fn new() -> Calendar {
        Calendar { shown_sub: CLOCK_MENU, sub: CLOCK_MENU, row: ROW_DATE, date_field: 0, time_field: 0, value: [0; 3] }
    }

    /// Clock_UpdateMenu (0x81328E60).
    fn update_menu(&mut self, p: &Pad, c: &mut Ctx) -> St {
        let mut next = St::Clock;
        if !c.fx_ready {
            return St::Clock;
        }
        if p.up {
            self.row = ROW_DATE;
        }
        if p.down {
            self.row = ROW_TIME;
        }
        if p.a {
            let t = *c.now;
            match self.row {
                ROW_DATE => {
                    self.value = [t[0] as i16, t[1] as i16 - 1, t[2] as i16];
                    self.sub = CLOCK_DATE;
                }
                ROW_TIME => {
                    self.value = [t[3] as i16, t[4] as i16, 0];
                    self.sub = CLOCK_TIME;
                }
                _ => {}
            }
        }
        if p.b && c.zoomed_in {
            next = St::ClockFace;
        }
        next
    }

    /// Clock_FixDate (0x81328FE4): the year wraps in 2000..2099 and the
    /// month in 0..11; the day wraps only while it is the field being
    /// edited, else it is clamped to the month.
    fn fix_date(&mut self) {
        let v = &mut self.value;
        if v[0] < 2000 {
            v[0] = 2099;
        }
        if v[0] > 2099 {
            v[0] = 2000;
        }
        if v[1] < 0 {
            v[1] = 11;
        }
        if v[1] > 11 {
            v[1] = 0;
        }
        let leap = (v[0] % 4 == 0 && v[0] % 100 != 0) || v[0] % 400 == 0;
        let days = if leap { DAYS_IN_MONTH_LEAP } else { DAYS_IN_MONTH }[v[1] as usize];
        if v[2] > days {
            v[2] = if self.date_field == 2 { 1 } else { days };
        }
        if v[2] < 1 {
            v[2] = days;
        }
    }

    /// Clock_FixTime (0x81329338): hours wrap 0..23, minutes and seconds
    /// 0..59.
    fn fix_time(&mut self) {
        let f = self.time_field as usize;
        let max = if f == 0 { 23 } else { 59 };
        if self.value[f] < 0 {
            self.value[f] = max;
        }
        if self.value[f] > max {
            self.value[f] = 0;
        }
    }

    /// Clock_UpdateDate (0x8132912C) and Clock_UpdateTime (0x81329404).
    fn update_edit(&mut self, p: &Pad, c: &mut Ctx) -> Option<Action> {
        let date = self.shown_sub == CLOCK_DATE;
        let step = if date { FIELD_STEP } else { 1 };
        let field = if date { &mut self.date_field } else { &mut self.time_field };
        let side = !(p.up || p.down);
        if p.left && side {
            *field -= step;
        }
        if p.right && side {
            *field += step;
        }
        if *field < 0 {
            *field = 2;
        }
        if *field > 2 {
            *field = 0;
        }
        let f = *field as usize;
        for (pressed, d) in [(p.down, -1), (p.up, 1)] {
            if pressed {
                self.value[f] += d;
                if date {
                    self.fix_date();
                } else {
                    self.fix_time();
                }
            }
        }
        let mut action = None;
        if p.a {
            action = self.commit(c);
            self.sub = CLOCK_MENU;
        }
        if p.b {
            self.sub = CLOCK_MENU;
        }
        action
    }

    /// Clock_Commit (0x81328CA4): the current time with the edited group
    /// put in.
    fn commit(&mut self, c: &mut Ctx) -> Option<Action> {
        let mut t = *c.now;
        let v = self.value.map(|x| x as i32);
        match self.shown_sub {
            CLOCK_DATE => {
                t[0] = v[0];
                t[1] = v[1] + 1;
                t[2] = v[2];
            }
            CLOCK_TIME => {
                t[3..].copy_from_slice(&v);
                if t[0] > 2099 {
                    t[0] = 2000;
                }
            }
            _ => return None,
        }
        *c.now = t;
        *c.note = Some(format!("Firmware clock set: {:02}/{:02}/{} {:02}:{:02}:{:02}", t[2], t[1], t[0], t[3], t[4], t[5]));
        Some(Action::SetTime(t))
    }

    /// Clock_Update (0x81329590), less FxClock_Update (the menu's
    /// `update_fx` runs it after). Returns the next menu state.
    pub fn update(&mut self, p: &Pad, c: &mut Ctx) -> (St, Option<Action>) {
        self.shown_sub = self.sub;
        match self.shown_sub {
            CLOCK_MENU => (self.update_menu(p, c), None),
            CLOCK_DATE | CLOCK_TIME => (St::Clock, self.update_edit(p, c)),
            _ => (St::Clock, None),
        }
    }

    /// Clock_IsTop (0x81329680).
    pub fn is_top(&self) -> bool {
        self.shown_sub == CLOCK_MENU
    }

    /// Clock_Draw (0x8132969C), less the backdrop and the mosaic: the
    /// title in white and the row labels in the text colour. `alpha` is
    /// the text fader.
    pub fn draw_text<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, lang: usize, alpha: i32) {
        let g = &ma.clock;
        let a = alpha as f32 / 255.0;
        // GameCoob: its own title in place of string 0.
        if let Some(p) = g.text(tag(b"mesg")) {
            ui.text(&ma.font, p, &cp1252("The firmware clock."), [1.0, 1.0, 1.0, a], 0.0, 0.0);
        }
        let tc = TEXT_COLOR.map(|x| x as f32 / 255.0);
        for k in [1usize, 2] {
            if let Some(st) = ma.clock_str[lang].get(k) {
                ui.string(&ma.font, g, &ma.bank, st, [tc[0], tc[1], tc[2], a], 0.0, 0.0);
            }
        }
    }
}
