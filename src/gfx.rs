//! The backbuffer. UEFI gives us a framebuffer and nothing else, so each
//! frame is rendered on the CPU and pushed to the screen with one GOP blit.

use alloc::vec;
use alloc::vec::Vec;
use uefi::proto::console::gop::{BltOp, BltPixel, BltRegion, GraphicsOutput};

pub struct Canvas {
    pub w: usize,
    pub h: usize,
    px: Vec<BltPixel>,
}

impl Canvas {
    pub fn new(w: usize, h: usize) -> Self {
        Self { w, h, px: vec![BltPixel::new(0, 0, 0); w * h] }
    }

    /// Show a GX-style frame (`sw` x `sh`, linear 0..1 RGB) the way a TV
    /// shows the GameCube's 448 active lines: in a centered 4:3 picture.
    pub fn blit_frame(&mut self, src: &[[f32; 3]], sw: usize, sh: usize) {
        let (w, h) = (self.w, self.h);
        let pic_h = h.min(w * 3 / 4);
        let pic_w = pic_h * 4 / 3;
        let act_h = pic_h * 448 / 480;
        let x0 = (w - pic_w) / 2;
        let y0 = (h - act_h) / 2;
        self.px.fill(BltPixel::new(0, 0, 0));
        let cols: Vec<usize> = (0..pic_w).map(|x| x * sw / pic_w).collect();
        for y in 0..act_h {
            let row = &src[(y * sh / act_h) * sw..][..sw];
            let dst = &mut self.px[(y0 + y) * w + x0..][..pic_w];
            for (d, &c) in dst.iter_mut().zip(cols.iter()) {
                let p = row[c];
                *d = BltPixel::new(
                    (p[0].clamp(0.0, 1.0) * 255.0) as u8,
                    (p[1].clamp(0.0, 1.0) * 255.0) as u8,
                    (p[2].clamp(0.0, 1.0) * 255.0) as u8,
                );
            }
        }
    }

    pub fn present(&self, gop: &mut GraphicsOutput) {
        let _ = gop.blt(BltOp::BufferToVideo {
            buffer: &self.px,
            src: BltRegion::Full,
            dest: (0, 0),
            dims: (self.w, self.h),
        });
    }
}
