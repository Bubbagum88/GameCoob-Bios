//! BS2 pad input (`pad.c`, 0x81302688..0x81302DF0): the IPL's copy of the
//! SDK's DEMOPad. Each frame `DEMOPadRead` merges the four controllers,
//! maps the sticks onto the d-pad, works out the new presses and the
//! d-pad key repeat (delay and rate from the display settings), and checks
//! the Z-button easter eggs. Everything the scene and the menu read comes
//! from here (`gMenuPad`).
//!
//! GameCoob: there is no SI, so the caller hands in the four controllers'
//! state (`PADRead`), already built from the keyboard or a gamepad by its
//! bindings. `PADClamp` and `PADReset` have nothing to do on that input and
//! are left out, and so is `fn_81302688` (an SRAM flag, read by
//! `Scene_Init`).

use super::bs2main::disp_settings;

/// `PADStatus.button` bits.
pub const PAD_BUTTON_LEFT: u16 = 0x0001;
pub const PAD_BUTTON_RIGHT: u16 = 0x0002;
pub const PAD_BUTTON_DOWN: u16 = 0x0004;
pub const PAD_BUTTON_UP: u16 = 0x0008;
pub const PAD_TRIGGER_Z: u16 = 0x0010;
pub const PAD_TRIGGER_R: u16 = 0x0020;
pub const PAD_TRIGGER_L: u16 = 0x0040;
pub const PAD_BUTTON_A: u16 = 0x0100;
pub const PAD_BUTTON_B: u16 = 0x0200;
pub const PAD_BUTTON_X: u16 = 0x0400;
pub const PAD_BUTTON_Y: u16 = 0x0800;
pub const PAD_BUTTON_START: u16 = 0x1000;

/// `PADStatus.err` values.
pub const PAD_ERR_NONE: i8 = 0;
pub const PAD_ERR_NO_CONTROLLER: i8 = -1;

/// `DEMOPadStatus.dirs` bits.
pub const DIR_UP: u16 = 0x1000;
pub const DIR_DOWN: u16 = 0x2000;
pub const DIR_LEFT: u16 = 0x4000;
pub const DIR_RIGHT: u16 = 0x8000;

/// One controller (`PADStatus`).
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct PadStatus {
    pub button: u16,
    pub stick_x: i8,
    pub stick_y: i8,
    pub substick_x: i8,
    pub substick_y: i8,
    // Nothing in BS2 reads the analog triggers and buttons.
    #[allow(dead_code)]
    pub trigger_left: u8,
    #[allow(dead_code)]
    pub trigger_right: u8,
    #[allow(dead_code)]
    pub analog_a: u8,
    #[allow(dead_code)]
    pub analog_b: u8,
    pub err: i8,
}

impl PadStatus {
    /// A connected controller with nothing pressed.
    pub const IDLE: PadStatus = PadStatus {
        button: 0,
        stick_x: 0,
        stick_y: 0,
        substick_x: 0,
        substick_y: 0,
        trigger_left: 0,
        trigger_right: 0,
        analog_a: 0,
        analog_b: 0,
        err: PAD_ERR_NONE,
    };
    /// An empty port.
    pub const NONE: PadStatus = PadStatus { err: PAD_ERR_NO_CONTROLLER, ..PadStatus::IDLE };
}

/// `DEMOPadStatus`: the merged pad as the scene and the menu read it.
#[derive(Clone, Copy, Default, Debug)]
pub struct MenuPad {
    pub pst: PadStatus,
    pub button_down: u16,
    pub button_up: u16,
    pub dirs: u16,
    pub dirs_new: u16,
    pub dirs_released: u16,
    pub stick_delta_x: i16,
    pub stick_delta_y: i16,
    pub substick_delta_x: i16,
    pub substick_delta_y: i16,
    pub angle: i16,
    pub magnitude: f32,
}

/// `PADButtonDown`
fn button_down(last: u16, now: u16) -> u16 {
    (last ^ now) & now
}

/// `PADButtonUp`
fn button_up(last: u16, now: u16) -> u16 {
    (last ^ now) & last
}

/// pad.c's state.
pub struct Pads {
    /// `gMenuPad`.
    pub menu: MenuPad,
    /// `lbl_814AF5D0`: the last good status per port.
    last: [PadStatus; 4],
    /// `lbl_814AD314`: connected controllers.
    pub connected: i32,
    /// `lbl_814AD30C`: frames the same directions have been held.
    repeat_count: i32,
    /// `lbl_814AD308`: last frame's directions.
    prev_dirs: u16,
    /// `lbl_814AD300`: key repeat fired this frame.
    repeat: bool,
    /// `lbl_814AD304`: 0 none, 1 Z on controller 1, 2 Z on all four.
    egg: i32,
}

impl Pads {
    /// `DEMOPadInit` (0x81302D2C).
    pub fn new() -> Pads {
        Pads { menu: MenuPad::default(), last: [PadStatus::default(); 4], connected: 0, repeat_count: 0, prev_dirs: 0, repeat: false, egg: 0 }
    }

    /// `fn_81302700` (0x81302700): the new presses, the d-pad repeat and the stick.
    fn update_menu_pad(&mut self, pad: &PadStatus) {
        let disp = disp_settings();
        let dp = &mut self.menu;
        let mut dirs = 0;
        if pad.button & PAD_BUTTON_LEFT != 0 {
            dirs |= DIR_LEFT;
        }
        if pad.button & PAD_BUTTON_RIGHT != 0 {
            dirs |= DIR_RIGHT;
        }
        if pad.button & PAD_BUTTON_DOWN != 0 {
            dirs |= DIR_DOWN;
        }
        if pad.button & PAD_BUTTON_UP != 0 {
            dirs |= DIR_UP;
        }

        // Held directions count up to the delay, then fire every `rate`
        // frames by forgetting last frame's directions.
        self.repeat = false;
        if self.prev_dirs == dirs && dirs != 0 {
            self.repeat_count += 1;
            let (delay, rate) = (disp.repeat_delay as i32, disp.repeat_rate as i32);
            if self.repeat_count >= delay {
                if self.repeat_count == delay {
                    dp.dirs = 0;
                    self.repeat = true;
                } else if self.repeat_count > delay + rate {
                    dp.dirs = 0;
                    self.repeat = true;
                    self.repeat_count -= rate;
                }
            }
        } else {
            self.repeat_count = 0;
        }
        self.prev_dirs = dirs;

        dp.dirs_new = button_down(dp.dirs, dirs);
        dp.dirs_released = button_up(dp.dirs, dirs);
        dp.dirs = dirs;

        dp.button_down = button_down(dp.pst.button, pad.button);
        dp.button_up = button_up(dp.pst.button, pad.button);

        dp.stick_delta_x = pad.stick_x as i16 - dp.pst.stick_x as i16;
        dp.stick_delta_y = pad.stick_y as i16 - dp.pst.stick_y as i16;
        dp.substick_delta_x = pad.substick_x as i16 - dp.pst.substick_x as i16;
        dp.substick_delta_y = pad.substick_y as i16 - dp.pst.substick_y as i16;

        // The raw stick, not scaled, so any tilt reads as 1.
        let (sx, sy) = (pad.stick_x as i32, pad.stick_y as i32);
        dp.magnitude = libm::sqrtf((sx * sx + sy * sy) as f32);
        if dp.magnitude > 1.0 {
            dp.magnitude = 1.0;
        }
        if dp.magnitude > 0.0 {
            let y = -sy;
            dp.angle = (32768.0 * libm::atan2(sx as f64, y as f64) / 3.1415926) as i16;
        }

        // B wins over A and START pressed in the same frame.
        if dp.button_down & PAD_BUTTON_B != 0 {
            dp.button_down &= !(PAD_BUTTON_A | PAD_BUTTON_START);
        }

        dp.pst = *pad;
    }

    /// `fn_81302A40` (0x81302A40): either stick past 35 presses the d-pad.
    fn stick_to_dpad(pad: &mut PadStatus) {
        for (x, y) in [(pad.stick_x, pad.stick_y), (pad.substick_x, pad.substick_y)] {
            if x < -35 {
                pad.button |= PAD_BUTTON_LEFT;
            }
            if x > 35 {
                pad.button |= PAD_BUTTON_RIGHT;
            }
            if y < -35 {
                pad.button |= PAD_BUTTON_DOWN;
            }
            if y > 35 {
                pad.button |= PAD_BUTTON_UP;
            }
        }
    }

    /// `DEMOPadRead` (0x81302B24), on the four controllers `pads`
    /// (`PADRead`'s result). Buttons of every connected controller are
    /// merged; the merged sticks are always centred.
    pub fn read(&mut self, pads: &[PadStatus; 4]) {
        let mut pads = *pads;
        let mut merged = PadStatus::default();
        self.connected = 0;
        for (pad, last) in pads.iter_mut().zip(self.last.iter_mut()) {
            if pad.err == PAD_ERR_NONE {
                self.connected += 1;
                Pads::stick_to_dpad(pad);
                merged.button |= pad.button;
                *last = *pad;
                last.err = 1;
            } else if pad.err == PAD_ERR_NO_CONTROLLER {
                last.err = 0;
            } else if last.err != 0 {
                // A transient error: keep the last good buttons.
                self.connected += 1;
                merged.button |= last.button;
            }
        }
        self.update_menu_pad(&merged);

        let z = |p: &PadStatus| p.err == PAD_ERR_NONE && p.button & PAD_TRIGGER_Z != 0;
        self.egg = if pads.iter().all(z) {
            2
        } else if z(&pads[0]) {
            1
        } else {
            0
        };
    }

    /// `Pad_IsRepeat` (0x81302DE0): this frame's directions are a repeat.
    pub fn is_repeat(&self) -> bool {
        self.repeat
    }

    /// `Pad_GetEasterEgg` (0x81302DE8).
    pub fn easter_egg(&self) -> i32 {
        self.egg
    }
}
