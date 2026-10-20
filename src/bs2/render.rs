//! Draws IPL models the way BS2's J3D renderer does: per-vertex GX
//! lighting, texgens, the material's TEV stages, alpha test, blending and
//! z mode, into a 640x448 GX-sized target.

use super::mtx::{self, Mtx};
use super::raster::{BlendMode, Prim, SVert, Sh, State, MAX_TC};
use super::shader::{FastTex, Shader};
use crate::ipl::j3d::{self, Model};
use alloc::vec::Vec;

pub const EFB_W: usize = 640;
pub const EFB_H: usize = 448;

/// Camera and projection for one frame.
pub struct View {
    pub view: Mtx,
    pub ortho: Option<[f32; 4]>, // top, bottom, left, right
    pub fovy: f32,
    pub aspect: f32,
    pub near: f32,
    pub far: f32,
    /// Light position in view space.
    pub light: [f32; 3],
    pub ambient: [f32; 4],
    /// Viewport size in pixels (the GX EFB is 640x448).
    pub vp: [f32; 2],
}

impl View {
    /// The intro camera: eye on +Z looking at (0, yoff, 0), fovy 20.
    pub fn intro(yoff: f32, ortho: bool) -> View {
        let fovy = 20.0f32;
        let dist = 224.0 / libm::tanf((fovy * 0.5).to_radians());
        let view = mtx::look_at([0.0, yoff, dist], [0.0, 1.0, 0.0], [0.0, yoff, 0.0]);
        let light = mtx::apply(&view, [400.0, 500.0, 200.0]);
        View {
            view,
            ortho: if ortho { Some([224.0, -224.0, -296.0, 292.0]) } else { None },
            fovy,
            aspect: 1.321_428_5,
            near: 50.0,
            far: 10000.0,
            light,
            ambient: [0.0, 0.0, 0.0, 0.0],
            vp: [EFB_W as f32, EFB_H as f32],
        }
    }

    /// View-space point to screen (x, y, depth 0..1, 1/w). None if clipped.
    pub fn project(&self, p: [f32; 3]) -> Option<[f32; 4]> {
        let d = -p[2];
        if d < self.near * 0.5 {
            return None;
        }
        let (nx, ny, rw) = match self.ortho {
            Some([t, b, l, r]) => ((2.0 * p[0] - (r + l)) / (r - l), (2.0 * p[1] - (t + b)) / (t - b), 1.0),
            None => {
                let c = 1.0 / libm::tanf((self.fovy * 0.5).to_radians());
                (c / self.aspect * p[0] / d, c * p[1] / d, 1.0 / d)
            }
        };
        let z = (d - self.near) / (self.far - self.near);
        Some([(nx + 1.0) * 0.5 * self.vp[0], (1.0 - ny) * 0.5 * self.vp[1], z, rw])
    }
}

/// A model with its materials compiled and textures converted once.
pub struct Prepared {
    pub model: Model,
    shaders: Vec<Shader>,
    texs: Vec<FastTex>,
}

impl Prepared {
    /// The compiled TEV program of material `i`.
    pub fn shader(&self, i: usize) -> Option<&Shader> {
        self.shaders.get(i)
    }

    pub fn new(model: Model) -> Prepared {
        let shaders = model.materials.iter().map(Shader::new).collect();
        let texs = model.textures.iter().map(FastTex::new).collect();
        Prepared { model, shaders, texs }
    }
}

/// Per-draw overrides.
pub struct DrawOpts<'a> {
    /// Model alpha (J3D model `alpha` field); below 255 multiplies alpha in
    /// an extra TEV stage and turns z compare and update off.
    pub alpha: u8,
    /// Joint matrices in model space; defaults to the bind pose.
    pub joints: Option<&'a [Mtx]>,
    /// Material color 0 overrides (material index, RGBA 0..255).
    pub mat_colors: &'a [(u16, [f32; 4])],
    /// Only draw these entries of `Model::draws`.
    pub only: Option<core::ops::Range<usize>>,
}

impl Default for DrawOpts<'_> {
    fn default() -> Self {
        DrawOpts { alpha: 255, joints: None, mat_colors: &[], only: None }
    }
}

fn norm(v: [f32; 3]) -> [f32; 3] {
    let l = libm::sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if l > 1e-12 { [v[0] / l, v[1] / l, v[2] / l] } else { v }
}

/// Queue a model's triangles. `base` is model-to-world (world matrix times
/// model scale).
pub fn draw_model<'a>(list: &mut Vec<Prim<'a>>, p: &'a Prepared, base: &Mtx, v: &View, o: &DrawOpts) {
    draw_model_with(list, p, None, base, v, o)
}

/// [`draw_model`] with every material's TEV program replaced by `shader`
/// (a copy with its own colour registers, see [`Shader::set_reg`]).
pub fn draw_model_with<'a>(list: &mut Vec<Prim<'a>>, p: &'a Prepared, shader: Option<&'a Shader>, base: &Mtx, v: &View, o: &DrawOpts) {
    let m = &p.model;
    if o.alpha == 0 {
        return;
    }
    let model_view = mtx::concat(&v.view, base);
    let range = o.only.clone().unwrap_or(0..m.draws.len());
    for d in m.draws.get(range).unwrap_or(&[]) {
        let Some(shape) = m.shapes.get(d.shape as usize) else { continue };
        let Some(mat0) = m.materials.get(d.material as usize) else { continue };
        let Some(shader) = shader.or(p.shaders.get(d.material as usize)) else { continue };
        let over = o.mat_colors.iter().find(|(i, _)| *i == d.material);
        let mut mat_over;
        let mat = match over {
            Some((_, c)) => {
                mat_over = mat0.clone();
                mat_over.mat_colors[0] = c.map(|x| x.clamp(0.0, 255.0) as u8);
                &mat_over
            }
            None => mat0,
        };
        let alpha = o.alpha as f32 / 255.0;
        let translucent = o.alpha != 255;
        let st = State {
            z_compare: !translucent && mat.z_mode.enable,
            z_update: !translucent && mat.z_mode.enable && mat.z_mode.update,
            blend: match mat.blend.mode {
                1 if mat.blend.dst == 1 => BlendMode::Add,
                1 => BlendMode::Alpha,
                _ if translucent => BlendMode::Alpha,
                _ => BlendMode::None,
            },
            cull: match mat.cull {
                j3d::CULL_FRONT => 1,
                j3d::CULL_BACK => 2,
                0 => 0,
                _ => continue,
            },
        };

        // Per-vertex transform, lighting and texgen, like the GX XF stage.
        let verts: Vec<Option<SVert>> = shape
            .vertices
            .iter()
            .map(|vx| {
                let jm = match o.joints {
                    Some(js) => js.get(vx.joint as usize).copied().unwrap_or(mtx::IDENTITY),
                    None => m.joint_world(vx.joint),
                };
                let mv = mtx::concat(&model_view, &jm);
                let p = mtx::apply(&mv, vx.pos);
                let s = v.project(p)?;
                let n = norm(mtx::apply_dir(&mv, vx.normal));
                let l = norm([v.light[0] - p[0], v.light[1] - p[1], v.light[2] - p[2]]);
                let diff = (n[0] * l[0] + n[1] * l[1] + n[2] * l[2]).max(0.0);
                let vc = vx.color.map(|c| c as f32 / 255.0);
                let ras = mat.ras_color(0, vc, [diff; 3], v.ambient);
                let mut uv = [[0.0; 2]; MAX_TC];
                for (g, out) in uv.iter_mut().enumerate() {
                    // J3D env mapping (texmtx mode 1): the stored matrix is
                    // applied to the view-space normal.
                    let env = mat.tex_gens.get(g).is_some_and(|tg| tg.src == 1);
                    *out = mat.texcoord(g, vx.pos, if env { n } else { vx.normal }, vx.uv);
                }
                Some(SVert { x: s[0], y: s[1], z: s[2], rw: s[3], color: ras, uv })
            })
            .collect();

        let sh = Sh::Mat { shader, texs: &p.texs, alpha };
        for tri in shape.indices.chunks_exact(3) {
            let (Some(a), Some(b), Some(c)) = (
                verts.get(tri[0] as usize).copied().flatten(),
                verts.get(tri[1] as usize).copied().flatten(),
                verts.get(tri[2] as usize).copied().flatten(),
            ) else {
                continue;
            };
            list.push(Prim { v: [a, b, c], st, sh });
        }
    }
}

/// Joint matrices for an animated pose: `local(j)` gives each joint's
/// (scale, rotation, translation); parents are composed in order.
pub fn pose(m: &Model, local: impl Fn(usize) -> Option<([f32; 3], [i16; 3], [f32; 3])>) -> Vec<Mtx> {
    let mut out: Vec<Option<Mtx>> = alloc::vec![None; m.joints.len()];
    fn resolve(m: &Model, i: usize, out: &mut Vec<Option<Mtx>>, local: &dyn Fn(usize) -> Option<([f32; 3], [i16; 3], [f32; 3])>, depth: u32) -> Mtx {
        if let Some(x) = out[i] {
            return x;
        }
        let j = &m.joints[i];
        let l = match local(i) {
            Some((s, r, t)) => j3d::srt_mtx(s, r, t),
            None => j.local(),
        };
        let w = match j.parent {
            Some(p) if (p as usize) < m.joints.len() && depth < 64 => mtx::concat(&resolve(m, p as usize, out, local, depth + 1), &l),
            _ => l,
        };
        out[i] = Some(w);
        w
    }
    for i in 0..m.joints.len() {
        resolve(m, i, &mut out, &local, 0);
    }
    out.into_iter().map(|x| x.unwrap_or(mtx::IDENTITY)).collect()
}
