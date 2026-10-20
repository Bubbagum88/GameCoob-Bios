//! The main menu: a big spinning cube that turns to a new colored face for each
//! option, and a list of options beside it. Settings are standard UEFI
//! variables, so changes here are what the firmware itself will honor.

use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec::Vec;
use core::time::Duration;
use uefi::proto::console::gop::GraphicsOutput;
use uefi::proto::console::text::{Key, ScanCode};
use uefi::runtime::{self, ResetType};
use uefi::{Status, boot, system};

use crate::clock::Clock;
use crate::gfx::{Camera, Canvas, Rgb, draw_box, mix, rgb, shade, text_width};
use crate::intro::Palette;
use crate::math::*;
use crate::vars::{self, BootEntry};

#[derive(Clone, Copy, PartialEq)]
enum Action {
    FirmwareSetup,
    Restart,
    PowerOff,
}

#[derive(Clone, Copy, PartialEq)]
enum Screen {
    Main,
    Boot,
    Settings,
    Confirm(Action),
}

struct Item {
    label: &'static str,
    hint: &'static str,
    color: Rgb,
}

const ITEMS: [Item; 5] = [
    Item { label: "Boot", hint: "Start an operating system", color: rgb(96, 116, 255) },
    Item { label: "Settings", hint: "Boot timeout and boot order", color: rgb(56, 196, 172) },
    Item { label: "Firmware Setup", hint: "Restart into the vendor BIOS screen", color: rgb(244, 172, 60) },
    Item { label: "Restart", hint: "Reboot this machine", color: rgb(176, 104, 244) },
    Item { label: "Power Off", hint: "Shut this machine down", color: rgb(240, 88, 104) },
];

/// Cube face (gfx FACES order: +X, -X, +Y, -Y, +Z, -Z) shown for each item,
/// matching the rotation `cube_target` produces.
fn cube_faces() -> [Rgb; 6] {
    [ITEMS[3].color, ITEMS[1].color, ITEMS[4].color, rgb(90, 90, 110), ITEMS[0].color, ITEMS[2].color]
}

/// (yaw, pitch) that brings item `i`'s face toward the camera.
fn cube_target(i: usize) -> (f32, f32) {
    match i {
        0..=3 => (i as f32 * PI * 0.5, 0.0),
        _ => (PI * 1.5, PI * 0.5),
    }
}

const MAX_TIMEOUT: u16 = 30;

struct Menu {
    screen: Screen,
    sel: usize,
    boot_sel: usize,
    entries: Vec<BootEntry>,
    timeout: u16,
    message: Option<(String, f32)>,
    yaw: f32,
    pitch: f32,
    clock_text: String,
    clock_at: f32,
    vendor: String,
    fw_ui: bool,
}

pub fn run(gop: &mut GraphicsOutput, cv: &mut Canvas, clock: &Clock, pal: Palette) -> ! {
    let mut m = Menu {
        screen: Screen::Main,
        sel: 0,
        boot_sel: 0,
        entries: vars::boot_entries(),
        timeout: vars::timeout().unwrap_or(5).min(MAX_TIMEOUT),
        message: None,
        yaw: -PI,
        pitch: 0.6,
        clock_text: String::new(),
        clock_at: -10.0,
        vendor: system::firmware_vendor().to_string(),
        fw_ui: vars::firmware_ui_supported(),
    };
    let start = clock.now();
    let mut last = start;
    loop {
        let now = clock.now();
        let dt = (now - last).min(0.1);
        last = now;
        let t = now - start;

        while let Ok(Some(key)) = system::with_stdin(|i| i.read_key()) {
            m.handle(key, t);
        }

        let (ty, tp) = cube_target(m.sel);
        let k = clamp01(dt * 7.0);
        m.yaw += (ty - m.yaw) * k;
        m.pitch += (tp - m.pitch) * k;

        if t - m.clock_at > 0.5 {
            m.clock_at = t;
            if let Ok(tm) = runtime::get_time() {
                m.clock_text = format!(
                    "{:04}-{:02}-{:02}  {:02}:{:02}",
                    tm.year(),
                    tm.month(),
                    tm.day(),
                    tm.hour(),
                    tm.minute()
                );
            }
        }

        m.draw(cv, &pal, t);
        cv.fade(clamp01(t / 0.4));
        cv.present(gop);

        let spent = clock.now() - now;
        if spent < 0.016 {
            boot::stall(Duration::from_micros(((0.016 - spent) * 1e6) as u64));
        }
    }
}

impl Menu {
    fn say(&mut self, msg: impl Into<String>, t: f32) {
        self.message = Some((msg.into(), t));
    }

    fn handle(&mut self, key: Key, t: f32) {
        let enter = matches!(key, Key::Printable(c) if char::from(c) == '\r');
        let (up, down, left, right, esc) = match key {
            Key::Special(s) => (
                s == ScanCode::UP,
                s == ScanCode::DOWN,
                s == ScanCode::LEFT,
                s == ScanCode::RIGHT,
                s == ScanCode::ESCAPE,
            ),
            _ => (false, false, false, false, false),
        };
        match self.screen {
            Screen::Main => {
                if up {
                    self.sel = (self.sel + ITEMS.len() - 1) % ITEMS.len();
                } else if down {
                    self.sel = (self.sel + 1) % ITEMS.len();
                } else if enter {
                    self.screen = match self.sel {
                        0 => {
                            self.entries = vars::boot_entries();
                            self.boot_sel = 0;
                            Screen::Boot
                        }
                        1 => Screen::Settings,
                        2 if !self.fw_ui => {
                            self.say("This firmware does not support rebooting into setup.", t);
                            Screen::Main
                        }
                        2 => Screen::Confirm(Action::FirmwareSetup),
                        3 => Screen::Confirm(Action::Restart),
                        _ => Screen::Confirm(Action::PowerOff),
                    };
                }
            }
            Screen::Boot => {
                let n = self.entries.len().max(1);
                if up {
                    self.boot_sel = (self.boot_sel + n - 1) % n;
                } else if down {
                    self.boot_sel = (self.boot_sel + 1) % n;
                } else if esc {
                    self.screen = Screen::Main;
                } else if enter {
                    if let Some(e) = self.entries.get(self.boot_sel) {
                        match vars::set_boot_next(e.num) {
                            Ok(()) => runtime::reset(ResetType::COLD, Status::SUCCESS, None),
                            Err(err) => self.say(format!("Could not set BootNext: {:?}", err.status()), t),
                        }
                    }
                }
            }
            Screen::Settings => {
                if esc || enter {
                    self.screen = Screen::Main;
                } else if left || right {
                    let new = if left {
                        self.timeout.saturating_sub(1)
                    } else {
                        (self.timeout + 1).min(MAX_TIMEOUT)
                    };
                    if new != self.timeout {
                        match vars::set_timeout(new) {
                            Ok(()) => {
                                self.timeout = new;
                                self.say(format!("Saved: firmware boot timeout is now {new} s"), t);
                            }
                            Err(err) => self.say(format!("Could not save Timeout: {:?}", err.status()), t),
                        }
                    }
                }
            }
            Screen::Confirm(action) => {
                if esc {
                    self.screen = Screen::Main;
                } else if enter {
                    match action {
                        Action::FirmwareSetup => match vars::request_firmware_ui() {
                            Ok(()) => runtime::reset(ResetType::COLD, Status::SUCCESS, None),
                            Err(err) => {
                                self.say(format!("Could not request setup: {:?}", err.status()), t);
                                self.screen = Screen::Main;
                            }
                        },
                        Action::Restart => runtime::reset(ResetType::COLD, Status::SUCCESS, None),
                        Action::PowerOff => runtime::reset(ResetType::SHUTDOWN, Status::SUCCESS, None),
                    }
                }
            }
        }
    }

    fn draw(&self, cv: &mut Canvas, pal: &Palette, t: f32) {
        let (w, h) = (cv.w as i32, cv.h as i32);
        let s = (h / 200).max(2);
        let accent = ITEMS[self.sel].color;
        cv.gradient(pal.bg_top, mix(pal.bg_bottom, accent, 0.12));

        // 3D: drifting background cubes and the big menu cube.
        let cam = Camera::look_at(v3(0.0, 0.4, 7.0), v3(0.0, 0.0, 0.0), 45.0, cv);
        for i in 0..14 {
            let fi = i as f32;
            let seed = fi * 12.9898;
            let x = sin(seed) * 7.0;
            let z = -6.0 - (fi * 1.7) % 8.0;
            let y = ((t * 0.25 + fi * 0.73) % 8.0) - 4.0;
            let r = Mat3::rot_y(t * 0.3 + fi).mul(&Mat3::rot_x(t * 0.2 + fi * 0.5));
            let c = shade(mix(pal.tile_side, accent, 0.3), 0.35);
            draw_box(cv, &cam, v3(x, y, z), v3(0.25, 0.25, 0.25), &r, &[c; 6]);
        }
        let bob = sin(t * 1.6) * 0.08;
        let wobble = Mat3::rot_y(sin(t * 0.9) * 0.18).mul(&Mat3::rot_x(sin(t * 0.7) * 0.12 - 0.25));
        let rot = wobble.mul(&Mat3::rot_x(self.pitch)).mul(&Mat3::rot_y(self.yaw));
        draw_box(cv, &cam, v3(-1.9, bob, 0.0), v3(1.0, 1.0, 1.0), &rot, &cube_faces());

        // Header and footer.
        let pad = s * 6;
        cv.text(pad, pad, s, "GAMECOOB", pal.text, 0.9);
        let cw = text_width(&self.clock_text, s);
        cv.text(w - pad - cw, pad, s, &self.clock_text, pal.text, 0.75);
        let fs = (s - 1).max(1);
        let foot = h - pad - fs * 8;
        let help = match self.screen {
            Screen::Main => "Up/Down: choose   Enter: select",
            Screen::Boot => "Up/Down: choose   Enter: boot   Esc: back",
            Screen::Settings => "Left/Right: change   Esc: back",
            Screen::Confirm(_) => "Enter: yes   Esc: no",
        };
        cv.text(pad, foot, fs, help, pal.text, 0.6);
        let vend = format!("Firmware: {}", self.vendor);
        let vw = text_width(&vend, fs);
        cv.text(w - pad - vw, pad + s * 12, fs, &vend, pal.text, 0.45);
        if let Some((msg, at)) = &self.message {
            let a = clamp01(4.0 - (t - at)) * 0.95;
            if a > 0.0 {
                cv.text_centered(w / 2, foot - fs * 16, fs, msg, rgb(255, 236, 170), a);
            }
        }

        // Option panel on the right.
        let px = w * 52 / 100;
        let pw = w - px - pad;
        let row = s * 8 + s * 7;
        match self.screen {
            Screen::Main | Screen::Confirm(_) => {
                let top = h / 2 - row * ITEMS.len() as i32 / 2;
                for (i, it) in ITEMS.iter().enumerate() {
                    let y = top + i as i32 * row;
                    let selected = i == self.sel;
                    if selected {
                        let pulse = 0.28 + 0.08 * sin(t * 4.0);
                        cv.rect(px, y - s * 3, pw, s * 14, it.color, pulse);
                        cv.rect(px, y - s * 3, s * 2, s * 14, it.color, 1.0);
                    }
                    let col = if selected { rgb(255, 255, 255) } else { mix(pal.text, it.color, 0.4) };
                    cv.text(px + s * 6, y, s, it.label, col, if selected { 1.0 } else { 0.65 });
                }
                let hint = ITEMS[self.sel].hint;
                cv.text(px + s * 6, top + row * ITEMS.len() as i32 + s * 2, fs, hint, pal.text, 0.6);
            }
            Screen::Boot => {
                let top = h / 4;
                cv.text(px, top, s, "Boot", accent, 1.0);
                if self.entries.is_empty() {
                    cv.text(px, top + row * 2, fs, "No boot entries found.", pal.text, 0.8);
                }
                let max_chars = ((pw - fs * 4) / (8 * fs) - 6).max(8) as usize;
                for (i, e) in self.entries.iter().enumerate().take(10) {
                    let y = top + row * 2 + i as i32 * (fs * 14);
                    let selected = i == self.boot_sel;
                    if selected {
                        cv.rect(px, y - fs * 3, pw, fs * 14, accent, 0.3);
                    }
                    let desc: String = e.desc.chars().take(max_chars).collect();
                    let line = format!("{:04X}  {}", e.num, desc);
                    cv.text(px + fs * 4, y, fs, &line, pal.text, if selected { 1.0 } else { 0.7 });
                }
            }
            Screen::Settings => {
                let top = h / 4;
                cv.text(px, top, s, "Settings", accent, 1.0);
                let y = top + row * 2;
                cv.rect(px, y - fs * 3, pw, fs * 14, accent, 0.3);
                let line = format!("Boot timeout   < {:>2} s >", self.timeout);
                cv.text(px + fs * 4, y, fs, &line, rgb(255, 255, 255), 1.0);
                cv.text(px + fs * 4, y + fs * 20, fs, "Boot order:", pal.text, 0.7);
                for (i, e) in self.entries.iter().enumerate().take(8) {
                    let line = format!("{}. {}", i + 1, e.desc);
                    let line: String = line.chars().take((pw / (8 * fs) - 6).max(8) as usize).collect();
                    cv.text(px + fs * 8, y + fs * (32 + i as i32 * 12), fs, &line, pal.text, 0.55);
                }
            }
        }

        if let Screen::Confirm(action) = self.screen {
            let q = match action {
                Action::FirmwareSetup => "Restart into firmware setup?",
                Action::Restart => "Restart now?",
                Action::PowerOff => "Power off now?",
            };
            let bw = text_width(q, s) + s * 16;
            let bh = s * 40;
            let (bx, by) = (w / 2 - bw / 2, h / 2 - bh / 2);
            cv.rect(0, 0, w, h, rgb(0, 0, 0), 0.45);
            cv.rect(bx, by, bw, bh, shade(pal.bg_bottom, 1.3), 0.95);
            cv.frame(bx, by, bw, bh, s, accent, 1.0);
            cv.text_centered(w / 2, by + s * 10, s, q, pal.text, 1.0);
            cv.text_centered(w / 2, by + s * 26, fs, "Enter: yes    Esc: no", accent, 1.0);
        }
    }
}
