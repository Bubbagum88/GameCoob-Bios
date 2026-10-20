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
