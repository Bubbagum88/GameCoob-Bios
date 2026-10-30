//! GameCoob: a UEFI boot app that plays the GameCube intro and cube menu,
//! rebuilt from the user's own IPL dump, for booting and changing firmware
//! boot settings.

#![no_std]
#![no_main]

extern crate alloc;

mod boot_intro;
mod bs2;
mod clock;
mod firmware;
mod gfx;
mod ipl;
mod sdk;
mod vars;

use core::time::Duration;
use uefi::prelude::*;
use uefi::proto::console::gop::GraphicsOutput;

#[entry]
fn main() -> Status {
    uefi::helpers::init().unwrap();
    // The firmware's 5-minute watchdog would reset us while idling in the menu.
    let _ = boot::set_watchdog_timer(0, 0x10000, None);
    // BS2Main builds the sine table first thing (SinTableInit(12), RandInit).
    bs2::mtx::sin_table_init(12);
    bs2::rnd::rand_init();

    // Checked before taking the screen, so the text console can still say why.
    let Some((assets, menu_assets)) = boot_intro::load() else {
        return no_dump();
    };

    let handle = match boot::get_handle_for_protocol::<GraphicsOutput>() {
        Ok(h) => h,
        Err(_) => {
            log::error!("No graphics output available");
            return Status::UNSUPPORTED;
        }
    };
    let mut gop = boot::open_protocol_exclusive::<GraphicsOutput>(handle).unwrap();
    let (w, h) = gop.current_mode_info().resolution();
    let mut canvas = gfx::Canvas::new(w, h);
    let clock = clock::Clock::calibrate();

    let _ = system::with_stdin(|i| i.reset(false));
    boot_intro::play(&mut gop, &mut canvas, &clock, &assets, &menu_assets)
}

/// Without a usable dump there is nothing to show: say where it goes, wait
/// for a key (or a few seconds), then hand back to the firmware, which boots
/// its next entry.
fn no_dump() -> Status {
    uefi::println!("GameCoob needs your own GameCube IPL dump.");
    uefi::println!("Copy it to \\EFI\\GameCoob\\ipl.bin on this boot partition.");
    uefi::println!("Continuing to the next boot entry...");
    let _ = system::with_stdin(|i| i.reset(false));
    for _ in 0..100 {
        if matches!(system::with_stdin(|i| i.read_key()), Ok(Some(_))) {
            break;
        }
        boot::stall(Duration::from_millis(100));
    }
    Status::NOT_FOUND
}
