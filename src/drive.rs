//! The PC's optical drive as Game Play's disc drive: a GameCube disc shows
//! its own banner and starts the same game from the emulator game list; a
//! PC disc the firmware can boot (El Torito with a UEFI image, such as an
//! OS installer) shows its label and boots on START.
//!
//! The drive is polled once a second with a zero-length read, which
//! only asks the drive whether a disc is in (SCSI TEST UNIT READY on most
//! firmware) and doesn't spin it. A new disc is read once, a frame after
//! "reading" is up, since the reads block the menu.

use crate::bs2::menu::backend::{Banner, DiscDrive, DiscStart, DiscState};
use crate::bs2::shader::FastTex;
use crate::clock::Clock;
use crate::launch::disc::{self, Disc, ReadAt, BANNER_H, BANNER_W};
use alloc::format;
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;
use uefi::boot::{self, OpenProtocolAttributes, OpenProtocolParams, ScopedProtocol};
use uefi::proto::device_path::DevicePath;
use uefi::proto::media::block::BlockIO;
use uefi::{Handle, Status};

/// Seconds between polls, while a disc spins up, and before the first
/// read of a new disc (so "reading" is up while the reads block).
const POLL_SECS: f32 = 1.0;
const SPIN_UP_SECS: f32 = 0.25;
const READ_DELAY_SECS: f32 = 0.1;
/// Failed reads of one disc before the drive gives up on it.
const MAX_RETRIES: u32 = 3;
/// Sectors per read.
const CHUNK: usize = 32;

/// What START does with the disc in the drive.
enum Target {
    None,
    /// Boot it through the firmware.
    Boot,
    /// Launch this game code in an emulator.
    Game(String),
}

pub struct OpticalDrive {
    handle: Handle,
    /// The drive's device path, for the boot entry.
    dp: Vec<u8>,
    state: DiscState,
    /// The media id of the disc read last (None: none read).
    read_id: Option<u32>,
    retries: u32,
    clock: Clock,
    /// When to poll next ([`Clock::now`]).
    next_poll: f32,
    banner: Option<Banner>,
    target: Target,
}

fn open_block(h: Handle) -> Option<ScopedProtocol<BlockIO>> {
    // SAFETY: GetProtocol doesn't take the drive from its driver; the
    // protocol is opened again every time, since the driver reinstalls it
    // when the disc changes.
    unsafe {
        boot::open_protocol::<BlockIO>(
            OpenProtocolParams { handle: h, agent: boot::image_handle(), controller: None },
            OpenProtocolAttributes::GetProtocol,
        )
    }
    .ok()
}

fn device_path(h: Handle) -> Option<Vec<u8>> {
    // SAFETY: as above.
    let dp = unsafe {
        boot::open_protocol::<DevicePath>(
            OpenProtocolParams { handle: h, agent: boot::image_handle(), controller: None },
            OpenProtocolAttributes::GetProtocol,
        )
    }
    .ok()?;
    Some(dp.as_bytes().to_vec())
}

/// Is this device path an optical drive's? Optical drives use 2048-byte
/// sectors, but an empty drive may not say so; QEMU's and most PCs'
/// drives sit on ATAPI or SATA/SCSI with a CD-ROM media node only on
/// their partitions, so the sector size and read-only flag decide.
fn is_optical(b: &BlockIO) -> bool {
    let m = b.media();
    m.is_removable_media() && !m.is_logical_partition() && m.block_size() == 2048
}

impl OpticalDrive {
    /// The first optical drive the firmware shows, if any.
    pub fn find(clock: Clock) -> Option<OpticalDrive> {
        let handles = boot::find_handles::<BlockIO>().ok()?;
        for h in handles {
            let Some(b) = open_block(h) else { continue };
            let m = b.media();
            log::info!(
                "block device: removable {} partition {} sector {} present {} read-only {}",
                m.is_removable_media(),
                m.is_logical_partition(),
                m.block_size(),
                m.is_media_present(),
                m.is_read_only()
            );
            if is_optical(&b) {
                let dp = device_path(h)?;
                log::info!("optical drive found");
                return Some(OpticalDrive {
                    handle: h,
                    dp,
                    state: DiscState::NoDisc,
                    read_id: None,
                    retries: 0,
                    clock,
                    next_poll: 0.0,
                    banner: None,
                    target: Target::None,
                });
            }
        }
        None
    }

    fn forget(&mut self) {
        self.read_id = None;
        self.banner = None;
        self.target = Target::None;
        self.retries = 0;
    }

    fn poll(&mut self) {
        self.next_poll = self.clock.now() + POLL_SECS;
        let Some(mut b) = open_block(self.handle) else {
            self.forget();
            self.state = DiscState::NoDisc;
            return;
        };
        let id = b.media().media_id();
        match b.read_blocks(id, 0, &mut []).map_err(|e| e.status()) {
            Ok(()) if self.read_id == Some(id) => {}
            Ok(()) if self.state == DiscState::Reading => self.read(&mut b, id),
            Ok(()) => {
                // A disc is in: say so, then read it next time round.
                self.forget();
                self.state = DiscState::Reading;
                self.next_poll = self.clock.now() + READ_DELAY_SECS;
            }
            Err(Status::NO_MEDIA) => {
                self.forget();
                self.state = DiscState::NoDisc;
            }
            Err(Status::MEDIA_CHANGED) | Err(Status::NOT_READY) => {
                self.forget();
                self.state = DiscState::Reading;
                self.next_poll = self.clock.now() + SPIN_UP_SECS;
            }
            Err(e) => {
                log::warn!("disc drive: {e:?}");
                self.read_failed();
            }
        }
    }

    fn read_failed(&mut self) {
        self.read_id = None;
        self.retries += 1;
        self.state = if self.retries >= MAX_RETRIES { DiscState::Wrong } else { DiscState::Retry };
    }

    /// Read and identify a new disc.
    fn read(&mut self, b: &mut BlockIO, id: u32) {
        let mut r = Reader { b, id, buf: vec![0u64; CHUNK * 2048 / 8] };
        let Some(d) = disc::identify(&mut r) else {
            // In a PC drive most GameCube discs end here: the drive
            // can't read them.
            log::warn!("disc drive: can't read the disc");
            self.read_failed();
            return;
        };
        self.read_id = Some(id);
        self.retries = 0;
        let (state, banner, target) = match d {
            Disc::GameCube { code, title, banner } => {
                log::info!("GameCube disc {code} {title}");
                let b = match banner {
                    Some(bn) => {
                        let t = bn.text(0).cloned().unwrap_or_default();
                        Banner {
                            image: FastTex::from_rgba(BANNER_W, BANNER_H, &bn.rgba),
                            short_name: t.short_name,
                            short_maker: t.short_maker,
                            long_name: if t.long_name.is_empty() { title } else { t.long_name },
                            long_maker: t.long_maker,
                            description: t.description,
                        }
                    }
                    None => pc_banner(&title, "", &format!("GameCube disc {code}")),
                };
                (DiscState::Ready, Some(b), Target::Game(code))
            }
            Disc::Pc { label, publisher, efi_boot: true, .. } => {
                log::info!("bootable PC disc {label}");
                let name = if label.is_empty() { String::from("PC disc") } else { label };
                (DiscState::Ready, Some(pc_banner(&name, &publisher, "Press START to boot this disc.")), Target::Boot)
            }
            Disc::Pc { label, bios_boot, .. } => {
                log::info!("PC disc {label}, no UEFI boot image (BIOS boot {bios_boot})");
                (DiscState::Wrong, None, Target::None)
            }
            Disc::Wii { code, .. } => {
                log::info!("Wii disc {code}");
                (DiscState::Wrong, None, Target::None)
            }
            Disc::Unknown => {
                log::info!("disc with no ISO 9660 or GameCube header");
                (DiscState::Wrong, None, Target::None)
            }
        };
        self.state = state;
        self.banner = banner;
        self.target = target;
    }
}

impl DiscDrive for OpticalDrive {
    fn update(&mut self) {
        if self.clock.now() >= self.next_poll {
            self.poll();
        }
    }

    fn state(&self) -> DiscState {
        self.state
    }

    fn banner(&self) -> Option<&Banner> {
        (self.state == DiscState::Ready).then_some(self.banner.as_ref()).flatten()
    }

    fn starts_disc(&self) -> bool {
        true
    }

    fn start(&mut self) -> DiscStart {
        match &self.target {
            Target::Game(code) => DiscStart::Game(code.clone()),
            Target::Boot => match crate::vars::boot_entry_for_device(&self.dp) {
                Ok(num) => {
                    log::info!("booting the disc (Boot{num:04X})");
                    DiscStart::Failed(crate::firmware::boot_entry(num))
                }
                Err(e) => DiscStart::Failed(format!("Could not make a boot entry for the disc: {:?}", e.status())),
            },
            Target::None => DiscStart::Failed("This disc can't be started.".into()),
        }
    }
}

/// Byte reads over 2048-byte sectors.
struct Reader<'a> {
    b: &'a mut BlockIO,
    id: u32,
    /// u64s so the buffer suits the drive's alignment.
    buf: Vec<u64>,
}

impl ReadAt for Reader<'_> {
    fn read_at(&mut self, offset: u64, out: &mut [u8]) -> bool {
        let mut done = 0;
        while done < out.len() {
            let at = offset + done as u64;
            let lba = at / 2048;
            let skip = (at % 2048) as usize;
            let want = (skip + out.len() - done).div_ceil(2048).min(CHUNK);
            // SAFETY: a byte view of the u64 buffer.
            let bytes = unsafe { core::slice::from_raw_parts_mut(self.buf.as_mut_ptr().cast::<u8>(), want * 2048) };
            if self.b.read_blocks(self.id, lba, bytes).is_err() {
                return false;
            }
            let n = (want * 2048 - skip).min(out.len() - done);
            out[done..done + n].copy_from_slice(&bytes[skip..skip + n]);
            done += n;
        }
        true
    }
}

/// A banner for a disc without one of its own: a disc on a dark blue
/// field.
fn pc_banner(name: &str, maker: &str, description: &str) -> Banner {
    let mut rgba = Vec::with_capacity(BANNER_W * BANNER_H * 4);
    for y in 0..BANNER_H {
        for x in 0..BANNER_W {
            let (fx, fy) = (x as f32 - 16.0, y as f32 - 15.5);
            let r = libm::sqrtf(fx * fx + fy * fy);
            let bg = [0.06 + 0.10 * y as f32 / 32.0, 0.08 + 0.12 * y as f32 / 32.0, 0.25 + 0.2 * x as f32 / 96.0];
            let c = if (3.5..13.5).contains(&r) {
                // Silver with a rainbow sheen around the hub.
                let a = libm::atan2f(fy, fx);
                let s = 0.75 + 0.2 * libm::sinf(a * 2.0);
                [s + 0.1 * libm::sinf(a * 3.0), s + 0.1 * libm::sinf(a * 3.0 + 2.1), s + 0.1 * libm::sinf(a * 3.0 + 4.2)]
            } else if (13.5..14.5).contains(&r) {
                [0.5, 0.5, 0.55]
            } else {
                bg
            };
            rgba.extend(c.map(|v| (v.clamp(0.0, 1.0) * 255.0) as u8));
            rgba.push(255);
        }
    }
    Banner {
        image: FastTex::from_rgba(BANNER_W, BANNER_H, &rgba),
        short_name: name.into(),
        short_maker: maker.into(),
        long_name: name.into(),
        long_maker: maker.into(),
        description: description.into(),
    }
}
