//! GameCoob: a UEFI boot app that plays the GameCube intro (rebuilt from the
//! user's own IPL dump, or an original cube intro without one) and then shows
//! a console-style menu for booting and changing firmware boot settings.

#![no_std]
#![no_main]

extern crate alloc;

mod boot_intro;
mod bs2;
mod clock;
mod gfx;
mod intro;
mod ipl;
mod math;
mod menu;
mod vars;

use uefi::prelude::*;
use uefi::proto::console::gop::GraphicsOutput;

#[entry]
fn main() -> Status {
    uefi::helpers::init().unwrap();
    // The firmware's 5-minute watchdog would reset us while idling in the menu.
    let _ = boot::set_watchdog_timer(0, 0x10000, None);

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
    let palette = match boot_intro::load() {
        Some(assets) => {
            boot_intro::play(&mut gop, &mut canvas, &clock, &assets);
            intro::NORMAL
        }
        None => intro::play(&mut gop, &mut canvas, &clock),
    };
    menu::run(&mut gop, &mut canvas, &clock, palette)
}
