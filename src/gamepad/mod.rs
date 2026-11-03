//! USB gamepads, read straight from the firmware's USB stack: firmware has
//! no gamepad drivers, so GameCoob finds the controllers among the USB
//! interfaces itself (UEFI `EFI_USB_IO_PROTOCOL`), starts each one, and
//! lets the host controller poll its interrupt IN endpoint in the
//! background. [`Gamepads::poll`] then just reads the latest report of
//! each. Devices plugged in or pulled out while GameCoob runs are picked up
//! by a rescan every second.
//!
//! [`report`] knows the controllers and their reports.

pub mod report;

use alloc::boxed::Box;
use alloc::vec::Vec;
use core::cell::UnsafeCell;
use core::ffi::c_void;
use report::{Decoder, InterfaceInfo, Pad, Start};
use uefi::boot::{self, OpenProtocolAttributes, OpenProtocolParams, ScopedProtocol, SearchType, Tpl};
use uefi::proto::usb::io::{ControlTransfer, UsbIo};
use uefi::{Handle, Identify};
use uefi_raw::protocol::usb::UsbTransferStatus;
use uefi_raw::protocol::usb::io::UsbIoProtocol;

/// The largest report kept.
const REPORT_MAX: usize = 64;
/// Reports kept between two polls. A slow frame still sees a button that
/// was pressed and let go in between (64 is half a second of a pad that
/// reports 125 times a second, even with no change).
const QUEUE: usize = 64;

/// What the host controller's callback writes and [`Gamepads::poll`] reads,
/// at raised TPL so the two never overlap.
struct Shared {
    /// The last [`QUEUE`] reports, report `n` at `n % QUEUE`.
    len: [usize; QUEUE],
    buf: [[u8; REPORT_MAX]; QUEUE],
    /// Reports received.
    count: u32,
    /// Failed transfers in a row.
    errors: u32,
}

/// The host controller's callback for each finished interrupt transfer.
unsafe extern "efiapi" fn on_report(data: *const c_void, len: usize, ctx: *const c_void, status: UsbTransferStatus) -> uefi::Status {
    // SAFETY: `ctx` is the device's `Shared`, which outlives the transfer
    // (it is leaked if the device goes away before the transfer is stopped),
    // and the host controller calls back at TPL_NOTIFY, the level the
    // poll raises to while it reads, so nothing else touches it now.
    let s = unsafe { &mut *(ctx as *mut Shared) };
    if !status.is_empty() || data.is_null() {
        s.errors = s.errors.saturating_add(1);
        return uefi::Status::SUCCESS;
    }
    let n = len.min(REPORT_MAX);
    let slot = s.count as usize % QUEUE;
    // SAFETY: the host controller hands `len` bytes at `data`.
    unsafe { core::ptr::copy_nonoverlapping(data as *const u8, s.buf[slot].as_mut_ptr(), n) };
    s.len[slot] = n;
    s.count = s.count.wrapping_add(1);
    s.errors = 0;
    uefi::Status::SUCCESS
}

struct Device {
    handle: Handle,
    /// Vendor and product ID.
    id: (u16, u16),
    io: ScopedProtocol<UsbIo>,
    ep_in: u8,
    shared: Box<UnsafeCell<Shared>>,
    seen: u32,
    dec: Decoder,
    /// Each pad's mapped buttons last frame, for actions on the press.
    prev: Vec<u32>,
}

impl Device {
    /// Stop the background polling (the device is still there).
    fn stop(&mut self) {
        let raw = io_raw(&mut self.io);
        // SAFETY: the protocol is open; stopping a transfer takes no buffers.
        let _ = unsafe { ((*raw).async_interrupt_transfer)(raw, self.ep_in, false.into(), 0, 0, None, core::ptr::null()) };
    }
}

/// The raw protocol behind `UsbIo`, for the asynchronous transfer the
/// wrapper does not offer.
fn io_raw(io: &mut ScopedProtocol<UsbIo>) -> *mut UsbIoProtocol {
    // `UsbIo` is a transparent wrapper around the raw protocol.
    let p: &mut UsbIo = io;
    p as *mut UsbIo as *mut UsbIoProtocol
}

/// PCI I/O, cut to what [`start_usb`] needs: reading config space.
#[repr(C)]
#[uefi::proto::unsafe_protocol("4cf5b200-68b8-4ca5-9eec-b23e3f50029a")]
struct PciIo {
    poll_mem: usize,
    poll_io: usize,
    mem_read: usize,
    mem_write: usize,
    io_read: usize,
    io_write: usize,
    /// `Pci.Read(This, Width, Offset, Count, Buffer)`.
    pci_read: unsafe extern "efiapi" fn(*mut PciIo, u32, u32, usize, *mut c_void) -> uefi::Status,
}

/// Connect every USB host controller and, recursively, the USB bus and
/// devices behind it. Firmware set to "fast boot" often leaves USB
/// unconnected when it boots something off a disk, and then there would be
/// no USB I/O handles at all (no pads, and no USB keyboard either). Where
/// the firmware already connected USB this changes nothing. Done once, at
/// start: enumerating the bus can take a moment.
pub fn start_usb() {
    let Ok(handles) = boot::locate_handle_buffer(SearchType::ByProtocol(&PciIo::GUID)) else { return };
    let mut started = 0;
    for h in handles.iter() {
        // SAFETY: only config space is read, and the protocol stays open
        // just for that.
        let class = unsafe {
            let Ok(mut pci) = boot::open_protocol::<PciIo>(
                OpenProtocolParams { handle: *h, agent: boot::image_handle(), controller: None },
                OpenProtocolAttributes::GetProtocol,
            ) else {
                continue;
            };
            let p: &mut PciIo = &mut pci;
            let mut cc = [0u8; 3];
            // Width 0 = 8-bit; 0x09..0x0B = programming interface, subclass, class.
            if (p.pci_read)(p, 0, 0x09, 3, cc.as_mut_ptr().cast()).is_error() {
                continue;
            }
            cc
        };
        // Class 0C subclass 03: a USB host controller (UHCI, OHCI, EHCI, xHCI).
        if class[2] == 0x0C && class[1] == 0x03 {
            if boot::connect_controller(*h, &[], None, true).is_ok() {
                started += 1;
            }
        }
    }
    say(format_args!("GameCoob: {started} USB host controllers connected"));
}

/// The USB gamepads.
pub struct Gamepads {
    devs: Vec<Device>,
    /// USB interfaces looked at and passed over (not controllers, or failed
    /// to start).
    skipped: Vec<Handle>,
    next_scan: f32,
}

impl Gamepads {
    pub fn new() -> Gamepads {
        Gamepads { devs: Vec::new(), skipped: Vec::new(), next_scan: 0.0 }
    }

    /// The pads at time `now` (seconds): each with the controller it plays
    /// as (an adapter's ports keep their numbers, other pads take the next
    /// controller in turn) and its buttons last frame.
    pub fn poll(&mut self, now: f32) -> Vec<(usize, Pad, u32)> {
        if now >= self.next_scan {
            self.next_scan = now + 1.0;
            self.scan();
        }
        let mut out = Vec::new();
        let mut next = 0;
        for d in &mut self.devs {
            let mut queue = [([0u8; REPORT_MAX], 0usize); QUEUE];
            let fresh;
            {
                // SAFETY: raising to TPL_NOTIFY keeps the callback out while
                // the reports are copied.
                let _tpl = unsafe { boot::raise_tpl(Tpl::NOTIFY) };
                // SAFETY: as above.
                let s = unsafe { &*d.shared.get() };
                fresh = (s.count.wrapping_sub(d.seen) as usize).min(QUEUE);
                for (k, q) in queue[..fresh].iter_mut().enumerate() {
                    let slot = s.count.wrapping_sub((fresh - k) as u32) as usize % QUEUE;
                    *q = (s.buf[slot], s.len[slot]);
                }
                d.seen = s.count;
            }
            // Every report since the last poll: the latest state, with any
            // button pressed in between held for this poll.
            let mut pads = d.dec.pads();
            for (k, (buf, n)) in queue[..fresh].iter().enumerate() {
                d.dec.feed(&buf[..*n]);
                let now = d.dec.pads();
                if k == 0 {
                    pads = now;
                } else {
                    report::hold_presses(&mut pads, now);
                }
            }
            let fixed = d.dec.fixed_ports();
            d.prev.resize(pads.len(), 0);
            for (i, p) in pads.iter().enumerate() {
                let Some(p) = p else { continue };
                let port = if fixed { i } else { next };
                if !fixed {
                    next += 1;
                }
                out.push((port & 3, *p, d.prev[i]));
                d.prev[i] = crate::input::pad_buttons(p);
            }
        }
        out
    }

    /// Drop devices that went away and start new ones.
    fn scan(&mut self) {
        let Ok(handles) = boot::locate_handle_buffer(SearchType::ByProtocol(&UsbIo::GUID)) else {
            // No USB at all (or every device gone).
            self.forget(|_| true);
            return;
        };
        let live = |h: &Handle| handles.iter().any(|x| x == h);
        self.forget(|d| !live(&d.handle));
        self.skipped.retain(|h| live(h));
        for h in handles.iter() {
            if self.devs.iter().any(|d| d.handle == *h) || self.skipped.contains(h) {
                continue;
            }
            match open(*h) {
                Some(d) => self.devs.push(d),
                None => self.skipped.push(*h),
            }
        }
        // Devices whose transfers keep failing are restarted on the next scan.
        let mut i = 0;
        while i < self.devs.len() {
            // SAFETY: read under the callback's TPL.
            let errors = {
                let _tpl = unsafe { boot::raise_tpl(Tpl::NOTIFY) };
                unsafe { (*self.devs[i].shared.get()).errors }
            };
            if errors > 8 {
                let mut d = self.devs.remove(i);
                d.stop();
                drop(d);
            } else {
                i += 1;
            }
        }
    }

    /// Drop the devices `gone` picks, which are no longer on the bus. Their
    /// protocol is gone with them, so their transfers are not stopped; the
    /// report buffer is leaked in case the host controller still calls back.
    fn forget(&mut self, gone: impl Fn(&Device) -> bool) {
        let mut i = 0;
        while i < self.devs.len() {
            if gone(&self.devs[i]) {
                let d = self.devs.remove(i);
                say(format_args!("GameCoob: USB gamepad {:04x}:{:04x} removed", d.id.0, d.id.1));
                Box::leak(d.shared);
                core::mem::forget(d.io);
            } else {
                i += 1;
            }
        }
    }
}

impl Drop for Gamepads {
    fn drop(&mut self) {
        for d in &mut self.devs {
            d.stop();
        }
    }
}

/// Open the USB interface `h` and start it if it is a controller.
fn open(h: Handle) -> Option<Device> {
    // Shared, not exclusive: no firmware driver claims a gamepad, and this
    // leaves alone the interfaces a driver did take (keyboards, disks).
    // SAFETY: the protocol is only used while the handle has it (see
    // `Gamepads::scan`).
    let mut io = unsafe {
        boot::open_protocol::<UsbIo>(
            OpenProtocolParams { handle: h, agent: boot::image_handle(), controller: None },
            OpenProtocolAttributes::GetProtocol,
        )
    }
    .ok()?;
    let dev = io.device_descriptor().ok()?;
    let ifc = io.interface_descriptor().ok()?;
    let info = InterfaceInfo {
        vendor: dev.id_vendor,
        product: dev.id_product,
        class: ifc.interface_class,
        subclass: ifc.interface_subclass,
        protocol: ifc.interface_protocol,
    };
    let (dec, start) = Decoder::probe(&info)?;

    // The interrupt endpoints.
    let (mut ep_in, mut ep_out) = (None, None);
    for i in 0..ifc.num_endpoints {
        let Ok(ep) = io.endpoint_descriptor(i) else { continue };
        if ep.attributes & 3 != 3 {
            continue;
        }
        if ep.endpoint_address & 0x80 != 0 {
            ep_in.get_or_insert(ep);
        } else {
            ep_out.get_or_insert(ep);
        }
    }
    let ep_in = ep_in?;

    let dec = match (dec, start) {
        (Some(dec), Start::None) => dec,
        (Some(dec), Start::Out(cmd)) => {
            io.sync_interrupt_send(ep_out?.endpoint_address, cmd, 100).ok()?;
            dec
        }
        (_, Start::ReportDescriptor) => {
            let desc = report_descriptor(&mut io, ifc.interface_number)?;
            let dec = Decoder::hid(&desc)?;
            // SET_IDLE 0: report only on change (some pads need it to report at all).
            let _ = io.control_transfer(0x21, 0x0A, 0, ifc.interface_number as u16, ControlTransfer::None, 100);
            dec
        }
        (None, _) => return None,
    };
    say(format_args!(
        "GameCoob: USB gamepad {:04x}:{:04x} (class {:02x}/{:02x}/{:02x})",
        info.vendor, info.product, info.class, info.subclass, info.protocol
    ));

    let shared = Box::new(UnsafeCell::new(Shared { len: [0; QUEUE], buf: [[0; REPORT_MAX]; QUEUE], count: 0, errors: 0 }));
    let len = (ep_in.max_packet_size as usize & 0x7FF).clamp(1, REPORT_MAX);
    let interval = ep_in.interval.clamp(1, 8) as usize;
    let raw = io_raw(&mut io);
    // SAFETY: `shared` lives as long as the transfer (see `on_report`).
    let st = unsafe {
        ((*raw).async_interrupt_transfer)(raw, ep_in.endpoint_address, true.into(), interval, len, Some(on_report), shared.get() as *const c_void)
    };
    if st.is_error() {
        say(format_args!("GameCoob: USB gamepad {:04x}:{:04x} did not start: {:?}", info.vendor, info.product, st));
        return None;
    }
    Some(Device { handle: h, id: (info.vendor, info.product), io, ep_in: ep_in.endpoint_address, shared, seen: 0, dec, prev: Vec::new() })
}

/// A line to the serial port (not the screen, which the menu owns).
fn say(args: core::fmt::Arguments) {
    crate::firmware::serial_write(&alloc::format!("{args}\r\n"));
}

/// The HID report descriptor of interface `ifc`, its length taken from the
/// HID descriptor in the configuration descriptor.
fn report_descriptor(io: &mut UsbIo, ifc: u8) -> Option<Vec<u8>> {
    let get = |io: &mut UsbIo, req_type: u8, value: u16, index: u16, len: usize| {
        let mut buf = alloc::vec![0u8; len];
        io.control_transfer(req_type, 0x06, value, index, ControlTransfer::DataIn(&mut buf), 500).ok()?;
        Some(buf)
    };
    let head = get(io, 0x80, 0x0200, 0, 9)?;
    let total = u16::from_le_bytes([head[2], head[3]]) as usize;
    let config = get(io, 0x80, 0x0200, 0, total.max(9))?;
    // Walk to our interface (alternate setting 0), then its HID descriptor.
    let mut i = 0;
    let mut ours = false;
    let mut len = None;
    while i + 2 <= config.len() {
        let (l, ty) = (config[i] as usize, config[i + 1]);
        if l < 2 {
            break;
        }
        match ty {
            0x04 if i + 4 <= config.len() => ours = config[i + 2] == ifc && config[i + 3] == 0,
            0x21 if ours && i + 9 <= config.len() && config[i + 6] == 0x22 => {
                len = Some(u16::from_le_bytes([config[i + 7], config[i + 8]]) as usize);
                break;
            }
            _ => {}
        }
        i += l;
    }
    get(io, 0x81, 0x2200, ifc as u16, len?.clamp(1, 4096))
}
