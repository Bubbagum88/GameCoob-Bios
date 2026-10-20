//! Standard UEFI global variables: boot entries, BootOrder, BootNext, Timeout,
//! and the OsIndications "boot to firmware UI" request.

use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;
use uefi::runtime::{self, VariableAttributes, VariableVendor};
use uefi::{CStr16, CString16, cstr16};

const GLOBAL: &VariableVendor = &VariableVendor::GLOBAL_VARIABLE;

/// LOAD_OPTION_ACTIVE / LOAD_OPTION_HIDDEN from the spec.
const LOAD_OPTION_ACTIVE: u32 = 0x1;
const LOAD_OPTION_HIDDEN: u32 = 0x8;
/// EFI_OS_INDICATIONS_BOOT_TO_FW_UI.
const BOOT_TO_FW_UI: u64 = 0x1;

fn attrs() -> VariableAttributes {
    VariableAttributes::NON_VOLATILE | VariableAttributes::BOOTSERVICE_ACCESS | VariableAttributes::RUNTIME_ACCESS
}

fn read(name: &CStr16) -> Option<Vec<u8>> {
    runtime::get_variable_boxed(name, GLOBAL).ok().map(|(b, _)| b.into_vec())
}

fn le_u16s(b: &[u8]) -> Vec<u16> {
    b.chunks_exact(2).map(|c| u16::from_le_bytes([c[0], c[1]])).collect()
}

pub struct BootEntry {
    pub num: u16,
    pub desc: String,
}

/// Active, visible entries in BootOrder order.
pub fn boot_entries() -> Vec<BootEntry> {
    let order = read(cstr16!("BootOrder")).map(|b| le_u16s(&b)).unwrap_or_default();
    order.into_iter().filter_map(boot_entry).collect()
}

fn boot_entry(num: u16) -> Option<BootEntry> {
    let name = CString16::try_from(format!("Boot{num:04X}").as_str()).ok()?;
    let data = read(&name)?;
    // EFI_LOAD_OPTION: u32 Attributes, u16 FilePathListLength, CHAR16 Description[]...
    if data.len() < 8 {
        return None;
    }
    let attr = u32::from_le_bytes([data[0], data[1], data[2], data[3]]);
    if attr & LOAD_OPTION_ACTIVE == 0 || attr & LOAD_OPTION_HIDDEN != 0 {
        return None;
    }
    let desc: Vec<u16> = le_u16s(&data[6..]).into_iter().take_while(|&c| c != 0).collect();
    Some(BootEntry { num, desc: String::from_utf16_lossy(&desc) })
}

pub fn timeout() -> Option<u16> {
    read(cstr16!("Timeout")).filter(|b| b.len() >= 2).map(|b| u16::from_le_bytes([b[0], b[1]]))
}

pub fn set_timeout(secs: u16) -> uefi::Result {
    runtime::set_variable(cstr16!("Timeout"), GLOBAL, attrs(), &secs.to_le_bytes())
}

/// Make the next boot (only) use Boot####.
pub fn set_boot_next(num: u16) -> uefi::Result {
    runtime::set_variable(cstr16!("BootNext"), GLOBAL, attrs(), &num.to_le_bytes())
}

pub fn firmware_ui_supported() -> bool {
    read(cstr16!("OsIndicationsSupported"))
        .filter(|b| b.len() >= 8)
        .map(|b| u64::from_le_bytes(b[..8].try_into().unwrap()) & BOOT_TO_FW_UI != 0)
        .unwrap_or(false)
}

/// Ask the firmware to open its setup screen on the next boot.
pub fn request_firmware_ui() -> uefi::Result {
    let cur = read(cstr16!("OsIndications"))
        .filter(|b| b.len() >= 8)
        .map(|b| u64::from_le_bytes(b[..8].try_into().unwrap()))
        .unwrap_or(0);
    runtime::set_variable(cstr16!("OsIndications"), GLOBAL, attrs(), &(cur | BOOT_TO_FW_UI).to_le_bytes())
}
