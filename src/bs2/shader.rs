//! A material's TEV stages compiled once into a compact form, and textures
//! converted to float texels, so the per-pixel path is plain arithmetic.
//! Semantics follow `Material::shade` (konst inputs read as 1.0 unless
//! [`Shader::set_konst`] sets them).

use crate::ipl::gx::{self, Texture};
use crate::ipl::j3d::{Material, TevStage};
use alloc::vec::Vec;

pub struct FastTex {
    w: i32,
    h: i32,
    wrap_s: u8,
    wrap_t: u8,
    texels: Vec<[f32; 4]>,
}

/// floor() without a libm call: x86-64's baseline SSE2 has no rounding
/// instruction, and libm's portable floorf is slow on the per-pixel path.
#[inline]
fn floor(x: f32) -> f32 {
    let i = x as i32 as f32;
    if i > x { i - 1.0 } else { i }
}

#[inline(always)]
fn wrap(i: i32, n: i32, mode: u8) -> i32 {
    // Every IPL texture is a power of two, so wrap with masks when possible.
    if n & (n - 1) == 0 {
        return match mode {
            gx::REPEAT => i & (n - 1),
            gx::MIRROR => {
                let m = i & (2 * n - 1);
                if m >= n { 2 * n - 1 - m } else { m }
            }
            _ => i.clamp(0, n - 1),
        };
    }
    match mode {
        gx::REPEAT => i.rem_euclid(n),
        gx::MIRROR => {
            let m = i.rem_euclid(2 * n);
            if m >= n { 2 * n - 1 - m } else { m }
        }
        _ => i.clamp(0, n - 1),
    }
}

impl FastTex {
    pub fn new(t: &Texture) -> FastTex {
        let texels = if t.width == 0 || t.height == 0 {
            alloc::vec![[1.0; 4]]
        } else {
            t.rgba.chunks_exact(4).map(|p| [p[0], p[1], p[2], p[3]].map(|c| c as f32 * (1.0 / 255.0))).collect()
        };
        FastTex { w: (t.width as i32).max(1), h: (t.height as i32).max(1), wrap_s: t.wrap_s, wrap_t: t.wrap_t, texels }
    }

    /// A 2D-pane texture: clamped, and intensity formats become white with
    /// the intensity as alpha (how BS2's 2D TEV setup uses them).
    pub fn new_ui(t: &Texture) -> FastTex {
        let mut f = FastTex::new(t);
        f.wrap_s = gx::CLAMP;
        f.wrap_t = gx::CLAMP;
        if matches!(t.format, gx::I4 | gx::I8) {
            for p in f.texels.iter_mut() {
                *p = [1.0, 1.0, 1.0, p[3]];
            }
        }
        f
    }

    /// A clamped texture from `w * h` RGBA bytes (pictures made at run time,
    /// such as save icons and disc banners).
    pub fn from_rgba(w: usize, h: usize, rgba: &[u8]) -> FastTex {
        let texels = rgba.chunks_exact(4).map(|p| [p[0], p[1], p[2], p[3]].map(|c| c as f32 * (1.0 / 255.0))).collect();
        FastTex { w: w.max(1) as i32, h: h.max(1) as i32, wrap_s: gx::CLAMP, wrap_t: gx::CLAMP, texels }
    }

    /// A white texture with the given coverage as alpha.
    pub fn from_alpha(w: usize, h: usize, a: &[u8]) -> FastTex {
        let texels = a.iter().map(|&v| [1.0, 1.0, 1.0, v as f32 * (1.0 / 255.0)]).collect();
        FastTex { w: w.max(1) as i32, h: h.max(1) as i32, wrap_s: gx::CLAMP, wrap_t: gx::CLAMP, texels }
    }

    /// The same texture as white with its intensity as alpha, keeping the
    /// wrap modes (for tiled intensity patterns).
    pub fn intensity_alpha(mut self) -> FastTex {
        for p in self.texels.iter_mut() {
            *p = [1.0, 1.0, 1.0, p[0]];
        }
        self
    }

    /// Texture size in texels.
    pub fn size(&self) -> (f32, f32) {
        (self.w as f32, self.h as f32)
    }

    #[inline]
    pub fn sample(&self, s: f32, t: f32) -> [f32; 4] {
        let fx = s * self.w as f32 - 0.5;
        let fy = t * self.h as f32 - 0.5;
        let (x0, y0) = (floor(fx), floor(fy));
        let (ax, ay) = (fx - x0, fy - y0);
        let (x0, y0) = (x0 as i32, y0 as i32);
        // Wrap each row and column once, not once per texel.
        let (xa, xb) = (wrap(x0, self.w, self.wrap_s), wrap(x0 + 1, self.w, self.wrap_s));
        let (ya, yb) = (wrap(y0, self.h, self.wrap_t) * self.w, wrap(y0 + 1, self.h, self.wrap_t) * self.w);
        let at = |i: i32| self.texels.get(i as usize).unwrap_or(&[1.0; 4]);
        let (a, b, c, d) = (at(ya + xa), at(ya + xb), at(yb + xa), at(yb + xb));
        let mut o = [0.0; 4];
        for i in 0..4 {
            let top = a[i] + (b[i] - a[i]) * ax;
            let bot = c[i] + (d[i] - c[i]) * ax;
            o[i] = top + (bot - top) * ay;
        }
        o
    }
}

/// Textures a [`Shader`] reads by index.
pub trait Texs {
    fn tex(&self, i: usize) -> Option<&FastTex>;
}

impl Texs for [FastTex] {
    #[inline]
    fn tex(&self, i: usize) -> Option<&FastTex> {
        self.get(i)
    }
}

impl Texs for [Option<&FastTex>] {
    #[inline]
    fn tex(&self, i: usize) -> Option<&FastTex> {
        self.get(i).copied().flatten()
    }
}

#[derive(Clone)]
struct Stage {
    tex: Option<(usize, usize)>, // (texture index in model, texcoord)
    ras: u8,                     // 0 = COLOR0A0, 1 = COLOR1A1, 2 = zero
    st: TevStage,
    cscale: f32,
    cbias: f32,
    ascale: f32,
    abias: f32,
    /// What GX_CC_KONST and GX_CA_KONST read (1.0 unless set).
    kc: [f32; 3],
    ka: f32,
}

#[derive(Clone)]
pub struct Shader {
    regs: [[f32; 4]; 4],
    stages: Vec<Stage>,
    alpha_always: bool,
    mat_alpha_test: Material,
}

#[inline]
fn c01(v: f32) -> f32 {
    v.clamp(0.0, 1.0)
}

fn scale(s: u8) -> f32 {
    match s {
        1 => 2.0,
        2 => 4.0,
        3 => 0.5,
        _ => 1.0,
    }
}

fn bias(b: u8) -> f32 {
    match b {
        1 => 0.5,
        2 => -0.5,
        _ => 0.0,
    }
}

impl Shader {
    pub fn new(m: &Material) -> Shader {
        let mut regs = [[0.0f32; 4]; 4];
        for (i, c) in m.tev_colors.iter().enumerate() {
            if let Some(c) = c {
                let r = if i == 3 { 0 } else { i + 1 };
                regs[r] = c.map(|x| x as f32 / 255.0);
            }
        }
        let stages = m
            .tev_stages
            .iter()
            .enumerate()
            .map(|(i, st)| {
                let ord = m.tev_orders.get(i).copied();
                let tex = ord.and_then(|o| {
                    if o.texmap == 0xFF {
                        return None;
                    }
                    let ti = m.textures.get(o.texmap as usize).copied().flatten()?;
                    Some((ti as usize, o.texcoord as usize))
                });
                let ras = match ord.map(|o| o.channel) {
                    Some(0 | 2 | 4) => 0,
                    Some(1 | 3 | 5) => 1,
                    _ => 2,
                };
                Stage {
                    tex,
                    ras,
                    st: *st,
                    cscale: scale(st.color_scale),
                    cbias: bias(st.color_bias),
                    ascale: scale(st.alpha_scale),
                    abias: bias(st.alpha_bias),
                    kc: [1.0; 3],
                    ka: 1.0,
                }
            })
            .collect();
        let ac = m.alpha_compare;
        let alpha_always = (ac.op == 1 && (ac.comp0 == 7 || ac.comp1 == 7)) || (ac.op == 0 && ac.comp0 == 7 && ac.comp1 == 7);
        Shader { regs, stages, alpha_always, mat_alpha_test: m.clone() }
    }

    /// Set the konst color and alpha stage `i` reads (`GXSetTevKColorSel`,
    /// `GXSetTevKAlphaSel`).
    pub fn set_konst(&mut self, i: usize, c: [f32; 3], a: f32) {
        if let Some(s) = self.stages.get_mut(i) {
            s.kc = c;
            s.ka = a;
        }
    }

    /// Run the stages over the textures in `texs` (a model's, or GX's
    /// loaded maps).
    #[inline]
    pub fn eval<T: Texs + ?Sized>(&self, ras0: [f32; 4], uv: &[[f32; 2]], texs: &T) -> Option<[f32; 4]> {
        let mut regs = self.regs;
        let mut last = 0usize;
        for s in &self.stages {
            let t = match s.tex {
                Some((ti, tc)) => match (texs.tex(ti), uv.get(tc)) {
                    (Some(tx), Some(c)) => tx.sample(c[0], c[1]),
                    _ => [1.0; 4],
                },
                None => [1.0; 4],
            };
            let r = match s.ras {
                0 => ras0,
                1 => [1.0; 4],
                _ => [0.0; 4],
            };
            // Resolve each of the four inputs once per stage (the GX TEV
            // input selects), then do the lerp on all three channels.
            let cin = |id: u8, regs: &[[f32; 4]; 4]| -> [f32; 3] {
                let v = match id {
                    0 | 2 | 4 | 6 => regs[(id / 2) as usize],
                    1 | 3 | 5 | 7 => [regs[(id / 2) as usize][3]; 4],
                    8 => t,
                    9 => [t[3]; 4],
                    10 => r,
                    11 => [r[3]; 4],
                    12 => [1.0; 4],
                    13 => [0.5; 4],
                    14 => return s.kc,
                    _ => [0.0; 4],
                };
                [v[0], v[1], v[2]]
            };
            let ain = |id: u8, regs: &[[f32; 4]; 4]| -> f32 {
                match id {
                    0..=3 => regs[id as usize][3],
                    4 => t[3],
                    5 => r[3],
                    6 => s.ka,
                    _ => 0.0,
                }
            };
            let st = &s.st;
            let [ca, cb, cc, cd] = st.color_in.map(|id| cin(id, &regs));
            let sub = st.color_op & 1 == 1;
            let mut outc = [0.0f32; 3];
            for k in 0..3 {
                let (a, b, c, d) = (c01(ca[k]), c01(cb[k]), c01(cc[k]), cd[k]);
                let l = a * (1.0 - c) + b * c;
                let v = ((if sub { d - l } else { d + l }) + s.cbias) * s.cscale;
                outc[k] = if st.color_clamp { c01(v) } else { v.clamp(-4.0, 4.0) };
            }
            let (a, b, c, d) = (
                c01(ain(st.alpha_in[0], &regs)),
                c01(ain(st.alpha_in[1], &regs)),
                c01(ain(st.alpha_in[2], &regs)),
                ain(st.alpha_in[3], &regs),
            );
            let l = a * (1.0 - c) + b * c;
            let v = ((if st.alpha_op & 1 == 1 { d - l } else { d + l }) + s.abias) * s.ascale;
            let outa = if st.alpha_clamp { c01(v) } else { v.clamp(-4.0, 4.0) };
            let cr = (st.color_reg & 3) as usize;
            regs[cr][0] = outc[0];
            regs[cr][1] = outc[1];
            regs[cr][2] = outc[2];
            regs[(st.alpha_reg & 3) as usize][3] = outa;
            last = cr;
        }
        let o = regs[last].map(c01);
        if !self.alpha_always && !self.mat_alpha_test.alpha_test((o[3] * 255.0) as u8) {
            return None;
        }
        Some(o)
    }
}
