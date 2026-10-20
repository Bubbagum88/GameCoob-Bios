//! Plays the real GameCube intro, rebuilt from BS2, when the user's own
//! `ipl.bin` is on the boot partition, and runs on into the rebuilt cube
//! menu. Without a usable dump, `main` explains where to put it.

use crate::bs2::menu::backend::{FakeCards, FakeDrive};
use crate::bs2::menu::{Action, Menu, MenuAssets, Pad, SysInfo};
use alloc::boxed::Box;
use crate::bs2::{Assets, Input, Intro, cube::Jingle, raster::{Job, Target}, render};
use crate::clock::Clock;
use crate::gfx::{Canvas, ResScale};
use crate::ipl::Ipl;
use alloc::string::{String, ToString};
use alloc::vec::Vec;
use core::ffi::c_void;
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
        Action::FirmwareSetup => Some(firmware::firmware_setup()),
        Action::Restart => firmware::restart(),
        Action::PowerOff => firmware::power_off(),
    }
}

/// The other processors, used to rasterize bands of each frame in parallel.
/// They only run pure computation on memory the frame owns: no firmware calls
/// and no allocation happen on them.
struct Cores {
    mp: ScopedProtocol<MpServices>,
    done: Event,
}

extern "efiapi" fn ap_work(arg: *mut c_void) {
    // SAFETY: `arg` is the `Job` the BSP keeps alive until every AP returns.
    let job = unsafe { &*(arg as *const Job) };
    job.work();
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
        Some(Cores { mp, done })
    }

    /// Rasterize `job` on every processor; returns once all are idle again.
    fn run(&self, job: &Job) {
        let arg = job as *const Job as *mut c_void;
        // SAFETY: the event handle stays owned by `self`.
        let ev = unsafe { self.done.unsafe_clone() };
        let started = self.mp.startup_all_aps(false, ap_work, arg, Some(ev), None).is_ok();
        job.work();
        if started {
            while !job.finished() {
                core::hint::spin_loop();
            }
            // The last AP may still be returning from `ap_work`.
            while !matches!(boot::check_event(&self.done), Ok(true)) {
                core::hint::spin_loop();
            }
        }
    }
}

/// Run the intro at 60 logic frames per second, then the cube menu. Never
/// returns: the menu's actions boot or reset.
///
/// Intro: Enter, Space or A is the A button (hold to spin the cube); Z at
/// boot picks the easter-egg jingle; Escape skips to the menu.
/// Menu: the arrows are the d-pad, Enter is A, Escape or Backspace is B and
/// Space or S is START. F10 opens the system dialog (firmware setup,
/// restart, power off); D opens and closes the stand-in disc drive's tray
/// and 1 / 2 pull, format-break and re-insert the stand-in memory cards.
pub fn play(gop: &mut GraphicsOutput, cv: &mut Canvas, clock: &Clock, assets: &Assets, ma: &MenuAssets) -> ! {
    let mut intro = Intro::new(assets);
    let mut menu: Option<Menu> = None;
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
    let mut a_until = -1.0f32;
    let mut egg: Option<Jingle> = None;

    let mut pad = Pad::default();
    let mut clock_at = 0.0;

    loop {
        let now = clock.now() - t0;
        loop {
            let key = match system::with_stdin(|i| i.read_key()) {
                Ok(Some(k)) => k,
                _ => break,
            };
            if menu.is_some() {
                // Presses are latched until the next logic frame.
                match key {
                    Key::Printable(c) => match char::from(c) {
                        '\r' => pad.a = true,
                        '\u{8}' => pad.b = true,
                        ' ' | 's' | 'S' => pad.start = true,
                        'd' | 'D' => pad.eject = true,
                        '1' => pad.slot[0] = true,
                        '2' => pad.slot[1] = true,
                        _ => {}
                    },
                    Key::Special(ScanCode::UP) => pad.up = true,
                    Key::Special(ScanCode::DOWN) => pad.down = true,
                    Key::Special(ScanCode::LEFT) => pad.left = true,
                    Key::Special(ScanCode::RIGHT) => pad.right = true,
                    Key::Special(ScanCode::ESCAPE) => pad.b = true,
                    Key::Special(ScanCode::FUNCTION_10) => pad.system = true,
                    Key::Special(ScanCode::FUNCTION_3) => show_stats = !show_stats,
                    _ => {}
                }
                continue;
            }
            match key {
                Key::Printable(c) => match char::from(c) {
                    '\r' | ' ' | 'a' | 'A' => a_until = now + 0.15,
                    'z' if intro.frame < 30 => egg = Some(Jingle::Pad1Z),
                    'Z' if intro.frame < 30 => egg = Some(Jingle::AllPadsZ),
                    _ => {}
                },
                Key::Special(ScanCode::ESCAPE) => {
                    // Skip straight to the menu.
                    while !intro.finished() {
                        intro.update(Input { a: false, egg: None });
                    }
                }
                _ => {}
            }
        }
        // Catch logic up to real time, drawing only the latest frame.
        let due = (now * 60.0) as u32;
        let mut stepped = false;
        while done_frames < due {
            done_frames += 1;
            stepped = true;
            if let Some(m) = &mut menu {
                if let Some(act) = m.update(&pad, &intro.sin) {
                    if let Some(msg) = perform(act) {
                        m.notify(msg);
                    }
                }
                m.tick_guide();
                m.update_fx(ma, &intro.sin);
                pad = Pad::default();
                continue;
            }
            intro.update(Input { a: now < a_until, egg });
            if intro.finished() {
                menu = Some(Menu::new(sys_info(), ma, Box::new(FakeDrive::new()), Box::new(FakeCards::new()), intro.wobble_frame, &intro.sin));
                // Keys pressed during the intro don't leak into the menu.
                pad = Pad::default();
            }
        }
        if let Some(m) = &mut menu {
            if now - clock_at > 0.25 {
                clock_at = now;
                m.info.time = now_time();
            }
        }
        if !stepped {
            continue;
        }
        let par = |job: &Job| match &cores {
            Some(c) => c.run(job),
            None => job.work(),
        };
        let (w, h) = res.size(cv);
        if (w, h) != (target.w, target.h) {
            target = Target::new(w, h);
        }
        let d0 = clock.now();
        intro.render_with_menu(&mut target, assets, ma, menu.as_ref(), &par);
        let d1 = clock.now();
        cv.blit_frame(&target.color, target.w, target.h);
        let d2 = clock.now();
        cv.present(gop);
        let d3 = clock.now();
        res.frame(d3 - d0);
        if stats.add(d3, [d1 - d0, d2 - d1, d3 - d2]) && (log_stats || show_stats) {
            stats.report(&target, cv);
        }
    }
}


/// Average frame timings, reported every couple of seconds.
#[derive(Default)]
struct Stats {
    since: f32,
    now: f32,
    frames: u32,
    sum: [f32; 3],
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

    fn report(&mut self, t: &Target, cv: &Canvas) {
        let n = self.frames.max(1) as f32;
        let ms = |s: f32| s / n * 1000.0;
        let line = alloc::format!(
            "GameCoob: {:.1} fps, render {}x{} {:.1} ms, scale to {}x{} {:.1} ms, present {:.1} ms (screen {}x{})\r\n",
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
        );
        serial_write(&line);
        *self = Stats { since: self.now, ..Stats::default() };
    }
}

/// Write to COM1 directly: the firmware's console would draw over the
/// picture. Harmless on a PC without the port.
fn serial_write(s: &str) {
    for b in s.bytes() {
        // SAFETY: a byte to the standard COM1 data port.
        unsafe { core::arch::asm!("out dx, al", in("dx") 0x3f8u16, in("al") b, options(nomem, nostack)) };
    }
}
