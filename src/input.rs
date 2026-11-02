//! GameCoob's input layer: the keyboard and USB gamepads through binding
//! tables. A binding sends a physical input either to an emulated
//! GameCube controller, which BS2's own pad code ([`crate::bs2::pad`])
//! then reads exactly as the IPL does, or straight to the app as an
//! [`AppAction`] (the system dialog, the reset switch, ...), which no
//! controller has.
//!
//! UEFI reports key presses but never releases. A key bound to a button
//! holds it for [`HOLD`] seconds after each press; the keyboard's own
//! repeat keeps it held while the key is down, so holding A spins the
//! cube and holding an arrow gets the IPL's d-pad repeat. Two presses of
//! the same key closer than [`HOLD`] read as one.
//!
//! Gamepads ([`crate::gamepad`]) report whole states, so they need no
//! holds. A GameCube controller on an adapter is read as it is, port for
//! port; any other pad goes through [`DEFAULT_PAD`], by button position.

use crate::bs2::pad::*;
use crate::gamepad::report::{GamepadState, Pad, btn};
use alloc::vec::Vec;

/// How long a key press holds its button, in seconds.
pub const HOLD: f32 = 0.15;

/// A key, independent of the firmware's key codes.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum KeyCode {
    /// A printable key as typed, so case matters ('\r' is Enter, '\u{8}'
    /// Backspace).
    Char(char),
    Up,
    Down,
    Left,
    Right,
    Esc,
    /// F1..F12.
    F(u8),
}

/// What the app does itself, outside the emulated controllers.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum AppAction {
    /// The system dialog (firmware setup, restart, power off).
    System,
    /// Open or close the stand-in disc drive's tray.
    Eject,
    /// Pull, break or re-insert the stand-in memory card in slot 0 or 1.
    Slot(u8),
    /// The console's reset switch.
    Reset,
    /// Skip the intro to the menu.
    SkipIntro,
    /// Show the frame timings.
    Stats,
}

/// Where a binding sends its input.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Target {
    /// Hold `button` (`PAD_BUTTON_*` / `PAD_TRIGGER_*` bits) on each
    /// controller in `pads` (bit 0 is controller 1; in a gamepad binding,
    /// the gamepad's own controller).
    Button { pads: u8, button: u16 },
    Action(AppAction),
}

#[derive(Clone, Copy, Debug)]
pub struct Binding {
    pub key: KeyCode,
    pub target: Target,
}

const fn pad1(key: KeyCode, button: u16) -> Binding {
    Binding { key, target: Target::Button { pads: 1, button } }
}

const fn act(key: KeyCode, a: AppAction) -> Binding {
    Binding { key, target: Target::Action(a) }
}

use KeyCode::*;

/// The built-in keyboard bindings. A key may have several.
pub const DEFAULT_KEYS: &[Binding] = &[
    pad1(Up, PAD_BUTTON_UP),
    pad1(Down, PAD_BUTTON_DOWN),
    pad1(Left, PAD_BUTTON_LEFT),
    pad1(Right, PAD_BUTTON_RIGHT),
    pad1(Char('\r'), PAD_BUTTON_A),
    pad1(Char('a'), PAD_BUTTON_A),
    pad1(Char('A'), PAD_BUTTON_A),
    pad1(Char('\u{8}'), PAD_BUTTON_B),
    pad1(Esc, PAD_BUTTON_B),
    act(Esc, AppAction::SkipIntro),
    pad1(Char(' '), PAD_BUTTON_START),
    pad1(Char('s'), PAD_BUTTON_START),
    pad1(Char('S'), PAD_BUTTON_START),
    pad1(Char('x'), PAD_BUTTON_X),
    pad1(Char('y'), PAD_BUTTON_Y),
    pad1(Char('q'), PAD_TRIGGER_L),
    pad1(Char('e'), PAD_TRIGGER_R),
    // z is Z on controller 1, shift-Z on all four (the boot easter eggs).
    pad1(Char('z'), PAD_TRIGGER_Z),
    Binding { key: Char('Z'), target: Target::Button { pads: 0xF, button: PAD_TRIGGER_Z } },
    act(F(10), AppAction::System),
    act(Char('d'), AppAction::Eject),
    act(Char('D'), AppAction::Eject),
    act(Char('1'), AppAction::Slot(0)),
    act(Char('2'), AppAction::Slot(1)),
    act(Char('r'), AppAction::Reset),
    act(Char('R'), AppAction::Reset),
    act(F(3), AppAction::Stats),
];

/// One binding's hold.
#[derive(Clone, Copy, Default)]
struct Hold {
    /// Held until this time.
    until: f32,
    /// Pressed since the last frame, so it counts even if the frame comes
    /// after `until`.
    fresh: bool,
    /// Held until [`Keyboard::unlatch`].
    latched: bool,
}

/// The keyboard as four controllers plus app actions.
pub struct Keyboard {
    bindings: Vec<Binding>,
    holds: Vec<Hold>,
    actions: Vec<AppAction>,
}

impl Keyboard {
    pub fn new(bindings: &[Binding]) -> Keyboard {
        Keyboard { bindings: bindings.to_vec(), holds: alloc::vec![Hold::default(); bindings.len()], actions: Vec::new() }
    }

    /// A key press at time `now` (seconds).
    pub fn press(&mut self, key: KeyCode, now: f32) {
        for (b, h) in self.bindings.iter().zip(self.holds.iter_mut()) {
            if b.key != key {
                continue;
            }
            match b.target {
                Target::Button { .. } => {
                    h.until = now + HOLD;
                    h.fresh = true;
                }
                Target::Action(a) => self.actions.push(a),
            }
        }
    }

    /// Keep `key`'s buttons held until [`Keyboard::unlatch`] (the boot
    /// easter eggs want Z held until the jingle).
    pub fn latch(&mut self, key: KeyCode) {
        for (b, h) in self.bindings.iter().zip(self.holds.iter_mut()) {
            if b.key == key {
                h.latched = true;
            }
        }
    }

    pub fn unlatch(&mut self) {
        self.holds.iter_mut().for_each(|h| h.latched = false);
    }

    /// The four controllers for one logic frame at time `now`. All four
    /// read as plugged in.
    pub fn pads(&mut self, now: f32) -> [PadStatus; 4] {
        let mut pads = [PadStatus::IDLE; 4];
        for (b, h) in self.bindings.iter().zip(self.holds.iter_mut()) {
            if let Target::Button { pads: mask, button } = b.target {
                if h.fresh || h.latched || now < h.until {
                    for (i, p) in pads.iter_mut().enumerate() {
                        if mask & (1 << i) != 0 {
                            p.button |= button;
                        }
                    }
                }
                h.fresh = false;
            }
        }
        pads
    }

    /// The actions pressed since the last call.
    pub fn take_actions(&mut self) -> Vec<AppAction> {
        core::mem::take(&mut self.actions)
    }
}

/// A gamepad binding: a [`btn`] bit and what it does.
#[derive(Clone, Copy, Debug)]
pub struct PadBinding {
    pub button: u32,
    pub target: Target,
}

const fn gp(button: u32, b: u16) -> PadBinding {
    PadBinding { button, target: Target::Button { pads: 1, button: b } }
}

/// The built-in gamepad bindings, for pads that are not GameCube
/// controllers. By position: the bottom face button is A and the right
/// one B (Xbox A / B, PlayStation cross / circle), the left one X and the
/// top one Y; the triggers are L and R and either shoulder is Z. The Xbox /
/// PS / Home button opens the system dialog.
pub const DEFAULT_PAD: &[PadBinding] = &[
    gp(btn::SOUTH, PAD_BUTTON_A),
    gp(btn::EAST, PAD_BUTTON_B),
    gp(btn::WEST, PAD_BUTTON_X),
    gp(btn::NORTH, PAD_BUTTON_Y),
    gp(btn::L2, PAD_TRIGGER_L),
    gp(btn::R2, PAD_TRIGGER_R),
    gp(btn::L1, PAD_TRIGGER_Z),
    gp(btn::R1, PAD_TRIGGER_Z),
    gp(btn::START, PAD_BUTTON_START),
    gp(btn::UP, PAD_BUTTON_UP),
    gp(btn::DOWN, PAD_BUTTON_DOWN),
    gp(btn::LEFT, PAD_BUTTON_LEFT),
    gp(btn::RIGHT, PAD_BUTTON_RIGHT),
    PadBinding { button: btn::GUIDE, target: Target::Action(AppAction::System) },
];

/// Add controller `s` into `p` (both plugged in, buttons merged, the
/// stick tilted furthest wins).
pub fn merge(p: &mut PadStatus, s: &PadStatus) {
    p.button |= s.button;
    let far = |x: i8, y: i8| (x as i32).pow(2) + (y as i32).pow(2);
    if far(s.stick_x, s.stick_y) > far(p.stick_x, p.stick_y) {
        (p.stick_x, p.stick_y) = (s.stick_x, s.stick_y);
    }
    if far(s.substick_x, s.substick_y) > far(p.substick_x, p.substick_y) {
        (p.substick_x, p.substick_y) = (s.substick_x, s.substick_y);
    }
    p.trigger_left = p.trigger_left.max(s.trigger_left);
    p.trigger_right = p.trigger_right.max(s.trigger_right);
    p.err = PAD_ERR_NONE;
}

/// One gamepad on the four controllers: `port` is the controller it plays
/// as, `prev` its buttons last frame (for actions, which fire on the press).
pub fn add_pad(pads: &mut [PadStatus; 4], actions: &mut Vec<AppAction>, bindings: &[PadBinding], port: usize, pad: &Pad, prev: u32) {
    let port = port & 3;
    match pad {
        Pad::Native(s) => merge(&mut pads[port], s),
        Pad::Mapped(g) => {
            let mut own = PadStatus { stick_x: g.lx, stick_y: g.ly, substick_x: g.rx, substick_y: g.ry, ..PadStatus::IDLE };
            for b in bindings.iter().filter(|b| g.buttons & b.button != 0) {
                match b.target {
                    Target::Button { pads: mask, button } => {
                        for i in 0..4 {
                            if mask & (1 << i) != 0 {
                                let to = (port + i) & 3;
                                if to == port {
                                    own.button |= button;
                                } else {
                                    pads[to].button |= button;
                                    pads[to].err = PAD_ERR_NONE;
                                }
                            }
                        }
                    }
                    Target::Action(a) if prev & b.button == 0 => actions.push(a),
                    Target::Action(_) => {}
                }
            }
            merge(&mut pads[port], &own);
        }
    }
}

/// The buttons of a mapped pad, for [`add_pad`]'s `prev` next frame.
pub fn pad_buttons(pad: &Pad) -> u32 {
    match pad {
        Pad::Mapped(GamepadState { buttons, .. }) => *buttons,
        Pad::Native(_) => 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn mapped_pad() {
        let mut pads = [PadStatus::IDLE; 4];
        let mut acts = Vec::new();
        let g = GamepadState { buttons: btn::SOUTH | btn::R1 | btn::GUIDE, lx: 50, ly: -60, rx: 0, ry: 0 };
        add_pad(&mut pads, &mut acts, DEFAULT_PAD, 1, &Pad::Mapped(g), 0);
        assert_eq!(pads[1].button, PAD_BUTTON_A | PAD_TRIGGER_Z);
        assert_eq!((pads[1].stick_x, pads[1].stick_y), (50, -60));
        assert_eq!(pads[0], PadStatus::IDLE);
        assert_eq!(acts, [AppAction::System]);
        // Still held: no second action.
        acts.clear();
        add_pad(&mut pads, &mut acts, DEFAULT_PAD, 1, &Pad::Mapped(g), g.buttons);
        assert!(acts.is_empty());
    }

    #[test]
    fn native_pad_merges() {
        let mut pads = [PadStatus::IDLE; 4];
        pads[0].button = PAD_BUTTON_B;
        let s = PadStatus { button: PAD_BUTTON_A, stick_x: -80, ..PadStatus::IDLE };
        add_pad(&mut pads, &mut Vec::new(), DEFAULT_PAD, 0, &Pad::Native(s), 0);
        assert_eq!(pads[0].button, PAD_BUTTON_A | PAD_BUTTON_B);
        assert_eq!(pads[0].stick_x, -80);
    }
}
