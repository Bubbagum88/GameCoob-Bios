//! Booting a PC disc image (`.iso`) from a file: the image is read into
//! memory, handed to the firmware's RAM disk driver as a virtual CD
//! (EFI_RAM_DISK_PROTOCOL, the way HTTP boot starts ISOs), and its
//! `\EFI\BOOT\BOOTX64.EFI` is started right away. A reset would lose the
//! RAM disk, so unlike everything else this doesn't go through BootNext.
//!
//! Whether the OS then finds its files depends on the OS: the boot loader
//! reads them through the firmware, but the kernel needs to see the RAM
//! disk itself (the firmware describes it in ACPI's NFIT table).

use crate::fs::read_file_at;
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;
use uefi::boot::{self, AllocateType, LoadImageSource, MemoryType};
use uefi::proto::device_path::build::{self, DevicePathBuilder};
use uefi::proto::device_path::{DevicePath, FfiDevicePath};
use uefi::proto::media::block::BlockIO;
use uefi::proto::media::fs::SimpleFileSystem;
use uefi::proto::unsafe_protocol;
use uefi::proto::BootPolicy;
use uefi::{cstr16, guid, Guid, Status};

/// EFI_RAM_DISK_PROTOCOL.
#[repr(C)]
struct RawRamDisk {
    register: unsafe extern "efiapi" fn(u64, u64, *const Guid, *const FfiDevicePath, *mut *const FfiDevicePath) -> Status,
    #[allow(dead_code)]
    unregister: unsafe extern "efiapi" fn(*const FfiDevicePath) -> Status,
}

#[unsafe_protocol("ab38a0df-6873-44a9-87e6-d4eb56148449")]
#[repr(transparent)]
struct RamDisk(RawRamDisk);

/// EFI_VIRTUAL_CD_GUID: an ISO image in volatile memory.
const VIRTUAL_CD: Guid = guid!("3d5abd30-4175-87ce-6d64-d2ade523c4bb");

/// Read the image at `path` on volume `vol` and boot it. Returns only on
/// failure (or when the started loader exits), with the reason.
pub fn boot_iso(vol: &str, path: &str, size: u64) -> String {
    match try_boot(vol, path, size) {
        Ok(()) => String::from("The disc's boot loader exited."),
        Err(e) => e,
    }
}

fn st(what: &str) -> impl Fn(uefi::Error) -> String + '_ {
    move |e| format!("{what}: {:?}", e.status())
}

fn try_boot(vol: &str, path: &str, size: u64) -> Result<(), String> {
    let rd_handle = boot::get_handle_for_protocol::<RamDisk>().map_err(|_| String::from("This PC's firmware can't boot disc images (no RAM disk support)."))?;
    let rd = boot::open_protocol_exclusive::<RamDisk>(rd_handle).map_err(st("RAM disk"))?;
    let len = usize::try_from(size).map_err(|_| String::from("The image is too big."))?;
    let pages = len.div_ceil(4096);
    // Reserved memory: the OS must leave the image alone.
    let mem = boot::allocate_pages(AllocateType::AnyPages, MemoryType::RESERVED, pages)
        .map_err(|_| format!("Not enough memory for the image ({} MiB).", size >> 20))?;
    // SAFETY: freshly allocated, `pages` pages long.
    let buf = unsafe { core::slice::from_raw_parts_mut(mem.as_ptr(), len) };
    log::info!("reading {path} ({} MiB) into a RAM disk", size >> 20);
    if let Err(e) = read_file_at(vol, path, 0, buf) {
        // SAFETY: allocated above and not handed out.
        let _ = unsafe { boot::free_pages(mem, pages) };
        return Err(format!("Could not read the image: {e}"));
    }
    let mut dp: *const FfiDevicePath = core::ptr::null();
    // SAFETY: the buffer stays allocated for good; the protocol fills `dp`.
    let s = unsafe { (rd.0.register)(mem.as_ptr() as u64, size, &VIRTUAL_CD, core::ptr::null(), &mut dp) };
    if s.is_error() || dp.is_null() {
        // SAFETY: as above.
        let _ = unsafe { boot::free_pages(mem, pages) };
        return Err(format!("The firmware refused the RAM disk: {s:?}"));
    }
    // SAFETY: the RAM disk driver owns this path while the disk exists.
    let disk_dp = unsafe { DevicePath::from_ffi_ptr(dp) };
    let disk = boot::locate_device_path::<BlockIO>(&mut &*disk_dp).map_err(st("RAM disk device"))?;
    // So the disc's El Torito image gets a partition and a file system.
    let _ = boot::connect_controller(disk, &[], None, true);
    let prefix = disk_dp.as_bytes();
    let prefix = &prefix[..prefix.len().saturating_sub(4)];
    let fs_handles = boot::find_handles::<SimpleFileSystem>().map_err(st("file systems"))?;
    for h in fs_handles {
        let Ok(fs_dp) = boot::open_protocol_exclusive::<DevicePath>(h) else { continue };
        if !fs_dp.as_bytes().starts_with(prefix) {
            continue;
        }
        let mut v = Vec::new();
        let mut b = DevicePathBuilder::with_vec(&mut v);
        for node in fs_dp.node_iter() {
            b = b.push(&node).map_err(|e| format!("{e:?}"))?;
        }
        let full = b
            .push(&build::media::FilePath { path_name: cstr16!("\\EFI\\BOOT\\BOOTX64.EFI") })
            .and_then(|b| b.finalize())
            .map_err(|e| format!("{e:?}"))?;
        drop(fs_dp);
        let img = match boot::load_image(boot::image_handle(), LoadImageSource::FromDevicePath { device_path: full, boot_policy: BootPolicy::ExactMatch }) {
            Ok(i) => i,
            Err(e) => {
                log::warn!("no boot loader on {:?}: {:?}", h, e.status());
                continue;
            }
        };
        log::info!("starting the image's boot loader");
        return boot::start_image(img).map_err(st("The boot loader stopped"));
    }
    Err(String::from("The image has no UEFI boot loader (\\EFI\\BOOT\\BOOTX64.EFI)."))
}
