//! Plays the real GameCube intro, rebuilt from BS2, when the user's own
//! `ipl.bin` is on the boot partition, and runs on into the rebuilt cube
//! menu. Without a usable dump, `main` explains where to put it.

use crate::bs2::bs2main::{Bs2Main, FrameInput};
use crate::bs2::menu::backend::{FakeCards, FakeDrive, MemCards};
use crate::bs2::menu::card_image::ImageCards;
use crate::bs2::menu::volumes::{self, Volumes};
use crate::fs::UefiVolumes;
use crate::bs2::menu::{Action, MenuAssets, SysInfo};
use alloc::boxed::Box;
use crate::bs2::{Assets, raster::{Job, Target, Work}, render};
use crate::bs2::pad::{PAD_ERR_NONE, PadStatus};
use crate::gamepad::Gamepads;
use crate::input::{self, AppAction, KeyCode, Keyboard};
use crate::clock::Clock;
use crate::gfx::{Canvas, ResScale};
use crate::ipl::Ipl;
use alloc::string::{String, ToString};
use alloc::vec::Vec;
use core::ffi::c_void;
use core::sync::atomic::{AtomicPtr, AtomicU32, Ordering};
use uefi::boot::{self, EventType, ScopedProtocol, Tpl};
use uefi::proto::pi::mp::MpServices;
use uefi::Event;
use uefi::fs::FileSystem;
use uefi::proto::console::gop::GraphicsOutput;
use uefi::proto::console::text::{Key, ScanCode};
use uefi::{cstr16, system};

/// Where the dump is looked for on the partition GameCoob booted from.
const PATHS: [&uefi::CStr16; 2] = [cstr16!("\\EFI\\GameCoob\\ipl.bin"), cstr16!("\\ipl.bin")];

fn read_dump() -> Option<Vec<u8>> {
    let sfs = boot::get_image_file_system(boot::image_handle()).ok()?;
    let mut fs = FileSystem::new(sfs);
    PATHS.iter().find_map(|p| fs.read(*p).ok())
}

/// Default card images for slots A and B, on the partition GameCoob booted
/// from. The first name found per slot is used.
const CARD_PATHS: [[&uefi::CStr16; 3]; 2] = [
    [cstr16!("\\EFI\\GameCoob\\cards\\slot-a.raw"), cstr16!("\\EFI\\GameCoob\\cards\\slot-a.mcp"), cstr16!("\\EFI\\GameCoob\\cards\\slot-a.gcp")],
    [cstr16!("\\EFI\\GameCoob\\cards\\slot-b.raw"), cstr16!("\\EFI\\GameCoob\\cards\\slot-b.mcp"), cstr16!("\\EFI\\GameCoob\\cards\\slot-b.gcp")],
];

/// Each slot gets the image it held at the end of the last boot (Card
/// Files remembers it), else the default image on the boot partition.
/// A slot left empty stays empty. Real cards if any slot was set this way,
/// else the made-up stand-ins. Changed cards are written back by the menu.
fn load_cards() -> Box<dyn MemCards> {
    let mut real = false;
    let mut cards = ImageCards::new();
    let id = UefiVolumes::boot_id().unwrap_or_default();
    let mut vols = UefiVolumes;
    for (slot, paths) in CARD_PATHS.iter().enumerate() {
        let name = ["A", "B"][slot];
        match vols.slot_memory(slot) {
            Some(h) if h.is_empty() => {
                real = true;
                continue;
            }
            Some(h) => {
                let (vid, path) = volumes::split_handle(&h);
                if let Ok(b) = vols.read(vid, path) {
                    log::info!("memory card in slot {name}: {path} (remembered)");
                    cards.insert(slot, h.clone(), b);
                    real = true;
                    continue;
                }
                log::warn!("slot {name}: remembered card {path} not found, using the default");
            }
            None => {}
        }
        for p in paths {
            let p = p.to_string();
            if let Ok(b) = vols.read(&id, &p) {
                log::info!("memory card in slot {name}: {p}");
                cards.insert(slot, volumes::handle(&id, &p), b);
                real = true;
                break;
            }
        }
    }
    if real { Box::new(cards) } else { Box::new(FakeCards::new()) }
}

/// Load the dump and its intro and menu assets, if present and valid.
pub fn load() -> Option<(Assets, MenuAssets)> {
    let rom = read_dump()?;
    let ipl = match Ipl::parse(&rom) {
        Ok(i) => i,
        Err(e) => {
            log::warn!("ipl.bin found but not usable: {e:?}");
            return None;
        }
    };
    let Some(a) = Assets::load(&ipl) else {
        log::warn!("ipl.bin parsed but the intro assets are missing");
        return None;
    };
    let Some(m) = MenuAssets::load(&ipl) else {
        log::warn!("ipl.bin parsed but the menu assets are missing");
        return None;
    };
    Some((a, m))
}

/// What the menu shows about this PC.
fn sys_info() -> SysInfo {
    SysInfo {
        time: now_time(),
        entries: crate::vars::boot_entries().into_iter().map(|e| (e.num, e.desc)).collect(),
        timeout: crate::vars::timeout().unwrap_or(5).min(crate::firmware::MAX_TIMEOUT),
        vendor: system::firmware_vendor().to_string(),
        fw_ui: crate::vars::firmware_ui_supported(),
    }
}

fn now_time() -> [i32; 6] {
    match uefi::runtime::get_time() {
        Ok(t) => [t.year() as i32, t.month() as i32, t.day() as i32, t.hour() as i32, t.minute() as i32, t.second() as i32],
        Err(_) => [2001, 9, 14, 0, 0, 0],
    }
}

/// Carry out a menu action; the ones that reset only come back on failure.
fn perform(a: Action) -> Option<String> {
    use crate::firmware;
    match a {
        Action::Boot(n) => Some(firmware::boot_entry(n)),
        Action::SetTimeout(s) => firmware::save_timeout(s).err(),
        Action::SetMinimal(on) => firmware::save_minimal(on).err(),
        Action::SetTime(t) => firmware::set_time(t).err(),
        Action::FirmwareSetup => Some(firmware::firmware_setup()),
        Action::Restart => firmware::restart(),
        Action::PowerOff => firmware::power_off(),
    }
}

/// The other processors, used to rasterize bands of each frame in parallel.
/// They only run pure computation on memory the frame owns: no firmware calls
/// and no allocation happen on them.
///
/// They are started once and then wait for work by watching `GEN`, rather
/// than being started again for every frame: waking a processor through the
/// firmware costs interrupts, which are slow in a VM.
struct Cores {
    aps: u32,
    // Kept alive while the processors run; they never return.
    _mp: ScopedProtocol<MpServices>,
    _done: Event,
}

/// Bumped once per frame to hand `JOB` to the waiting processors.
static GEN: AtomicU32 = AtomicU32::new(0);
/// The current work, a `*const &dyn Work`, valid while `BUSY` is nonzero.
static JOB: AtomicPtr<c_void> = AtomicPtr::new(core::ptr::null_mut());
/// Processors still working on the current frame.
static BUSY: AtomicU32 = AtomicU32::new(0);
/// Pieces of the current work the other processors did.
static AP_DID: AtomicU32 = AtomicU32::new(0);

extern "efiapi" fn ap_loop(_: *mut c_void) {
    let mut seen = 0;
    loop {
        let g = GEN.load(Ordering::Acquire);
        if g == seen {
            core::hint::spin_loop();
            continue;
        }
        seen = g;
        // SAFETY: the BSP keeps the work alive until `BUSY` drops to zero.
        let w = unsafe { *(JOB.load(Ordering::Acquire) as *const &dyn Work) };
        AP_DID.fetch_add(w.work() as u32, Ordering::Relaxed);
        BUSY.fetch_sub(1, Ordering::AcqRel);
    }
}

impl Cores {
    fn open() -> Option<Cores> {
        let h = boot::get_handle_for_protocol::<MpServices>().ok()?;
        let mp = boot::open_protocol_exclusive::<MpServices>(h).ok()?;
        let n = mp.get_number_of_processors().ok()?;
        if n.enabled < 2 {
            return None;
        }
        // SAFETY: a plain event with no notification function.
        let done = unsafe { boot::create_event(EventType::empty(), Tpl::CALLBACK, None, None) }.ok()?;
        // SAFETY: the event handle stays owned by `Cores`.
        let ev = unsafe { done.unsafe_clone() };
        // Non-blocking (an event is given), so the loops keep running.
        mp.startup_all_aps(false, ap_loop, core::ptr::null_mut(), Some(ev), None).ok()?;
        Some(Cores { aps: n.enabled as u32 - 1, _mp: mp, _done: done })
    }

    /// Run `w` on every processor; returns once all are idle again, with
    /// how many pieces the others did.
    fn run(&self, w: &dyn Work) -> u32 {
        AP_DID.store(0, Ordering::Relaxed);
        BUSY.store(self.aps, Ordering::Release);
        JOB.store(&w as *const &dyn Work as *mut c_void, Ordering::Release);
        GEN.fetch_add(1, Ordering::AcqRel);
        let _ = w.work();
        while BUSY.load(Ordering::Acquire) != 0 {
            core::hint::spin_loop();
        }
        AP_DID.load(Ordering::Relaxed)
    }
}

/// Run [`Bs2Main`] at 60 logic frames per second: the intro, then the cube
/// menu. Never returns: the menu's actions boot or reset.
///
/// Keys go through [`input::DEFAULT_KEYS`]: the arrows are the d-pad, Enter
/// or A is A (hold to spin the cube), Escape or Backspace is B, Space or S is
/// START, X / Y / Q / E are X, Y, L and R, and z (shift-Z: all four
/// controllers) held at boot picks an easter-egg jingle. Escape also skips
/// the intro. F10 opens the system dialog (firmware setup, restart, power
/// off); D opens and closes the stand-in disc drive's tray, 1 / 2 pull,
/// format-break and re-insert the stand-in memory cards (or pull and put
/// back the card images), R is the
/// console's reset switch and F3 shows frame timings.
///
/// USB gamepads ([`crate::gamepad`]) play alongside: GameCube controllers
/// on an adapter as themselves, other pads through
/// [`input::DEFAULT_PAD`].
pub fn play(gop: &mut GraphicsOutput, cv: &mut Canvas, clock: &Clock, assets: &Assets, ma: &MenuAssets) -> ! {
    crate::bs2::menu::set_minimal(crate::vars::minimal());
    let mut bs2 = Bs2Main::new(assets, Box::new(FakeDrive::new()), load_cards(), Box::new(UefiVolumes));
    let cores = Cores::open();
    // Rendered at the screen's resolution, adjusted to keep 60 fps.
    let mut res = ResScale::new(cv);
    let mut stats = Stats::default();
    // Frame timings go to the first serial port: always under OVMF (QEMU),
    // after F3 elsewhere.
    let log_stats = system::firmware_vendor().to_string().contains("EDK II");
    let mut show_stats = false;
    let mut target = Target::new(render::EFB_W, render::EFB_H);
    let t0 = clock.now();
    let mut done_frames = 0u32;
    let mut keys = Keyboard::new(input::DEFAULT_KEYS);
    let mut gamepads = Gamepads::new();
    let mut clock_at = 0.0;

    loop {
        let now = clock.now() - t0;
        while let Ok(Some(key)) = system::with_stdin(|i| i.read_key()) {
            let Some(k) = key_code(key) else { continue };
            keys.press(k, now);
            // The easter eggs: Z pressed while the cube falls stays held
            // until the menu, so the jingle can see it.
            if matches!(k, KeyCode::Char('z' | 'Z')) && bs2.menu.is_none() && bs2.intro.frame < 30 {
                keys.latch(k);
            }
        }
        // Catch logic up to real time, drawing only the latest frame.
        let due = (now * 60.0) as u32;
        // The gamepads, read when logic frames are due: their buttons hold
        // for each of those frames, their actions fire once.
        let mut gp = [PadStatus::NONE; 4];
        let mut gp_actions = Vec::new();
        if done_frames < due {
            for (port, pad, prev) in gamepads.poll(now) {
                input::add_pad(&mut gp, &mut gp_actions, input::DEFAULT_PAD, port, &pad, prev);
            }
        }
        let mut stepped = false;
        while done_frames < due {
            done_frames += 1;
            stepped = true;
            let mut pads = keys.pads(now);
            for (p, g) in pads.iter_mut().zip(&gp) {
                if g.err == PAD_ERR_NONE {
                    input::merge(p, g);
                }
            }
            let mut input = FrameInput { pads, ..FrameInput::default() };
            let mut actions = keys.take_actions();
            actions.append(&mut gp_actions);
            for a in actions {
                match a {
                    AppAction::System => input.system = true,
                    AppAction::Eject => input.eject = true,
                    AppAction::Slot(s) => input.slot[s as usize & 1] = true,
                    AppAction::Reset => input.reset = true,
                    AppAction::SkipIntro if bs2.menu.is_none() => bs2.skip_intro(),
                    AppAction::SkipIntro => {}
                    AppAction::Stats => show_stats = !show_stats,
                }
            }
            if let Some(act) = bs2.update(assets, ma, &input, sys_info) {
                if let Some(msg) = perform(act) {
                    if let Some(m) = &mut bs2.menu {
                        m.notify(msg);
                    }
                }
            }
            if bs2.menu.is_some() {
                keys.unlatch();
            }
        }
        if let Some(m) = &mut bs2.menu {
            if now - clock_at > 0.25 {
                clock_at = now;
                m.info.time = now_time();
            }
        }
        if !stepped {
            continue;
        }
        let run = |w: &dyn Work| match &cores {
            Some(c) => c.run(w),
            None => {
                let _ = w.work();
                0
            }
        };
        let par = |job: &Job| stats.ap_bands.set(stats.ap_bands.get() + run(job));
        let (w, h) = res.size(cv);
        if (w, h) != (target.w, target.h) {
            target = Target::new(w, h);
        }
        let d0 = clock.now();
        bs2.render(&mut target, assets, ma, &par);
        let d1 = clock.now();
        cv.blit_frame(&target.color, target.w, target.h, &|w| {
            run(w);
        });
        let d2 = clock.now();
        cv.present(gop);
        let d3 = clock.now();
        res.frame(d3 - d0);
        if stats.add(d3, [d1 - d0, d2 - d1, d3 - d2]) && (log_stats || show_stats) {
            stats.report(&target, cv, cores.as_ref().map_or(1, |c| c.aps + 1));
        }
    }
}


/// The firmware's key as a [`KeyCode`].
fn key_code(key: Key) -> Option<KeyCode> {
    Some(match key {
        Key::Printable(c) => KeyCode::Char(char::from(c)),
        Key::Special(ScanCode::UP) => KeyCode::Up,
        Key::Special(ScanCode::DOWN) => KeyCode::Down,
        Key::Special(ScanCode::LEFT) => KeyCode::Left,
        Key::Special(ScanCode::RIGHT) => KeyCode::Right,
        Key::Special(ScanCode::ESCAPE) => KeyCode::Esc,
        Key::Special(s) if (ScanCode::FUNCTION_1.0..=ScanCode::FUNCTION_10.0).contains(&s.0) => {
            KeyCode::F((s.0 - ScanCode::FUNCTION_1.0 + 1) as u8)
        }
        Key::Special(ScanCode::FUNCTION_11) => KeyCode::F(11),
        Key::Special(ScanCode::FUNCTION_12) => KeyCode::F(12),
        _ => return None,
    })
}

/// Average frame timings, reported every couple of seconds.
#[derive(Default)]
struct Stats {
    since: f32,
    now: f32,
    frames: u32,
    sum: [f32; 3],
    /// Raster bands the other processors did.
    ap_bands: core::cell::Cell<u32>,
}

impl Stats {
    /// Add one frame's (render, scale, present) seconds; true when a
    /// report is due.
    fn add(&mut self, now: f32, t: [f32; 3]) -> bool {
        for (s, t) in self.sum.iter_mut().zip(t) {
            *s += t;
        }
        self.frames += 1;
        self.now = now;
        now - self.since >= 2.0
    }

    fn report(&mut self, t: &Target, cv: &Canvas, cpus: u32) {
        let n = self.frames.max(1) as f32;
        // Share of the frame's bands (16 rows each) the other processors did.
        let bands = n * t.h.div_ceil(16) as f32;
        let others = self.ap_bands.get() as f32 * 100.0 / bands;
        let ms = |s: f32| s / n * 1000.0;
        let line = alloc::format!(
            "GameCoob: {:.1} fps, render {}x{} {:.1} ms, scale to {}x{} {:.1} ms, present {:.1} ms (screen {}x{}), {} CPUs, {:.0}% of rows on the others\r\n",
            n / (self.now - self.since),
            t.w,
            t.h,
            ms(self.sum[0]),
            cv.pw,
            cv.ph,
            ms(self.sum[1]),
            ms(self.sum[2]),
            cv.w,
            cv.h,
            cpus,
            others,
        );
        crate::firmware::serial_write(&line);
        *self = Stats { since: self.now, ..Stats::default() };
    }
}
