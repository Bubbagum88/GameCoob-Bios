//! Firmware actions the menu carries out. Settings are standard UEFI
//! variables, so changes here are what the firmware itself will honor. The
//! actions that reset only return on failure, with a message saying why.

use alloc::format;
use alloc::string::String;
use uefi::Status;
use uefi::runtime::{self, ResetType};

use crate::vars;

pub const MAX_TIMEOUT: u16 = 30;

/// Write to COM1 directly: the firmware's console would draw over the
/// picture. Harmless on a PC without the port.
pub fn serial_write(s: &str) {
    for b in s.bytes() {
        // SAFETY: a byte to the standard COM1 data port.
        unsafe { core::arch::asm!("out dx, al", in("dx") 0x3f8u16, in("al") b, options(nomem, nostack)) };
    }
}

/// Restart the machine. The serial line tells scripts\run-qemu.ps1 to
/// start QEMU again: QEMU's Windows Hypervisor Platform backend can't
/// reset a running VM, so the script runs it with -no-reboot.
fn reset() -> ! {
    serial_write("GameCoob: reset\r\n");
    runtime::reset(ResetType::COLD, Status::SUCCESS, None)
}

/// Boot Boot#### next (BootNext), then restart.
pub fn boot_entry(num: u16) -> String {
    match vars::set_boot_next(num) {
        Ok(()) => reset(),
        Err(err) => format!("Could not set BootNext: {:?}", err.status()),
    }
}

/// Restart into the firmware's own setup screen.
pub fn firmware_setup() -> String {
    match vars::request_firmware_ui() {
        Ok(()) => reset(),
        Err(err) => format!("Could not request setup: {:?}", err.status()),
    }
}

pub fn restart() -> ! {
    reset()
}

pub fn power_off() -> ! {
    runtime::reset(ResetType::SHUTDOWN, Status::SUCCESS, None)
}

/// Store the firmware boot Timeout variable.
pub fn save_timeout(secs: u16) -> Result<(), String> {
    vars::set_timeout(secs).map_err(|err| format!("Could not save Timeout: {:?}", err.status()))
}
