//! Holding the screen black until the display can show the intro.
//!
//! GameCoob never changes the screen mode: it draws in the one the firmware
//! set. But a monitor or TV on an external port (HDMI, DisplayPort, DVI,
//! VGA, or one behind a dock) often needs one to three seconds after the
//! firmware's mode set before it shows a picture, and with fast boot the
//! app can start inside that window. The intro's first seconds (the cube
//! falling, the music's start) would then be lost on such a screen, so the
//! app holds black for a while first. A built-in laptop panel is ready at
//! once, and so is a virtual machine's window.
//!
//! What is attached is read from the outputs' device paths: graphics
//! drivers put an ACPI `_ADR` node on each output, whose display type
//! (bits 11:8, ACPI spec "Display Output Device" _ADR) says VGA, TV,
//! external digital or internal panel. The hold is counted from the app's
//! start, so the time spent loading the dump counts toward it.
//!
//! The hold is the `GameCoobIntroDelay` variable ([`crate::vars`]): Auto
//! (unset) picks it from what is attached, or a fixed number of seconds.
//! Keys pressed while black are kept for the intro (the easter-egg Z).

use core::sync::atomic::{AtomicU8, Ordering};
use core::time::Duration;
use uefi::boot::{self, OpenProtocolAttributes, OpenProtocolParams};
use uefi::proto::console::gop::{EdidDiscovered, GraphicsOutput};
use uefi::proto::device_path::{DevicePath, DeviceSubType, DeviceType};
use uefi::Handle;

/// What the graphics outputs say is attached.
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub struct Outputs {
    /// Outputs with a GOP of their own.
    pub count: u32,
    /// A built-in panel (eDP, LVDS, DSI).
    pub panel: bool,
    /// An external digital monitor (HDMI, DisplayPort, DVI).
    pub external: bool,
    /// A TV (the ACPI type for TVs and HDTVs).
    pub tv: bool,
    /// An analog VGA monitor.
    pub vga: bool,
    /// Running under a hypervisor (CPUID says so).
    pub vm: bool,
}

/// The ACPI display type of an `_ADR` value: 1 VGA, 2 TV, 3 external
/// digital, 4 internal panel, 0 other. Old-style ids (bit 31 clear) put
/// 0x100 = CRT, 0x200 = TV and 0x400 = LCD in the same bits.
fn display_type(adr: u32) -> u32 {
    (adr >> 8) & 0xF
}

/// The `_ADR` values on the device path of `h`, if it has one.
fn adrs(h: Handle, out: &mut impl FnMut(u32)) {
    // SAFETY: only read here, while the handle's protocols are in place.
    let Ok(dp) = (unsafe {
        boot::open_protocol::<DevicePath>(
            OpenProtocolParams { handle: h, agent: boot::image_handle(), controller: None },
            OpenProtocolAttributes::GetProtocol,
        )
    }) else {
        return;
    };
    for node in dp.node_iter() {
        if node.device_type() == DeviceType::ACPI && node.sub_type() == DeviceSubType::ACPI_ADR {
            // One or more u32s, for outputs that show the same picture.
            for c in node.data().chunks_exact(4) {
                out(u32::from_le_bytes([c[0], c[1], c[2], c[3]]));
            }
        }
    }
}

/// The EDID's three-letter maker id ("SAM", "DEL"...), for the log.
fn edid_maker(h: Handle) -> Option<[u8; 3]> {
    // SAFETY: as in `adrs`.
    let e = unsafe {
        boot::open_protocol::<EdidDiscovered>(
            OpenProtocolParams { handle: h, agent: boot::image_handle(), controller: None },
            OpenProtocolAttributes::GetProtocol,
        )
    }
    .ok()?;
    let b = e.edid()?;
    if b.len() < 10 || b[..8] != [0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0] {
        return None;
    }
    let id = u16::from_be_bytes([b[8], b[9]]);
    let l = |s: u16| b'@' + ((id >> s) & 0x1F) as u8;
    Some([l(10), l(5), l(0)])
}

fn hypervisor() -> bool {
    // CPUID.1:ECX bit 31, set by every hypervisor for its guests.
    #[allow(unused_unsafe)]
    unsafe { core::arch::x86_64::__cpuid(1).ecx & (1 << 31) != 0 }
}

/// Look at every graphics output (not only the one drawn on: the firmware's
/// console can show it on all of them).
pub fn outputs() -> Outputs {
    let mut o = Outputs { vm: hypervisor(), ..Outputs::default() };
    let handles = boot::find_handles::<GraphicsOutput>().unwrap_or_default();
    for &h in handles.iter() {
        o.count += 1;
        let mut types = 0u32;
        adrs(h, &mut |adr| {
            types |= 1 << display_type(adr);
            match display_type(adr) {
                1 => o.vga = true,
                2 => o.tv = true,
                3 => o.external = true,
                4 => o.panel = true,
                _ => {}
            }
        });
        match edid_maker(h) {
            Some(m) => log::info!(
                "Display: output {} types {:#x}, EDID {}",
                o.count,
                types,
                core::str::from_utf8(&m).unwrap_or("?")
            ),
            None => log::info!("Display: output {} types {:#x}, no EDID", o.count, types),
        }
    }
    o
}

/// The automatic hold, in seconds: long for TVs (they are the slowest to
/// sync, and often scale too), shorter for monitors, none for a panel or
/// a virtual machine.
pub fn auto_delay(o: &Outputs) -> f32 {
    if o.tv {
        3.0
    } else if o.external {
        2.0
    } else if o.vm || o.panel {
        // A VM's screen is a window (OVMF's emulated card calls itself
        // VGA); a GPU passed through to it still reports its real ports.
        0.0
    } else if o.vga {
        2.0
    } else {
        // Real hardware whose driver names no output type: likely a
        // desktop's add-in card on an external port.
        1.5
    }
}

/// The hold the setting asks for: `None` is Auto.
pub fn delay(setting: Option<u8>, o: &Outputs) -> f32 {
    match setting {
        Some(s) => s as f32,
        None => auto_delay(o),
    }
}

/// Clear the screen to black and wait until `secs` after `started` (on
/// `now`'s clock, in seconds).
pub fn hold(gop: &mut GraphicsOutput, secs: f32, started: f32, now: &dyn Fn() -> f32) {
    use uefi::proto::console::gop::{BltOp, BltPixel};
    let (w, h) = gop.current_mode_info().resolution();
    let _ = gop.blt(BltOp::VideoFill { color: BltPixel::new(0, 0, 0), dest: (0, 0), dims: (w, h) });
    let wait = secs - (now() - started);
    if wait > 0.0 {
        log::info!("Display: holding black {wait:.1} s");
        boot::stall(Duration::from_micros((wait * 1e6) as u64));
    }
}

/// The setting (0xFF = Auto) and Auto's pick in tenths of a second, for
/// the system dialog's row.
static SETTING: AtomicU8 = AtomicU8::new(0xFF);
static AUTO_TENTHS: AtomicU8 = AtomicU8::new(0);

pub fn set_setting(s: Option<u8>) {
    SETTING.store(s.unwrap_or(0xFF), Ordering::Relaxed);
}

pub fn setting() -> Option<u8> {
    Some(SETTING.load(Ordering::Relaxed)).filter(|&s| s != 0xFF)
}

pub fn set_auto(secs: f32) {
    AUTO_TENTHS.store((secs * 10.0) as u8, Ordering::Relaxed);
}

/// Auto's pick on this PC, in seconds.
pub fn auto() -> f32 {
    AUTO_TENTHS.load(Ordering::Relaxed) as f32 / 10.0
}

/// The choices the dialog row steps through.
pub const CHOICES: [Option<u8>; 6] = [None, Some(0), Some(1), Some(2), Some(3), Some(5)];

/// The choice after `s`.
pub fn next(s: Option<u8>) -> Option<u8> {
    let i = CHOICES.iter().position(|&c| c == s).unwrap_or(0);
    CHOICES[(i + 1) % CHOICES.len()]
}
