//! The firmware's file systems for the Card Files explorer and the card
//! write-back: every handle with SimpleFileSystem (FAT partitions, USB
//! sticks; firmware reads no NTFS or ext4), named by its device path.

use crate::bs2::menu::volumes::{Entry, Volume, Volumes};
use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec::Vec;
use uefi::boot::{self, ScopedProtocol};
use uefi::fs::FileSystem;
use uefi::proto::device_path::text::{AllowShortcuts, DisplayOnly};
use uefi::proto::device_path::DevicePath;
use uefi::proto::loaded_image::LoadedImage;
use uefi::proto::media::file::{File, FileSystemInfo};
use uefi::proto::media::fs::SimpleFileSystem;
use uefi::runtime::{self, VariableAttributes, VariableVendor};
use uefi::{cstr16, guid, CStr16, CString16, Handle};

/// GameCoob's own UEFI variables.
const VENDOR: VariableVendor = VariableVendor(guid!("edc5e45c-31db-47a4-8969-2fe623cc0605"));
/// The card image left in each slot, as a UTF-8 volume handle (one 0
/// byte: left empty).
const SLOT_VARS: [&CStr16; 2] = [cstr16!("CardSlotA"), cstr16!("CardSlotB")];

pub struct UefiVolumes;

fn device_id(h: Handle) -> Option<String> {
    let dp = boot::open_protocol_exclusive::<DevicePath>(h).ok()?;
    dp.to_string16(DisplayOnly(false), AllowShortcuts(false)).ok().map(|s| s.to_string())
}

/// The file system handle GameCoob was loaded from.
fn boot_handle() -> Option<Handle> {
    let li = boot::open_protocol_exclusive::<LoadedImage>(boot::image_handle()).ok()?;
    let dev = li.device()?;
    let dp = boot::open_protocol_exclusive::<DevicePath>(dev).ok()?;
    boot::locate_device_path::<SimpleFileSystem>(&mut &*dp).ok()
}

fn handles() -> Vec<Handle> {
    let mut v = boot::find_handles::<SimpleFileSystem>().unwrap_or_default();
    if let Some(b) = boot_handle() {
        if let Some(i) = v.iter().position(|&h| h == b) {
            v.remove(i);
            v.insert(0, b);
        }
    }
    v
}

fn open(id: &str) -> Result<FileSystem, String> {
    let h = handles().into_iter().find(|&h| device_id(h).as_deref() == Some(id)).ok_or("drive is gone")?;
    let sfs: ScopedProtocol<SimpleFileSystem> = boot::open_protocol_exclusive(h).map_err(|e| format!("{:?}", e.status()))?;
    Ok(FileSystem::new(sfs))
}

fn path16(p: &str) -> Result<CString16, String> {
    CString16::try_from(p).map_err(|_| String::from("bad name"))
}

fn err(e: uefi::fs::Error) -> String {
    match e {
        uefi::fs::Error::Io(io) => format!("{:?}", io.uefi_error.status()),
        e => format!("{e:?}"),
    }
}

impl UefiVolumes {
    /// The boot volume's id, for the card images loaded at boot.
    pub fn boot_id() -> Option<String> {
        boot_handle().and_then(device_id)
    }
}

impl Volumes for UefiVolumes {
    fn volumes(&mut self) -> Vec<Volume> {
        let boot = boot_handle();
        let mut out = Vec::new();
        for h in handles() {
            let Some(id) = device_id(h) else { continue };
            let Ok(mut sfs) = boot::open_protocol_exclusive::<SimpleFileSystem>(h) else { continue };
            let Ok(mut root) = sfs.open_volume() else { continue };
            let (label, size, free) = match root.get_boxed_info::<FileSystemInfo>() {
                Ok(i) => (i.volume_label().to_string(), i.volume_size(), i.free_space()),
                Err(_) => (String::new(), 0, 0),
            };
            out.push(Volume { id, label: label.trim().to_string(), size, free, boot: Some(h) == boot });
        }
        out
    }

    fn list(&mut self, id: &str, path: &str) -> Result<Vec<Entry>, String> {
        let mut fs = open(id)?;
        let p = path16(path)?;
        let mut out = Vec::new();
        for e in fs.read_dir(p.as_ref()).map_err(err)? {
            let Ok(e) = e else { continue };
            let name = e.file_name().to_string();
            if name == "." || name == ".." {
                continue;
            }
            out.push(Entry { name, dir: e.is_directory(), size: e.file_size() });
        }
        Ok(out)
    }

    fn read(&mut self, id: &str, path: &str) -> Result<Vec<u8>, String> {
        open(id)?.read(path16(path)?.as_ref()).map_err(err)
    }

    fn write(&mut self, id: &str, path: &str, bytes: &[u8]) -> Result<(), String> {
        open(id)?.write(path16(path)?.as_ref(), bytes).map_err(err)
    }

    fn mkdir(&mut self, id: &str, path: &str) -> Result<(), String> {
        let mut fs = open(id)?;
        let p = path16(path)?;
        if fs.try_exists(p.as_ref()).unwrap_or(false) {
            return Ok(());
        }
        fs.create_dir(p.as_ref()).map_err(err)
    }

    fn slot_memory(&mut self, slot: usize) -> Option<String> {
        let (b, _) = runtime::get_variable_boxed(SLOT_VARS[slot & 1], &VENDOR).ok()?;
        let b = b.into_vec();
        if b == [0] {
            return Some(String::new());
        }
        String::from_utf8(b).ok()
    }

    fn set_slot_memory(&mut self, slot: usize, handle: &str) -> Result<(), String> {
        let attrs = VariableAttributes::NON_VOLATILE | VariableAttributes::BOOTSERVICE_ACCESS;
        // A zero-size write would delete the variable, so empty is one 0.
        let data = if handle.is_empty() { &[0u8][..] } else { handle.as_bytes() };
        runtime::set_variable(SLOT_VARS[slot & 1], &VENDOR, attrs, data).map_err(|e| format!("{:?}", e.status()))
    }
}
