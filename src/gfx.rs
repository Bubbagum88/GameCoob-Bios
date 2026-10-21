//! The backbuffer. UEFI gives us a framebuffer and nothing else, so each
//! frame is rendered on the CPU and pushed to the screen with one GOP blit.

use alloc::vec;
use alloc::vec::Vec;
use core::sync::atomic::{AtomicUsize, Ordering};
use crate::bs2::raster::Work;
use uefi::proto::console::gop::{BltOp, BltPixel, BltRegion, GraphicsOutput};

/// The GameCube's 480-line picture has 448 active lines.
const PIC_H: usize = 480;
const ACT_H: usize = 448;

pub struct Canvas {
    pub w: usize,
    pub h: usize,
    /// Where the active picture sits on screen, and its size in screen
    /// pixels: the resolution worth rendering at.
    x0: usize,
    y0: usize,
    pub pw: usize,
    pub ph: usize,
    px: Vec<BltPixel>,
    span: Vec<(usize, usize)>,
    cleared: bool,
}

impl Canvas {
    /// A centered 4:3 picture filling the screen's height (or width, on a
    /// screen narrower than 4:3), the way a TV shows the GameCube's 448
    /// active lines. The screen mode is left as the firmware set it, which
    /// is the panel's native one on real hardware and the window's in a VM.
    pub fn new(w: usize, h: usize) -> Self {
        let pic_h = h.min(w * 3 / 4);
        let pic_w = pic_h * 4 / 3;
        let ph = pic_h * ACT_H / PIC_H;
        Self {
            w,
            h,
            x0: (w - pic_w) / 2,
            y0: (h - ph) / 2,
            pw: pic_w,
            ph,
            px: vec![BltPixel::new(0, 0, 0); pic_w * ph],
            span: Vec::new(),
            cleared: false,
        }
    }

    /// Scale a frame (`sw` x `sh`, linear 0..1 RGB) into the picture, on
    /// as many processors as `par` runs the work on. A smaller frame is
    /// enlarged with nearest-neighbour; a larger one is box-filtered down,
    /// which smooths its edges (supersampling).
    pub fn blit_frame(&mut self, src: &[[f32; 3]], sw: usize, sh: usize, par: &dyn Fn(&dyn Work)) {
        let (pw, ph) = (self.pw, self.ph);
        assert!(src.len() >= sw * sh);
        // Source column span of each picture column (one column when enlarging).
        self.span.clear();
        self.span.extend((0..pw).map(|x| {
            let x0 = x * sw / pw;
            (x0, ((x + 1) * sw / pw).max(x0 + 1).min(sw))
        }));
        let job = Scale {
            src: src.as_ptr(),
            sw,
            sh,
            dst: self.px.as_mut_ptr(),
            pw,
            ph,
            span: &self.span,
            down: sw >= pw && sh >= ph,
            next: AtomicUsize::new(0),
        };
        par(&job);
    }

    /// Push the picture; the black border around it is drawn only once.
    pub fn present(&mut self, gop: &mut GraphicsOutput) {
        if !self.cleared {
            self.cleared = true;
            let _ = gop.blt(BltOp::VideoFill { color: BltPixel::new(0, 0, 0), dest: (0, 0), dims: (self.w, self.h) });
        }
        let _ = gop.blt(BltOp::BufferToVideo {
            buffer: &self.px,
            src: BltRegion::Full,
            dest: (self.x0, self.y0),
            dims: (self.pw, self.ph),
        });
    }
}

#[inline]
fn to_px(p: [f32; 3]) -> BltPixel {
    BltPixel::new(
        (p[0].clamp(0.0, 1.0) * 255.0) as u8,
        (p[1].clamp(0.0, 1.0) * 255.0) as u8,
        (p[2].clamp(0.0, 1.0) * 255.0) as u8,
    )
}

/// Picks the render resolution each frame from how long frames take: the
/// screen's own resolution by default, more (supersampled, for smooth
/// edges) when there is time to spare, less when frames run late, never
/// below the GameCube's own 640x448.
pub struct ResScale {
    scale: f32,
    min: f32,
    max: f32,
    spent: f32,
    frames: u32,
}

/// Frame time to stay under (60 fps with some headroom), and the time
/// below which there is room for more pixels.
const SLOW: f32 = 1.0 / 55.0;
const FAST: f32 = 1.0 / 90.0;
/// Most pixels ever rendered per frame (2x2 supersampling at 1080p).
const MAX_PIXELS: f32 = 4.0 * 1440.0 * 1008.0;

impl ResScale {
    pub fn new(cv: &Canvas) -> ResScale {
        let native = (cv.pw * cv.ph) as f32;
        let min = (640.0 / cv.pw as f32).max(448.0 / cv.ph as f32).min(1.0);
        let max = libm::sqrtf(MAX_PIXELS / native).clamp(1.0, 2.0);
        ResScale { scale: 1.0, min, max, spent: 0.0, frames: 0 }
    }

    /// The render size for the next frame.
    pub fn size(&self, cv: &Canvas) -> (usize, usize) {
        let w = (libm::roundf(cv.pw as f32 * self.scale) as usize).max(1);
        let h = (libm::roundf(cv.ph as f32 * self.scale) as usize).max(1);
        (w, h)
    }

    /// Record how long a frame took (render to present, in seconds).
    pub fn frame(&mut self, secs: f32) {
        self.spent += secs;
        self.frames += 1;
        // A quarter second or 15 frames, whichever comes first.
        if self.frames < 15 && self.spent < 0.25 {
            return;
        }
        let avg = self.spent / self.frames as f32;
        self.spent = 0.0;
        self.frames = 0;
        if avg > SLOW {
            // Pixels scale with scale^2; aim just under the budget.
            let k = libm::sqrtf(SLOW * 0.9 / avg).clamp(0.5, 0.95);
            self.scale = (self.scale * k).max(self.min);
        } else if avg < FAST {
            self.scale = (self.scale * 1.1).min(self.max);
        }
    }
}

/// One frame's scale, split into bands of rows that processors claim.
struct Scale<'a> {
    src: *const [f32; 3],
    sw: usize,
    sh: usize,
    dst: *mut BltPixel,
    pw: usize,
    ph: usize,
    span: &'a [(usize, usize)],
    down: bool,
    next: AtomicUsize,
}

// Bands are disjoint picture rows, claimed once each through `next`; the
// source is only read.
unsafe impl Sync for Scale<'_> {}

const SCALE_BAND: usize = 8;

impl Work for Scale<'_> {
    fn work(&self) -> usize {
        let mut did = 0;
        loop {
            let b = self.next.fetch_add(1, Ordering::AcqRel);
            let y0 = b * SCALE_BAND;
            if y0 >= self.ph {
                return did;
            }
            did += 1;
            for y in y0..(y0 + SCALE_BAND).min(self.ph) {
                // SAFETY: row `y` belongs to this band only; `src` holds
                // `sw * sh` pixels (checked in `blit_frame`).
                let dst = unsafe { core::slice::from_raw_parts_mut(self.dst.add(y * self.pw), self.pw) };
                let (r0, r1) = (y * self.sh / self.ph, ((y + 1) * self.sh / self.ph).max(y * self.sh / self.ph + 1).min(self.sh));
                if !self.down {
                    let row = unsafe { core::slice::from_raw_parts(self.src.add(r0 * self.sw), self.sw) };
                    for (d, &(c, _)) in dst.iter_mut().zip(self.span) {
                        *d = to_px(row[c]);
                    }
                    continue;
                }
                let k_rows = 1.0 / (r1 - r0) as f32;
                for (d, &(c0, c1)) in dst.iter_mut().zip(self.span) {
                    let mut a = [0.0f32; 3];
                    for sy in r0..r1 {
                        let row = unsafe { core::slice::from_raw_parts(self.src.add(sy * self.sw + c0), c1 - c0) };
                        for p in row {
                            a[0] += p[0];
                            a[1] += p[1];
                            a[2] += p[2];
                        }
                    }
                    let k = k_rows / (c1 - c0) as f32;
                    *d = to_px([a[0] * k, a[1] * k, a[2] * k]);
                }
            }
        }
    }
}
