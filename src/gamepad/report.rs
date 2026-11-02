//! Gamepad reports, without the firmware: which USB controllers GameCoob
//! knows, how to start them, and how to read their input reports. The
//! firmware side ([`super`]) finds the devices and hands each report here.
//!
//! Three kinds of controller:
//! - The Wii U GameCube controller adapter (057e:0337), and the third-party
//!   adapters in their "Wii U" mode, which report as the same device: four
//!   real GameCube controllers, read as they are, port for port.
//! - Xbox 360 and Xbox One wired controllers (and the many pads that speak
//!   their protocols), which are not HID devices.
//! - Every other HID joystick or gamepad, read through its HID report
//!   descriptor. A HID device with several joysticks in it (an adapter in
//!   "PC" mode, one report ID per port) gives one pad per joystick.
//!
//! Everything but the GameCube adapter comes out as a [`GamepadState`]: the
//! buttons by position (south, east, ...), mapped onto GameCube buttons by
//! [`crate::input`]'s bindings.

use crate::bs2::pad::*;
use alloc::vec::Vec;

/// [`GamepadState::buttons`] bits, by position on an Xbox-style pad (south
/// is A on an Xbox pad, cross on a PlayStation pad).
pub mod btn {
    pub const SOUTH: u32 = 1 << 0;
    pub const EAST: u32 = 1 << 1;
    pub const WEST: u32 = 1 << 2;
    pub const NORTH: u32 = 1 << 3;
    /// The shoulder buttons.
    pub const L1: u32 = 1 << 4;
    pub const R1: u32 = 1 << 5;
    /// The triggers (an analog trigger past half way).
    pub const L2: u32 = 1 << 6;
    pub const R2: u32 = 1 << 7;
    /// Back / View / Share.
    pub const SELECT: u32 = 1 << 8;
    /// Start / Menu / Options.
    pub const START: u32 = 1 << 9;
    /// The Xbox / PS / Home button.
    pub const GUIDE: u32 = 1 << 10;
    /// The stick clicks.
    pub const L3: u32 = 1 << 11;
    pub const R3: u32 = 1 << 12;
    pub const UP: u32 = 1 << 13;
    pub const DOWN: u32 = 1 << 14;
    pub const LEFT: u32 = 1 << 15;
    pub const RIGHT: u32 = 1 << 16;
}

/// One non-GameCube pad. The sticks are on the GameCube's scale (about
/// +-100 at full tilt, up and right positive) with a small dead zone.
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct GamepadState {
    pub buttons: u32,
    pub lx: i8,
    pub ly: i8,
    pub rx: i8,
    pub ry: i8,
}

/// One pad of a device, as last reported.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Pad {
    /// A real GameCube controller on an adapter.
    Native(PadStatus),
    /// Any other pad, mapped by the bindings.
    Mapped(GamepadState),
}

/// What a USB interface looks like, for [`Decoder::probe`].
#[derive(Clone, Copy, Debug)]
pub struct InterfaceInfo {
    pub vendor: u16,
    pub product: u16,
    pub class: u8,
    pub subclass: u8,
    pub protocol: u8,
}

/// What to send the device before it reports.
pub enum Start {
    /// Nothing.
    None,
    /// These bytes to the interrupt OUT endpoint.
    Out(&'static [u8]),
    /// Needs its HID report descriptor (then [`Decoder::hid`]).
    ReportDescriptor,
}

/// The Wii U adapter's "start polling" command.
const WIIU_START: &[u8] = &[0x13];
/// The Xbox One controller's power-on packet.
const XBOX_ONE_START: &[u8] = &[0x05, 0x20, 0x00, 0x01, 0x00];

/// A device's report reader.
pub enum Decoder {
    WiiU(WiiU),
    Xbox360(GamepadState),
    XboxOne(GamepadState),
    Hid(Hid),
}

impl Decoder {
    /// The reader for a USB interface, or `None` if it is not a controller
    /// GameCoob knows. A HID interface needs its report descriptor next.
    pub fn probe(i: &InterfaceInfo) -> Option<(Option<Decoder>, Start)> {
        match (i.vendor, i.product, i.class, i.subclass, i.protocol) {
            (0x057E, 0x0337, ..) => Some((Some(Decoder::WiiU(WiiU::default())), Start::Out(WIIU_START))),
            (_, _, 0xFF, 0x5D, 0x01) => Some((Some(Decoder::Xbox360(GamepadState::default())), Start::None)),
            (_, _, 0xFF, 0x47, 0xD0) => Some((Some(Decoder::XboxOne(GamepadState::default())), Start::Out(XBOX_ONE_START))),
            // HID, but not a boot keyboard or mouse: the firmware drives those.
            (_, _, 0x03, 0x01, 1 | 2) => None,
            (_, _, 0x03, ..) => Some((None, Start::ReportDescriptor)),
            _ => None,
        }
    }

    /// The reader for a HID interface with report descriptor `desc`, or
    /// `None` if it holds no joystick or gamepad.
    pub fn hid(desc: &[u8]) -> Option<Decoder> {
        let hid = Hid::parse(desc);
        (!hid.pads.is_empty()).then_some(Decoder::Hid(hid))
    }

    /// Whether the pads keep their port: an adapter's port 2 is controller 2.
    pub fn fixed_ports(&self) -> bool {
        matches!(self, Decoder::WiiU(_))
    }

    /// Read one input report.
    pub fn feed(&mut self, r: &[u8]) {
        match self {
            Decoder::WiiU(w) => w.feed(r),
            Decoder::Xbox360(s) => xbox360(s, r),
            Decoder::XboxOne(s) => xbox_one(s, r),
            Decoder::Hid(h) => h.feed(r),
        }
    }

    /// The pads as last reported, `None` for an empty adapter port.
    pub fn pads(&self) -> Vec<Option<Pad>> {
        match self {
            Decoder::WiiU(w) => w.ports.iter().map(|p| p.map(|p| Pad::Native(p.status))).collect(),
            Decoder::Xbox360(s) | Decoder::XboxOne(s) => alloc::vec![Some(Pad::Mapped(*s))],
            Decoder::Hid(h) => h.pads.iter().map(|p| Some(Pad::Mapped(p.state))).collect(),
        }
    }
}

/// Move `pads` on to the next report's `now`, keeping the buttons `pads`
/// held: a press shorter than a frame still counts.
pub fn hold_presses(pads: &mut Vec<Option<Pad>>, now: Vec<Option<Pad>>) {
    let old = core::mem::replace(pads, now);
    for (p, o) in pads.iter_mut().zip(old) {
        match (p, o) {
            (Some(Pad::Native(p)), Some(Pad::Native(o))) => p.button |= o.button,
            (Some(Pad::Mapped(p)), Some(Pad::Mapped(o))) => p.buttons |= o.buttons,
            _ => {}
        }
    }
}

/// A stick axis from -128..127 to the GameCube's scale.
fn stick(v: i32) -> i8 {
    let v = v.clamp(-128, 127);
    if v.abs() < 16 { 0 } else { (v * 100 / 128) as i8 }
}

/// A signed 16-bit axis to the GameCube's scale.
fn stick16(v: i16) -> i8 {
    stick(v as i32 >> 8)
}

fn bits(byte: u8, map: &[(u8, u32)]) -> u32 {
    map.iter().filter(|(b, _)| byte & (1 << b) != 0).fold(0, |a, (_, m)| a | m)
}

// ---------------------------------------------------------------------------
// Wii U GameCube adapter

/// One adapter port.
#[derive(Clone, Copy, Debug)]
pub struct WiiUPort {
    pub status: PadStatus,
    /// The stick positions when the controller was plugged in, which read
    /// as centred (a GameCube controller calibrates the same way).
    origin: [u8; 4],
}

#[derive(Default)]
pub struct WiiU {
    pub ports: [Option<WiiUPort>; 4],
}

impl WiiU {
    /// A 37-byte report: 0x21, then 9 bytes per port (type, buttons x2,
    /// stick x/y, C-stick x/y, L, R).
    fn feed(&mut self, r: &[u8]) {
        if r.len() < 37 || r[0] != 0x21 {
            return;
        }
        for (i, port) in self.ports.iter_mut().enumerate() {
            let p = &r[1 + 9 * i..10 + 9 * i];
            // Type: 1 wired, 2 wireless (WaveBird), 0 nothing plugged in.
            if (p[0] >> 4) & 3 == 0 {
                *port = None;
                continue;
            }
            let raw = [p[3], p[4], p[5], p[6]];
            let origin = port.map_or(raw, |o| o.origin);
            let axis = |n: usize| (raw[n] as i32 - origin[n] as i32).clamp(-128, 127) as i8;
            let button = bits(
                p[1],
                &[
                    (0, PAD_BUTTON_A as u32),
                    (1, PAD_BUTTON_B as u32),
                    (2, PAD_BUTTON_X as u32),
                    (3, PAD_BUTTON_Y as u32),
                    (4, PAD_BUTTON_LEFT as u32),
                    (5, PAD_BUTTON_RIGHT as u32),
                    (6, PAD_BUTTON_DOWN as u32),
                    (7, PAD_BUTTON_UP as u32),
                ],
            ) | bits(p[2], &[(0, PAD_BUTTON_START as u32), (1, PAD_TRIGGER_Z as u32), (2, PAD_TRIGGER_R as u32), (3, PAD_TRIGGER_L as u32)]);
            let status = PadStatus {
                button: button as u16,
                stick_x: axis(0),
                stick_y: axis(1),
                substick_x: axis(2),
                substick_y: axis(3),
                trigger_left: p[7],
                trigger_right: p[8],
                ..PadStatus::IDLE
            };
            *port = Some(WiiUPort { status, origin });
        }
    }
}

// ---------------------------------------------------------------------------
// Xbox

/// An Xbox 360 report: 0x00 0x14, buttons x2, triggers, then four 16-bit sticks.
fn xbox360(s: &mut GamepadState, r: &[u8]) {
    if r.len() < 14 || r[0] != 0x00 || r[1] < 0x14 {
        return;
    }
    let mut b = bits(
        r[2],
        &[(0, btn::UP), (1, btn::DOWN), (2, btn::LEFT), (3, btn::RIGHT), (4, btn::START), (5, btn::SELECT), (6, btn::L3), (7, btn::R3)],
    ) | bits(r[3], &[(0, btn::L1), (1, btn::R1), (2, btn::GUIDE), (4, btn::SOUTH), (5, btn::EAST), (6, btn::WEST), (7, btn::NORTH)]);
    if r[4] > 0x80 {
        b |= btn::L2;
    }
    if r[5] > 0x80 {
        b |= btn::R2;
    }
    let i16at = |n: usize| i16::from_le_bytes([r[n], r[n + 1]]);
    *s = GamepadState { buttons: b, lx: stick16(i16at(6)), ly: stick16(i16at(8)), rx: stick16(i16at(10)), ry: stick16(i16at(12)) };
}

/// An Xbox One report: 0x20 (input; buttons x2, two 10-bit triggers, four
/// 16-bit sticks) or 0x07 (the Xbox button).
fn xbox_one(s: &mut GamepadState, r: &[u8]) {
    match r.first() {
        Some(0x20) if r.len() >= 18 => {
            let guide = s.buttons & btn::GUIDE;
            let mut b = bits(r[4], &[(2, btn::START), (3, btn::SELECT), (4, btn::SOUTH), (5, btn::EAST), (6, btn::WEST), (7, btn::NORTH)])
                | bits(
                    r[5],
                    &[(0, btn::UP), (1, btn::DOWN), (2, btn::LEFT), (3, btn::RIGHT), (4, btn::L1), (5, btn::R1), (6, btn::L3), (7, btn::R3)],
                );
            let u16at = |n: usize| u16::from_le_bytes([r[n], r[n + 1]]);
            let i16at = |n: usize| i16::from_le_bytes([r[n], r[n + 1]]);
            if u16at(6) > 512 {
                b |= btn::L2;
            }
            if u16at(8) > 512 {
                b |= btn::R2;
            }
            *s = GamepadState { buttons: b | guide, lx: stick16(i16at(10)), ly: stick16(i16at(12)), rx: stick16(i16at(14)), ry: stick16(i16at(16)) };
        }
        Some(0x07) if r.len() >= 5 => {
            s.buttons = (s.buttons & !btn::GUIDE) | if r[4] & 1 != 0 { btn::GUIDE } else { 0 };
        }
        _ => {}
    }
}

// ---------------------------------------------------------------------------
// HID

/// One input field of a HID report.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Field {
    pub report_id: u8,
    /// Bit offset after the report ID byte.
    pub bit: u32,
    pub size: u8,
    pub min: i32,
    pub max: i32,
    pub page: u16,
    pub usage: u16,
}

impl Field {
    fn read(&self, r: &[u8]) -> Option<i32> {
        if self.size == 0 || self.size > 32 {
            return None;
        }
        let mut v: u32 = 0;
        for i in 0..self.size as u32 {
            let b = self.bit + i;
            let byte = *r.get(b as usize / 8)?;
            v |= (((byte >> (b % 8)) & 1) as u32) << i;
        }
        Some(if self.min < 0 && self.size < 32 && v & (1 << (self.size - 1)) != 0 {
            (v | !((1u32 << self.size) - 1)) as i32
        } else {
            v as i32
        })
    }

    /// The value from -128 (logical minimum) to 127 (maximum).
    fn axis(&self, v: i32) -> i32 {
        let span = (self.max as i64 - self.min as i64).max(1);
        ((v as i64 - self.min as i64) * 255 / span - 128) as i32
    }
}

/// HID usages GameCoob reads.
mod usage {
    pub const DESKTOP: u16 = 0x01;
    pub const SIMULATION: u16 = 0x02;
    pub const BUTTON: u16 = 0x09;
    pub const JOYSTICK: u16 = 0x04;
    pub const GAMEPAD: u16 = 0x05;
    pub const MULTI_AXIS: u16 = 0x08;
    pub const X: u16 = 0x30;
    pub const Y: u16 = 0x31;
    pub const Z: u16 = 0x32;
    pub const RX: u16 = 0x33;
    pub const RY: u16 = 0x34;
    pub const RZ: u16 = 0x35;
    pub const HAT: u16 = 0x39;
    pub const DPAD_UP: u16 = 0x90;
    pub const DPAD_DOWN: u16 = 0x91;
    pub const DPAD_RIGHT: u16 = 0x92;
    pub const DPAD_LEFT: u16 = 0x93;
    pub const ACCELERATOR: u16 = 0xC4;
    pub const BRAKE: u16 = 0xC5;
}

/// HID buttons 1.. by position: the order most DirectInput pads use
/// (PlayStation pads, Logitech, most generic USB pads). Pads that differ
/// get remapped on the Controls page.
const HID_BUTTONS: [u32; 13] = [
    btn::WEST,
    btn::SOUTH,
    btn::EAST,
    btn::NORTH,
    btn::L1,
    btn::R1,
    btn::L2,
    btn::R2,
    btn::SELECT,
    btn::START,
    btn::L3,
    btn::R3,
    btn::GUIDE,
];

/// One joystick or gamepad collection of a HID device.
#[derive(Clone, Debug, Default)]
pub struct HidPad {
    pub fields: Vec<Field>,
    pub state: GamepadState,
}

impl HidPad {
    fn find(&self, page: u16, u: u16) -> Option<&Field> {
        self.fields.iter().find(|f| f.page == page && f.usage == u)
    }

    fn update(&mut self, id: u8, r: &[u8]) {
        if !self.fields.iter().any(|f| f.report_id == id) {
            return;
        }
        let axis = |f: Option<&Field>| f.filter(|f| f.report_id == id).and_then(|f| Some(f.axis(f.read(r)?)));
        let mut s = GamepadState::default();
        for f in self.fields.iter().filter(|f| f.report_id == id) {
            let Some(v) = f.read(r) else { continue };
            let on = v != 0;
            match (f.page, f.usage) {
                (usage::BUTTON, n @ 1..=13) if on => s.buttons |= HID_BUTTONS[n as usize - 1],
                (usage::DESKTOP, usage::DPAD_UP) if on => s.buttons |= btn::UP,
                (usage::DESKTOP, usage::DPAD_DOWN) if on => s.buttons |= btn::DOWN,
                (usage::DESKTOP, usage::DPAD_RIGHT) if on => s.buttons |= btn::RIGHT,
                (usage::DESKTOP, usage::DPAD_LEFT) if on => s.buttons |= btn::LEFT,
                (usage::DESKTOP, usage::HAT) => s.buttons |= hat(f, v),
                _ => {}
            }
        }
        let (d, sim) = (usage::DESKTOP, usage::SIMULATION);
        let lx = axis(self.find(d, usage::X)).unwrap_or(0);
        let ly = axis(self.find(d, usage::Y)).unwrap_or(0);
        // The right stick is Z / Rz (PlayStation style, the triggers then
        // on Rx / Ry) or Rx / Ry.
        let zrz = self.find(d, usage::Z).is_some() && self.find(d, usage::RZ).is_some();
        let (rx, ry, lt, rt) = if zrz {
            (axis(self.find(d, usage::Z)), axis(self.find(d, usage::RZ)), axis(self.find(d, usage::RX)), axis(self.find(d, usage::RY)))
        } else {
            (axis(self.find(d, usage::RX)), axis(self.find(d, usage::RY)), None, None)
        };
        let lt = lt.or(axis(self.find(sim, usage::BRAKE)));
        let rt = rt.or(axis(self.find(sim, usage::ACCELERATOR)));
        if lt.is_some_and(|v| v > 0) {
            s.buttons |= btn::L2;
        }
        if rt.is_some_and(|v| v > 0) {
            s.buttons |= btn::R2;
        }
        s.lx = stick(lx);
        s.ly = stick(-ly - 1);
        s.rx = stick(rx.unwrap_or(0));
        s.ry = stick(-ry.unwrap_or(-1) - 1);
        self.state = s;
    }
}

/// A hat switch's d-pad buttons: 0 is up, then clockwise in eighths (or
/// quarters for a 4-way hat); anything else is centred.
fn hat(f: &Field, v: i32) -> u32 {
    let n = v - f.min;
    let eighths = if f.max - f.min == 3 { n * 2 } else { n };
    match eighths {
        0 => btn::UP,
        1 => btn::UP | btn::RIGHT,
        2 => btn::RIGHT,
        3 => btn::DOWN | btn::RIGHT,
        4 => btn::DOWN,
        5 => btn::DOWN | btn::LEFT,
        6 => btn::LEFT,
        7 => btn::UP | btn::LEFT,
        _ => 0,
    }
}

/// A HID device's joysticks and gamepads.
#[derive(Clone, Debug, Default)]
pub struct Hid {
    /// Reports start with a report ID byte.
    pub ids: bool,
    pub pads: Vec<HidPad>,
}

#[derive(Clone, Copy, Default)]
struct Globals {
    page: u16,
    min: i32,
    max: i32,
    size: u32,
    count: u32,
    id: u8,
}

impl Hid {
    /// Parse a HID report descriptor, keeping the input fields of each
    /// top-level joystick, gamepad or multi-axis collection.
    pub fn parse(desc: &[u8]) -> Hid {
        let mut hid = Hid::default();
        let mut g = Globals::default();
        let mut stack: Vec<Globals> = Vec::new();
        // Local items: usages (page << 16 | usage), usage minimum / maximum.
        let mut usages: Vec<u32> = Vec::new();
        let (mut umin, mut umax): (Option<u32>, Option<u32>) = (None, None);
        // Bits used so far in each report ID's input report.
        let mut offsets: Vec<(u8, u32)> = Vec::new();
        let mut depth = 0u32;
        // The pad the current top-level collection is, if it is one.
        let mut pad: Option<usize> = None;
        let mut i = 0;
        while i < desc.len() {
            let b = desc[i];
            if b == 0xFE {
                // A long item.
                let n = *desc.get(i + 1).unwrap_or(&0) as usize;
                i += 3 + n;
                continue;
            }
            let n = [0, 1, 2, 4][(b & 3) as usize];
            let Some(data) = desc.get(i + 1..i + 1 + n) else { break };
            i += 1 + n;
            let mut u = 0u32;
            for (k, d) in data.iter().enumerate() {
                u |= (*d as u32) << (8 * k);
            }
            let s = match n {
                1 => u as u8 as i8 as i32,
                2 => u as u16 as i16 as i32,
                _ => u as i32,
            };
            let full = |u: u32| if n == 4 { u } else { (g.page as u32) << 16 | u };
            match b & 0xFC {
                // Main items.
                0x80 => {
                    // Input.
                    let at = match offsets.iter_mut().find(|(id, _)| *id == g.id) {
                        Some((_, at)) => at,
                        None => {
                            offsets.push((g.id, 0));
                            &mut offsets.last_mut().unwrap().1
                        }
                    };
                    let constant = u & 1 != 0;
                    let variable = u & 2 != 0;
                    if let (Some(p), false, true) = (pad, constant, variable) {
                        for k in 0..g.count {
                            let us = if let Some(last) = usages.last() {
                                *usages.get(k as usize).unwrap_or(last)
                            } else if let (Some(lo), Some(hi)) = (umin, umax) {
                                (lo + k).min(hi)
                            } else {
                                0
                            };
                            hid.pads[p].fields.push(Field {
                                report_id: g.id,
                                bit: *at + k * g.size,
                                size: g.size.min(32) as u8,
                                min: g.min,
                                max: g.max,
                                page: (us >> 16) as u16,
                                usage: us as u16,
                            });
                        }
                    }
                    *at += g.size * g.count;
                }
                0xA0 => {
                    // Collection: a top-level application collection is a
                    // pad if its usage is a joystick or gamepad.
                    if depth == 0 {
                        pad = None;
                        let us = usages.first().copied().unwrap_or(0);
                        let (page, us) = ((us >> 16) as u16, us as u16);
                        if page == usage::DESKTOP && matches!(us, usage::JOYSTICK | usage::GAMEPAD | usage::MULTI_AXIS) {
                            hid.pads.push(HidPad::default());
                            pad = Some(hid.pads.len() - 1);
                        }
                    }
                    depth += 1;
                }
                0xC0 => depth = depth.saturating_sub(1),
                0x90 | 0xB0 => {} // Output, Feature.
                // Global items.
                0x04 => g.page = u as u16,
                0x14 => g.min = s,
                0x24 => g.max = s,
                0x74 => g.size = u,
                0x84 => {
                    g.id = u as u8;
                    hid.ids = true;
                }
                0x94 => g.count = u,
                0xA4 => stack.push(g),
                0xB4 => g = stack.pop().unwrap_or(g),
                // Local items.
                0x08 => usages.push(full(u)),
                0x18 => umin = Some(full(u)),
                0x28 => umax = Some(full(u)),
                _ => {}
            }
            if b & 0x0C == 0 {
                // Locals last until the next main item.
                usages.clear();
                umin = None;
                umax = None;
            }
        }
        // A non-negative minimum means an unsigned maximum.
        for f in hid.pads.iter_mut().flat_map(|p| p.fields.iter_mut()) {
            if f.min >= 0 && f.max < 0 && f.size < 32 {
                f.max = (f.max as u32 & ((1u32 << f.size) - 1)) as i32;
            }
        }
        hid
    }

    fn feed(&mut self, r: &[u8]) {
        let (id, body) = if self.ids {
            match r.split_first() {
                Some((id, body)) => (*id, body),
                None => return,
            }
        } else {
            (0, r)
        };
        for p in &mut self.pads {
            p.update(id, body);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A DualShock 4's report descriptor, cut to its input report 1.
    const DS4: &[u8] = &[
        0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01, // gamepad, report 1
        0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35, // X Y Z Rz
        0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02, // 4 x 8 bits
        0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46, 0x3B, 0x01, 0x65, 0x14, // hat 0..7
        0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00, // 1 x 4 bits
        0x05, 0x09, 0x19, 0x01, 0x29, 0x0E, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0E, 0x81, 0x02, // 14 buttons
        0x06, 0x00, 0xFF, 0x09, 0x20, 0x75, 0x06, 0x95, 0x01, 0x15, 0x00, 0x25, 0x7F, 0x81, 0x02, // counter
        0x05, 0x01, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, // Rx Ry
        0xC0,
    ];

    fn ds4_report(lx: u8, ly: u8, hat: u8, buttons: u16, l2: u8) -> Vec<u8> {
        let b = (hat as u32) | (buttons as u32) << 4;
        alloc::vec![0x01, lx, ly, 0x80, 0x80, b as u8, (b >> 8) as u8, (b >> 16) as u8, l2, 0]
    }

    #[test]
    fn ds4_layout() {
        let Some(Decoder::Hid(h)) = Decoder::hid(DS4) else { panic!() };
        assert!(h.ids);
        assert_eq!(h.pads.len(), 1);
        let us: Vec<_> = h.pads[0].fields.iter().map(|f| (f.page, f.usage, f.bit, f.size)).collect();
        assert_eq!(&us[..5], &[(1, 0x30, 0, 8), (1, 0x31, 8, 8), (1, 0x32, 16, 8), (1, 0x35, 24, 8), (1, 0x39, 32, 4)]);
        assert_eq!(us[5], (9, 1, 36, 1));
        assert_eq!(*us.last().unwrap(), (1, 0x34, 64, 8));
    }

    #[test]
    fn ds4_input() {
        let mut d = Decoder::hid(DS4).unwrap();
        // Centred, hat released (8), nothing pressed.
        d.feed(&ds4_report(0x80, 0x80, 8, 0, 0));
        assert_eq!(d.pads(), alloc::vec![Some(Pad::Mapped(GamepadState::default()))]);
        // Stick full left and up, hat right, cross (button 2) and options
        // (button 10), L2 half down.
        d.feed(&ds4_report(0x00, 0x00, 2, 1 << 1 | 1 << 9, 0xC0));
        let Some(Pad::Mapped(s)) = d.pads()[0] else { panic!() };
        assert_eq!(s.buttons, btn::RIGHT | btn::SOUTH | btn::START | btn::L2);
        assert_eq!((s.lx, s.ly), (-100, 99));
        // Hat up-left, square (button 1).
        d.feed(&ds4_report(0xFF, 0xFF, 7, 1, 0));
        let Some(Pad::Mapped(s)) = d.pads()[0] else { panic!() };
        assert_eq!(s.buttons, btn::UP | btn::LEFT | btn::WEST);
        assert_eq!((s.lx, s.ly), (99, -100));
        // Another report ID changes nothing.
        d.feed(&[0x05, 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0, 0]);
        let Some(Pad::Mapped(s2)) = d.pads()[0] else { panic!() };
        assert_eq!(s, s2);
    }

    /// Two joysticks in one device, one report ID each, as an adapter in
    /// PC mode reports its ports.
    #[test]
    fn two_joysticks() {
        let one = |id: u8| {
            alloc::vec![
                0x05, 0x01, 0x09, 0x04, 0xA1, 0x01, 0x85, id, // joystick
                0x05, 0x09, 0x19, 0x01, 0x29, 0x08, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
                0x05, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x09, 0x30, 0x09, 0x31, // pointer: X Y
                0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0xC0, // signed
                0xC0,
            ]
        };
        let desc: Vec<u8> = [one(1), one(2)].concat();
        let mut d = Decoder::hid(&desc).unwrap();
        d.feed(&[2, 0b0000_0010, 0x81, 0x00]);
        let pads = d.pads();
        assert_eq!(pads[0], Some(Pad::Mapped(GamepadState::default())));
        assert_eq!(pads[1], Some(Pad::Mapped(GamepadState { buttons: btn::SOUTH, lx: -100, ly: 0, rx: 0, ry: 0 })));
    }

    #[test]
    fn short_presses_hold() {
        let mapped = |b| Some(Pad::Mapped(GamepadState { buttons: b, ..GamepadState::default() }));
        let mut pads = alloc::vec![mapped(btn::SOUTH), None];
        hold_presses(&mut pads, alloc::vec![mapped(btn::EAST), mapped(btn::NORTH)]);
        assert_eq!(pads, [mapped(btn::SOUTH | btn::EAST), mapped(btn::NORTH)]);
        hold_presses(&mut pads, alloc::vec![None, mapped(0)]);
        assert_eq!(pads, [None, mapped(btn::NORTH)]);
    }

    #[test]
    fn keyboards_are_not_pads() {
        let kbd = [0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0xC0];
        assert!(Decoder::hid(&kbd).is_none());
        let boot_kbd = InterfaceInfo { vendor: 1, product: 2, class: 3, subclass: 1, protocol: 1 };
        assert!(Decoder::probe(&boot_kbd).is_none());
    }

    #[test]
    fn wiiu_adapter() {
        let info = InterfaceInfo { vendor: 0x057E, product: 0x0337, class: 3, subclass: 0, protocol: 0 };
        let Some((Some(mut d), Start::Out(cmd))) = Decoder::probe(&info) else { panic!() };
        assert_eq!(cmd, &[0x13]);
        assert!(d.fixed_ports());
        let mut r = [0u8; 37];
        r[0] = 0x21;
        // Port 2: wired, stick resting a little off centre.
        r[10..19].copy_from_slice(&[0x10, 0, 0, 130, 125, 128, 128, 30, 30]);
        d.feed(&r);
        let p = d.pads();
        assert_eq!((p[0], p[2], p[3]), (None, None, None));
        assert_eq!(p[1], Some(Pad::Native(PadStatus { trigger_left: 30, trigger_right: 30, ..PadStatus::IDLE })));
        // A, d-pad up, START, Z; stick pushed right from its origin.
        r[10..19].copy_from_slice(&[0x10, 0x81, 0x03, 230, 125, 128, 128, 30, 30]);
        d.feed(&r);
        let Some(Pad::Native(s)) = d.pads()[1] else { panic!() };
        assert_eq!(s.button, PAD_BUTTON_A | PAD_BUTTON_UP | PAD_BUTTON_START | PAD_TRIGGER_Z);
        assert_eq!((s.stick_x, s.stick_y), (100, 0));
        // Unplugged.
        r[10] = 0;
        d.feed(&r);
        assert_eq!(d.pads()[1], None);
    }

    #[test]
    fn xbox360_pad() {
        let info = InterfaceInfo { vendor: 0x045E, product: 0x028E, class: 0xFF, subclass: 0x5D, protocol: 1 };
        let Some((Some(mut d), Start::None)) = Decoder::probe(&info) else { panic!() };
        let mut r = [0u8; 20];
        r[1] = 0x14;
        r[2] = 0x10 | 0x01; // START, up
        r[3] = 0x10 | 0x04; // A, guide
        r[5] = 0xFF; // RT
        r[6..8].copy_from_slice(&(-32768i16).to_le_bytes());
        r[8..10].copy_from_slice(&32767i16.to_le_bytes());
        d.feed(&r);
        assert_eq!(
            d.pads(),
            alloc::vec![Some(Pad::Mapped(GamepadState { buttons: btn::START | btn::UP | btn::SOUTH | btn::GUIDE | btn::R2, lx: -100, ly: 99, rx: 0, ry: 0 }))]
        );
    }

    #[test]
    fn xbox_one_pad() {
        let info = InterfaceInfo { vendor: 0x045E, product: 0x02EA, class: 0xFF, subclass: 0x47, protocol: 0xD0 };
        let Some((Some(mut d), Start::Out(_))) = Decoder::probe(&info) else { panic!() };
        let mut r = [0u8; 18];
        r[0] = 0x20;
        r[4] = 0x20; // B
        r[5] = 0x10; // LB
        r[6..8].copy_from_slice(&1023u16.to_le_bytes());
        d.feed(&r);
        d.feed(&[0x07, 0x30, 0x01, 0x02, 0x01, 0x5B]);
        let Some(Pad::Mapped(s)) = d.pads()[0] else { panic!() };
        assert_eq!(s.buttons, btn::EAST | btn::L1 | btn::L2 | btn::GUIDE);
    }
}
