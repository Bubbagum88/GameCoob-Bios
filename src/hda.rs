//! A small Intel High Definition Audio output driver: finds the first HDA
//! controller on PCI, routes every analog output pin (speaker, headphone,
//! line out) of its codecs to a DAC, and plays one cyclic 48 kHz 16-bit
//! stereo stream that [`Hda::pump`] keeps filled from the main loop.
//!
//! No interrupts: the firmware owns them. The stream is a ring the
//! controller loops over forever; the main loop reads the controller's
//! position and writes ahead of it. Played-out parts of the ring are zeroed
//! again, so a late refill (a slow frame) plays silence, never stale sound.
//!
//! Register and verb numbers are from Intel's "High Definition Audio
//! Specification" rev 1.0a.

use alloc::vec::Vec;
use core::ptr::{NonNull, read_volatile, write_volatile};
use core::time::Duration;
use uefi::boot::{self, AllocateType, MemoryType, OpenProtocolAttributes, OpenProtocolParams, SearchType};
use uefi::proto::pci::PciIoAddress;
use uefi::proto::pci::root_bridge::PciRootBridgeIo;

/// Output sample rate (the stream format below is 48 kHz).
pub const RATE: u32 = 48_000;
/// Stream format: 48 kHz base, 16 bits, 2 channels.
const FMT_48K_16_STEREO: u16 = 0x0011;
/// Stream tag the DACs listen to.
const TAG: u8 = 1;

/// Ring length in stereo frames (~0.68 s) and its buffer descriptors.
const RING: usize = 32768;
const BDL_ENTRIES: usize = 4;

// Controller registers.
const GCAP: usize = 0x00;
const GCTL: usize = 0x08;
const STATESTS: usize = 0x0E;
const INTCTL: usize = 0x20;
const CORBLBASE: usize = 0x40;
const CORBUBASE: usize = 0x44;
const CORBWP: usize = 0x48;
const CORBRP: usize = 0x4A;
const CORBCTL: usize = 0x4C;
const CORBSIZE: usize = 0x4E;
const RIRBLBASE: usize = 0x50;
const RIRBUBASE: usize = 0x54;
const RIRBWP: usize = 0x58;
const RINTCNT: usize = 0x5A;
const RIRBCTL: usize = 0x5C;
const RIRBSTS: usize = 0x5D;
const RIRBSIZE: usize = 0x5E;
const ICOI: usize = 0x60;
const ICII: usize = 0x64;
const ICIS: usize = 0x68;
const DPLBASE: usize = 0x70;
// Stream descriptor registers (from the descriptor's base).
const SD_CTL: usize = 0x00;
const SD_STS: usize = 0x03;
const SD_LPIB: usize = 0x04;
const SD_CBL: usize = 0x08;
const SD_LVI: usize = 0x0C;
const SD_FMT: usize = 0x12;
const SD_BDPL: usize = 0x18;
const SD_BDPU: usize = 0x1C;

// Codec verbs and parameters.
const GET_PARAM: u32 = 0xF00;
const GET_CONN: u32 = 0xF02;
const GET_CONFIG: u32 = 0xF1C;
const SET_SELECT: u32 = 0x701;
const SET_POWER: u32 = 0x705;
const SET_STREAM: u32 = 0x706;
const SET_PIN: u32 = 0x707;
const SET_EAPD: u32 = 0x70C;
const SET_FORMAT: u32 = 0x2;
const SET_AMP: u32 = 0x3;
const P_NODES: u32 = 0x04;
const P_FG_TYPE: u32 = 0x05;
const P_WIDGET_CAPS: u32 = 0x09;
const P_PIN_CAPS: u32 = 0x0C;
const P_IN_AMP: u32 = 0x0D;
const P_CONN_LEN: u32 = 0x0E;
const P_OUT_AMP: u32 = 0x12;

// Widget types (widget caps bits 23:20).
const W_OUT: u32 = 0;
const W_MIXER: u32 = 2;
const W_PIN: u32 = 4;

/// One buffer descriptor list entry.
#[repr(C)]
struct Bdle {
    addr: u64,
    len: u32,
    flags: u32,
}

fn stall_us(us: u64) {
    boot::stall(Duration::from_micros(us));
}

/// Page-aligned DMA memory below 4 GiB (some controllers lack 64-bit DMA),
/// zeroed. UEFI maps memory 1:1, so the pointer is the bus address.
fn dma_alloc(bytes: usize) -> Option<NonNull<u8>> {
    let pages = bytes.div_ceil(4096);
    let p = boot::allocate_pages(AllocateType::MaxAddress(0xFFFF_F000), MemoryType::LOADER_DATA, pages).ok()?;
    // SAFETY: freshly allocated, `pages * 4096` bytes.
    unsafe { core::ptr::write_bytes(p.as_ptr(), 0, pages * 4096) };
    Some(p)
}

/// How codec commands are sent.
enum Cmd {
    /// The command ring (CORB) and response ring (RIRB).
    Rings { corb: *mut u32, rirb: *mut u64, corb_n: u16, rirb_n: u16, rirb_rp: u16 },
    /// The optional immediate command registers.
    Immediate,
}

pub struct Hda {
    mmio: *mut u8,
    sd: usize,
    cmd: Cmd,
    ring: *mut i16,
    /// Frames played and written so far (absolute, wrapping).
    played: u64,
    written: u64,
    last_lpib: u32,
    /// How far ahead of the play position to keep the ring filled, in frames.
    ahead: u64,
    /// Main-loop gap estimate (frames), for `ahead`.
    gap: u64,
    scratch: Vec<i16>,
}

impl Hda {
    fn r8(&self, r: usize) -> u8 {
        unsafe { read_volatile(self.mmio.add(r)) }
    }
    fn r16(&self, r: usize) -> u16 {
        unsafe { read_volatile(self.mmio.add(r) as *const u16) }
    }
    fn r32(&self, r: usize) -> u32 {
        unsafe { read_volatile(self.mmio.add(r) as *const u32) }
    }
    fn w8(&self, r: usize, v: u8) {
        unsafe { write_volatile(self.mmio.add(r), v) }
    }
    fn w16(&self, r: usize, v: u16) {
        unsafe { write_volatile(self.mmio.add(r) as *mut u16, v) }
    }
    fn w32(&self, r: usize, v: u32) {
        unsafe { write_volatile(self.mmio.add(r) as *mut u32, v) }
    }

    /// Wait up to `ms` for `f` to hold.
    fn wait(&self, ms: u64, f: impl Fn(&Hda) -> bool) -> bool {
        for _ in 0..ms * 20 {
            if f(self) {
                return true;
            }
            stall_us(50);
        }
        f(self)
    }

    /// Find, reset and set up the first HDA controller and its codecs, and
    /// start the (silent) output stream.
    pub fn open() -> Option<Hda> {
        let mmio = find_controller()?;
        let mut h = Hda {
            mmio,
            sd: 0,
            cmd: Cmd::Immediate,
            ring: core::ptr::null_mut(),
            played: 0,
            written: 0,
            last_lpib: 0,
            ahead: (RATE / 20) as u64,
            gap: 0,
            scratch: Vec::new(),
        };
        h.reset()?;
        let codecs = h.r16(STATESTS) & 0x7FFF;
        log::info!("HDA: controller at {:p}, codecs {codecs:#x}", mmio);
        h.setup_rings();
        let mut routed = 0;
        for cad in 0..15u32 {
            if codecs & (1 << cad) != 0 {
                routed += h.setup_codec(cad);
            }
        }
        if routed == 0 {
            log::warn!("HDA: no output pin could be routed to a DAC");
            return None;
        }
        h.start_stream()?;
        log::info!("HDA: {routed} output pin(s), 48 kHz stereo stream running");
        Some(h)
    }

    fn reset(&mut self) -> Option<()> {
        // Stop the rings and every stream the firmware may have left running.
        self.w8(CORBCTL, 0);
        self.w8(RIRBCTL, 0);
        let gcap = self.r16(GCAP);
        let streams = ((gcap >> 8) & 15) + ((gcap >> 12) & 15) + ((gcap >> 3) & 31);
        for s in 0..streams as usize {
            self.w8(0x80 + s * 0x20 + SD_CTL, 0);
        }
        self.w32(DPLBASE, 0);
        self.w32(INTCTL, 0);
        self.w32(GCTL, self.r32(GCTL) & !1);
        if !self.wait(100, |h| h.r32(GCTL) & 1 == 0) {
            log::warn!("HDA: controller did not enter reset");
            return None;
        }
        stall_us(200);
        self.w32(GCTL, self.r32(GCTL) | 1);
        if !self.wait(100, |h| h.r32(GCTL) & 1 == 1) {
            log::warn!("HDA: controller did not leave reset");
            return None;
        }
        // Codecs need 521 us after reset to ask for an address.
        stall_us(1000);
        self.wait(50, |h| h.r16(STATESTS) & 0x7FFF != 0);
        (self.r16(STATESTS) & 0x7FFF != 0).then_some(())
    }

    /// Ring size register value and entry count: the largest size the
    /// capability bits (7:4) offer.
    fn ring_size(cap: u8) -> (u8, u16) {
        if cap & 0x40 != 0 {
            (2, 256)
        } else if cap & 0x20 != 0 {
            (1, 16)
        } else {
            (0, 2)
        }
    }

    /// Set up CORB/RIRB; keep the immediate interface if they don't work.
    fn setup_rings(&mut self) {
        let Some(mem) = dma_alloc(4096) else { return };
        let corb = mem.as_ptr() as *mut u32;
        // The RIRB gets its own allocation (256 entries of 8 bytes).
        let Some(rmem) = dma_alloc(2048) else { return };
        let rirb = rmem.as_ptr() as *mut u64;
        let (cs, corb_n) = Self::ring_size(self.r8(CORBSIZE));
        let (rs, rirb_n) = Self::ring_size(self.r8(RIRBSIZE));
        self.w8(CORBSIZE, (self.r8(CORBSIZE) & !3) | cs);
        self.w32(CORBLBASE, corb as u64 as u32);
        self.w32(CORBUBASE, (corb as u64 >> 32) as u32);
        self.w16(CORBWP, 0);
        // Reset the read pointer: set bit 15, see it set, clear it.
        self.w16(CORBRP, 0x8000);
        self.wait(10, |h| h.r16(CORBRP) & 0x8000 != 0);
        self.w16(CORBRP, 0);
        self.wait(10, |h| h.r16(CORBRP) & 0x8000 == 0);
        self.w8(RIRBSIZE, (self.r8(RIRBSIZE) & !3) | rs);
        self.w32(RIRBLBASE, rirb as u64 as u32);
        self.w32(RIRBUBASE, (rirb as u64 >> 32) as u32);
        self.w16(RIRBWP, 0x8000);
        // Some controllers (QEMU's among them) hold further responses once
        // this many arrived until RIRBSTS is cleared, so clear it per reply.
        self.w16(RINTCNT, 0xFF);
        self.w8(CORBCTL, 2);
        self.w8(RIRBCTL, 2);
        self.cmd = Cmd::Rings { corb, rirb, corb_n, rirb_n, rirb_rp: 0 };
        // Check they work with a harmless query of codec 0's root node.
        let codec = (self.r16(STATESTS) & 0x7FFF).trailing_zeros();
        if self.verb(codec, 0, GET_PARAM, 0).is_none() {
            log::info!("HDA: command rings unresponsive, using immediate commands");
            self.w8(CORBCTL, 0);
            self.w8(RIRBCTL, 0);
            self.cmd = Cmd::Immediate;
        }
    }

    /// Send one 12-bit verb with an 8-bit payload; returns the response.
    fn verb(&mut self, cad: u32, nid: u32, verb: u32, payload: u32) -> Option<u32> {
        self.command(cad << 28 | (nid & 0x7F) << 20 | (verb & 0xFFF) << 8 | (payload & 0xFF))
    }

    /// Send one 4-bit verb with a 16-bit payload.
    fn verb4(&mut self, cad: u32, nid: u32, verb: u32, payload: u32) -> Option<u32> {
        self.command(cad << 28 | (nid & 0x7F) << 20 | (verb & 0xF) << 16 | (payload & 0xFFFF))
    }

    fn command(&mut self, c: u32) -> Option<u32> {
        match self.cmd {
            Cmd::Rings { corb, rirb, corb_n, rirb_n, rirb_rp } => {
                let wp = (self.r16(CORBWP) & 0xFF).wrapping_add(1) % corb_n;
                unsafe { write_volatile(corb.add(wp as usize), c) };
                self.w16(CORBWP, wp);
                let mut rp = rirb_rp;
                for _ in 0..2000 {
                    let hw = self.r16(RIRBWP) & 0xFF;
                    while rp != hw {
                        rp = (rp + 1) % rirb_n;
                        let e = unsafe { read_volatile(rirb.add(rp as usize)) };
                        // Skip unsolicited responses (bit 4 of the extended word).
                        self.w8(RIRBSTS, 5);
                        if (e >> 32) & 0x10 == 0 {
                            self.cmd = Cmd::Rings { corb, rirb, corb_n, rirb_n, rirb_rp: rp };
                            return Some(e as u32);
                        }
                    }
                    stall_us(10);
                }
                self.cmd = Cmd::Rings { corb, rirb, corb_n, rirb_n, rirb_rp: rp };
                None
            }
            Cmd::Immediate => {
                if !self.wait(10, |h| h.r16(ICIS) & 1 == 0) {
                    return None;
                }
                self.w16(ICIS, 2);
                self.w32(ICOI, c);
                self.w16(ICIS, 1);
                if !self.wait(10, |h| h.r16(ICIS) & 3 == 2) {
                    return None;
                }
                Some(self.r32(ICII))
            }
        }
    }

    fn param(&mut self, cad: u32, nid: u32, p: u32) -> u32 {
        self.verb(cad, nid, GET_PARAM, p).unwrap_or(0)
    }

    /// The connection list of a widget (node ids it can take input from).
    fn connections(&mut self, cad: u32, nid: u32) -> Vec<u32> {
        let l = self.param(cad, nid, P_CONN_LEN);
        let (long, n) = (l & 0x80 != 0, l & 0x7F);
        let (per, bits) = if long { (2, 16) } else { (4, 8) };
        let mut out: Vec<u32> = Vec::new();
        let mut i = 0;
        while i < n {
            let r = self.verb(cad, nid, GET_CONN, i).unwrap_or(0);
            for k in 0..per.min(n - i) {
                let e = (r >> (k * bits)) & ((1 << bits) - 1);
                let (range, id) = (e >> (bits - 1) & 1 != 0, e & ((1 << (bits - 1)) - 1));
                match (range, out.last()) {
                    (true, Some(&prev)) => out.extend(prev + 1..=id),
                    _ => out.push(id),
                }
            }
            i += per;
        }
        out
    }

    /// Route every usable output pin of codec `cad` to a DAC on stream
    /// [`TAG`]; returns the pins routed.
    fn setup_codec(&mut self, cad: u32) -> u32 {
        let root = self.param(cad, 0, P_NODES);
        let mut routed = 0;
        for fg in (root >> 16 & 0xFF)..(root >> 16 & 0xFF) + (root & 0xFF) {
            if self.param(cad, fg, P_FG_TYPE) & 0xFF != 1 {
                continue;
            }
            self.verb(cad, fg, SET_POWER, 0);
            stall_us(10_000);
            let afg_out_amp = self.param(cad, fg, P_OUT_AMP);
            let afg_in_amp = self.param(cad, fg, P_IN_AMP);
            let nodes = self.param(cad, fg, P_NODES);
            let first = nodes >> 16 & 0xFF;
            let widgets: Vec<(u32, u32)> =
                (first..first + (nodes & 0xFF)).map(|n| (n, self.param(cad, n, P_WIDGET_CAPS))).collect();
            let caps = |n: u32| widgets.iter().find(|w| w.0 == n).map_or(0, |w| w.1);
            let mut pins = Vec::new();
            for &(n, c) in &widgets {
                if c >> 20 & 15 != W_PIN || self.param(cad, n, P_PIN_CAPS) & 0x10 == 0 {
                    continue;
                }
                let cfg = self.verb(cad, n, GET_CONFIG, 0).unwrap_or(0);
                let dev = cfg >> 20 & 15;
                // Port not connected, or a digital/modem/input device.
                if cfg >> 30 == 1 || !(dev <= 2 || cfg == 0) {
                    continue;
                }
                pins.push((n, dev));
            }
            for (pin, dev) in pins {
                let mut path = Vec::new();
                if !self.find_dac(cad, pin, &caps, &mut path, 0) {
                    continue;
                }
                for (k, &n) in path.iter().enumerate() {
                    let c = caps(n);
                    self.verb(cad, n, SET_POWER, 0);
                    let next = path.get(k + 1).copied();
                    let conns = if next.is_some() { self.connections(cad, n) } else { Vec::new() };
                    let idx = next.and_then(|x| conns.iter().position(|&y| y == x)).unwrap_or(0) as u32;
                    let ty = c >> 20 & 15;
                    if next.is_some() && conns.len() > 1 && ty != W_MIXER {
                        self.verb(cad, n, SET_SELECT, idx);
                    }
                    // Unmute at 0 dB: output amp, and the used mixer input.
                    if c & 4 != 0 {
                        let a = if c & 8 != 0 { self.param(cad, n, P_OUT_AMP) } else { afg_out_amp };
                        self.verb4(cad, n, SET_AMP, 0xB000 | (a & 0x7F));
                    }
                    if c & 2 != 0 && ty == W_MIXER {
                        let a = if c & 8 != 0 { self.param(cad, n, P_IN_AMP) } else { afg_in_amp };
                        self.verb4(cad, n, SET_AMP, 0x7000 | idx << 8 | (a & 0x7F));
                    }
                    if ty == W_OUT {
                        self.verb(cad, n, SET_STREAM, (TAG as u32) << 4);
                        self.verb4(cad, n, SET_FORMAT, FMT_48K_16_STEREO as u32);
                    }
                }
                // Pin out enable (plus headphone amp for headphones), EAPD on.
                self.verb(cad, pin, SET_PIN, if dev == 2 { 0xC0 } else { 0x40 });
                if self.param(cad, pin, P_PIN_CAPS) & 0x10000 != 0 {
                    self.verb(cad, pin, SET_EAPD, 2);
                }
                routed += 1;
            }
        }
        routed
    }

    /// Depth-first search from `n` along connections to an audio output
    /// converter; `path` gets the nodes from `n` to the DAC.
    fn find_dac(&mut self, cad: u32, n: u32, caps: &dyn Fn(u32) -> u32, path: &mut Vec<u32>, depth: u32) -> bool {
        if depth > 5 || path.contains(&n) {
            return false;
        }
        path.push(n);
        let c = caps(n);
        if c >> 20 & 15 == W_OUT {
            return true;
        }
        // Input-only widgets (selectors feeding ADCs etc.) still connect on.
        for m in self.connections(cad, n) {
            if self.find_dac(cad, m, caps, path, depth + 1) {
                return true;
            }
        }
        path.pop();
        false
    }

    fn start_stream(&mut self) -> Option<()> {
        let gcap = self.r16(GCAP);
        if (gcap >> 12) & 15 == 0 {
            log::warn!("HDA: controller has no output streams");
            return None;
        }
        // Output descriptors follow the input ones.
        self.sd = 0x80 + ((gcap >> 8) & 15) as usize * 0x20;
        let bytes = RING * 4;
        let ring = dma_alloc(bytes)?;
        let bdl = dma_alloc(BDL_ENTRIES * 16)?.as_ptr() as *mut Bdle;
        let part = bytes / BDL_ENTRIES;
        for i in 0..BDL_ENTRIES {
            let e = Bdle { addr: ring.as_ptr() as u64 + (i * part) as u64, len: part as u32, flags: 0 };
            unsafe { write_volatile(bdl.add(i), e) };
        }
        self.ring = ring.as_ptr() as *mut i16;
        let sd = self.sd;
        self.w8(sd + SD_CTL, 1);
        self.wait(10, |h| h.r8(sd + SD_CTL) & 1 == 1);
        self.w8(sd + SD_CTL, 0);
        self.wait(10, |h| h.r8(sd + SD_CTL) & 1 == 0);
        self.w32(sd + SD_BDPL, bdl as u64 as u32);
        self.w32(sd + SD_BDPU, (bdl as u64 >> 32) as u32);
        self.w32(sd + SD_CBL, bytes as u32);
        self.w16(sd + SD_LVI, (BDL_ENTRIES - 1) as u16);
        self.w16(sd + SD_FMT, FMT_48K_16_STEREO);
        self.w8(sd + SD_CTL + 2, TAG << 4);
        self.w8(sd + SD_STS, 0x1C);
        self.w8(sd + SD_CTL, 2);
        Some(())
    }

    /// Frames the controller has played since the last call.
    fn advance(&mut self) -> u64 {
        let lpib = self.r32(self.sd + SD_LPIB) % (RING as u32 * 4);
        let d = (lpib + RING as u32 * 4 - self.last_lpib) % (RING as u32 * 4);
        self.last_lpib = lpib;
        (d / 4) as u64
    }

    /// Keep the ring filled ahead of the play position; `fill` renders
    /// interleaved stereo frames. Call as often as possible (every pass of
    /// the main loop).
    pub fn pump(&mut self, fill: &mut dyn FnMut(&mut [i16])) {
        let d = self.advance();
        let from = self.played;
        self.played += d;
        // Zero what was played, so an underrun plays silence.
        self.zero(from, self.played);
        // Track the longest recent gap between calls (decaying) and keep
        // that much plus margin buffered, from 20 ms up to most of the ring.
        self.gap = (self.gap * 15 / 16).max(d);
        self.ahead = (self.gap * 3 / 2 + (RATE / 50) as u64).clamp((RATE / 50) as u64, RING as u64 * 3 / 4);
        if self.written < self.played {
            // Underrun: the controller ran past what was written.
            self.written = self.played;
        }
        let target = self.played + self.ahead;
        while self.written < target {
            let at = (self.written % RING as u64) as usize;
            let n = ((target - self.written) as usize).min(RING - at);
            self.scratch.resize(n * 2, 0);
            fill(&mut self.scratch);
            // SAFETY: `at + n <= RING`, the ring is RING stereo frames.
            unsafe { core::ptr::copy_nonoverlapping(self.scratch.as_ptr(), self.ring.add(at * 2), n * 2) };
            self.written += n as u64;
        }
    }

    fn zero(&mut self, from: u64, to: u64) {
        let mut f = from.max(to.saturating_sub(RING as u64));
        while f < to {
            let at = (f % RING as u64) as usize;
            let n = ((to - f) as usize).min(RING - at);
            unsafe { core::ptr::write_bytes(self.ring.add(at * 2), 0, n * 2) };
            f += n as u64;
        }
    }

    /// Stop the stream and put the controller back in reset, so the next
    /// boot stage (an OS driver) starts clean.
    pub fn close(&mut self) {
        self.w8(self.sd + SD_CTL, 0);
        self.w8(CORBCTL, 0);
        self.w8(RIRBCTL, 0);
        self.w32(GCTL, self.r32(GCTL) & !1);
    }
}

/// The MMIO base of the first HDA controller (PCI class 04, subclass 03),
/// with memory decoding and bus mastering turned on.
fn find_controller() -> Option<*mut u8> {
    let handles = boot::locate_handle_buffer(SearchType::from_proto::<PciRootBridgeIo>()).ok()?;
    for &h in handles.iter() {
        // SAFETY: shared (non-exclusive) access, as the PCI bus driver also
        // uses it; only configuration space is touched.
        let params = OpenProtocolParams { handle: h, agent: boot::image_handle(), controller: None };
        let Ok(mut rb) = (unsafe { boot::open_protocol::<PciRootBridgeIo>(params, OpenProtocolAttributes::GetProtocol) })
        else {
            continue;
        };
        let Ok(tree) = rb.enumerate() else { continue };
        for &a in tree.iter() {
            let pci = rb.pci();
            let class: u32 = pci.read_one(a.with_register(0x08)).unwrap_or(0);
            if class >> 16 != 0x0403 {
                continue;
            }
            let id: u32 = pci.read_one(a.with_register(0x00)).unwrap_or(0);
            let bar0: u32 = pci.read_one(a.with_register(0x10)).unwrap_or(0);
            if bar0 & 1 != 0 {
                continue;
            }
            let mut base = (bar0 & !0xF) as u64;
            if bar0 >> 1 & 3 == 2 {
                let hi: u32 = pci.read_one(a.with_register(0x14)).unwrap_or(0);
                base |= (hi as u64) << 32;
            }
            if base == 0 {
                continue;
            }
            let cmd: u16 = pci.read_one(a.with_register(0x04)).unwrap_or(0);
            let _ = pci.write_one::<u16>(a.with_register(0x04), cmd | 0x6);
            // Intel controllers: traffic class 0 (TCSEL), as Linux does.
            if id & 0xFFFF == 0x8086 {
                let t: u8 = pci.read_one(a.with_register(0x44)).unwrap_or(0);
                let _ = pci.write_one::<u8>(a.with_register(0x44), t & !7);
            }
            let PciIoAddress { bus, dev, fun, .. } = a;
            log::info!("HDA: {:04x}:{:04x} at {bus:02x}:{dev:02x}.{fun}", id & 0xFFFF, id >> 16);
            return Some(base as *mut u8);
        }
    }
    None
}
