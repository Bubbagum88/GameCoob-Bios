//! Software renderer: a backbuffer, a flat-shaded triangle rasterizer with a
//! depth buffer, a pinhole camera, and an 8x8 bitmap font. UEFI gives us a
//! framebuffer and nothing else, so everything is drawn on the CPU and pushed
//! to the screen with one GOP blit per frame.

use alloc::vec;
use alloc::vec::Vec;
use font8x8::legacy::BASIC_LEGACY;
use uefi::proto::console::gop::{BltOp, BltPixel, BltRegion, GraphicsOutput};

use crate::math::{Mat3, Vec3, v3};

pub type Rgb = u32;

pub const fn rgb(r: u8, g: u8, b: u8) -> Rgb {
    ((r as u32) << 16) | ((g as u32) << 8) | b as u32
}

fn split(c: Rgb) -> (f32, f32, f32) {
    (((c >> 16) & 0xff) as f32, ((c >> 8) & 0xff) as f32, (c & 0xff) as f32)
}

fn join(r: f32, g: f32, b: f32) -> Rgb {
    rgb(r.clamp(0.0, 255.0) as u8, g.clamp(0.0, 255.0) as u8, b.clamp(0.0, 255.0) as u8)
}

/// Multiply a color's brightness by `k`.
pub fn shade(c: Rgb, k: f32) -> Rgb {
    let (r, g, b) = split(c);
    join(r * k, g * k, b * k)
}

/// Linear blend from `a` to `b`.
pub fn mix(a: Rgb, b: Rgb, t: f32) -> Rgb {
    let (ar, ag, ab) = split(a);
    let (br, bg, bb) = split(b);
    join(ar + (br - ar) * t, ag + (bg - ag) * t, ab + (bb - ab) * t)
}

pub struct Canvas {
    pub w: usize,
    pub h: usize,
    px: Vec<BltPixel>,
    /// 1/z per pixel; larger is nearer, 0 is empty.
    depth: Vec<f32>,
}

impl Canvas {
    pub fn new(w: usize, h: usize) -> Self {
        Self { w, h, px: vec![BltPixel::new(0, 0, 0); w * h], depth: vec![0.0; w * h] }
    }

    pub fn clear_depth(&mut self) {
        self.depth.fill(0.0);
    }

    /// Vertical gradient fill; also clears depth.
    pub fn gradient(&mut self, top: Rgb, bottom: Rgb) {
        for y in 0..self.h {
            let c = BltPixel::from(mix(top, bottom, y as f32 / self.h as f32));
            self.px[y * self.w..(y + 1) * self.w].fill(c);
        }
        self.clear_depth();
    }

    #[inline]
    fn put(&mut self, x: usize, y: usize, c: Rgb) {
        self.px[y * self.w + x] = BltPixel::from(c);
    }

    #[inline]
    fn get(&self, x: usize, y: usize) -> Rgb {
        let p = &self.px[y * self.w + x];
        rgb(p.red, p.green, p.blue)
    }

    #[inline]
    fn blend_px(&mut self, x: usize, y: usize, c: Rgb, a: f32) {
        if a >= 1.0 {
            self.put(x, y, c);
        } else if a > 0.0 {
            let d = self.get(x, y);
            self.put(x, y, mix(d, c, a));
        }
    }

    /// Alpha-blended axis-aligned rectangle, clipped to the canvas.
    pub fn rect(&mut self, x: i32, y: i32, w: i32, h: i32, c: Rgb, a: f32) {
        let x0 = x.max(0) as usize;
        let y0 = y.max(0) as usize;
        let x1 = ((x + w).max(0) as usize).min(self.w);
        let y1 = ((y + h).max(0) as usize).min(self.h);
        for yy in y0..y1 {
            for xx in x0..x1 {
                self.blend_px(xx, yy, c, a);
            }
        }
    }

    /// Rectangle outline `t` pixels thick.
    pub fn frame(&mut self, x: i32, y: i32, w: i32, h: i32, t: i32, c: Rgb, a: f32) {
        self.rect(x, y, w, t, c, a);
        self.rect(x, y + h - t, w, t, c, a);
        self.rect(x, y + t, t, h - 2 * t, c, a);
        self.rect(x + w - t, y + t, t, h - 2 * t, c, a);
    }

    /// Darken (k < 1) the whole frame, used for fades.
    pub fn fade(&mut self, k: f32) {
        if k >= 1.0 {
            return;
        }
        for p in self.px.iter_mut() {
            p.red = (p.red as f32 * k) as u8;
            p.green = (p.green as f32 * k) as u8;
            p.blue = (p.blue as f32 * k) as u8;
        }
    }

    /// Flat-shaded triangle. Each vertex is (screen x, screen y, 1/z).
    pub fn tri(&mut self, a: [f32; 3], b: [f32; 3], c: [f32; 3], col: Rgb) {
        let area = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
        if area.abs() < 1e-6 {
            return;
        }
        let inv = 1.0 / area;
        let minx = libm::floorf(a[0].min(b[0]).min(c[0])).max(0.0) as usize;
        let maxx = (libm::ceilf(a[0].max(b[0]).max(c[0])) as i64).min(self.w as i64 - 1);
        let miny = libm::floorf(a[1].min(b[1]).min(c[1])).max(0.0) as usize;
        let maxy = (libm::ceilf(a[1].max(b[1]).max(c[1])) as i64).min(self.h as i64 - 1);
        if maxx < 0 || maxy < 0 {
            return;
        }
        let px = BltPixel::from(col);
        for y in miny..=maxy as usize {
            let fy = y as f32 + 0.5;
            for x in minx..=maxx as usize {
                let fx = x as f32 + 0.5;
                // Barycentric weights; all share area's sign when inside.
                let w0 = ((b[0] - fx) * (c[1] - fy) - (b[1] - fy) * (c[0] - fx)) * inv;
                let w1 = ((c[0] - fx) * (a[1] - fy) - (c[1] - fy) * (a[0] - fx)) * inv;
                let w2 = 1.0 - w0 - w1;
                if w0 < 0.0 || w1 < 0.0 || w2 < 0.0 {
                    continue;
                }
                let z = w0 * a[2] + w1 * b[2] + w2 * c[2];
                let i = y * self.w + x;
                if z > self.depth[i] {
                    self.depth[i] = z;
                    self.px[i] = px;
                }
            }
        }
    }

    /// Draw text with the 8x8 font scaled by `s`. Returns the drawn width.
    pub fn text(&mut self, x: i32, y: i32, s: i32, text: &str, c: Rgb, a: f32) -> i32 {
        let mut cx = x;
        for ch in text.chars() {
            let glyph = BASIC_LEGACY.get(ch as usize).copied().unwrap_or(BASIC_LEGACY[b'?' as usize]);
            for (row, bits) in glyph.iter().enumerate() {
                for col in 0..8 {
                    if bits & (1 << col) != 0 {
                        self.rect(cx + col * s, y + row as i32 * s, s, s, c, a);
                    }
                }
            }
            cx += 8 * s;
        }
        cx - x
    }

    pub fn text_centered(&mut self, cx: i32, y: i32, s: i32, text: &str, c: Rgb, a: f32) {
        let w = text_width(text, s);
        self.text(cx - w / 2, y, s, text, c, a);
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

pub fn text_width(text: &str, s: i32) -> i32 {
    text.chars().count() as i32 * 8 * s
}

/// Pinhole camera looking from `eye` at `target`, y up.
pub struct Camera {
    eye: Vec3,
    right: Vec3,
    up: Vec3,
    fwd: Vec3,
    focal: f32,
    cx: f32,
    cy: f32,
}

impl Camera {
    pub fn look_at(eye: Vec3, target: Vec3, fov_deg: f32, canvas: &Canvas) -> Self {
        let fwd = (target - eye).norm();
        let right = fwd.cross(v3(0.0, 1.0, 0.0)).norm();
        let up = right.cross(fwd);
        let half = fov_deg.to_radians() * 0.5;
        let focal = (canvas.h as f32 * 0.5) / (libm::tanf(half));
        Self { eye, right, up, fwd, focal, cx: canvas.w as f32 * 0.5, cy: canvas.h as f32 * 0.5 }
    }

    /// Project to (screen x, screen y, 1/z), or None if behind the camera.
    pub fn project(&self, p: Vec3) -> Option<[f32; 3]> {
        let d = p - self.eye;
        let z = d.dot(self.fwd);
        if z < 0.05 {
            return None;
        }
        let iz = 1.0 / z;
        Some([self.cx + d.dot(self.right) * iz * self.focal, self.cy - d.dot(self.up) * iz * self.focal, iz])
    }
}

/// Face order: +X, -X, +Y (top), -Y, +Z, -Z.
const FACES: [(Vec3, [usize; 4]); 6] = [
    (v3(1.0, 0.0, 0.0), [1, 3, 7, 5]),
    (v3(-1.0, 0.0, 0.0), [0, 4, 6, 2]),
    (v3(0.0, 1.0, 0.0), [2, 6, 7, 3]),
    (v3(0.0, -1.0, 0.0), [0, 1, 5, 4]),
    (v3(0.0, 0.0, 1.0), [4, 5, 7, 6]),
    (v3(0.0, 0.0, -1.0), [0, 2, 3, 1]),
];

/// A light shining down from upper-left-front.
pub fn light_dir() -> Vec3 {
    v3(-0.35, 0.85, 0.45).norm()
}

/// Draw a box centered at `c` with half-extents `half`, rotated by `rot`,
/// one color per face (see `FACES` order), Lambert-shaded.
pub fn draw_box(cv: &mut Canvas, cam: &Camera, c: Vec3, half: Vec3, rot: &Mat3, colors: &[Rgb; 6]) {
    let mut corners = [[0.0f32; 3]; 8];
    let mut world = [Vec3::default(); 8];
    for (i, w) in world.iter_mut().enumerate() {
        let local = v3(
            if i & 1 != 0 { half.x } else { -half.x },
            if i & 2 != 0 { half.y } else { -half.y },
            if i & 4 != 0 { half.z } else { -half.z },
        );
        *w = c + rot.apply(local);
        match cam.project(*w) {
            Some(p) => corners[i] = p,
            None => return,
        }
    }
    let light = light_dir();
    for (fi, (n, idx)) in FACES.iter().enumerate() {
        let n = rot.apply(*n);
        let fc = (world[idx[0]] + world[idx[2]]) * 0.5;
        if n.dot(fc - cam.eye) >= 0.0 {
            continue; // facing away
        }
        let k = 0.35 + 0.75 * n.dot(light).max(0.0);
        let col = shade(colors[fi], k);
        let [a, b, cc, d] = idx.map(|i| corners[i]);
        cv.tri(a, b, cc, col);
        cv.tri(a, cc, d, col);
    }
}
