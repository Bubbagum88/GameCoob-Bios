//! The firmware's setup options over UEFI: the HII database's package
//! lists, the config routing protocol's export of current values, and
//! RouteConfig to hand changes to the drivers that own them (the same calls
//! the firmware's own setup screen makes). The parsing is in `src/hii`.

use crate::hii::Firmware;
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;
use core::ptr;
use uefi::boot::{self, OpenProtocolAttributes, OpenProtocolParams, ScopedProtocol, SearchType};
use uefi::proto::hii::database::HiiDatabase;
use uefi::runtime::{self, VariableVendor};
use uefi::{CString16, Guid, Status};
use uefi_raw::protocol::hii::config::HiiConfigRoutingProtocol;

#[repr(transparent)]
#[uefi::proto::unsafe_protocol("587e72d7-cc50-4f79-8209-ca291fc1a10f")]
struct Routing(HiiConfigRoutingProtocol);

/// Open a protocol without taking it from anyone (GetProtocol).
fn open<P: uefi::proto::ProtocolPointer + ?Sized>() -> Result<ScopedProtocol<P>, String> {
    let h = boot::locate_handle_buffer(SearchType::ByProtocol(&P::GUID))
        .map_err(|e| format!("not found ({:?})", e.status()))?
        .first()
        .copied()
        .ok_or_else(|| String::from("not found"))?;
    // SAFETY: nothing here uninstalls or disconnects the protocol while
    // it is open, and it is only used for the duration of one call.
    unsafe {
        boot::open_protocol::<P>(
            OpenProtocolParams { handle: h, agent: boot::image_handle(), controller: None },
            OpenProtocolAttributes::GetProtocol,
        )
    }
    .map_err(|e| format!("{:?}", e.status()))
}

/// A NUL-terminated UTF-16 string the firmware allocated, as text; frees it.
///
/// # Safety
/// `p` is null or a pool allocation holding a NUL-terminated UCS-2 string.
unsafe fn take_pool_str(p: *mut u16) -> String {
    if p.is_null() {
        return String::new();
    }
    let mut units = Vec::new();
    let mut i = 0;
    loop {
        // SAFETY: up to and including the NUL, per the contract.
        let c = unsafe { *p.add(i) };
        if c == 0 {
            break;
        }
        units.push(c);
        i += 1;
    }
    // SAFETY: allocated from the pool by the firmware.
    unsafe {
        let _ = boot::free_pool(ptr::NonNull::new_unchecked(p.cast()));
    }
    char::decode_utf16(units).map(|c| c.unwrap_or('?')).collect()
}

pub struct UefiSetup;

impl Firmware for UefiSetup {
    fn hii_export(&mut self) -> Result<Vec<u8>, String> {
        let db = open::<HiiDatabase>().map_err(|e| format!("No HII database: {e}"))?;
        db.export_all_raw().map(|b| b.into_vec()).map_err(|e| format!("HII export failed: {:?}", e.status()))
    }

    fn config_export(&mut self) -> Result<String, String> {
        let r = open::<Routing>().map_err(|e| format!("No config routing: {e}"))?;
        let mut out: *mut u16 = ptr::null_mut();
        // SAFETY: the protocol is open; `out` receives a pool string.
        let st = unsafe { (r.0.export_config)(&r.0, &mut out) };
        // SAFETY: null or the firmware's pool string.
        let s = unsafe { take_pool_str(out) };
        if st.is_error() && s.is_empty() {
            return Err(format!("Config export failed: {st:?}"));
        }
        Ok(s)
    }

    fn variable(&mut self, name: &str, guid: &[u8; 16]) -> Option<Vec<u8>> {
        let n = CString16::try_from(name).ok()?;
        let v = VariableVendor(Guid::from_bytes(*guid));
        runtime::get_variable_boxed(&n, &v).ok().map(|(b, _)| b.into_vec())
    }

    fn route_config(&mut self, cfg: &str) -> Result<(), String> {
        let r = open::<Routing>().map_err(|e| format!("No config routing: {e}"))?;
        let s = CString16::try_from(cfg).map_err(|_| String::from("Bad config string"))?;
        let mut progress: *const u16 = ptr::null();
        // SAFETY: `s` is NUL-terminated and outlives the call.
        let st = unsafe { (r.0.route_config)(&r.0, s.as_ptr().cast(), &mut progress) };
        if st == Status::SUCCESS {
            Ok(())
        } else {
            // How far the firmware got, for the note.
            let at = if progress.is_null() { 0 } else { (progress as usize).wrapping_sub(s.as_ptr() as *const u16 as usize) / 2 };
            Err(format!("The firmware refused it ({st:?} at {at})"))
        }
    }
}
