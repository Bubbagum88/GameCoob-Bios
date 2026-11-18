//! GameCoob's Controls page (Settings, down): which key and which gamepad
//! button press each GameCube button and each of the app's own actions.
//! Not in BS2.
//!
//! Pick a row and press A, then press the key or gamepad button it should
//! be: a key changes the keyboard column, a button the controller column.
//! The new key or button leaves whatever it did before (Skip Intro, which
//! only works in the intro, keeps sharing its key with a button). Nothing pressed
//! for a few seconds leaves the row as it was. The last row puts every
//! binding back to the defaults. GameCube controllers on an adapter are
//! read as themselves and never remapped.

use super::subcube::word_label;
use super::{fit, with_a, MenuAssets, Pad, SHADOW, WHITE};
use super::super::sound::{self, se};
use super::super::ui::{cp1252, Ui};
use crate::bs2::pad::{PAD_BUTTON_A, PAD_BUTTON_B};
use crate::input::{key_name, pad_button_name, Bindings, Control, KeyCode, CONTROLS};
use alloc::string::String;
use alloc::vec::Vec;

/// Rows on screen at once.
const VISIBLE: usize = 9;
/// Frames a capture waits for a key or button (5 s at 50 Hz).
const CAPTURE: u32 = 250;
/// Frames after A before a capture listens, so the A that started it (and
/// its key repeat) is not taken as the new binding.
const GRACE: u32 = 20;

/// What a frame of the page did.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Upd {
    Stay,
    /// B: back to the cube's side.
    Back,
    /// The bindings changed: save them and use them.
    Changed,
}

pub struct Controls {
    sel: usize,
    top: usize,
    /// Frames left of a capture on `sel`, if one is on.
    capture: Option<u32>,
    grace: u32,
}

/// Rows: the controls, then "Reset to defaults".
fn rows() -> usize {
    CONTROLS.len() + 1
}

impl Controls {
    pub fn new() -> Controls {
        Controls { sel: 0, top: 0, capture: None, grace: 0 }
    }

    /// Opening the page: the top row, no capture.
    pub fn enter(&mut self) {
        *self = Controls::new();
    }

    /// Waiting for a key or button: the app keeps its own keys (reset,
    /// the system dialog, ...) to itself until then.
    pub fn capturing(&self) -> bool {
        self.capture.is_some()
    }

    /// One frame. `p` is the menu's pad; its `key` and `pad_press` are the
    /// raw key and gamepad buttons pressed this frame.
    pub fn update(&mut self, p: &Pad, b: &mut Bindings, note: &mut Option<String>) -> Upd {
        if let Some(left) = self.capture {
            if self.grace > 0 {
                self.grace -= 1;
                return Upd::Stay;
            }
            let (c, name) = CONTROLS[self.sel];
            if let Some(k) = p.key {
                b.bind_key(c, k);
                self.capture = None;
                sound::play(se::ENTER);
                *note = Some(alloc::format!("{name}: {} on the keyboard.", key_name(k)));
                return Upd::Changed;
            }
            if p.pad_press != 0 {
                let bit = 1u32 << p.pad_press.trailing_zeros();
                b.bind_pad(c, bit);
                self.capture = None;
                sound::play(se::ENTER);
                *note = Some(alloc::format!("{name}: {} on controllers.", pad_button_name(bit)));
                return Upd::Changed;
            }
            if left == 0 {
                self.capture = None;
                sound::play(se::FACE_BACK);
                *note = Some(String::from("Nothing pressed: no change."));
            } else {
                self.capture = Some(left - 1);
            }
            return Upd::Stay;
        }
        // Enter and Escape always work here as A and B if a remap left
        // those with no key at all.
        let a = p.a || (p.key == Some(KeyCode::Char('\r')) && b.keys_for(Control::Button(PAD_BUTTON_A)).is_empty());
        let back = p.b || (p.key == Some(KeyCode::Esc) && b.keys_for(Control::Button(PAD_BUTTON_B)).is_empty());
        if back {
            return Upd::Back;
        }
        let n = rows();
        if p.up && self.sel > 0 {
            self.sel -= 1;
            sound::play(se::CURSOR);
        }
        if p.down && self.sel + 1 < n {
            self.sel += 1;
            sound::play(se::CURSOR);
        }
        if self.sel < self.top {
            self.top = self.sel;
        }
        if self.sel >= self.top + VISIBLE {
            self.top = self.sel + 1 - VISIBLE;
        }
        if a {
            if self.sel == CONTROLS.len() {
                if *b == Bindings::default() {
                    *note = Some(String::from("Already the defaults."));
                    return Upd::Stay;
                }
                *b = Bindings::default();
                sound::play(se::ENTER);
                *note = Some(String::from("All controls back to the defaults."));
                return Upd::Changed;
            }
            sound::play(se::ENTER);
            self.capture = Some(CAPTURE);
            self.grace = GRACE;
        }
        Upd::Stay
    }

    /// The page over the backdrop (584 x 448 units), at alpha `a`.
    pub fn draw<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, b: &Bindings, a: f32) {
        let (white, shadow) = (with_a(WHITE, a), with_a(SHADOW, a));
        let dim = with_a([0.75, 0.8, 0.9, 1.0], a);
        let gold = with_a([1.0, 0.93, 0.6, 1.0], a);
        word_label(ui, ma, 292.0, 62.0, 1.0, "Controls", white);
        const SIZE: f32 = 18.0;
        const COLS: [f32; 3] = [78.0, 262.0, 420.0];
        const WIDTHS: [f32; 3] = [176.0, 150.0, 140.0];
        let text = |ui: &mut Ui<'a, '_>, x: f32, y: f32, w: f32, s: &str, c| {
            let s = fit(&ma.font, &cp1252(s), w, SIZE);
            ui.label(&ma.font, x + 2.0, y + 2.0, SIZE, &s, shadow, 2);
            ui.label(&ma.font, x, y, SIZE, &s, c, 2);
        };
        let y0 = 104.0;
        for (i, h) in ["Button", "Keyboard", "Controller"].into_iter().enumerate() {
            text(ui, COLS[i], y0, WIDTHS[i], h, dim);
        }
        let step = 26.0;
        for (vis, row) in (self.top..rows().min(self.top + VISIBLE)).enumerate() {
            let y = y0 + 32.0 + vis as f32 * step;
            let on = row == self.sel;
            let c = if on { gold } else { white };
            if on {
                ui.label(&ma.font, COLS[0] - 18.0, y, SIZE, b">", c, 2);
            }
            let Some(&(ctl, name)) = CONTROLS.get(row) else {
                text(ui, COLS[0], y, 400.0, "Reset all to defaults", c);
                continue;
            };
            text(ui, COLS[0], y, WIDTHS[0], name, c);
            if on && self.capture.is_some() {
                let secs = self.capture.unwrap_or(0).div_ceil(50).max(1);
                let s = alloc::format!("Press a key or button... {secs}");
                text(ui, COLS[1], y, WIDTHS[1] + WIDTHS[2], &s, gold);
                continue;
            }
            let keys: Vec<String> = b.keys_for(ctl).into_iter().map(key_name).collect();
            let pads: Vec<&str> = b.pad_for(ctl).into_iter().map(pad_button_name).collect();
            let (keys, pads) = (keys.join(", "), pads.join(", "));
            text(ui, COLS[1], y, WIDTHS[1], if keys.is_empty() { "-" } else { &keys }, c);
            text(ui, COLS[2], y, WIDTHS[2], if pads.is_empty() { "-" } else { &pads }, c);
        }
        // More rows above or below.
        if self.top > 0 {
            text(ui, COLS[0] - 18.0, y0 + 32.0 - 14.0, 20.0, "^", dim);
        }
        if self.top + VISIBLE < rows() {
            text(ui, COLS[0] - 18.0, y0 + 32.0 + (VISIBLE as f32 - 0.4) * step, 20.0, "v", dim);
        }
    }
}
