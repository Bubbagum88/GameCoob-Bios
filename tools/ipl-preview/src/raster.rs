//! A small CPU rasterizer that draws a parsed J3D model the way GX would:
//! per-vertex lighting and texgen, perspective-correct interpolation, TEV via
//! `Material::shade`, alpha test, z-mode and blend mode.

use crate::ipl::gx::Texture;
use crate::ipl::j3d::{self, Material, Model, Shape};

type V3 = [f32; 3];

fn sub(a: V3, b: V3) -> V3 {
    [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
}
fn dot(a: V3, b: V3) -> f32 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}
fn cross(a: V3, b: V3) -> V3 {
    [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
}
fn norm(a: V3) -> V3 {
    let l = dot(a, a).sqrt();
    if l > 1e-12 { [a[0] / l, a[1] / l, a[2] / l] } else { a }
}

/// GX treats triangles that are clockwise on screen as front-facing.
/// (Checked with closed IPL meshes: their GX-order triangles are clockwise
/// seen from outside, and they are drawn with CULL_BACK.)
const FRONT_IS_CW: bool = true;

/// Fraction of a shape's triangles whose right-hand normal (counter-clockwise
/// seen from that side) points away from the shape's centre. ~1.0 for a
/// closed mesh with CCW-outward winding, ~0.0 for CW-outward.
pub fn outward_ratio(m: &Model, s: &Shape) -> f32 {
    let pts: Vec<V3> = s.vertices.iter().map(|v| m.world_pos(v)).collect();
    if pts.is_empty() || s.indices.is_empty() {
        return 0.0;
    }
    let mut c = [0.0; 3];
    for p in &pts {
        for k in 0..3 {
            c[k] += p[k] / pts.len() as f32;
        }
    }
    let mut out = 0;
    let tris = s.indices.chunks(3);
    let n = tris.len();
    for t in tris {
        let (a, b, d) = (pts[t[0] as usize], pts[t[1] as usize], pts[t[2] as usize]);
        let nn = cross(sub(b, a), sub(d, a));
        let cen = [(a[0] + b[0] + d[0]) / 3.0, (a[1] + b[1] + d[1]) / 3.0, (a[2] + b[2] + d[2]) / 3.0];
        if dot(nn, sub(cen, c)) > 0.0 {
            out += 1;
        }
    }
    out as f32 / n as f32
}

struct Camera {
    eye: V3,
    right: V3,
    up: V3,
    fwd: V3,
    focal: f32,
}

impl Camera {
    fn to_view(&self, p: V3) -> V3 {
        let d = sub(p, self.eye);
        [dot(d, self.right), dot(d, self.up), dot(d, self.fwd)]
    }
    fn dir_to_view(&self, d: V3) -> V3 {
        [dot(d, self.right), dot(d, self.up), dot(d, self.fwd)]
    }
}

#[derive(Clone, Copy, Default)]
struct PV {
    sx: f32,
    sy: f32,
    inv_w: f32,
    ras: [f32; 4],
    tc: [[f32; 2]; 2],
}

/// Render `m` from a 3/4 view framed to its bounds.
pub fn render_model(m: &Model, w: usize, h: usize) -> Vec<u8> {
    let (lo, hi) = m.bounds();
    let c = [(lo[0] + hi[0]) * 0.5, (lo[1] + hi[1]) * 0.5, (lo[2] + hi[2]) * 0.5];
    let r = (dot(sub(hi, lo), sub(hi, lo))).sqrt() * 0.5;
    let r = if r > 0.0 { r } else { 1.0 };
    let fov = 40f32.to_radians();
    let dir = norm([0.55, 0.45, 0.75]);
    let dist = r / (fov * 0.5).sin() * 1.02;
    let eye = [c[0] + dir[0] * dist, c[1] + dir[1] * dist, c[2] + dir[2] * dist];
    let fwd = norm(sub(c, eye));
    let right = norm(cross(fwd, [0.0, 1.0, 0.0]));
    let up = cross(right, fwd);
    let cam = Camera { eye, right, up, fwd, focal: (h as f32 * 0.5) / (fov * 0.5).tan() };

    let mut color = vec![0f32; w * h * 4];
    for y in 0..h {
        for x in 0..w {
            let g = 0.10 + 0.06 * (y as f32 / h as f32);
            let o = (y * w + x) * 4;
            color[o..o + 4].copy_from_slice(&[g, g, g * 1.2, 1.0]);
        }
    }
    let mut depth = vec![f32::INFINITY; w * h];
    let light_view = norm([-0.4, 0.6, -0.7]); // towards the light, view space (z forward)
    let ambient = [0.35, 0.35, 0.35, 1.0];

    // Opaque draws first, then translucent ones, each in INF1 order.
    let mut order: Vec<&j3d::Draw> = m.draws.iter().collect();
    order.sort_by_key(|d| m.materials.get(d.material as usize).map(|mt| (mt.blend.mode == 1) as u8).unwrap_or(0));
    for d in order {
        let shape = &m.shapes[d.shape as usize];
        let mut mat = m.materials.get(d.material as usize).cloned().unwrap_or_default();
        // The IPL fades some materials in at runtime from alpha 0; show them.
        for mc in mat.mat_colors.iter_mut() {
            if mc[3] == 0 {
                mc[3] = 255;
            }
        }
        let pv: Vec<Option<PV>> = shape
            .vertices
            .iter()
            .map(|v| {
                let wm = m.joint_world(v.joint);
                let wp = j3d::mtx_point(&wm, v.pos);
                let vp = cam.to_view(wp);
                if vp[2] < r * 0.01 {
                    return None;
                }
                let wn = norm(j3d::mtx_dir(&wm, v.normal));
                let vn = cam.dir_to_view(wn);
                let diff = dot(vn, light_view).max(0.0);
                let vc = [v.color[0] as f32 / 255.0, v.color[1] as f32 / 255.0, v.color[2] as f32 / 255.0, v.color[3] as f32 / 255.0];
                let ras = mat.ras_color(0, vc, [diff; 3], ambient);
                let mut tc = [[0.0; 2]; 2];
                for (g, t) in tc.iter_mut().enumerate() {
                    // Mode-1 texture matrices on normal-sourced coords: use the
                    // view-space normal (env-map hypothesis; GX view space looks down -z).
                    let n = match mat.tex_gens.get(g) {
                        Some(tg) if tg.src == 1 => [vn[0], vn[1], -vn[2]],
                        _ => v.normal,
                    };
                    *t = mat.texcoord(g, v.pos, n, v.uv);
                }
                Some(PV { sx: w as f32 * 0.5 + vp[0] / vp[2] * cam.focal, sy: h as f32 * 0.5 - vp[1] / vp[2] * cam.focal, inv_w: 1.0 / vp[2], ras, tc })
            })
            .collect();
        for t in shape.indices.chunks(3) {
            let (Some(a), Some(b), Some(cc)) = (pv[t[0] as usize], pv[t[1] as usize], pv[t[2] as usize]) else { continue };
            let area = (b.sx - a.sx) * (cc.sy - a.sy) - (cc.sx - a.sx) * (b.sy - a.sy);
            if area.abs() < 1e-9 {
                continue;
            }
            // Screen y points down, so area > 0 is clockwise on screen.
            let front = (area > 0.0) == FRONT_IS_CW;
            let culled = match mat.cull {
                j3d::CULL_FRONT => front,
                j3d::CULL_BACK => !front,
                j3d::CULL_ALL => true,
                _ => false,
            };
            if culled {
                continue;
            }
            raster_tri(&mut color, &mut depth, w, h, [a, b, cc], area, &mat, &m.textures);
        }
    }
    color.iter().map(|v| (v.clamp(0.0, 1.0) * 255.0 + 0.5) as u8).collect()
}

#[allow(clippy::too_many_arguments)]
fn raster_tri(color: &mut [f32], depth: &mut [f32], w: usize, h: usize, v: [PV; 3], area: f32, mat: &Material, texs: &[Texture]) {
    let minx = v.iter().map(|p| p.sx).fold(f32::MAX, f32::min).floor().max(0.0) as usize;
    let maxx = (v.iter().map(|p| p.sx).fold(f32::MIN, f32::max).ceil() as isize).min(w as isize - 1);
    let miny = v.iter().map(|p| p.sy).fold(f32::MAX, f32::min).floor().max(0.0) as usize;
    let maxy = (v.iter().map(|p| p.sy).fold(f32::MIN, f32::max).ceil() as isize).min(h as isize - 1);
    if maxx < 0 || maxy < 0 {
        return;
    }
    let edge = |a: &PV, b: &PV, x: f32, y: f32| (b.sx - a.sx) * (y - a.sy) - (x - a.sx) * (b.sy - a.sy);
    for y in miny..=maxy as usize {
        for x in minx..=maxx as usize {
            let (px, py) = (x as f32 + 0.5, y as f32 + 0.5);
            let w0 = edge(&v[1], &v[2], px, py) / area;
            let w1 = edge(&v[2], &v[0], px, py) / area;
            let w2 = 1.0 - w0 - w1;
            if w0 < 0.0 || w1 < 0.0 || w2 < 0.0 {
                continue;
            }
            let iw = w0 * v[0].inv_w + w1 * v[1].inv_w + w2 * v[2].inv_w;
            let z = 1.0 / iw;
            let i = y * w + x;
            let zm = mat.z_mode;
            if zm.enable {
                let d = depth[i];
                let pass = match zm.func {
                    0 => false,
                    1 => z < d,
                    2 => z == d,
                    3 => z <= d,
                    4 => z > d,
                    5 => z != d,
                    6 => z >= d,
                    _ => true,
                };
                if !pass {
                    continue;
                }
            }
            let (p0, p1, p2) = (w0 * v[0].inv_w * z, w1 * v[1].inv_w * z, w2 * v[2].inv_w * z);
            let lerp4 = |f: fn(&PV) -> [f32; 4]| {
                let (a, b, c) = (f(&v[0]), f(&v[1]), f(&v[2]));
                [0, 1, 2, 3].map(|k| a[k] * p0 + b[k] * p1 + c[k] * p2)
            };
            let ras = lerp4(|p| p.ras);
            let tcs = [0, 1].map(|g| [0, 1].map(|k| v[0].tc[g][k] * p0 + v[1].tc[g][k] * p1 + v[2].tc[g][k] * p2));
            let mut tex = |map: u8, tc: u8| -> [f32; 4] {
                let Some(ti) = mat.textures.get(map as usize).copied().flatten() else { return [1.0; 4] };
                let Some(t) = texs.get(ti as usize) else { return [1.0; 4] };
                let uv = tcs.get(tc as usize).copied().unwrap_or([0.0, 0.0]);
                t.sample_bilinear(uv[0], uv[1])
            };
            let src = mat.shade([ras, [1.0; 4]], &mut tex);
            if !mat.alpha_test((src[3] * 255.0) as u8) {
                continue;
            }
            let o = i * 4;
            let dst = [color[o], color[o + 1], color[o + 2], color[o + 3]];
            let b = mat.blend;
            let out = match b.mode {
                1 => {
                    let fs = factor(b.src, src, dst, true);
                    let fd = factor(b.dst, src, dst, false);
                    [0, 1, 2, 3].map(|k| src[k] * fs[k] + dst[k] * fd[k])
                }
                3 => [0, 1, 2, 3].map(|k| (dst[k] - src[k]).max(0.0)),
                _ => src,
            };
            color[o..o + 4].copy_from_slice(&out);
            if zm.enable && zm.update {
                depth[i] = z;
            }
        }
    }
}

fn factor(f: u8, src: [f32; 4], dst: [f32; 4], is_src: bool) -> [f32; 4] {
    let other = if is_src { dst } else { src };
    match f {
        0 => [0.0; 4],
        1 => [1.0; 4],
        2 => other,
        3 => other.map(|x| 1.0 - x),
        4 => [src[3]; 4],
        5 => [1.0 - src[3]; 4],
        6 => [dst[3]; 4],
        _ => [1.0 - dst[3]; 4],
    }
}

/// Lay textures out 4 per row in 264x136 cells over a checkerboard.
pub fn contact_sheet(texs: &[(String, Texture)]) -> (usize, usize, Vec<u8>) {
    let (cw, ch, per_row) = (264usize, 136usize, 4usize);
    let rows = texs.len().div_ceil(per_row).max(1);
    let (w, h) = (cw * per_row, ch * rows);
    let mut img = vec![0u8; w * h * 4];
    for y in 0..h {
        for x in 0..w {
            let v = if ((x / 8) + (y / 8)) % 2 == 0 { 90 } else { 140 };
            img[(y * w + x) * 4..(y * w + x) * 4 + 4].copy_from_slice(&[v, v, v, 255]);
        }
    }
    for (n, (_, t)) in texs.iter().enumerate() {
        let (ox, oy) = ((n % per_row) * cw + 4, (n / per_row) * ch + 4);
        let (tw, th) = (t.width as usize, t.height as usize);
        let s = ((cw - 8) as f32 / tw as f32).min((ch - 8) as f32 / th as f32).min(4.0);
        let (dw, dh) = ((tw as f32 * s) as usize, (th as f32 * s) as usize);
        for y in 0..dh {
            for x in 0..dw {
                let (sx, sy) = (((x as f32 / s) as usize).min(tw - 1), ((y as f32 / s) as usize).min(th - 1));
                let p = &t.rgba[(sy * tw + sx) * 4..(sy * tw + sx) * 4 + 4];
                let o = ((oy + y) * w + ox + x) * 4;
                let a = p[3] as f32 / 255.0;
                for k in 0..3 {
                    img[o + k] = (p[k] as f32 * a + img[o + k] as f32 * (1.0 - a)) as u8;
                }
            }
        }
    }
    (w, h, img)
}
