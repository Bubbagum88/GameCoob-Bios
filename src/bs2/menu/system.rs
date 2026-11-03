//! The system dialog (GameCoob's own, opened with F10 from anywhere in the
//! menu): Firmware Setup, Restart and Power Off, each confirmed with
//! Yes / No, and the Minimal Mode, Intro Logo and Intro Delay switches
//! (no confirmation). It is drawn with BS2's dialog layout (res 108, the one
//! the console uses for its language and "settings lost" prompts).

use super::{Action, Fader, MenuAssets, Pad, WHITE, with_a};
use super::super::ui::{cp1252, Ui};
use crate::ipl::glh::tag;

const ITEMS: [&str; 7] = ["Firmware Setup", "Restart", "Power Off", "Minimal Mode", "Intro Logo", "Intro Delay", "Cancel"];
/// The Minimal Mode row: A flips it at once.
const ROW_MINIMAL: usize = 3;
/// The Intro Logo row: A swaps the intro's wordmark, GameCube or GameCoob.
const ROW_LOGO: usize = 4;
/// The Intro Delay row: A steps the black hold before the intro (for
/// monitors and TVs slow to show a picture): Auto, then 0 to 5 seconds.
const ROW_DELAY: usize = 5;
const QUESTIONS: [&str; 3] = [
    "Restart this PC into its\nfirmware setup screen?",
    "Restart this PC now?",
    "Turn this PC off now?",
];

pub struct System {
    pub open: bool,
    fader: Fader,
    row: usize,
    /// Some(yes) while asking for confirmation.
    confirm: Option<bool>,
    fw_ui: bool,
}

impl System {
    pub fn new(fw_ui: bool) -> System {
        System { open: false, fader: Fader::default(), row: 0, confirm: None, fw_ui }
    }

    pub fn toggle(&mut self) {
        self.open = !self.open;
        self.row = 0;
        self.confirm = None;
    }

    /// True while the dialog is on screen (it takes all input).
    pub fn visible(&self) -> bool {
        self.open || self.fader.alpha() > 0
    }

    /// `minimal` is minimal mode's current state, for its row.
    pub fn update(&mut self, p: &Pad, minimal: bool) -> Option<Action> {
        self.fader.step_if(self.open);
        if !self.open {
            return None;
        }
        if let Some(yes) = self.confirm {
            if p.up {
                self.confirm = Some(true);
            }
            if p.down {
                self.confirm = Some(false);
            }
            if p.b {
                self.confirm = None;
            }
            if p.a {
                self.confirm = None;
                if yes {
                    return Some(match self.row {
                        0 => Action::FirmwareSetup,
                        1 => Action::Restart,
                        _ => Action::PowerOff,
                    });
                }
            }
            return None;
        }
        if p.up && self.row > 0 {
            self.row -= 1;
        }
        if p.down && self.row + 1 < ITEMS.len() {
            self.row += 1;
            if self.row == 0 && !self.fw_ui {
                self.row = 1;
            }
        }
        if p.b || (p.a && self.row == ITEMS.len() - 1) {
            self.open = false;
        } else if p.a && self.row == ROW_MINIMAL {
            return Some(Action::SetMinimal(!minimal));
        } else if p.a && self.row == ROW_LOGO {
            return Some(Action::SetCoobLogo(!crate::bs2::wordmark::coob()));
        } else if p.a && self.row == ROW_DELAY {
            return Some(Action::SetIntroDelay(crate::display::next(crate::display::setting())));
        } else if p.a && (self.row != 0 || self.fw_ui) {
            self.confirm = Some(false);
        }
        None
    }

    pub fn draw<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, lang: usize, pulse: f32) {
        let a = self.fader.alpha() as f32 / 255.0;
        if a <= 0.0 {
            return;
        }
        // Dim what is behind.
        ui.fill(-40.0, -40.0, 664.0, 528.0, [0.0, 0.0, 0.0, 0.55 * a]);
        let d = &ma.dialog;
        let min_row = cp1252(&alloc::format!("{}: {}", ITEMS[ROW_MINIMAL], if super::minimal() { "On" } else { "Off" }));
        let logo = if crate::bs2::wordmark::coob() { "GameCoob" } else { "GameCube" };
        let logo_row = cp1252(&alloc::format!("{}: {}", ITEMS[ROW_LOGO], logo));
        let delay = match crate::display::setting() {
            Some(s) => alloc::format!("{s} s"),
            None => alloc::format!("Auto ({} s)", crate::display::auto()),
        };
        let delay_row = cp1252(&alloc::format!("{}: {}", ITEMS[ROW_DELAY], delay));
        let (q, items): (&str, [Option<&[u8]>; 7]) = match self.confirm {
            Some(_) => {
                let s = &ma.dialog_str[lang];
                let yes = s.get(1).map(|s| &s.text[..]).unwrap_or(b"Yes");
                let no = s.get(2).map(|s| &s.text[..]).unwrap_or(b"No");
                (QUESTIONS[self.row.min(2)], [Some(yes), Some(no), None, None, None, None, None])
            }
            None => (
                "What should this PC do?",
                [ITEMS[0].as_bytes(), ITEMS[1].as_bytes(), ITEMS[2].as_bytes(), &min_row[..], &logo_row[..], &delay_row[..], ITEMS[6].as_bytes()]
                    .map(Some),
            ),
        };
        // The dialog's big frame as the panel, grown by `GROW` to fit seven
        // rows, the question on top.
        const GROW: f32 = 51.0;
        if let Some(f) = d.frame(tag(b"mes0")).next() {
            let (w, h) = (f.inner_w, f.inner_h + GROW);
            ui.frame_rect(f, &ma.bank, with_a(WHITE, a), f.x - w / 2.0, f.y - h / 2.0, w, h);
        }
        if let Some(p) = d.text(tag(b"txt0")) {
            ui.text(&ma.font, p, &cp1252(q), with_a(WHITE, a), 0.0, -64.0 - GROW / 2.0);
        }
        let sel = match self.confirm {
            Some(yes) => usize::from(!yes),
            None => self.row,
        };
        // The rows, the selected one boxed in the dialog's second frame.
        let size = 21.0;
        let n = items.iter().flatten().count();
        let wmax = items.iter().flatten().map(|t| ma.font.width(t, size, 0.0)).fold(0.0f32, f32::max);
        let top = 186.0 - GROW / 2.0;
        let pitch = 25.0;
        if let Some(f) = d.frame(tag(b"mes1")).next() {
            let w = wmax + 24.0;
            let y = top + sel.min(n.saturating_sub(1)) as f32 * pitch;
            ui.frame_rect(f, &ma.bank, with_a(WHITE, a * (0.75 + 0.25 * pulse)), 292.0 - w / 2.0, y - 3.0, w, size + 6.0);
        }
        for (i, item) in items.iter().enumerate() {
            let Some(text) = item else { continue };
            let dim = i == 0 && self.confirm.is_none() && !self.fw_ui;
            let c = if dim { [0.55, 0.55, 0.6, 1.0] } else { WHITE };
            let k = if i == sel { 1.0 } else { 0.75 };
            ui.label(&ma.font, 292.0, top + i as f32 * pitch, size, text, with_a(c, a * k), 0);
        }
    }
}
