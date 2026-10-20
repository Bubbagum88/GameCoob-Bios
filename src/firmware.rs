//! Firmware actions the menu carries out. Settings are standard UEFI
//! variables, so changes here are what the firmware itself will honor. The
//! actions that reset only return on failure, with a message saying why.

use alloc::format;
use alloc::string::String;
use uefi::Status;
use uefi::runtime::{self, ResetType};

use crate::vars;

pub const MAX_TIMEOUT: u16 = 30;

/// Boot Boot#### next (BootNext), then restart.
pub fn boot_entry(num: u16) -> String {
    match vars::set_boot_next(num) {
        Ok(()) => runtime::reset(ResetType::COLD, Status::SUCCESS, None),
        Err(err) => format!("Could not set BootNext: {:?}", err.status()),
    }
}

/// Restart into the firmware's own setup screen.
pub fn firmware_setup() -> String {
    match vars::request_firmware_ui() {
        Ok(()) => runtime::reset(ResetType::COLD, Status::SUCCESS, None),
        Err(err) => format!("Could not request setup: {:?}", err.status()),
    }
}

pub fn restart() -> ! {
    runtime::reset(ResetType::COLD, Status::SUCCESS, None)
}

pub fn power_off() -> ! {
    runtime::reset(ResetType::SHUTDOWN, Status::SUCCESS, None)
}

/// Store the firmware boot Timeout variable.
pub fn save_timeout(secs: u16) -> Result<(), String> {
    vars::set_timeout(secs).map_err(|err| format!("Could not save Timeout: {:?}", err.status()))
}

/// Set the firmware's wall clock (SetTime) to year, month 1..12, day, hour,
/// minute, second, keeping its time zone and daylight setting.
pub fn set_time(t: [i32; 6]) -> Result<(), String> {
    use uefi::runtime::{Daylight, Time, TimeParams};
    let (time_zone, daylight) = match runtime::get_time() {
        Ok(now) => (now.time_zone(), now.daylight()),
        Err(_) => (None, Daylight::empty()),
    };
    let [year, month, day, hour, minute, second] = t;
    let time = Time::new(TimeParams {
        year: year as u16,
        month: month as u8,
        day: day as u8,
        hour: hour as u8,
        minute: minute as u8,
        second: second as u8,
        nanosecond: 0,
        time_zone,
        daylight,
    })
    .map_err(|e| format!("Bad date: {e:?}"))?;
    // SAFETY: the app is single-threaded on the firmware side; the other
    // cores only rasterize and never call the firmware.
    unsafe { runtime::set_time(&time) }.map_err(|err| format!("Could not set the clock: {:?}", err.status()))
}
