//! Plays the real GameCube intro, rebuilt from BS2, when the user's own
//! `ipl.bin` is on the boot partition. Without it, the original GameCoob
//! intro plays instead.

use crate::bs2::{Assets, Input, Intro, cube::Jingle, raster::{Job, Target}, render};
use crate::clock::Clock;
use crate::gfx::Canvas;
use crate::ipl::Ipl;
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

/// Load the dump and its intro assets, if present and valid.
pub fn load() -> Option<Assets> {
    let rom = read_dump()?;
    let ipl = match Ipl::parse(&rom) {
        Ok(i) => i,
        Err(e) => {
            log::warn!("ipl.bin found but not usable: {e:?}");
            return None;
        }
    };
    let a = Assets::load(&ipl);
    if a.is_none() {
        log::warn!("ipl.bin parsed but the intro assets are missing");
    }
    a
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

/// Run the intro at 60 logic frames per second until it hands over to the
/// menu. Enter, Space or A is the A button (hold to spin the cube); Z at
/// boot picks the easter-egg jingle; Escape skips.
pub fn play(gop: &mut GraphicsOutput, cv: &mut Canvas, clock: &Clock, assets: &Assets) {
    let mut intro = Intro::new(assets);
    let cores = Cores::open();
    // Full GX resolution unless this machine can't keep up, then half.
    let mut target = Target::new(render::EFB_W, render::EFB_H);
    let mut slow_frames = 0;
    let t0 = clock.now();
    let mut done_frames = 0u32;
    let mut a_until = -1.0f32;
    let mut egg: Option<Jingle> = None;

    loop {
        let now = clock.now() - t0;
        loop {
            match system::with_stdin(|i| i.read_key()) {
                Ok(Some(Key::Printable(c))) => match char::from(c) {
                    '\r' | ' ' | 'a' | 'A' => a_until = now + 0.15,
                    'z' if intro.frame < 30 => egg = Some(Jingle::Pad1Z),
                    'Z' if intro.frame < 30 => egg = Some(Jingle::AllPadsZ),
                    _ => {}
                },
                Ok(Some(Key::Special(ScanCode::ESCAPE))) => return,
                Ok(Some(_)) => {}
                _ => break,
            }
        }
        // Catch logic up to real time, drawing only the latest frame.
        let due = (now * 60.0) as u32;
        let mut stepped = false;
        while done_frames < due {
            intro.update(Input { a: now < a_until, egg });
            done_frames += 1;
            stepped = true;
            if intro.finished() {
                return;
            }
        }
        if !stepped {
            continue;
        }
        let d0 = clock.now();
        intro.render(&mut target, assets, &|job| match &cores {
            Some(c) => c.run(job),
            None => job.work(),
        });
        if clock.now() - d0 > 1.0 / 30.0 && target.w == render::EFB_W {
            slow_frames += 1;
            if slow_frames > 10 {
                target = Target::new(render::EFB_W / 2, render::EFB_H / 2);
            }
        }
        cv.blit_frame(&target.color, target.w, target.h);
        cv.present(gop);
    }
}

