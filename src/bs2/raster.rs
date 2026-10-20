//! A small GX-like software rasterizer: perspective-correct attributes,
//! per-pixel TEV shading, alpha blending and a z buffer with separate compare
//! and update switches. Frames are built as a display list on one processor
//! and rasterized in bands, possibly on several.

use super::shader::{FastTex, Shader};
use alloc::vec;
use alloc::vec::Vec;
use core::sync::atomic::{AtomicUsize, Ordering};

pub const MAX_TC: usize = 2;

/// A vertex after projection.
#[derive(Clone, Copy, Default)]
pub struct SVert {
    /// Pixel coordinates.
    pub x: f32,
    pub y: f32,
    /// Depth, 0 near .. 1 far.
    pub z: f32,
    /// 1/w for perspective correction (1 for orthographic).
    pub rw: f32,
    pub color: [f32; 4],
    pub uv: [[f32; 2]; MAX_TC],
}

/// Interpolated values handed to the shading callback.
pub struct Frag {
    pub color: [f32; 4],
    pub uv: [[f32; 2]; MAX_TC],
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum BlendMode {
    None,
    /// src * a + dst * (1 - a)
    Alpha,
    /// src * a + dst
    Add,
}

#[derive(Clone, Copy)]
pub struct State {
    pub z_compare: bool,
    pub z_update: bool,
    pub blend: BlendMode,
    /// 0 none, 1 cull front (clockwise on screen), 2 cull back.
    pub cull: u8,
}

pub struct Target {
    pub w: usize,
    pub h: usize,
    pub color: Vec<[f32; 3]>,
    pub depth: Vec<f32>,
}

impl Target {
    pub fn new(w: usize, h: usize) -> Self {
        Target { w, h, color: vec![[0.0; 3]; w * h], depth: vec![1.0; w * h] }
    }

    pub fn clear(&mut self, c: [f32; 3]) {
        self.color.fill(c);
        self.depth.fill(1.0);
    }
}

/// How a primitive's pixels are colored.
#[derive(Clone, Copy)]
pub enum Sh<'a> {
    /// A model material; `alpha` is the model alpha multiplier.
    Mat { shader: &'a Shader, texs: &'a [FastTex], alpha: f32 },
    /// Vertex color times texture (the intro's trail tiles).
    Modulate { tex: &'a FastTex },
}

/// One triangle ready to rasterize.
#[derive(Clone, Copy)]
pub struct Prim<'a> {
    pub v: [SVert; 3],
    pub st: State,
    pub sh: Sh<'a>,
}

impl Sh<'_> {
    #[inline]
    fn shade(&self, f: &Frag) -> Option<[f32; 4]> {
        match *self {
            Sh::Mat { shader, texs, alpha } => {
                let mut c = shader.eval(f.color, &f.uv, texs)?;
                c[3] *= alpha;
                Some(c)
            }
            Sh::Modulate { tex } => {
                let t = tex.sample(f.uv[0][0], f.uv[0][1]);
                Some([f.color[0] * t[0], f.color[1] * t[1], f.color[2] * t[2], f.color[3] * t[3]])
            }
        }
    }
}

/// A display list rasterized in horizontal bands, so several processors can
/// each take bands (`work`) without touching the same rows.
pub struct Job<'a> {
    color: *mut [f32; 3],
    depth: *mut f32,
    w: usize,
    h: usize,
    prims: &'a [Prim<'a>],
    band_h: usize,
    next: AtomicUsize,
    done: AtomicUsize,
}

// Bands are disjoint row ranges, claimed once each through `next`.
unsafe impl Sync for Job<'_> {}
unsafe impl Send for Job<'_> {}

impl<'a> Job<'a> {
    pub fn new(t: &'a mut Target, prims: &'a [Prim<'a>], band_h: usize) -> Job<'a> {
        Job {
            color: t.color.as_mut_ptr(),
            depth: t.depth.as_mut_ptr(),
            w: t.w,
            h: t.h,
            prims,
            band_h: band_h.max(1),
            next: AtomicUsize::new(0),
            done: AtomicUsize::new(0),
        }
    }

    pub fn bands(&self) -> usize {
        self.h.div_ceil(self.band_h)
    }

    /// Rasterize bands until none are left. Safe to call from many threads.
    pub fn work(&self) {
        loop {
            let b = self.next.fetch_add(1, Ordering::AcqRel);
            if b >= self.bands() {
                return;
            }
            let y0 = b * self.band_h;
            let y1 = (y0 + self.band_h).min(self.h);
            for p in self.prims {
                // SAFETY: rows y0..y1 belong to this band only.
                unsafe { self.tri(p, y0 as i64, y1 as i64 - 1) };
            }
            self.done.fetch_add(1, Ordering::AcqRel);
        }
    }

    /// True once every band has been rasterized.
    pub fn finished(&self) -> bool {
        self.done.load(Ordering::Acquire) >= self.bands()
    }

    unsafe fn tri(&self, p: &Prim, band_y0: i64, band_y1: i64) {
        let st = p.st;
        let [a, b, c] = p.v;
        let area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        if area.abs() < 1e-8 {
            return;
        }
        // GX front faces are clockwise in screen space (y down) => area > 0.
        match st.cull {
            1 if area > 0.0 => return,
            2 if area < 0.0 => return,
            _ => {}
        }
        let inv = 1.0 / area;
        let minx = libm::floorf(a.x.min(b.x).min(c.x)).max(0.0) as i64;
        let maxx = (libm::ceilf(a.x.max(b.x).max(c.x)) as i64).min(self.w as i64 - 1);
        let miny = (libm::floorf(a.y.min(b.y).min(c.y)).max(0.0) as i64).max(band_y0);
        let maxy = (libm::ceilf(a.y.max(b.y).max(c.y)) as i64).min(self.h as i64 - 1).min(band_y1);
        if maxx < minx || maxy < miny {
            return;
        }
        // Edge functions as planes in pixel space: w(x, y) = ex * x + ey * y + e0.
        let e = |p: &SVert, q: &SVert| {
            let ex = -(q.y - p.y) * inv;
            let ey = (q.x - p.x) * inv;
            // w at (x, y) for the vertex opposite edge p->q
            (ex, ey, ((q.y - p.y) * p.x - (q.x - p.x) * p.y) * inv)
        };
        let (e0, e1) = (e(&b, &c), e(&c, &a));
        let mut frag = Frag { color: [0.0; 4], uv: [[0.0; 2]; MAX_TC] };
        for y in miny..=maxy {
            let fy = y as f32 + 0.5;
            // Clip the row to where all three weights are >= 0.
            let (mut lo, mut hi) = (minx as f32 + 0.5, maxx as f32 + 0.5);
            let mut empty = false;
            for (ex, ey, e0c) in [
                (e0.0, e0.1, e0.2),
                (e1.0, e1.1, e1.2),
                (-e0.0 - e1.0, -e0.1 - e1.1, 1.0 - e0.2 - e1.2),
            ] {
                let base = ey * fy + e0c;
                if ex.abs() < 1e-12 {
                    if base < -1e-6 {
                        empty = true;
                    }
                } else {
                    let x0 = -base / ex;
                    if ex > 0.0 { lo = lo.max(x0) } else { hi = hi.min(x0) }
                }
            }
            if empty || lo > hi {
                continue;
            }
            let xa = (libm::ceilf(lo - 0.5) as i64).max(minx);
            let xb = (libm::floorf(hi - 0.5) as i64).min(maxx);
            for x in xa..=xb {
                let fx = x as f32 + 0.5;
                let w0 = (e0.0 * fx + e0.1 * fy + e0.2).max(0.0);
                let w1 = (e1.0 * fx + e1.1 * fy + e1.2).max(0.0);
                let w2 = (1.0 - w0 - w1).max(0.0);
                let z = w0 * a.z + w1 * b.z + w2 * c.z;
                if !(0.0..=1.0).contains(&z) {
                    continue;
                }
                let i = y as usize * self.w + x as usize;
                let dz = unsafe { &mut *self.depth.add(i) };
                if st.z_compare && z > *dz {
                    continue;
                }
                // Perspective-correct weights.
                let (p0, p1, p2) = (w0 * a.rw, w1 * b.rw, w2 * c.rw);
                let s = 1.0 / (p0 + p1 + p2);
                let (p0, p1, p2) = (p0 * s, p1 * s, p2 * s);
                let f = &mut frag;
                for k in 0..4 {
                    f.color[k] = p0 * a.color[k] + p1 * b.color[k] + p2 * c.color[k];
                }
                for t in 0..MAX_TC {
                    for k in 0..2 {
                        f.uv[t][k] = p0 * a.uv[t][k] + p1 * b.uv[t][k] + p2 * c.uv[t][k];
                    }
                }
                let Some(out) = p.sh.shade(f) else { continue };
                let d = unsafe { &mut *self.color.add(i) };
                match st.blend {
                    BlendMode::None => *d = [out[0], out[1], out[2]],
                    BlendMode::Alpha => {
                        let al = out[3].clamp(0.0, 1.0);
                        for k in 0..3 {
                            d[k] = out[k] * al + d[k] * (1.0 - al);
                        }
                    }
                    BlendMode::Add => {
                        let al = out[3].clamp(0.0, 1.0);
                        for k in 0..3 {
                            d[k] = (d[k] + out[k] * al).min(1.0);
                        }
                    }
                }
                if st.z_update {
                    *dz = z;
                }
            }
        }
    }
}
