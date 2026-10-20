//! J3D model (`.bmd`) parsing into renderable data.
//!
//! Every model in the known IPL dumps is the early `J3D1bmd1` format with
//! sections INF1, VTX1, EVP1, DRW1, JNT1, SHP1, MAT1, TEX1. All but MAT1 share
//! their layout with the later, well documented `J3D2bmd3` format. MAT1 was
//! worked out from the IPL's own models (see [`Material`]); `J3D2` files are
//! accepted for geometry, joints and textures, but their MAT3 materials are
//! not decoded (each material is then [`Material::default`]).
//!
//! Positions and normals of a [`Shape`] are in the space of the joint named
//! by each [`Vertex::joint`]; [`Joint::world`] is that joint's bind-pose
//! matrix, so `world * pos` places the vertex in model space. [`Model::draws`]
//! lists the shapes in INF1 scene-graph order with their material and joint.

use super::bti;
use super::gx::Texture;
use super::read::{at, be16, be32, be8, bef32, bei16, bytes};
use super::{Error, Result};
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;

/// A 3x4 row-major affine matrix (`m[row][col]`, column 3 = translation).
pub type Mtx = [[f32; 4]; 3];

/// Limits that keep corrupt input from exhausting memory (the largest IPL
/// model has 21 shapes and under 3000 vertices).
pub const MAX_ITEMS: usize = 4096;
pub const MAX_VERTICES: usize = 1 << 18;
pub const MAX_TEXELS: usize = 1 << 22;

fn count_at(b: &[u8], off: usize) -> Result<usize> {
    let n = be16(b, off)? as usize;
    if n > MAX_ITEMS {
        return Err(Error::BadValue(off));
    }
    Ok(n)
}

pub const IDENTITY: Mtx = [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0]];

/// `a * b` for affine 3x4 matrices.
pub fn mtx_mul(a: &Mtx, b: &Mtx) -> Mtx {
    let mut o = [[0.0; 4]; 3];
    for (r, row) in o.iter_mut().enumerate() {
        for (c, v) in row.iter_mut().enumerate() {
            *v = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c];
            if c == 3 {
                *v += a[r][3];
            }
        }
    }
    o
}

/// Transform a point.
pub fn mtx_point(m: &Mtx, p: [f32; 3]) -> [f32; 3] {
    let f = |r: usize| m[r][0] * p[0] + m[r][1] * p[1] + m[r][2] * p[2] + m[r][3];
    [f(0), f(1), f(2)]
}

/// Transform a direction (no translation, not renormalized).
pub fn mtx_dir(m: &Mtx, d: [f32; 3]) -> [f32; 3] {
    let f = |r: usize| m[r][0] * d[0] + m[r][1] * d[1] + m[r][2] * d[2];
    [f(0), f(1), f(2)]
}

/// J3D joint matrix: translate * Rz * Ry * Rx * scale. Angles are s16 with
/// 0x8000 = 180 degrees.
pub fn srt_mtx(scale: [f32; 3], rot: [i16; 3], trans: [f32; 3]) -> Mtx {
    let ang = |a: i16| a as f32 * (core::f32::consts::PI / 32768.0);
    let (sx, cx) = (libm::sinf(ang(rot[0])), libm::cosf(ang(rot[0])));
    let (sy, cy) = (libm::sinf(ang(rot[1])), libm::cosf(ang(rot[1])));
    let (sz, cz) = (libm::sinf(ang(rot[2])), libm::cosf(ang(rot[2])));
    let r = [
        [cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz],
        [cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz],
        [-sy, sx * cy, cx * cy],
    ];
    let mut m = [[0.0; 4]; 3];
    for i in 0..3 {
        for j in 0..3 {
            m[i][j] = r[i][j] * scale[j];
        }
        m[i][3] = trans[i];
    }
    m
}

/// One JNT1 joint.
#[derive(Clone, Debug)]
pub struct Joint {
    pub name: String,
    /// Parent joint from the INF1 hierarchy (`None` for a root).
    pub parent: Option<u16>,
    /// Raw u16 matrix-type / flags word.
    pub flags: u16,
    pub scale: [f32; 3],
    /// s16 Euler angles, 0x8000 = 180 degrees, applied X then Y then Z.
    pub rotation: [i16; 3],
    pub translation: [f32; 3],
    pub radius: f32,
    pub bbox_min: [f32; 3],
    pub bbox_max: [f32; 3],
    /// Bind-pose model-space matrix (parent world * local SRT).
    pub world: Mtx,
}

impl Joint {
    /// The joint's own SRT matrix (relative to its parent).
    pub fn local(&self) -> Mtx {
        srt_mtx(self.scale, self.rotation, self.translation)
    }
}

/// An expanded vertex.
#[derive(Clone, Copy, Debug, Default)]
pub struct Vertex {
    /// In the space of [`Vertex::joint`].
    pub pos: [f32; 3],
    /// Zero when the shape has no normals.
    pub normal: [f32; 3],
    /// White when the shape has no COLOR0.
    pub color: [u8; 4],
    /// TEX0 (zero when absent).
    pub uv: [f32; 2],
    /// Joint whose [`Joint::world`] places this vertex.
    pub joint: u16,
}

/// One SHP1 shape as a plain triangle list.
#[derive(Clone, Debug)]
pub struct Shape {
    /// 0 = normal, 1 = billboard, 2 = Y billboard, 3 = multi-matrix.
    pub matrix_type: u8,
    pub vertices: Vec<Vertex>,
    /// Triangles, three indices each, winding as GX would see it.
    pub indices: Vec<u32>,
    pub has_normal: bool,
    pub has_color: bool,
    pub has_uv: bool,
    /// Joint of the first packet (all IPL shapes have one packet).
    pub joint: u16,
    pub bbox_min: [f32; 3],
    pub bbox_max: [f32; 3],
}

/// Lighting setup of one color channel (`GXSetChanCtrl`).
#[derive(Clone, Copy, Debug, Default)]
pub struct ColorChan {
    pub lighting: bool,
    /// Material source: 0 = material color register, 1 = vertex color.
    pub mat_src: u8,
    pub light_mask: u8,
    /// 0 = none, 1 = signed, 2 = clamp.
    pub diffuse_fn: u8,
    /// 0 = specular, 1 = spot, 2 = none.
    pub attn_fn: u8,
    /// Ambient source (0xFF in every IPL MAT1: none stored).
    pub amb_src: u8,
}

/// A texture coordinate generator (`GXSetTexCoordGen`).
#[derive(Clone, Copy, Debug)]
pub struct TexGen {
    /// 0 = MTX3x4, 1 = MTX2x4, 2..9 = bump, 10 = SRTG.
    pub kind: u8,
    /// 0 = position, 1 = normal, 4..11 = TEX0..TEX7, ...
    pub src: u8,
    /// 30 + 3n = TEXMTXn, 60 = identity.
    pub mtx: u8,
}

/// A MAT1 texture matrix: a raw 3x4 matrix plus a mode byte.
#[derive(Clone, Copy, Debug)]
pub struct TexMtx {
    pub m: Mtx,
    /// Every IPL texture matrix has 1 here; probably J3D's "env-map basic"
    /// mode (normal-sourced coords taken in view space). Unconfirmed.
    pub mode: u8,
}

/// Which texcoord, texture map and raster channel a TEV stage reads.
#[derive(Clone, Copy, Debug)]
pub struct TevOrder {
    pub texcoord: u8,
    pub texmap: u8,
    /// 4 = COLOR0A0, 5 = COLOR1A1, 0xFF = none.
    pub channel: u8,
}

/// One TEV stage.
#[derive(Clone, Copy, Debug)]
pub struct TevStage {
    /// a, b, c, d of `d + lerp(a, b, c)`, GX_CC_* ids.
    pub color_in: [u8; 4],
    pub color_op: u8,
    pub color_bias: u8,
    pub color_scale: u8,
    pub color_clamp: bool,
    pub color_reg: u8,
    /// GX_CA_* ids.
    pub alpha_in: [u8; 4],
    pub alpha_op: u8,
    pub alpha_bias: u8,
    pub alpha_scale: u8,
    pub alpha_clamp: bool,
    pub alpha_reg: u8,
}

/// `GXSetAlphaCompare`.
#[derive(Clone, Copy, Debug)]
pub struct AlphaCompare {
    pub comp0: u8,
    pub ref0: u8,
    /// 0 = AND, 1 = OR, 2 = XOR, 3 = XNOR.
    pub op: u8,
    pub comp1: u8,
    pub ref1: u8,
}

/// `GXSetBlendMode`.
#[derive(Clone, Copy, Debug)]
pub struct Blend {
    /// 0 = none, 1 = blend, 2 = logic, 3 = subtract.
    pub mode: u8,
    /// 0 = ZERO, 1 = ONE, 2 = SRCCLR/DSTCLR, 3 = INV.., 4 = SRCALPHA, 5 = INVSRCALPHA, 6 = DSTALPHA, 7 = INVDSTALPHA.
    pub src: u8,
    pub dst: u8,
    pub logic_op: u8,
}

/// `GXSetZMode`.
#[derive(Clone, Copy, Debug)]
pub struct ZMode {
    pub enable: bool,
    /// GX_NEVER..GX_ALWAYS (3 = LEQUAL).
    pub func: u8,
    pub update: bool,
}

/// GX cull modes.
pub const CULL_NONE: u8 = 0;
pub const CULL_FRONT: u8 = 1;
pub const CULL_BACK: u8 = 2;
pub const CULL_ALL: u8 = 3;

/// A material, decoded from a 0x98-byte MAT1 entry and its shared tables.
///
/// Confirmed from data: table order and all value layouts (they match the
/// documented MAT3 value tables). Inferred: the entry field order (MAT3's
/// minus ambient colors, lights, post-tex-gens, konst colors and swap modes),
/// the meaning of [`TexMtx::mode`], and that `tev_colors[0..3]` load
/// GX_TEVREG0..2 and `tev_colors[3]` loads TEVPREV.
#[derive(Clone, Debug)]
pub struct Material {
    pub name: String,
    /// 1 = opaque, 2 = alpha-tested, 4 = translucent (J3D draw-list flag).
    pub flag: u8,
    /// [`CULL_NONE`], [`CULL_FRONT`], [`CULL_BACK`], [`CULL_ALL`].
    pub cull: u8,
    /// Number of color channels (`GXSetNumChans`).
    pub chan_count: u8,
    /// COLOR0, ALPHA0, COLOR1, ALPHA1.
    pub chans: [Option<ColorChan>; 4],
    /// Material color register of channel 0 and 1.
    pub mat_colors: [[u8; 4]; 2],
    pub tex_gens: Vec<TexGen>,
    /// TEXMTX0..7.
    pub tex_mtx: [Option<TexMtx>; 8],
    /// Texture of GX_TEXMAP0..7 (index into [`Model::textures`]).
    pub textures: [Option<u16>; 8],
    /// One per stage.
    pub tev_orders: Vec<TevOrder>,
    /// s16 RGBA loaded into TEVREG0, TEVREG1, TEVREG2 and TEVPREV.
    pub tev_colors: [Option<[i16; 4]>; 4],
    pub tev_stages: Vec<TevStage>,
    pub alpha_compare: AlphaCompare,
    pub blend: Blend,
    pub z_mode: ZMode,
    pub z_comp_loc: bool,
    pub dither: bool,
    /// The raw MAT1 entry (0x98 bytes) for fields not decoded here.
    pub raw: Vec<u8>,
}

impl Default for Material {
    fn default() -> Self {
        Material {
            name: String::new(),
            flag: 1,
            cull: CULL_BACK,
            chan_count: 1,
            chans: [Some(ColorChan { diffuse_fn: 2, attn_fn: 2, amb_src: 0xFF, ..Default::default() }), None, None, None],
            mat_colors: [[255; 4]; 2],
            tex_gens: Vec::new(),
            tex_mtx: [None; 8],
            textures: [None; 8],
            tev_orders: vec![TevOrder { texcoord: 0xFF, texmap: 0xFF, channel: 4 }],
            tev_colors: [None; 4],
            tev_stages: vec![TevStage {
                color_in: [15, 15, 15, 10],
                color_op: 0,
                color_bias: 0,
                color_scale: 0,
                color_clamp: true,
                color_reg: 0,
                alpha_in: [7, 7, 7, 5],
                alpha_op: 0,
                alpha_bias: 0,
                alpha_scale: 0,
                alpha_clamp: true,
                alpha_reg: 0,
            }],
            alpha_compare: AlphaCompare { comp0: 7, ref0: 0, op: 1, comp1: 7, ref1: 0 },
            blend: Blend { mode: 0, src: 1, dst: 0, logic_op: 3 },
            z_mode: ZMode { enable: true, func: 3, update: true },
            z_comp_loc: true,
            dither: false,
            raw: Vec::new(),
        }
    }
}

fn clamp01(v: f32) -> f32 {
    v.clamp(0.0, 1.0)
}

impl Material {
    /// First texture (GX_TEXMAP0's), if any.
    pub fn texture(&self) -> Option<u16> {
        self.textures[0]
    }

    /// Whether channel 0 (COLOR0) is lit.
    pub fn lighting(&self) -> bool {
        self.chans[0].is_some_and(|c| c.lighting)
    }

    /// Rasterized color of channel `chan` (0 or 1), 0..1 RGBA.
    /// `vertex` is the vertex color, `light` the summed diffuse light
    /// (only used by lit channels) and `ambient` the ambient color, which
    /// MAT1 does not store.
    pub fn ras_color(&self, chan: usize, vertex: [f32; 4], light: [f32; 3], ambient: [f32; 4]) -> [f32; 4] {
        let mut out = [1.0; 4];
        for (part, range) in [(0usize, 0..3usize), (1, 3..4)] {
            let Some(c) = self.chans.get(chan * 2 + part).copied().flatten() else { continue };
            let reg = self.mat_colors.get(chan).copied().unwrap_or([255; 4]);
            for i in range {
                let mat = if c.mat_src == 1 { vertex[i] } else { reg[i] as f32 / 255.0 };
                out[i] = if c.lighting {
                    let l = if i < 3 { light[i] } else { light[0] };
                    mat * clamp01(ambient[i] + l)
                } else {
                    mat
                };
            }
        }
        out
    }

    /// Texture coordinate `gen` from position, normal and TEX0, through its
    /// TexGen and texture matrix. The caller picks the space of `nrm` (the
    /// object-space normal is what GX feeds; see [`TexMtx::mode`]).
    pub fn texcoord(&self, gen_idx: usize, pos: [f32; 3], nrm: [f32; 3], uv0: [f32; 2]) -> [f32; 2] {
        let Some(g) = self.tex_gens.get(gen_idx) else { return uv0 };
        let v = match g.src {
            0 => [pos[0], pos[1], pos[2], 1.0],
            1 => [nrm[0], nrm[1], nrm[2], 1.0],
            _ => [uv0[0], uv0[1], 1.0, 1.0],
        };
        let m = if g.mtx >= 30 && g.mtx < 60 && (g.mtx - 30) % 3 == 0 {
            self.tex_mtx[((g.mtx - 30) / 3) as usize].map(|t| t.m)
        } else {
            None
        };
        let Some(m) = m else { return [v[0], v[1]] };
        let row = |r: usize| m[r][0] * v[0] + m[r][1] * v[1] + m[r][2] * v[2] + m[r][3] * v[3];
        let (s, t) = (row(0), row(1));
        if g.kind == 0 {
            let q = row(2);
            if q.abs() > 1e-6 {
                return [s / q, t / q];
            }
        }
        [s, t]
    }

    /// Evaluate the TEV stages in floating point. `ras` holds the two
    /// channel colors from [`Material::ras_color`]; `tex(texmap, texcoord)`
    /// samples a texture (return white for none). Konst inputs read as 1.0
    /// (MAT1 stores no konst colors; the hardware default selection is 1).
    pub fn shade(&self, ras: [[f32; 4]; 2], tex: &mut dyn FnMut(u8, u8) -> [f32; 4]) -> [f32; 4] {
        // regs: PREV, REG0, REG1, REG2
        let mut regs = [[0.0f32; 4]; 4];
        for (i, c) in self.tev_colors.iter().enumerate() {
            if let Some(c) = c {
                let r = if i == 3 { 0 } else { i + 1 };
                regs[r] = [c[0] as f32 / 255.0, c[1] as f32 / 255.0, c[2] as f32 / 255.0, c[3] as f32 / 255.0];
            }
        }
        let mut last = 0usize;
        for (i, st) in self.tev_stages.iter().enumerate() {
            let ord = self.tev_orders.get(i).copied().unwrap_or(TevOrder { texcoord: 0xFF, texmap: 0xFF, channel: 0xFF });
            let t = if ord.texmap == 0xFF { [1.0; 4] } else { tex(ord.texmap, ord.texcoord) };
            let r = match ord.channel {
                0 | 2 | 4 => ras[0],
                1 | 3 | 5 => ras[1],
                _ => [0.0; 4],
            };
            let cc = |id: u8, regs: &[[f32; 4]; 4]| -> [f32; 3] {
                let rgb = |c: [f32; 4]| [c[0], c[1], c[2]];
                let aaa = |c: [f32; 4]| [c[3], c[3], c[3]];
                match id {
                    0 => rgb(regs[0]),
                    1 => aaa(regs[0]),
                    2 => rgb(regs[1]),
                    3 => aaa(regs[1]),
                    4 => rgb(regs[2]),
                    5 => aaa(regs[2]),
                    6 => rgb(regs[3]),
                    7 => aaa(regs[3]),
                    8 => rgb(t),
                    9 => aaa(t),
                    10 => rgb(r),
                    11 => aaa(r),
                    12 | 14 => [1.0; 3],
                    13 => [0.5; 3],
                    _ => [0.0; 3],
                }
            };
            let ca = |id: u8, regs: &[[f32; 4]; 4]| -> f32 {
                match id {
                    0 => regs[0][3],
                    1 => regs[1][3],
                    2 => regs[2][3],
                    3 => regs[3][3],
                    4 => t[3],
                    5 => r[3],
                    6 => 1.0,
                    _ => 0.0,
                }
            };
            let combine = |a: f32, b: f32, c: f32, d: f32, op: u8, bias: u8, scale: u8, clamp: bool| -> f32 {
                let (a, b, c) = (clamp01(a), clamp01(b), clamp01(c));
                let l = a * (1.0 - c) + b * c;
                let mut v = if op & 1 == 1 { d - l } else { d + l };
                v += match bias {
                    1 => 0.5,
                    2 => -0.5,
                    _ => 0.0,
                };
                v *= match scale {
                    1 => 2.0,
                    2 => 4.0,
                    3 => 0.5,
                    _ => 1.0,
                };
                if clamp { clamp01(v) } else { v.clamp(-4.0, 4.0) }
            };
            let ci = st.color_in.map(|id| cc(id, &regs));
            let ai = st.alpha_in.map(|id| ca(id, &regs));
            let mut outc = [0.0f32; 3];
            for (k, o) in outc.iter_mut().enumerate() {
                *o = combine(ci[0][k], ci[1][k], ci[2][k], ci[3][k], st.color_op, st.color_bias, st.color_scale, st.color_clamp);
            }
            let outa = combine(ai[0], ai[1], ai[2], ai[3], st.alpha_op, st.alpha_bias, st.alpha_scale, st.alpha_clamp);
            let cr = (st.color_reg & 3) as usize;
            regs[cr][..3].copy_from_slice(&outc);
            regs[(st.alpha_reg & 3) as usize][3] = outa;
            last = cr;
        }
        let o = regs[last];
        [clamp01(o[0]), clamp01(o[1]), clamp01(o[2]), clamp01(o[3])]
    }

    /// Does the alpha test pass for fragment alpha `a` (0..255)?
    pub fn alpha_test(&self, a: u8) -> bool {
        let cmp = |c: u8, r: u8| match c {
            0 => false,
            1 => a < r,
            2 => a == r,
            3 => a <= r,
            4 => a > r,
            5 => a != r,
            6 => a >= r,
            _ => true,
        };
        let ac = self.alpha_compare;
        let (x, y) = (cmp(ac.comp0, ac.ref0), cmp(ac.comp1, ac.ref1));
        match ac.op {
            0 => x && y,
            1 => x || y,
            2 => x != y,
            _ => x == y,
        }
    }
}

/// One drawable: a shape with its material and joint, in INF1 order.
#[derive(Clone, Copy, Debug)]
pub struct Draw {
    pub shape: u16,
    pub material: u16,
    pub joint: u16,
}

/// A parsed J3D model.
#[derive(Clone, Debug)]
pub struct Model {
    /// e.g. `J3D1bmd1`.
    pub kind: [u8; 8],
    pub joints: Vec<Joint>,
    pub shapes: Vec<Shape>,
    pub materials: Vec<Material>,
    pub textures: Vec<Texture>,
    pub draws: Vec<Draw>,
    /// Whether MAT1 materials were decoded (false for MAT3 files).
    pub materials_decoded: bool,
}

struct Sections {
    inf1: Option<usize>,
    vtx1: Option<usize>,
    evp1: Option<usize>,
    drw1: Option<usize>,
    jnt1: Option<usize>,
    shp1: Option<usize>,
    mat1: Option<usize>,
    mat3: Option<usize>,
    tex1: Option<usize>,
}

fn sections(b: &[u8]) -> Result<Sections> {
    let magic = bytes(b, 0, 8)?;
    if &magic[..3] != b"J3D" || &magic[4..7] != b"bmd" && &magic[4..7] != b"bdl" {
        return Err(Error::BadMagic(0));
    }
    let count = be32(b, 0x0C)?;
    let mut s = Sections { inf1: None, vtx1: None, evp1: None, drw1: None, jnt1: None, shp1: None, mat1: None, mat3: None, tex1: None };
    let mut p = 0x20usize;
    for _ in 0..count.min(64) {
        let m = bytes(b, p, 4)?;
        let size = be32(b, p + 4)? as usize;
        if size < 8 {
            return Err(Error::BadValue(p + 4));
        }
        let slot = match m {
            b"INF1" => &mut s.inf1,
            b"VTX1" => &mut s.vtx1,
            b"EVP1" => &mut s.evp1,
            b"DRW1" => &mut s.drw1,
            b"JNT1" => &mut s.jnt1,
            b"SHP1" => &mut s.shp1,
            b"MAT1" => &mut s.mat1,
            b"MAT2" | b"MAT3" => &mut s.mat3,
            b"TEX1" => &mut s.tex1,
            _ => &mut None,
        };
        *slot = Some(p);
        p = p.checked_add((size + 0x1F) & !0x1F).ok_or(Error::BadValue(p))?;
        if p >= b.len() {
            break;
        }
    }
    Ok(s)
}

/// J3D name table: u16 count, pad, then (hash, offset) pairs; offsets are
/// relative to the table.
fn name_table(b: &[u8], off: usize) -> Result<Vec<String>> {
    let n = count_at(b, off)?;
    let mut out = Vec::with_capacity(n);
    for i in 0..n {
        let so = off + be16(b, off + 6 + i * 4)? as usize;
        let tail = b.get(so..).ok_or(Error::Truncated(so))?;
        let end = tail.iter().position(|&c| c == 0).unwrap_or(tail.len()).min(256);
        out.push(String::from_utf8_lossy(&tail[..end]).into_owned());
    }
    Ok(out)
}

fn name_or(names: &[String], i: usize) -> String {
    names.get(i).cloned().unwrap_or_default()
}

// ---------------------------------------------------------------- VTX1

#[derive(Clone, Copy)]
struct VtxFmt {
    off: usize,
    comps: u32,
    ty: u32,
    shift: u8,
}

struct Vtx {
    pos: Option<VtxFmt>,
    nrm: Option<VtxFmt>,
    col0: Option<VtxFmt>,
    tex0: Option<VtxFmt>,
}

fn parse_vtx1(b: &[u8], s: usize) -> Result<Vtx> {
    let fmt_off = s + be32(b, s + 8)? as usize;
    let mut v = Vtx { pos: None, nrm: None, col0: None, tex0: None };
    let mut p = fmt_off;
    for _ in 0..32 {
        let attr = be32(b, p)?;
        if attr == 0xFF {
            break;
        }
        let comps = be32(b, p + 4)?;
        let ty = be32(b, p + 8)?;
        let shift = be8(b, p + 12)?;
        // Data offsets: POS, NRM, NBT, COL0, COL1, TEX0..7.
        let slot = match attr {
            9 => Some((0, &mut v.pos)),
            10 => Some((1, &mut v.nrm)),
            11 => Some((3, &mut v.col0)),
            13 => Some((5, &mut v.tex0)),
            _ => None,
        };
        if let Some((idx, dst)) = slot {
            let o = be32(b, s + 0x0C + idx * 4)?;
            if o != 0 {
                *dst = Some(VtxFmt { off: s + o as usize, comps, ty, shift });
            }
        }
        p += 16;
    }
    Ok(v)
}

fn comp_size(ty: u32) -> usize {
    match ty {
        0 | 1 => 1,
        2 | 3 => 2,
        _ => 4,
    }
}

fn read_comp(b: &[u8], off: usize, ty: u32, shift: u8) -> Result<f32> {
    let scale = 1.0 / (1u32 << shift.min(31)) as f32;
    Ok(match ty {
        0 => be8(b, off)? as f32 * scale,
        1 => be8(b, off)? as i8 as f32 * scale,
        2 => be16(b, off)? as f32 * scale,
        3 => bei16(b, off)? as f32 * scale,
        4 => bef32(b, off)?,
        t => return Err(Error::Unsupported(t)),
    })
}

fn read_vec<const N: usize>(b: &[u8], f: &VtxFmt, n: usize, idx: usize) -> Result<[f32; N]> {
    let cs = comp_size(f.ty);
    let base = f.off + idx * n * cs;
    let mut out = [0.0; N];
    for (k, o) in out.iter_mut().enumerate().take(n) {
        *o = read_comp(b, base + k * cs, f.ty, f.shift)?;
    }
    Ok(out)
}

fn read_color(b: &[u8], f: &VtxFmt, idx: usize) -> Result<[u8; 4]> {
    Ok(match f.ty {
        0 => super::gx::rgb565(be16(b, f.off + idx * 2)?),
        1 => {
            let s = bytes(b, f.off + idx * 3, 3)?;
            [s[0], s[1], s[2], 255]
        }
        2 => {
            let s = bytes(b, f.off + idx * 4, 4)?;
            [s[0], s[1], s[2], 255]
        }
        3 => {
            let v = be16(b, f.off + idx * 2)?;
            let n = |x: u16| ((x & 15) * 17) as u8;
            [n(v >> 12), n(v >> 8), n(v >> 4), n(v)]
        }
        4 => {
            let s = bytes(b, f.off + idx * 3, 3)?;
            let v = (s[0] as u32) << 16 | (s[1] as u32) << 8 | s[2] as u32;
            let n = |x: u32| (((x & 63) << 2) | ((x & 63) >> 4)) as u8;
            [n(v >> 18), n(v >> 12), n(v >> 6), n(v)]
        }
        5 => {
            let s = bytes(b, f.off + idx * 4, 4)?;
            [s[0], s[1], s[2], s[3]]
        }
        t => return Err(Error::Unsupported(t)),
    })
}

// ---------------------------------------------------------------- DRW1 / EVP1 / JNT1

/// DRW1 entry -> joint (a weighted entry takes its heaviest joint).
fn parse_drw1(b: &[u8], s: usize, evp1: Option<usize>) -> Result<Vec<u16>> {
    let n = count_at(b, s + 8)?;
    let w_off = s + be32(b, s + 0x0C)? as usize;
    let d_off = s + be32(b, s + 0x10)? as usize;
    let mut out = Vec::with_capacity(n);
    for i in 0..n {
        let weighted = be8(b, w_off + i)? != 0;
        let d = be16(b, d_off + i * 2)?;
        out.push(if weighted { evp1_main_joint(b, evp1, d as usize).unwrap_or(0) } else { d });
    }
    Ok(out)
}

fn evp1_main_joint(b: &[u8], evp1: Option<usize>, env: usize) -> Option<u16> {
    let s = evp1?;
    let n = be16(b, s + 8).ok()? as usize;
    if env >= n {
        return None;
    }
    let counts = s + be32(b, s + 0x0C).ok()? as usize;
    let indices = s + be32(b, s + 0x10).ok()? as usize;
    let weights = s + be32(b, s + 0x14).ok()? as usize;
    let mut first = 0usize;
    for i in 0..env {
        first += be8(b, counts + i).ok()? as usize;
    }
    let cnt = be8(b, counts + env).ok()? as usize;
    let mut best = (None, f32::MIN);
    for k in 0..cnt {
        let j = be16(b, indices + (first + k) * 2).ok()?;
        let w = bef32(b, weights + (first + k) * 4).ok()?;
        if w > best.1 {
            best = (Some(j), w);
        }
    }
    best.0
}

fn parse_jnt1(b: &[u8], s: usize) -> Result<Vec<Joint>> {
    let n = count_at(b, s + 8)?;
    let ent = s + be32(b, s + 0x0C)? as usize;
    let remap = s + be32(b, s + 0x10)? as usize;
    let names_off = be32(b, s + 0x14)?;
    let names = if names_off != 0 { name_table(b, s + names_off as usize).unwrap_or_default() } else { Vec::new() };
    let mut out = Vec::with_capacity(n);
    for i in 0..n {
        let e = ent + be16(b, remap + i * 2)? as usize * 0x40;
        let f3 = |o: usize| -> Result<[f32; 3]> { Ok([bef32(b, e + o)?, bef32(b, e + o + 4)?, bef32(b, e + o + 8)?]) };
        out.push(Joint {
            name: name_or(&names, i),
            parent: None,
            flags: be16(b, e)?,
            scale: f3(0x04)?,
            rotation: [bei16(b, e + 0x10)?, bei16(b, e + 0x12)?, bei16(b, e + 0x14)?],
            translation: f3(0x18)?,
            radius: bef32(b, e + 0x24)?,
            bbox_min: f3(0x28)?,
            bbox_max: f3(0x34)?,
            world: IDENTITY,
        });
    }
    Ok(out)
}

// ---------------------------------------------------------------- SHP1

fn parse_shp1(b: &[u8], s: usize, vtx: &Vtx, drw: &[u16]) -> Result<Vec<Shape>> {
    let n = count_at(b, s + 8)?;
    let mut total = 0usize;
    let o = |i: usize| -> Result<usize> { Ok(s + be32(b, s + 0x0C + i * 4)? as usize) };
    let (ent, remap, attrs, mtx_table, prims, mtx_data, packets) = (o(0)?, o(1)?, o(3)?, o(4)?, o(5)?, o(6)?, o(7)?);
    let mut out = Vec::with_capacity(n);
    for i in 0..n {
        let e = ent + be16(b, remap + i * 2)? as usize * 0x28;
        let matrix_type = be8(b, e)?;
        let packet_count = be16(b, e + 2)? as usize;
        let attr_off = attrs + be16(b, e + 4)? as usize;
        let first_mtx = be16(b, e + 6)? as usize;
        let first_pkt = be16(b, e + 8)? as usize;
        let f3 = |o: usize| -> Result<[f32; 3]> { Ok([bef32(b, e + o)?, bef32(b, e + o + 4)?, bef32(b, e + o + 8)?]) };
        // Vertex descriptor: (attr, type) pairs until attr 0xFF.
        let mut desc: Vec<(u32, u32)> = Vec::new();
        for k in 0..26 {
            let a = be32(b, attr_off + k * 8)?;
            if a == 0xFF {
                break;
            }
            desc.push((a, be32(b, attr_off + k * 8 + 4)?));
        }
        let has = |a: u32| desc.iter().any(|d| d.0 == a);
        let mut shape = Shape {
            matrix_type,
            vertices: Vec::new(),
            indices: Vec::new(),
            has_normal: has(10) && vtx.nrm.is_some(),
            has_color: has(11) && vtx.col0.is_some(),
            has_uv: has(13) && vtx.tex0.is_some(),
            joint: 0,
            bbox_min: f3(0x10)?,
            bbox_max: f3(0x1C)?,
        };
        let mut mtx_list = [0u16; 10];
        for p in 0..packet_count {
            // Matrix data: u16 ?, u16 count, u32 first index into the table.
            let md = mtx_data + (first_mtx + p) * 8;
            let cnt = (be16(b, md + 2)? as usize).min(10);
            let first = be32(b, md + 4)? as usize;
            for k in 0..cnt {
                let d = be16(b, mtx_table + (first + k) * 2)?;
                if d != 0xFFFF {
                    mtx_list[k] = drw.get(d as usize).copied().unwrap_or(0);
                }
            }
            if p == 0 {
                shape.joint = mtx_list[0];
            }
            let pl = packets + (first_pkt + p) * 8;
            let size = be32(b, pl)? as usize;
            let start = prims + be32(b, pl + 4)? as usize;
            parse_prims(b, start, start + size, &desc, vtx, &mtx_list, &mut shape)?;
        }
        total += shape.vertices.len();
        if total > MAX_VERTICES {
            return Err(Error::BadValue(e));
        }
        out.push(shape);
    }
    Ok(out)
}

fn parse_prims(b: &[u8], mut p: usize, end: usize, desc: &[(u32, u32)], vtx: &Vtx, mtx: &[u16; 10], shape: &mut Shape) -> Result<()> {
    while p < end {
        let op = be8(b, p)?;
        if op == 0 {
            break; // padding to the packet's end
        }
        let count = be16(b, p + 1)? as usize;
        p += 3;
        if shape.vertices.len() + count > MAX_VERTICES {
            return Err(Error::BadValue(p - 2));
        }
        let base = shape.vertices.len() as u32;
        for _ in 0..count {
            let mut v = Vertex { color: [255; 4], joint: mtx[0], ..Default::default() };
            for &(attr, ty) in desc {
                let idx = match ty {
                    1 if attr <= 8 => {
                        let x = be8(b, p)? as usize;
                        p += 1;
                        x
                    }
                    2 => {
                        let x = be8(b, p)? as usize;
                        p += 1;
                        x
                    }
                    3 => {
                        let x = be16(b, p)? as usize;
                        p += 2;
                        x
                    }
                    0 => continue,
                    t => return Err(Error::Unsupported(t)), // direct vertex data
                };
                match attr {
                    0 => v.joint = mtx.get(idx / 3).copied().unwrap_or(mtx[0]),
                    9 => {
                        if let Some(f) = &vtx.pos {
                            let n = if f.comps == 0 { 2 } else { 3 };
                            v.pos = read_vec::<3>(b, f, n, idx)?;
                        }
                    }
                    10 => {
                        if let Some(f) = &vtx.nrm {
                            let stride = if f.comps == 1 { 9 } else { 3 };
                            v.normal = read_vec::<3>(b, f, stride, idx)?;
                        }
                    }
                    11 => {
                        if let Some(f) = &vtx.col0 {
                            v.color = read_color(b, f, idx)?;
                        }
                    }
                    13 => {
                        if let Some(f) = &vtx.tex0 {
                            let n = if f.comps == 0 { 1 } else { 2 };
                            v.uv = read_vec::<2>(b, f, n, idx)?;
                        }
                    }
                    _ => {}
                }
            }
            shape.vertices.push(v);
        }
        let c = count as u32;
        let idx = &mut shape.indices;
        match op & 0xF8 {
            0x80 => {
                // quads
                for q in (0..c.saturating_sub(3)).step_by(4) {
                    idx.extend_from_slice(&[base + q, base + q + 1, base + q + 2, base + q, base + q + 2, base + q + 3]);
                }
            }
            0x90 => {
                for t in (0..c.saturating_sub(2)).step_by(3) {
                    idx.extend_from_slice(&[base + t, base + t + 1, base + t + 2]);
                }
            }
            0x98 => {
                for t in 0..c.saturating_sub(2) {
                    if t & 1 == 0 {
                        idx.extend_from_slice(&[base + t, base + t + 1, base + t + 2]);
                    } else {
                        idx.extend_from_slice(&[base + t + 1, base + t, base + t + 2]);
                    }
                }
            }
            0xA0 => {
                for t in 1..c.saturating_sub(1) {
                    idx.extend_from_slice(&[base, base + t, base + t + 1]);
                }
            }
            0xA8 | 0xB0 | 0xB8 => {} // lines and points are not drawn
            _ => return Err(Error::BadValue(p - 3)),
        }
    }
    Ok(())
}

// ---------------------------------------------------------------- MAT1

fn parse_mat1(b: &[u8], s: usize, tex_count: usize) -> Result<Vec<Material>> {
    let n = count_at(b, s + 8)?;
    let mut t = [0usize; 22];
    for (i, o) in t.iter_mut().enumerate() {
        let v = be32(b, s + 0x0C + i * 4)?;
        *o = if v == 0 { 0 } else { s + v as usize };
    }
    let names = if t[2] != 0 { name_table(b, t[2]).unwrap_or_default() } else { Vec::new() };
    // Table readers: index -1 (0xFFFF) means "not set".
    let u8_at = |table: usize, i: usize| -> Result<u8> { be8(b, table + i) };
    let tex_no = |i: u16| -> Option<u16> {
        let v = be16(b, t[10] + i as usize * 2).ok()?;
        ((v as usize) < tex_count).then_some(v)
    };
    let mut out = Vec::with_capacity(n);
    for m in 0..n {
        let e = t[0] + be16(b, t[1] + m * 2)? as usize * 0x98;
        let raw = bytes(b, e, 0x98)?;
        let idx = |o: usize| -> Option<usize> {
            let v = u16::from_be_bytes([raw[o], raw[o + 1]]);
            (v != 0xFFFF).then_some(v as usize)
        };
        let mut mat = Material { name: name_or(&names, m), flag: raw[0], raw: raw.to_vec(), ..Default::default() };
        mat.cull = be32(b, t[3] + raw[1] as usize * 4)? as u8;
        mat.chan_count = u8_at(t[5], raw[2] as usize)?;
        for k in 0..2 {
            if let Some(i) = idx(0x08 + k * 2) {
                let c = bytes(b, t[4] + i * 4, 4)?;
                mat.mat_colors[k] = [c[0], c[1], c[2], c[3]];
            }
        }
        for k in 0..4 {
            mat.chans[k] = match idx(0x0C + k * 2) {
                Some(i) => {
                    let c = bytes(b, t[6] + i * 8, 8)?;
                    Some(ColorChan { lighting: c[0] != 0, mat_src: c[1], light_mask: c[2], diffuse_fn: c[3], attn_fn: c[4], amb_src: c[5] })
                }
                None => None,
            };
        }
        let ngen = u8_at(t[7], raw[3] as usize)? as usize;
        mat.tex_gens.clear();
        for k in 0..ngen.min(8) {
            if let Some(i) = idx(0x14 + k * 2) {
                let g = bytes(b, t[8] + i * 4, 4)?;
                mat.tex_gens.push(TexGen { kind: g[0], src: g[1], mtx: g[2] });
            }
        }
        for k in 0..8 {
            if let Some(i) = idx(0x24 + k * 2) {
                let base = t[9] + i * 0x34;
                let mut mm = [[0.0f32; 4]; 3];
                for (r, row) in mm.iter_mut().enumerate() {
                    for (c, v) in row.iter_mut().enumerate() {
                        *v = bef32(b, base + (r * 4 + c) * 4)?;
                    }
                }
                mat.tex_mtx[k] = Some(TexMtx { m: mm, mode: be8(b, base + 0x30)? });
            }
        }
        for k in 0..8 {
            mat.textures[k] = idx(0x38 + k * 2).and_then(|i| tex_no(i as u16));
        }
        let nstage = u8_at(t[13], raw[4] as usize)? as usize;
        mat.tev_orders.clear();
        mat.tev_stages.clear();
        for k in 0..nstage.min(16) {
            let ord = match idx(0x48 + k * 2) {
                Some(i) => {
                    let o = bytes(b, t[11] + i * 4, 4)?;
                    TevOrder { texcoord: o[0], texmap: o[1], channel: o[2] }
                }
                None => TevOrder { texcoord: 0xFF, texmap: 0xFF, channel: 0xFF },
            };
            let Some(si) = idx(0x70 + k * 2) else { break };
            let st = bytes(b, t[14] + si * 0x14, 0x14)?;
            mat.tev_orders.push(ord);
            mat.tev_stages.push(TevStage {
                color_in: [st[1], st[2], st[3], st[4]],
                color_op: st[5],
                color_bias: st[6],
                color_scale: st[7],
                color_clamp: st[8] != 0,
                color_reg: st[9],
                alpha_in: [st[10], st[11], st[12], st[13]],
                alpha_op: st[14],
                alpha_bias: st[15],
                alpha_scale: st[16],
                alpha_clamp: st[17] != 0,
                alpha_reg: st[18],
            });
        }
        for k in 0..4 {
            if let Some(i) = idx(0x68 + k * 2) {
                let c = t[12] + i * 8;
                mat.tev_colors[k] = Some([bei16(b, c)?, bei16(b, c + 2)?, bei16(b, c + 4)?, bei16(b, c + 6)?]);
            }
        }
        if let Some(i) = idx(0x92) {
            let a = bytes(b, t[16] + i * 8, 5)?;
            mat.alpha_compare = AlphaCompare { comp0: a[0], ref0: a[1], op: a[2], comp1: a[3], ref1: a[4] };
        }
        if let Some(i) = idx(0x94) {
            let a = bytes(b, t[17] + i * 4, 4)?;
            mat.blend = Blend { mode: a[0], src: a[1], dst: a[2], logic_op: a[3] };
        }
        let z = bytes(b, t[18] + raw[6] as usize * 4, 3)?;
        mat.z_mode = ZMode { enable: z[0] != 0, func: z[1], update: z[2] != 0 };
        if t[19] != 0 {
            mat.z_comp_loc = u8_at(t[19], raw[5] as usize)? != 0;
        }
        if t[20] != 0 {
            mat.dither = u8_at(t[20], raw[7] as usize)? != 0;
        }
        out.push(mat);
    }
    Ok(out)
}

/// MAT2/MAT3 (not decoded): count and names only.
fn parse_mat3_names(b: &[u8], s: usize) -> Result<Vec<Material>> {
    let n = count_at(b, s + 8)?;
    let names_off = be32(b, s + 0x14)?;
    let names = if names_off != 0 { name_table(b, s + names_off as usize).unwrap_or_default() } else { Vec::new() };
    Ok((0..n).map(|i| Material { name: name_or(&names, i), ..Default::default() }).collect())
}

// ---------------------------------------------------------------- TEX1

fn parse_tex1(b: &[u8], s: usize) -> Result<Vec<Texture>> {
    let n = count_at(b, s + 8)?;
    let hdr = s + be32(b, s + 0x0C)? as usize;
    let names_off = be32(b, s + 0x10)?;
    let names = if names_off != 0 { name_table(b, s + names_off as usize).unwrap_or_default() } else { Vec::new() };
    let mut texels = 0usize;
    let mut out = Vec::with_capacity(n);
    for i in 0..n {
        let h = bti::header(b, hdr + i * 0x20)?;
        texels += h.width as usize * h.height as usize;
        if texels > MAX_TEXELS {
            return Err(Error::BadValue(hdr + i * 0x20));
        }
        out.push(bti::decode_at(b, hdr + i * 0x20, name_or(&names, i))?);
    }
    Ok(out)
}

// ---------------------------------------------------------------- INF1

fn walk_inf1(b: &[u8], s: usize, joints: &mut [Joint], draws: &mut Vec<Draw>) -> Result<()> {
    let mut p = at(s, be32(b, s + 0x14)?)?;
    // Stack of open nodes: (type, index).
    let mut stack: Vec<(u16, u16)> = Vec::new();
    let mut last: (u16, u16) = (0, 0);
    let mut done = vec![false; joints.len()];
    for _ in 0..0x10000 {
        let ty = be16(b, p)?;
        let i = be16(b, p + 2)?;
        p += 4;
        let nearest = |stack: &[(u16, u16)], want: u16| stack.iter().rev().find(|n| n.0 == want).map(|n| n.1);
        match ty {
            0x00 => return Ok(()),
            0x01 => stack.push(last),
            0x02 => {
                stack.pop();
            }
            0x10 => {
                let parent = nearest(&stack, 0x10);
                let ji = i as usize;
                if ji < joints.len() {
                    let pw = parent.and_then(|pi| joints.get(pi as usize)).map(|j| j.world).unwrap_or(IDENTITY);
                    joints[ji].parent = parent;
                    joints[ji].world = mtx_mul(&pw, &joints[ji].local());
                    done[ji] = true;
                }
                last = (ty, i);
            }
            0x11 => last = (ty, i),
            0x12 => {
                draws.push(Draw { shape: i, material: nearest(&stack, 0x11).unwrap_or(0), joint: nearest(&stack, 0x10).unwrap_or(0) });
                last = (ty, i);
            }
            _ => return Err(Error::BadValue(p - 4)),
        }
    }
    // Joints the hierarchy never reached keep their local matrix.
    for (j, d) in joints.iter_mut().zip(done) {
        if !d {
            j.world = j.local();
        }
    }
    Ok(())
}

impl Model {
    /// Parse a J3D model file.
    pub fn parse(b: &[u8]) -> Result<Model> {
        let s = sections(b)?;
        let need = |o: Option<usize>| o.ok_or(Error::BadMagic(0));
        let mut kind = [0u8; 8];
        kind.copy_from_slice(bytes(b, 0, 8)?);
        let vtx = parse_vtx1(b, need(s.vtx1)?)?;
        let drw = match s.drw1 {
            Some(d) => parse_drw1(b, d, s.evp1)?,
            None => Vec::new(),
        };
        let mut joints = match s.jnt1 {
            Some(j) => parse_jnt1(b, j)?,
            None => Vec::new(),
        };
        let shapes = parse_shp1(b, need(s.shp1)?, &vtx, &drw)?;
        let textures = match s.tex1 {
            Some(t) => parse_tex1(b, t)?,
            None => Vec::new(),
        };
        let (materials, materials_decoded) = match (s.mat1, s.mat3) {
            (Some(m), _) => (parse_mat1(b, m, textures.len())?, true),
            (None, Some(m)) => (parse_mat3_names(b, m)?, false),
            _ => (Vec::new(), false),
        };
        let mut draws = Vec::new();
        match s.inf1 {
            Some(i) => walk_inf1(b, i, &mut joints, &mut draws)?,
            None => {
                for j in joints.iter_mut() {
                    j.world = j.local();
                }
            }
        }
        if draws.is_empty() {
            draws = (0..shapes.len() as u16).map(|i| Draw { shape: i, material: 0, joint: shapes[i as usize].joint }).collect();
        }
        // Drop draws that point outside the tables rather than fail later.
        draws.retain(|d| (d.shape as usize) < shapes.len());
        Ok(Model { kind, joints, shapes, materials, textures, draws, materials_decoded })
    }

    /// Bind-pose world matrix of `joint` (identity when out of range).
    pub fn joint_world(&self, joint: u16) -> Mtx {
        self.joints.get(joint as usize).map(|j| j.world).unwrap_or(IDENTITY)
    }

    /// Model-space position of a vertex.
    pub fn world_pos(&self, v: &Vertex) -> [f32; 3] {
        mtx_point(&self.joint_world(v.joint), v.pos)
    }

    /// Model-space bounding box of every drawn vertex.
    pub fn bounds(&self) -> ([f32; 3], [f32; 3]) {
        let mut lo = [f32::MAX; 3];
        let mut hi = [f32::MIN; 3];
        for d in &self.draws {
            for v in &self.shapes[d.shape as usize].vertices {
                let p = self.world_pos(v);
                for k in 0..3 {
                    lo[k] = lo[k].min(p[k]);
                    hi[k] = hi[k].max(p[k]);
                }
            }
        }
        if lo[0] > hi[0] {
            return ([0.0; 3], [0.0; 3]);
        }
        (lo, hi)
    }

    /// Total vertex and triangle counts over all shapes.
    pub fn counts(&self) -> (usize, usize) {
        self.shapes.iter().fold((0, 0), |(v, t), s| (v + s.vertices.len(), t + s.indices.len() / 3))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn srt_rotation() {
        // 90 degrees about Z maps +X to +Y.
        let m = srt_mtx([1.0; 3], [0, 0, 0x4000], [1.0, 2.0, 3.0]);
        let p = mtx_point(&m, [1.0, 0.0, 0.0]);
        assert!((p[0] - 1.0).abs() < 1e-5 && (p[1] - 3.0).abs() < 1e-5 && (p[2] - 3.0).abs() < 1e-5);
    }

    #[test]
    fn garbage_is_an_error() {
        assert!(Model::parse(&[]).is_err());
        assert!(Model::parse(b"J3D1bmd1\0\0\0\0\0\0\0\x08").is_err());
        let mut b = vec![0u8; 0x40];
        b[..8].copy_from_slice(b"J3D1bmd1");
        b[0x0F] = 1;
        b[0x20..0x24].copy_from_slice(b"VTX1");
        b[0x27] = 0x20;
        assert!(Model::parse(&b).is_err());
    }
}
