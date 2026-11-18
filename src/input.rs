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
use alloc::string::{String, ToString};
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

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
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

    /// Whether `key` holds `button` (on any controller).
    pub fn binds(&self, key: KeyCode, button: u16) -> bool {
        self.bindings.iter().any(|b| b.key == key && matches!(b.target, Target::Button { button: x, .. } if x & button != 0))
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
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
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

// ---------------------------------------------------------------------
// The Controls page's bindings
// ---------------------------------------------------------------------

/// One row of the Controls page: a GameCube button or an app action.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Control {
    /// A `PAD_BUTTON_*` / `PAD_TRIGGER_*` bit.
    Button(u16),
    Action(AppAction),
}

impl Control {
    fn of(t: Target) -> Control {
        match t {
            Target::Button { button, .. } => Control::Button(button),
            Target::Action(a) => Control::Action(a),
        }
    }

    /// Whether one key can't be both `self` and `t`. Skip Intro is the
    /// exception: it only works in the intro, so it shares its key with a
    /// button (Escape is B and skips the intro).
    fn clashes(self, t: Target) -> bool {
        let skip = Control::Action(AppAction::SkipIntro);
        let other = Control::of(t);
        let button = |c: Control| matches!(c, Control::Button(_));
        !((self == skip && button(other)) || (other == skip && button(self)))
    }

    fn target(self) -> Target {
        match self {
            Control::Button(b) => Target::Button { pads: 1, button: b },
            Control::Action(a) => Target::Action(a),
        }
    }
}

/// The Controls page's rows, in order, with their names.
pub const CONTROLS: &[(Control, &str)] = &[
    (Control::Button(PAD_BUTTON_A), "A"),
    (Control::Button(PAD_BUTTON_B), "B"),
    (Control::Button(PAD_BUTTON_X), "X"),
    (Control::Button(PAD_BUTTON_Y), "Y"),
    (Control::Button(PAD_TRIGGER_Z), "Z"),
    (Control::Button(PAD_TRIGGER_L), "L"),
    (Control::Button(PAD_TRIGGER_R), "R"),
    (Control::Button(PAD_BUTTON_START), "START"),
    (Control::Button(PAD_BUTTON_UP), "D-Pad Up"),
    (Control::Button(PAD_BUTTON_DOWN), "D-Pad Down"),
    (Control::Button(PAD_BUTTON_LEFT), "D-Pad Left"),
    (Control::Button(PAD_BUTTON_RIGHT), "D-Pad Right"),
    (Control::Action(AppAction::System), "System Menu"),
    (Control::Action(AppAction::Eject), "Disc Tray"),
    (Control::Action(AppAction::Slot(0)), "Slot A Card"),
    (Control::Action(AppAction::Slot(1)), "Slot B Card"),
    (Control::Action(AppAction::Reset), "Reset"),
    (Control::Action(AppAction::SkipIntro), "Skip Intro"),
    (Control::Action(AppAction::Stats), "Frame Times"),
];

/// The controls that always get a key back if a remap took their last
/// one, so the menu can still be driven from the keyboard.
const ESSENTIAL: [u16; 7] = [PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_START, PAD_BUTTON_UP, PAD_BUTTON_DOWN, PAD_BUTTON_LEFT, PAD_BUTTON_RIGHT];

/// A letter as its lower case (both cases are one key on the page).
fn fold(k: KeyCode) -> KeyCode {
    match k {
        Char(c) => Char(c.to_ascii_lowercase()),
        k => k,
    }
}

/// A key's name on the Controls page.
pub fn key_name(k: KeyCode) -> String {
    match k {
        Char('\r') => "Enter".into(),
        Char('\u{8}') => "Backspace".into(),
        Char('\t') => "Tab".into(),
        Char(' ') => "Space".into(),
        Char(c) => c.to_ascii_uppercase().to_string(),
        Up => "Up".into(),
        Down => "Down".into(),
        Left => "Left".into(),
        Right => "Right".into(),
        Esc => "Esc".into(),
        F(n) => alloc::format!("F{n}"),
    }
}

/// A gamepad button's name on the Controls page: by position, with the
/// Xbox and PlayStation names of the shoulders.
pub fn pad_button_name(b: u32) -> &'static str {
    match b {
        btn::SOUTH => "Bottom",
        btn::EAST => "Right",
        btn::WEST => "Left",
        btn::NORTH => "Top",
        btn::L1 => "LB/L1",
        btn::R1 => "RB/R1",
        btn::L2 => "LT/L2",
        btn::R2 => "RT/R2",
        btn::SELECT => "Back",
        btn::START => "Start",
        btn::GUIDE => "Home",
        btn::L3 => "L-Stick",
        btn::R3 => "R-Stick",
        btn::UP => "D-Up",
        btn::DOWN => "D-Down",
        btn::LEFT => "D-Left",
        btn::RIGHT => "D-Right",
        _ => "?",
    }
}

/// The keyboard's and the gamepads' bindings, as the Controls page edits
/// them and `GameCoobControls` keeps them.
#[derive(Clone, PartialEq, Eq, Debug)]
pub struct Bindings {
    pub keys: Vec<Binding>,
    pub pad: Vec<PadBinding>,
}

impl Default for Bindings {
    fn default() -> Bindings {
        Bindings { keys: DEFAULT_KEYS.to_vec(), pad: DEFAULT_PAD.to_vec() }
    }
}

impl Bindings {
    /// The keys bound to `c`, each once (a letter's two cases are one).
    pub fn keys_for(&self, c: Control) -> Vec<KeyCode> {
        let mut v: Vec<KeyCode> = Vec::new();
        for b in self.keys.iter().filter(|b| Control::of(b.target) == c) {
            let k = fold(b.key);
            if !v.contains(&k) {
                v.push(k);
            }
        }
        v
    }

    /// The gamepad buttons bound to `c`, in [`btn`] order.
    pub fn pad_for(&self, c: Control) -> Vec<u32> {
        let mask = self.pad.iter().filter(|b| Control::of(b.target) == c).fold(0u32, |m, b| m | b.button);
        (0..32).map(|i| 1u32 << i).filter(|b| mask & b != 0).collect()
    }

    /// Make `k` the only key of `c`. The key leaves whatever button or
    /// action it had (but Skip Intro keeps it); a letter binds both its cases, and shift-letter on
    /// Z is Z on all four controllers (the boot easter egg).
    pub fn bind_key(&mut self, c: Control, k: KeyCode) {
        let k = fold(k);
        self.keys.retain(|b| Control::of(b.target) != c && !(c.clashes(b.target) && fold(b.key) == k));
        let t = c.target();
        self.keys.push(Binding { key: k, target: t });
        if let Char(l) = k {
            if l.is_ascii_lowercase() {
                let up = match t {
                    Target::Button { button: PAD_TRIGGER_Z, .. } => Target::Button { pads: 0xF, button: PAD_TRIGGER_Z },
                    t => t,
                };
                self.keys.push(Binding { key: Char(l.to_ascii_uppercase()), target: up });
            }
        }
        self.sanitize();
    }

    /// Make gamepad button `b` (a [`btn`] bit) the only one of `c`; it
    /// leaves whatever it had.
    pub fn bind_pad(&mut self, c: Control, b: u32) {
        self.pad.retain(|p| Control::of(p.target) != c && !(c.clashes(p.target) && p.button == b));
        self.pad.push(PadBinding { button: b, target: c.target() });
        self.sanitize();
    }

    /// Give the menu's buttons (A, B, START, the d-pad) back their default
    /// keys and gamepad buttons if a remap took the last one, where those
    /// are free.
    fn sanitize(&mut self) {
        for e in ESSENTIAL {
            let c = Control::Button(e);
            if self.keys_for(c).is_empty() {
                for d in DEFAULT_KEYS.iter().filter(|d| Control::of(d.target) == c) {
                    let taken = self.keys.iter().any(|b| c.clashes(b.target) && fold(b.key) == fold(d.key));
                    if !taken {
                        self.keys.push(*d);
                    }
                }
            }
            if self.pad_for(c).is_empty() {
                for d in DEFAULT_PAD.iter().filter(|d| Control::of(d.target) == c) {
                    let taken = self.pad.iter().any(|b| c.clashes(b.target) && b.button == d.button);
                    if !taken {
                        self.pad.push(*d);
                    }
                }
            }
        }
    }

    /// The bytes saved in `GameCoobControls`: "GCB1", then the key count
    /// (u16) and 9 bytes a key binding (key tag, u32 value, target), then
    /// the gamepad count (u16) and 8 bytes a binding (u32 button, target).
    /// A target is 4 bytes: 0, controller mask, u16 button; or 1, action,
    /// action argument, 0.
    pub fn encode(&self) -> Vec<u8> {
        let mut v = Vec::from(*MAGIC);
        let target = |v: &mut Vec<u8>, t: Target| match t {
            Target::Button { pads, button } => {
                v.extend([0, pads]);
                v.extend(button.to_le_bytes());
            }
            Target::Action(a) => {
                let (code, arg) = match a {
                    AppAction::System => (0, 0),
                    AppAction::Eject => (1, 0),
                    AppAction::Slot(s) => (2, s),
                    AppAction::Reset => (3, 0),
                    AppAction::SkipIntro => (4, 0),
                    AppAction::Stats => (5, 0),
                };
                v.extend([1, code, arg, 0]);
            }
        };
        v.extend((self.keys.len() as u16).to_le_bytes());
        for b in &self.keys {
            let (tag, val) = match b.key {
                Char(c) => (0u8, c as u32),
                Up => (1, 0),
                Down => (2, 0),
                Left => (3, 0),
                Right => (4, 0),
                Esc => (5, 0),
                F(n) => (6, n as u32),
            };
            v.push(tag);
            v.extend(val.to_le_bytes());
            target(&mut v, b.target);
        }
        v.extend((self.pad.len() as u16).to_le_bytes());
        for b in &self.pad {
            v.extend(b.button.to_le_bytes());
            target(&mut v, b.target);
        }
        v
    }

    /// [`Bindings::encode`]'s bytes back, or None if they are not that.
    pub fn decode(b: &[u8]) -> Option<Bindings> {
        let mut r = b.strip_prefix(MAGIC)?;
        fn take<'a>(r: &mut &'a [u8], n: usize) -> Option<&'a [u8]> {
            let (h, t) = r.split_at_checked(n)?;
            *r = t;
            Some(h)
        }
        fn u16le(r: &mut &[u8]) -> Option<u16> {
            take(r, 2).map(|b| u16::from_le_bytes([b[0], b[1]]))
        }
        fn u32le(r: &mut &[u8]) -> Option<u32> {
            take(r, 4).map(|b| u32::from_le_bytes([b[0], b[1], b[2], b[3]]))
        }
        fn target(r: &mut &[u8]) -> Option<Target> {
            let t = take(r, 4)?;
            Some(match t[0] {
                0 => Target::Button { pads: t[1], button: u16::from_le_bytes([t[2], t[3]]) },
                1 => Target::Action(match t[1] {
                    0 => AppAction::System,
                    1 => AppAction::Eject,
                    2 => AppAction::Slot(t[2] & 1),
                    3 => AppAction::Reset,
                    4 => AppAction::SkipIntro,
                    5 => AppAction::Stats,
                    _ => return None,
                }),
                _ => return None,
            })
        }
        let mut out = Bindings { keys: Vec::new(), pad: Vec::new() };
        for _ in 0..u16le(&mut r)? {
            let tag = take(&mut r, 1)?[0];
            let val = u32le(&mut r)?;
            let key = match tag {
                0 => Char(char::from_u32(val)?),
                1 => Up,
                2 => Down,
                3 => Left,
                4 => Right,
                5 => Esc,
                6 => F(val as u8),
                _ => return None,
            };
            out.keys.push(Binding { key, target: target(&mut r)? });
        }
        for _ in 0..u16le(&mut r)? {
            let button = u32le(&mut r)?;
            out.pad.push(PadBinding { button, target: target(&mut r)? });
        }
        r.is_empty().then_some(out)
    }
}

const MAGIC: &[u8; 4] = b"GCB1";

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

    #[test]
    fn remap_key_steals_and_round_trips() {
        let mut b = Bindings::default();
        let x = Control::Button(PAD_BUTTON_X);
        // 'y' was Y: it becomes X's only key (both cases), Y loses it.
        b.bind_key(x, Char('Y'));
        assert_eq!(b.keys_for(x), [Char('y')]);
        assert!(b.keys_for(Control::Button(PAD_BUTTON_Y)).is_empty());
        let k = Keyboard::new(&b.keys);
        assert!(k.binds(Char('Y'), PAD_BUTTON_X) && k.binds(Char('y'), PAD_BUTTON_X));
        // R on Y stops being the reset switch.
        b.bind_key(Control::Button(PAD_BUTTON_Y), Char('r'));
        assert!(b.keys_for(Control::Action(AppAction::Reset)).is_empty());
        // Esc on START leaves B but still skips the intro.
        b.bind_key(Control::Button(PAD_BUTTON_START), Esc);
        assert_eq!(b.keys_for(Control::Button(PAD_BUTTON_B)), [Char('\u{8}')]);
        assert_eq!(b.keys_for(Control::Action(AppAction::SkipIntro)), [Esc]);
        // Shift on a new Z key is still Z on all four controllers.
        b.bind_key(Control::Button(PAD_TRIGGER_Z), Char('k'));
        assert!(b.keys.contains(&Binding { key: Char('K'), target: Target::Button { pads: 0xF, button: PAD_TRIGGER_Z } }));
        b.bind_pad(Control::Action(AppAction::Reset), btn::SELECT);
        b.bind_pad(Control::Button(PAD_BUTTON_A), btn::EAST);
        assert_eq!(b.pad_for(Control::Button(PAD_BUTTON_A)), [btn::EAST]);
        assert_eq!(Bindings::decode(&b.encode()), Some(b.clone()));
        assert_eq!(Bindings::decode(b"GCB1\x01\x00"), None);
        assert_eq!(Bindings::decode(&Bindings::default().encode()), Some(Bindings::default()));
    }

    #[test]
    fn menu_buttons_keep_a_key() {
        let mut b = Bindings::default();
        let a = Control::Button(PAD_BUTTON_A);
        // Take all of A's keys: Enter goes to B, then 'a' to START.
        b.bind_key(Control::Button(PAD_BUTTON_B), Char('\r'));
        b.bind_key(Control::Button(PAD_BUTTON_START), Char('a'));
        // Nothing of A's defaults is free, so it stays empty...
        assert!(b.keys_for(a).is_empty());
        // A new key for A works as usual.
        b.bind_key(a, Char('j'));
        assert_eq!(b.keys_for(a), [Char('j')]);
        // B keeps just Enter (it had keys left, so no defaults came back).
        assert_eq!(b.keys_for(Control::Button(PAD_BUTTON_B)), [Char('\r')]);
        // Taking B's last gamepad button: its default (the right face
        // button) is now X's, so B has none; moving X away frees it again.
        b.bind_pad(Control::Button(PAD_BUTTON_X), btn::EAST);
        assert!(b.pad_for(Control::Button(PAD_BUTTON_B)).is_empty());
        b.bind_pad(Control::Button(PAD_BUTTON_X), btn::WEST);
        assert_eq!(b.pad_for(Control::Button(PAD_BUTTON_B)), [btn::EAST]);
    }
}
