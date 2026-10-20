//! The backbuffer. UEFI gives us a framebuffer and nothing else, so each
//! frame is rendered on the CPU and pushed to the screen with one GOP blit.

use alloc::vec;
use alloc::vec::Vec;
use uefi::proto::console::gop::{BltOp, BltPixel, BltRegion, GraphicsOutput};

/// The GameCube's 640x480 picture, of which 448 lines are active.
const PIC_W: usize = 640;
const PIC_H: usize = 480;
const ACT_H: usize = 448;

pub struct Canvas {
    pub w: usize,
    pub h: usize,
    /// Where the active picture sits on screen, and its size.
    x0: usize,
    y0: usize,
    pw: usize,
    ph: usize,
    px: Vec<BltPixel>,
    row: Vec<BltPixel>,
    cleared: bool,
}

impl Canvas {
    /// A centered 4:3 picture, the way a TV shows the GameCube's 448 active
    /// lines. Whole-number scales keep every texel the same size, so one is
    /// used when it fills most of the screen; otherwise the picture fills
    /// the height.
    pub fn new(w: usize, h: usize) -> Self {
        let k = (w / PIC_W).min(h / PIC_H);
        let (pic_w, pic_h) = if k >= 1 && k * PIC_H * 5 >= h * 4 {
            (k * PIC_W, k * PIC_H)
        } else {
            let pic_h = h.min(w * 3 / 4);
            (pic_h * 4 / 3, pic_h)
        };
        let ph = pic_h * ACT_H / PIC_H;
        Self {
            w,
            h,
            x0: (w - pic_w) / 2,
            y0: (h - ph) / 2,
            pw: pic_w,
            ph,
            px: vec![BltPixel::new(0, 0, 0); pic_w * ph],
            row: Vec::new(),
            cleared: false,
        }
    }

    /// Scale a GX-style frame (`sw` x `sh`, linear 0..1 RGB) into the
    /// picture. Each source pixel is converted once and repeated rows are
    /// copied, so the cost barely grows with the screen size.
    pub fn blit_frame(&mut self, src: &[[f32; 3]], sw: usize, sh: usize) {
        let (pw, ph) = (self.pw, self.ph);
        self.row.resize(sw, BltPixel::new(0, 0, 0));
        let cols: Vec<usize> = (0..pw).map(|x| x * sw / pw).collect();
        let mut last = usize::MAX;
        for y in 0..ph {
            let sy = y * sh / ph;
            if sy == last {
                self.px.copy_within((y - 1) * pw..y * pw, y * pw);
                continue;
            }
            last = sy;
            for (d, p) in self.row.iter_mut().zip(&src[sy * sw..][..sw]) {
                *d = BltPixel::new(
                    (p[0].clamp(0.0, 1.0) * 255.0) as u8,
                    (p[1].clamp(0.0, 1.0) * 255.0) as u8,
                    (p[2].clamp(0.0, 1.0) * 255.0) as u8,
                );
            }
            let dst = &mut self.px[y * pw..][..pw];
            for (d, &c) in dst.iter_mut().zip(cols.iter()) {
                *d = self.row[c];
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

/// Switch to the screen mode that shows the 640x480 picture sharpest. The
/// current mode is usually the panel's native one, so it stays whenever it
/// already fits a whole-number scale of 2 or more; otherwise the mode with
/// the largest such scale (smallest of those, for a cheaper blit) wins,
/// never larger than the current mode on either axis.
pub fn pick_mode(gop: &mut GraphicsOutput) {
    let scale = |(w, h): (usize, usize)| (w / PIC_W).min(h / PIC_H);
    let (cw, ch) = gop.current_mode_info().resolution();
    if scale((cw, ch)) >= 2 {
        return;
    }
    let best = gop
        .modes()
        .filter(|m| {
            let (w, h) = m.info().resolution();
            w <= cw.max(2 * PIC_W) && h <= ch.max(2 * PIC_H) && scale((w, h)) >= 2
        })
        .min_by_key(|m| {
            let (w, h) = m.info().resolution();
            (usize::MAX - scale((w, h)), w * h)
        });
    if let Some(m) = best {
        let _ = gop.set_mode(&m);
    }
}
