//! The system dialog (GameCoob's own, opened with F10 from anywhere in the
//! menu): Firmware Setup, Restart and Power Off, each confirmed with
//! Yes / No. It is drawn with BS2's dialog layout (res 108, the one the
//! console uses for its language and "settings lost" prompts).

use super::{Action, Fader, MenuAssets, Pad, WHITE, with_a};
use super::super::ui::{cp1252, Ui};
use crate::ipl::glh::tag;

const ITEMS: [&str; 4] = ["Firmware Setup", "Restart", "Power Off", "Cancel"];
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

    pub fn update(&mut self, p: &Pad) -> Option<Action> {
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
        let (q, items): (&str, [Option<&[u8]>; 4]) = match self.confirm {
            Some(_) => {
                let s = &ma.dialog_str[lang];
                let yes = s.get(1).map(|s| &s.text[..]).unwrap_or(b"Yes");
                let no = s.get(2).map(|s| &s.text[..]).unwrap_or(b"No");
                (QUESTIONS[self.row.min(2)], [Some(yes), Some(no), None, None])
            }
            None => ("What should this PC do?", [ITEMS[0], ITEMS[1], ITEMS[2], ITEMS[3]].map(|s| Some(s.as_bytes()))),
        };
        // The dialog's big frame as the panel, the question on top.
        if let Some(f) = d.frame(tag(b"mes0")).next() {
            ui.frame(f, &ma.bank, with_a(WHITE, a));
        }
        if let Some(p) = d.text(tag(b"txt0")) {
            ui.text(&ma.font, p, &cp1252(q), with_a(WHITE, a), 0.0, -64.0);
        }
        let sel = match self.confirm {
            Some(yes) => usize::from(!yes),
            None => self.row,
        };
        // The rows, the selected one boxed in the dialog's second frame.
        let size = 22.0;
        let n = items.iter().flatten().count();
        let wmax = items.iter().flatten().map(|t| ma.font.width(t, size, 0.0)).fold(0.0f32, f32::max);
        let top = 196.0;
        let pitch = 30.0;
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
