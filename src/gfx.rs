//! The backbuffer. UEFI gives us a framebuffer and nothing else, so each
//! frame is rendered on the CPU and pushed to the screen with one GOP blit.

use alloc::vec;
use alloc::vec::Vec;
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
    acc: Vec<[f32; 3]>,
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
            acc: Vec::new(),
            cleared: false,
        }
    }

    /// Scale a frame (`sw` x `sh`, linear 0..1 RGB) into the picture. A
    /// smaller frame is enlarged with nearest-neighbour; a larger one is
    /// box-filtered down, which smooths its edges (supersampling).
    pub fn blit_frame(&mut self, src: &[[f32; 3]], sw: usize, sh: usize) {
        let (pw, ph) = (self.pw, self.ph);
        if sw >= pw && sh >= ph {
            return self.downsample(src, sw, sh);
        }
        let cols: Vec<usize> = (0..pw).map(|x| x * sw / pw).collect();
        let mut last = usize::MAX;
        for y in 0..ph {
            let sy = y * sh / ph;
            if sy == last {
                self.px.copy_within((y - 1) * pw..y * pw, y * pw);
                continue;
            }
            last = sy;
            let row = &src[sy * sw..][..sw];
            for (d, &c) in self.px[y * pw..][..pw].iter_mut().zip(cols.iter()) {
                *d = to_px(row[c]);
            }
        }
    }

    fn downsample(&mut self, src: &[[f32; 3]], sw: usize, sh: usize) {
        let (pw, ph) = (self.pw, self.ph);
        // Source column span of each picture column.
        let span: Vec<(usize, usize)> = (0..pw).map(|x| (x * sw / pw, ((x + 1) * sw / pw).max(x * sw / pw + 1))).collect();
        self.acc.resize(pw, [0.0; 3]);
        for y in 0..ph {
            let (y0, y1) = (y * sh / ph, ((y + 1) * sh / ph).max(y * sh / ph + 1));
            self.acc.fill([0.0; 3]);
            for sy in y0..y1 {
                let row = &src[sy * sw..][..sw];
                for (a, &(x0, x1)) in self.acc.iter_mut().zip(span.iter()) {
                    for p in &row[x0..x1] {
                        a[0] += p[0];
                        a[1] += p[1];
                        a[2] += p[2];
                    }
                }
            }
            let rows = (y1 - y0) as f32;
            for ((d, a), &(x0, x1)) in self.px[y * pw..][..pw].iter_mut().zip(self.acc.iter()).zip(span.iter()) {
                let k = 1.0 / (rows * (x1 - x0) as f32);
                *d = to_px([a[0] * k, a[1] * k, a[2] * k]);
            }
        }
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
