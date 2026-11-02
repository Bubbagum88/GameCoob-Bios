//! GX on the CPU: the part of the GameCube graphics API that BS2 calls, kept
//! as plain state and turned into triangles for the software rasterizer
//! ([`crate::bs2::raster`]).
//!
//! The pipeline follows the hardware stages:
//!
//! * **XF** (per vertex, on [`Gx::end`]): the current position/normal matrix,
//!   the projection, the viewport, the two color channels with their lights
//!   (`GXSetChanCtrl`, `GXInitLight*`) and the texture coordinate generators.
//! * **TEV** (per pixel): the stage setup is compiled once into a
//!   [`Shader`] whenever it changes, with the texture maps loaded at the
//!   time; the rasterizer runs it for every pixel.
//! * **PE**: z mode, blend mode and cull mode become the triangle's
//!   [`State`].
//!
//! Vertices are sent in immediate mode (`GXBegin`, `GXPosition*`, ...), with
//! the fixed-point scale of `GXSetVtxAttrFmt`. Display lists, indexed
//! arrays and the second color channel in TEV (read as white) are not there
//! yet.
//!
//! The registers ([`GxRegs`]) outlive a frame, as on the hardware: start
//! the next frame's [`Gx`] from [`Gx::regs`].

// The SDK surface BS2 uses; not every call has a ported caller yet.
#![allow(dead_code)]

use super::mtx::{self, Mtx, Mtx44, Vec3};
use crate::bs2::raster::{BlendMode, Prim, SVert, Sh, State, MAX_TC};
use crate::bs2::shader::{FastTex, Shader};
use crate::ipl::j3d::{AlphaCompare, Material, TevOrder, TevStage, TexGen};
use alloc::boxed::Box;
use alloc::vec::Vec;
use core::cell::UnsafeCell;

/// `GXPrimitive`
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Primitive {
    Quads,
    Triangles,
    TriangleStrip,
    TriangleFan,
}

pub const GX_QUADS: u8 = 0x80;
pub const GX_TRIANGLES: u8 = 0x90;
pub const GX_TRIANGLESTRIP: u8 = 0x98;
pub const GX_TRIANGLEFAN: u8 = 0xA0;

impl Primitive {
    /// From a raw `GXPrimitive` value (BS2 often passes the number).
    pub fn from_raw(v: u8) -> Option<Primitive> {
        Some(match v {
            GX_QUADS => Primitive::Quads,
            GX_TRIANGLES => Primitive::Triangles,
            GX_TRIANGLESTRIP => Primitive::TriangleStrip,
            GX_TRIANGLEFAN => Primitive::TriangleFan,
            _ => return None,
        })
    }
}

/// `GXAttr` values used with [`Gx::set_vtx_attr_fmt`].
pub const GX_VA_POS: u8 = 9;
pub const GX_VA_NRM: u8 = 10;
pub const GX_VA_CLR0: u8 = 11;
pub const GX_VA_CLR1: u8 = 12;
pub const GX_VA_TEX0: u8 = 13;

/// `GXCompType` for positions, normals and texture coordinates.
pub const GX_U8: u8 = 0;
pub const GX_S8: u8 = 1;
pub const GX_U16: u8 = 2;
pub const GX_S16: u8 = 3;
pub const GX_F32: u8 = 4;

/// `GXPosNrmMtx` / `GXTexMtx`
pub const GX_PNMTX0: u32 = 0;
pub const GX_TEXMTX0: u32 = 30;
pub const GX_IDENTITY: u32 = 60;

/// `GXChannelID`
pub const GX_COLOR0: u8 = 0;
pub const GX_COLOR1: u8 = 1;
pub const GX_ALPHA0: u8 = 2;
pub const GX_ALPHA1: u8 = 3;
pub const GX_COLOR0A0: u8 = 4;
pub const GX_COLOR1A1: u8 = 5;
pub const GX_COLOR_NULL: u8 = 0xFF;

/// `GXColorSrc`
pub const GX_SRC_REG: u8 = 0;
pub const GX_SRC_VTX: u8 = 1;

/// `GXDiffuseFn`
pub const GX_DF_NONE: u8 = 0;
pub const GX_DF_SIGN: u8 = 1;
pub const GX_DF_CLAMP: u8 = 2;

/// `GXAttnFn`
pub const GX_AF_SPEC: u8 = 0;
pub const GX_AF_SPOT: u8 = 1;
pub const GX_AF_NONE: u8 = 2;

/// `GXLightID` bits (the ones BS2 uses).
pub const GX_LIGHT0: u8 = 1;
pub const GX_LIGHT3: u8 = 8;
pub const GX_LIGHT4: u8 = 0x10;
pub const GX_LIGHT5: u8 = 0x20;

/// `GXTexGenType` / `GXTexGenSrc`
pub const GX_TG_MTX3X4: u8 = 0;
pub const GX_TG_MTX2X4: u8 = 1;
pub const GX_TG_POS: u8 = 0;
pub const GX_TG_NRM: u8 = 1;
pub const GX_TG_TEX0: u8 = 4;

/// `GXTexCoordID` / `GXTexMapID` "none"
pub const GX_TEXCOORD_NULL: u8 = 0xFF;
pub const GX_TEXMAP_NULL: u8 = 0xFF;

/// `GXTevColorArg`
pub const GX_CC_CPREV: u8 = 0;
pub const GX_CC_TEXC: u8 = 8;
pub const GX_CC_TEXA: u8 = 9;
pub const GX_CC_RASC: u8 = 10;
pub const GX_CC_RASA: u8 = 11;
pub const GX_CC_ONE: u8 = 12;
pub const GX_CC_HALF: u8 = 13;
pub const GX_CC_KONST: u8 = 14;
pub const GX_CC_ZERO: u8 = 15;

/// `GXTevAlphaArg`
pub const GX_CA_APREV: u8 = 0;
pub const GX_CA_TEXA: u8 = 4;
pub const GX_CA_RASA: u8 = 5;
pub const GX_CA_KONST: u8 = 6;
pub const GX_CA_ZERO: u8 = 7;

/// `GXTevOp`, `GXTevBias`, `GXTevScale`, `GXTevRegID`
pub const GX_TEV_ADD: u8 = 0;
pub const GX_TEV_SUB: u8 = 1;
pub const GX_TB_ZERO: u8 = 0;
pub const GX_CS_SCALE_1: u8 = 0;
pub const GX_TEVPREV: u8 = 0;

/// `GXTevMode` for [`Gx::set_tev_op`].
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum TevMode {
    Modulate,
    Decal,
    Blend,
    Replace,
    PassClr,
}

/// `GXTevKColorSel` / `GXTevKAlphaSel` (the ones BS2 uses).
pub const GX_TEV_KCSEL_1_4: u8 = 6;
pub const GX_TEV_KASEL_1: u8 = 0;
pub const GX_TEV_KASEL_K0_A: u8 = 0x1C;

/// `GXTevKColorID`
pub const GX_KCOLOR0: u8 = 0;

/// `GXCompare`
pub const GX_NEVER: u8 = 0;
pub const GX_LEQUAL: u8 = 3;
pub const GX_ALWAYS: u8 = 7;

/// `GXBlendMode` and `GXBlendFactor`
pub const GX_BM_NONE: u8 = 0;
pub const GX_BM_BLEND: u8 = 1;
pub const GX_BL_ZERO: u8 = 0;
pub const GX_BL_ONE: u8 = 1;
pub const GX_BL_SRCALPHA: u8 = 4;
pub const GX_BL_INVSRCALPHA: u8 = 5;

/// `GXCullMode`
pub const GX_CULL_NONE: u8 = 0;
pub const GX_CULL_FRONT: u8 = 1;
pub const GX_CULL_BACK: u8 = 2;
pub const GX_CULL_ALL: u8 = 3;

/// `GXSpotFn`
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum SpotFn {
    Off,
    Flat,
    Cos,
    Cos2,
    Sharp,
    Ring1,
    Ring2,
}

/// `GXDistAttnFn`
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum DistAttnFn {
    Off,
    Gentle,
    Medium,
    Steep,
}

/// `GXLightObj`: position and direction in view space, color, and the
/// angle (`a`) and distance (`k`) attenuation coefficients.
#[derive(Clone, Copy)]
pub struct LightObj {
    pub pos: Vec3,
    pub dir: Vec3,
    pub color: [u8; 4],
    pub a: [f32; 3],
    pub k: [f32; 3],
}

impl Default for LightObj {
    fn default() -> Self {
        LightObj { pos: [0.0; 3], dir: [0.0; 3], color: [0; 4], a: [1.0, 0.0, 0.0], k: [1.0, 0.0, 0.0] }
    }
}

impl LightObj {
    /// `GXInitLightPos`
    pub fn init_pos(&mut self, x: f32, y: f32, z: f32) {
        self.pos = [x, y, z];
    }

    /// `GXInitLightDir`: stored negated, as the SDK does.
    pub fn init_dir(&mut self, x: f32, y: f32, z: f32) {
        self.dir = [-x, -y, -z];
    }

    /// `GXInitLightColor`
    pub fn init_color(&mut self, c: [u8; 4]) {
        self.color = c;
    }

    /// `GXInitLightAttn`
    pub fn init_attn(&mut self, a: [f32; 3], k: [f32; 3]) {
        self.a = a;
        self.k = k;
    }

    /// `GXInitLightSpot`: angle attenuation for a spot of `cutoff` degrees.
    pub fn init_spot(&mut self, cutoff: f32, f: SpotFn) {
        let f = if cutoff <= 0.0 || cutoff > 90.0 { SpotFn::Off } else { f };
        let cr = libm::cosf(cutoff * core::f32::consts::PI / 180.0);
        let d = (1.0 - cr) * (1.0 - cr);
        self.a = match f {
            SpotFn::Off => [1.0, 0.0, 0.0],
            SpotFn::Flat => [-1000.0 * cr, 1000.0, 0.0],
            SpotFn::Cos => [-cr / (1.0 - cr), 1.0 / (1.0 - cr), 0.0],
            SpotFn::Cos2 => [0.0, -cr / (1.0 - cr), 1.0 / (1.0 - cr)],
            SpotFn::Sharp => [cr * (cr - 2.0) / d, 2.0 / d, -1.0 / d],
            SpotFn::Ring1 => [-4.0 * cr / d, 4.0 * (1.0 + cr) / d, -4.0 / d],
            SpotFn::Ring2 => [1.0 - 2.0 * cr * cr / d, 4.0 * cr / d, -2.0 / d],
        };
    }

    /// `GXInitLightDistAttn`: brightness `ref_br` at distance `ref_dist`.
    pub fn init_dist_attn(&mut self, ref_dist: f32, ref_br: f32, f: DistAttnFn) {
        let f = if ref_dist < 0.0 || ref_br <= 0.0 || ref_br >= 1.0 { DistAttnFn::Off } else { f };
        let q = (1.0 - ref_br) / (ref_br * ref_dist);
        self.k = match f {
            DistAttnFn::Off => [1.0, 0.0, 0.0],
            DistAttnFn::Gentle => [1.0, q, 0.0],
            DistAttnFn::Medium => [1.0, 0.5 * q, 0.5 * (1.0 - ref_br) / (ref_br * ref_dist * ref_dist)],
            DistAttnFn::Steep => [1.0, 0.0, (1.0 - ref_br) / (ref_br * ref_dist * ref_dist)],
        };
    }
}

/// `GXSetChanCtrl` for one of COLOR0, COLOR1, ALPHA0, ALPHA1.
#[derive(Clone, Copy)]
struct ChanCtrl {
    enable: bool,
    amb_src: u8,
    mat_src: u8,
    light_mask: u8,
    diff_fn: u8,
    attn_fn: u8,
}

impl Default for ChanCtrl {
    fn default() -> Self {
        ChanCtrl { enable: false, amb_src: GX_SRC_REG, mat_src: GX_SRC_VTX, light_mask: 0, diff_fn: GX_DF_NONE, attn_fn: GX_AF_NONE }
    }
}

/// What the rasterizer needs to shade a triangle drawn through GX: the
/// compiled TEV stages and the texture maps loaded when they were drawn.
pub struct GxDraw<'a> {
    pub shader: Shader,
    pub maps: [Option<&'a FastTex>; 8],
}

/// Holds every [`GxDraw`] of a frame so the frame's triangles can point at
/// them. Entries are only added, never moved or dropped, until the arena is.
pub struct GxArena<'a> {
    draws: UnsafeCell<Vec<Box<GxDraw<'a>>>>,
}

impl<'a> GxArena<'a> {
    pub fn new() -> Self {
        GxArena { draws: UnsafeCell::new(Vec::new()) }
    }

    fn alloc(&self, d: GxDraw<'a>) -> &GxDraw<'a> {
        // SAFETY: the arena is not Sync and no reference into the Vec itself
        // escapes; each Box's contents stay at the same address until the
        // arena is dropped, which the returned borrow of `self` prevents.
        unsafe {
            let v = &mut *self.draws.get();
            v.push(Box::new(d));
            let p: *const GxDraw<'a> = &**v.last().unwrap();
            &*p
        }
    }
}

/// One vertex as sent, before XF.
#[derive(Clone, Copy, Default)]
struct VtxIn {
    pos: Vec3,
    nrm: Vec3,
    clr: [[f32; 4]; 2],
    tex: [[f32; 2]; 8],
}

/// The GX registers: everything a GX call sets, kept from one frame to the
/// next as on the hardware (BS2 relies on that: the logo text is drawn
/// before the frame's `fn_81304CE8` reset, and the spin-out never resets).
/// Texture maps are not here; they point into the frame's assets and are
/// loaded again each frame.
#[derive(Clone, Copy)]
pub struct GxRegs {
    // XF
    viewport: [f32; 4],
    proj: Mtx44,
    pos_mtx: [Mtx; 10],
    nrm_mtx: [Mtx; 10],
    tex_mtx: [Mtx; 10],
    cur_mtx: u32,
    /// Per format and attribute (POS..TEX7): component type and fraction bits.
    attr_fmt: [[(u8, u8); 13]; 8],
    num_chans: u8,
    chans: [ChanCtrl; 4],
    mat_color: [[u8; 4]; 2],
    amb_color: [[u8; 4]; 2],
    lights: [LightObj; 8],
    num_tex_gens: u8,
    tex_gens: [TexGen; 8],

    // TEV and PE
    num_tev_stages: u8,
    tev_orders: [TevOrder; 16],
    tev_stages: [TevStage; 16],
    tev_regs: [[i16; 4]; 4],
    kcolor: [[u8; 4]; 4],
    kcsel: [u8; 16],
    kasel: [u8; 16],
    alpha_compare: AlphaCompare,
    blend: (u8, u8, u8),
    z_mode: (bool, u8, bool),
    cull: u8,
    color_update: bool,
}

impl GxRegs {
    /// The state `GXInit` leaves, drawing into a `w` x `h` viewport.
    pub fn init(w: f32, h: f32) -> GxRegs {
        let tg = |i: u8| TexGen { kind: GX_TG_MTX2X4, src: GX_TG_TEX0 + i, mtx: GX_IDENTITY as u8 };
        let order = |i: u8| match i {
            0..=7 => TevOrder { texcoord: i, texmap: i, channel: GX_COLOR0A0 },
            _ => TevOrder { texcoord: GX_TEXCOORD_NULL, texmap: GX_TEXMAP_NULL, channel: GX_COLOR_NULL },
        };
        GxRegs {
            viewport: [0.0, 0.0, w, h],
            proj: mtx::ortho(1.0, -1.0, -1.0, 1.0, 0.0, 1.0),
            pos_mtx: [mtx::IDENTITY; 10],
            nrm_mtx: [mtx::IDENTITY; 10],
            tex_mtx: [mtx::IDENTITY; 10],
            cur_mtx: GX_PNMTX0,
            attr_fmt: [[(GX_F32, 0); 13]; 8],
            num_chans: 0,
            chans: [ChanCtrl::default(); 4],
            mat_color: [[255; 4]; 2],
            amb_color: [[0; 4]; 2],
            lights: [LightObj::default(); 8],
            num_tex_gens: 0,
            tex_gens: core::array::from_fn(|i| tg(i as u8)),
            num_tev_stages: 1,
            tev_orders: core::array::from_fn(|i| order(i as u8)),
            tev_stages: [PASS_STAGE; 16],
            tev_regs: [[0; 4]; 4],
            kcolor: [[0; 4]; 4],
            kcsel: [GX_TEV_KCSEL_1_4; 16],
            kasel: [GX_TEV_KASEL_1; 16],
            alpha_compare: AlphaCompare { comp0: GX_ALWAYS, ref0: 0, op: 0, comp1: GX_ALWAYS, ref1: 0 },
            blend: (GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO),
            z_mode: (true, GX_LEQUAL, true),
            cull: GX_CULL_BACK,
            color_update: true,
        }
    }
}

/// The GX state machine and the frame's triangle list.
pub struct Gx<'a> {
    arena: &'a GxArena<'a>,
    /// The frame's triangles, in draw order. Other renderers may push here
    /// too, so GX draws and theirs stay in order.
    pub list: Vec<Prim<'a>>,
    r: GxRegs,
    tex_maps: [Option<&'a FastTex>; 8],
    shader: Option<&'a GxDraw<'a>>,

    // Immediate mode
    prim: Option<(Primitive, u8)>,
    verts: Vec<VtxIn>,
    cur: VtxIn,
    tex_n: usize,
    open: bool,
}

const PASS_STAGE: TevStage = TevStage {
    color_in: [GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC],
    color_op: GX_TEV_ADD,
    color_bias: GX_TB_ZERO,
    color_scale: GX_CS_SCALE_1,
    color_clamp: true,
    color_reg: GX_TEVPREV,
    alpha_in: [GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA],
    alpha_op: GX_TEV_ADD,
    alpha_bias: GX_TB_ZERO,
    alpha_scale: GX_CS_SCALE_1,
    alpha_clamp: true,
    alpha_reg: GX_TEVPREV,
};

impl<'a> Gx<'a> {
    /// GX as `GXInit` leaves it, drawing into a `w` x `h` viewport.
    pub fn new(arena: &'a GxArena<'a>, w: f32, h: f32) -> Gx<'a> {
        Gx::with_regs(arena, GxRegs::init(w, h))
    }

    /// GX with the registers a previous frame left (see [`Gx::regs`]).
    pub fn with_regs(arena: &'a GxArena<'a>, r: GxRegs) -> Gx<'a> {
        Gx {
            arena,
            list: Vec::new(),
            r,
            tex_maps: [None; 8],
            shader: None,
            prim: None,
            verts: Vec::new(),
            cur: VtxIn::default(),
            tex_n: 0,
            open: false,
        }
    }

    /// The registers as they stand, to start the next frame from.
    pub fn regs(&self) -> GxRegs {
        self.r
    }

    // ---- Transform ----

    /// `GXSetViewport` (the near/far depth range is always 0..1 here).
    pub fn set_viewport(&mut self, x: f32, y: f32, w: f32, h: f32) {
        self.r.viewport = [x, y, w, h];
    }

    /// `GXSetProjection`
    pub fn set_projection(&mut self, m: &Mtx44) {
        self.r.proj = *m;
    }

    /// `GXLoadPosMtxImm`
    pub fn load_pos_mtx_imm(&mut self, m: &Mtx, id: u32) {
        if let Some(s) = self.r.pos_mtx.get_mut((id / 3) as usize) {
            *s = *m;
        }
    }

    /// `GXLoadNrmMtxImm`
    pub fn load_nrm_mtx_imm(&mut self, m: &Mtx, id: u32) {
        if let Some(s) = self.r.nrm_mtx.get_mut((id / 3) as usize) {
            *s = *m;
        }
    }

    /// `GXLoadTexMtxImm`
    pub fn load_tex_mtx_imm(&mut self, m: &Mtx, id: u32) {
        if let Some(s) = id.checked_sub(GX_TEXMTX0).and_then(|i| self.r.tex_mtx.get_mut((i / 3) as usize)) {
            *s = *m;
        }
    }

    /// `GXSetCurrentMtx`
    pub fn set_current_mtx(&mut self, id: u32) {
        self.r.cur_mtx = id;
    }

    // ---- Vertex format ----

    /// `GXClearVtxDesc` / `GXSetVtxDesc`: immediate mode needs no
    /// descriptor, each call says what it sends.
    pub fn clear_vtx_desc(&mut self) {}

    /// `GXSetVtxAttrFmt`: only the component type and fraction bits matter
    /// for immediate mode.
    pub fn set_vtx_attr_fmt(&mut self, fmt: u8, attr: u8, _cnt: u8, ty: u8, frac: u8) {
        if let Some(a) = attr.checked_sub(GX_VA_POS).and_then(|i| self.r.attr_fmt.get_mut(fmt as usize)?.get_mut(i as usize)) {
            *a = (ty, frac);
        }
    }

    fn fixed(&self, attr: u8, raw: i32) -> f32 {
        let fmt = self.prim.map(|p| p.1).unwrap_or(0) as usize;
        let (ty, frac) = self.r.attr_fmt[fmt][(attr - GX_VA_POS) as usize];
        // The wire carries the low bits; the format says how to read them.
        let v = match ty {
            GX_U8 => raw as u8 as i32,
            GX_S8 => raw as i8 as i32,
            GX_U16 => raw as u16 as i32,
            GX_S16 => raw as i16 as i32,
            _ => raw,
        };
        v as f32 / (1u32 << frac) as f32
    }

    // ---- Lighting ----

    /// `GXSetNumChans`
    pub fn set_num_chans(&mut self, n: u8) {
        self.r.num_chans = n;
    }

    /// `GXSetChanCtrl`
    #[allow(clippy::too_many_arguments)]
    pub fn set_chan_ctrl(&mut self, chan: u8, enable: bool, amb_src: u8, mat_src: u8, light_mask: u8, diff_fn: u8, attn_fn: u8) {
        let c = ChanCtrl { enable, amb_src, mat_src, light_mask, diff_fn, attn_fn };
        let idx: &[usize] = match chan {
            GX_COLOR0 => &[0],
            GX_COLOR1 => &[1],
            GX_ALPHA0 => &[2],
            GX_ALPHA1 => &[3],
            GX_COLOR0A0 => &[0, 2],
            GX_COLOR1A1 => &[1, 3],
            _ => &[],
        };
        for &i in idx {
            self.r.chans[i] = c;
        }
    }

    fn chan_regs(chan: u8) -> &'static [usize] {
        match chan {
            GX_COLOR0 | GX_ALPHA0 | GX_COLOR0A0 => &[0],
            GX_COLOR1 | GX_ALPHA1 | GX_COLOR1A1 => &[1],
            _ => &[],
        }
    }

    /// `GXSetChanMatColor`. As on the hardware, COLOR0 writes only RGB and
    /// ALPHA0 only alpha.
    pub fn set_chan_mat_color(&mut self, chan: u8, c: [u8; 4]) {
        for &i in Self::chan_regs(chan) {
            Self::write_chan_color(&mut self.r.mat_color[i], chan, c);
        }
    }

    /// `GXSetChanAmbColor`
    pub fn set_chan_amb_color(&mut self, chan: u8, c: [u8; 4]) {
        for &i in Self::chan_regs(chan) {
            Self::write_chan_color(&mut self.r.amb_color[i], chan, c);
        }
    }

    fn write_chan_color(dst: &mut [u8; 4], chan: u8, c: [u8; 4]) {
        match chan {
            GX_COLOR0 | GX_COLOR1 => dst[..3].copy_from_slice(&c[..3]),
            GX_ALPHA0 | GX_ALPHA1 => dst[3] = c[3],
            _ => *dst = c,
        }
    }

    /// `GXLoadLightObjImm`: `id` is the light's bit (GX_LIGHT0 = 1).
    pub fn load_light_obj_imm(&mut self, l: &LightObj, id: u8) {
        if id != 0 {
            self.r.lights[id.trailing_zeros() as usize & 7] = *l;
        }
    }

    // ---- Texturing ----

    /// `GXSetNumTexGens`
    pub fn set_num_tex_gens(&mut self, n: u8) {
        self.r.num_tex_gens = n;
    }

    /// `GXSetTexCoordGen` / `GXSetTexCoordGen2` (no normalize or post
    /// matrix).
    pub fn set_tex_coord_gen(&mut self, dst: u8, kind: u8, src: u8, mtx: u32) {
        if let Some(g) = self.r.tex_gens.get_mut(dst as usize) {
            *g = TexGen { kind, src, mtx: mtx as u8 };
        }
    }

    /// `GXLoadTexObj`: a texture decoded once at load time takes the place
    /// of the texture object.
    pub fn load_tex_obj(&mut self, tex: &'a FastTex, map: u8) {
        if let Some(m) = self.tex_maps.get_mut(map as usize) {
            if !m.is_some_and(|t| core::ptr::eq(t, tex)) {
                *m = Some(tex);
                self.shader = None;
            }
        }
    }

    // ---- TEV ----

    fn tev_mut(&mut self, stage: u8) -> Option<&mut TevStage> {
        self.shader = None;
        self.r.tev_stages.get_mut(stage as usize)
    }

    /// `GXSetNumTevStages`
    pub fn set_num_tev_stages(&mut self, n: u8) {
        self.r.num_tev_stages = n;
        self.shader = None;
    }

    /// `GXSetTevOrder`
    pub fn set_tev_order(&mut self, stage: u8, coord: u8, map: u8, color: u8) {
        if let Some(o) = self.r.tev_orders.get_mut(stage as usize) {
            *o = TevOrder { texcoord: coord, texmap: map, channel: color };
            self.shader = None;
        }
    }

    /// `GXSetTevColorIn`
    pub fn set_tev_color_in(&mut self, stage: u8, a: u8, b: u8, c: u8, d: u8) {
        if let Some(s) = self.tev_mut(stage) {
            s.color_in = [a, b, c, d];
        }
    }

    /// `GXSetTevAlphaIn`
    pub fn set_tev_alpha_in(&mut self, stage: u8, a: u8, b: u8, c: u8, d: u8) {
        if let Some(s) = self.tev_mut(stage) {
            s.alpha_in = [a, b, c, d];
        }
    }

    /// `GXSetTevColorOp`
    pub fn set_tev_color_op(&mut self, stage: u8, op: u8, bias: u8, scale: u8, clamp: bool, out: u8) {
        if let Some(s) = self.tev_mut(stage) {
            (s.color_op, s.color_bias, s.color_scale, s.color_clamp, s.color_reg) = (op, bias, scale, clamp, out);
        }
    }

    /// `GXSetTevAlphaOp`
    pub fn set_tev_alpha_op(&mut self, stage: u8, op: u8, bias: u8, scale: u8, clamp: bool, out: u8) {
        if let Some(s) = self.tev_mut(stage) {
            (s.alpha_op, s.alpha_bias, s.alpha_scale, s.alpha_clamp, s.alpha_reg) = (op, bias, scale, clamp, out);
        }
    }

    /// `GXSetTevOp`: the SDK's canned stage setups.
    pub fn set_tev_op(&mut self, stage: u8, mode: TevMode) {
        // Stage 0 starts from the rasterized color; later stages from PREV.
        let (carg, aarg) = if stage == 0 { (GX_CC_RASC, GX_CA_RASA) } else { (GX_CC_CPREV, GX_CA_APREV) };
        let (c, a) = match mode {
            TevMode::Modulate => ([GX_CC_ZERO, GX_CC_TEXC, carg, GX_CC_ZERO], [GX_CA_ZERO, GX_CA_TEXA, aarg, GX_CA_ZERO]),
            TevMode::Decal => ([carg, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO], [GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, aarg]),
            TevMode::Blend => ([carg, GX_CC_ONE, GX_CC_TEXC, GX_CC_ZERO], [GX_CA_ZERO, GX_CA_TEXA, aarg, GX_CA_ZERO]),
            TevMode::Replace => ([GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC], [GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA]),
            TevMode::PassClr => ([GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, carg], [GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, aarg]),
        };
        self.set_tev_color_in(stage, c[0], c[1], c[2], c[3]);
        self.set_tev_alpha_in(stage, a[0], a[1], a[2], a[3]);
        self.set_tev_color_op(stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
        self.set_tev_alpha_op(stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
    }

    /// `GXSetTevColor` / `GXSetTevColorS10`: `id` 0 is TEVPREV, 1..3 REG0..2.
    pub fn set_tev_color_s10(&mut self, id: u8, c: [i16; 4]) {
        if let Some(r) = self.r.tev_regs.get_mut(id as usize) {
            *r = c;
            self.shader = None;
        }
    }

    /// `GXSetTevKColor`
    pub fn set_tev_kcolor(&mut self, id: u8, c: [u8; 4]) {
        if let Some(k) = self.r.kcolor.get_mut(id as usize) {
            *k = c;
            self.shader = None;
        }
    }

    /// `GXSetTevKColorSel`
    pub fn set_tev_kcolor_sel(&mut self, stage: u8, sel: u8) {
        if let Some(k) = self.r.kcsel.get_mut(stage as usize) {
            *k = sel;
            self.shader = None;
        }
    }

    /// `GXSetTevKAlphaSel`
    pub fn set_tev_kalpha_sel(&mut self, stage: u8, sel: u8) {
        if let Some(k) = self.r.kasel.get_mut(stage as usize) {
            *k = sel;
            self.shader = None;
        }
    }

    /// The value a konst selection reads: 0..7 are the fixed fractions
    /// 1, 7/8 .. 1/8; 0x0C..0x0F a whole KCOLOR (color only); 0x10..0x1F one
    /// component (R, G, B, A in groups of four) of KCOLOR0..3.
    fn konst(&self, sel: u8) -> [f32; 4] {
        let k = |i: u8| self.r.kcolor[(i & 3) as usize].map(|c| c as f32 / 255.0);
        match sel {
            0..=7 => [(8 - sel) as f32 / 8.0; 4],
            0x0C..=0x0F => k(sel),
            0x10..=0x1F => [k(sel)[((sel - 0x10) / 4) as usize]; 4],
            _ => [0.0; 4],
        }
    }

    // ---- Pixel engine ----

    /// `GXSetAlphaCompare`
    pub fn set_alpha_compare(&mut self, comp0: u8, ref0: u8, op: u8, comp1: u8, ref1: u8) {
        self.r.alpha_compare = AlphaCompare { comp0, ref0, op, comp1, ref1 };
        self.shader = None;
    }

    /// `GXSetBlendMode` (logic ops are not supported).
    pub fn set_blend_mode(&mut self, mode: u8, src: u8, dst: u8) {
        self.r.blend = (mode, src, dst);
    }

    /// `GXSetZMode`
    pub fn set_z_mode(&mut self, enable: bool, func: u8, update: bool) {
        self.r.z_mode = (enable, func, update);
    }

    /// `GXSetCullMode`
    pub fn set_cull_mode(&mut self, mode: u8) {
        self.r.cull = mode;
    }

    /// `GXSetColorUpdate`. With color updates off nothing is drawn (the
    /// rasterizer has no depth-only pass).
    pub fn set_color_update(&mut self, on: bool) {
        self.r.color_update = on;
    }

    /// `GXSetAlphaUpdate`, `GXSetZCompLoc`: no destination alpha and no early
    /// z here, so nothing to do.
    pub fn set_alpha_update(&mut self, _on: bool) {}

    // ---- Immediate mode ----

    /// `GXBegin`
    pub fn begin(&mut self, prim: u8, fmt: u8, _nverts: u16) {
        self.prim = Primitive::from_raw(prim).map(|p| (p, fmt));
        self.verts.clear();
        self.open = false;
    }

    fn next_vertex(&mut self) {
        if self.open {
            self.verts.push(self.cur);
        }
        self.open = true;
        self.tex_n = 0;
    }

    /// `GXPosition3f32`
    pub fn position3f32(&mut self, x: f32, y: f32, z: f32) {
        self.next_vertex();
        self.cur.pos = [x, y, z];
    }

    /// `GXPosition3s16`
    pub fn position3s16(&mut self, x: i16, y: i16, z: i16) {
        self.position3_raw(x as i32, y as i32, z as i32);
    }

    /// `GXPosition3u16`
    pub fn position3u16(&mut self, x: u16, y: u16, z: u16) {
        self.position3_raw(x as i32, y as i32, z as i32);
    }

    fn position3_raw(&mut self, x: i32, y: i32, z: i32) {
        self.next_vertex();
        self.cur.pos = [self.fixed(GX_VA_POS, x), self.fixed(GX_VA_POS, y), self.fixed(GX_VA_POS, z)];
    }

    /// `GXNormal3f32`
    pub fn normal3f32(&mut self, x: f32, y: f32, z: f32) {
        self.cur.nrm = [x, y, z];
    }

    /// `GXNormal3s16`
    pub fn normal3s16(&mut self, x: i16, y: i16, z: i16) {
        self.cur.nrm = [x, y, z].map(|v| self.fixed(GX_VA_NRM, v as i32));
    }

    /// `GXColor1u32`: RGBA8 packed as 0xRRGGBBAA.
    pub fn color1u32(&mut self, c: u32) {
        self.cur.clr[0] = c.to_be_bytes().map(|b| b as f32 / 255.0);
    }

    /// `GXTexCoord2f32`
    pub fn tex_coord2f32(&mut self, s: f32, t: f32) {
        self.push_tex([s, t]);
    }

    /// `GXTexCoord2s16`
    pub fn tex_coord2s16(&mut self, s: i16, t: i16) {
        self.tex_coord_raw(s as i32, t as i32);
    }

    /// `GXTexCoord2u16`
    pub fn tex_coord2u16(&mut self, s: u16, t: u16) {
        self.tex_coord_raw(s as i32, t as i32);
    }

    fn tex_coord_raw(&mut self, s: i32, t: i32) {
        let attr = GX_VA_TEX0 + self.tex_n as u8;
        if attr < GX_VA_POS + 13 {
            self.push_tex([self.fixed(attr, s), self.fixed(attr, t)]);
        }
    }

    fn push_tex(&mut self, st: [f32; 2]) {
        if let Some(t) = self.cur.tex.get_mut(self.tex_n) {
            *t = st;
        }
        self.tex_n += 1;
    }

    /// `GXEnd`: transform and light the vertices and queue the primitive.
    pub fn end(&mut self) {
        if self.open {
            self.verts.push(self.cur);
            self.open = false;
        }
        let Some((prim, _)) = self.prim.take() else { return };
        if !self.r.color_update || self.r.cull == GX_CULL_ALL {
            return;
        }
        let verts = core::mem::take(&mut self.verts);
        let out: Vec<Option<SVert>> = verts.iter().map(|v| self.xf(v)).collect();
        let st = self.pe_state();
        let sh = Sh::Gx(self.shader());
        let mut tri = |a: usize, b: usize, c: usize| {
            if let (Some(Some(a)), Some(Some(b)), Some(Some(c))) = (out.get(a), out.get(b), out.get(c)) {
                self.list.push(Prim { v: [*a, *b, *c], st, sh });
            }
        };
        let n = out.len();
        match prim {
            Primitive::Quads => {
                for q in (0..n / 4).map(|i| i * 4) {
                    tri(q, q + 1, q + 2);
                    tri(q, q + 2, q + 3);
                }
            }
            Primitive::Triangles => (0..n / 3).for_each(|i| tri(i * 3, i * 3 + 1, i * 3 + 2)),
            Primitive::TriangleStrip => {
                for i in 2..n {
                    if i % 2 == 0 { tri(i - 2, i - 1, i) } else { tri(i - 1, i - 2, i) }
                }
            }
            Primitive::TriangleFan => (2..n).for_each(|i| tri(0, i - 1, i)),
        }
        self.verts = verts;
    }

    fn pe_state(&self) -> State {
        let (enable, _func, update) = self.r.z_mode;
        let (mode, _src, dst) = self.r.blend;
        State {
            z_compare: enable,
            z_update: enable && update,
            blend: match mode {
                GX_BM_BLEND if dst == GX_BL_ONE => BlendMode::Add,
                GX_BM_BLEND => BlendMode::Alpha,
                _ => BlendMode::None,
            },
            cull: match self.r.cull {
                GX_CULL_FRONT => 1,
                GX_CULL_BACK => 2,
                _ => 0,
            },
        }
    }

    /// The TEV setup as a compiled shader, rebuilt only after it changed.
    fn shader(&mut self) -> &'a GxDraw<'a> {
        if let Some(s) = self.shader {
            return s;
        }
        let n = (self.r.num_tev_stages as usize).clamp(1, 16);
        let reg = |id: usize| Some(self.r.tev_regs[id]);
        let mat = Material {
            tev_orders: self.r.tev_orders[..n].to_vec(),
            tev_stages: self.r.tev_stages[..n].to_vec(),
            // Material order is REG0, REG1, REG2, PREV.
            tev_colors: [reg(1), reg(2), reg(3), reg(0)],
            textures: core::array::from_fn(|i| Some(i as u16)),
            alpha_compare: self.r.alpha_compare,
            ..Material::default()
        };
        let mut shader = Shader::new(&mat);
        for i in 0..n {
            let c = self.konst(self.r.kcsel[i]);
            shader.set_konst(i, [c[0], c[1], c[2]], self.konst(self.r.kasel[i])[3]);
        }
        let d = self.arena.alloc(GxDraw { shader, maps: self.tex_maps });
        self.shader = Some(d);
        d
    }

    /// XF: one vertex to screen space with its lit color and texcoords, or
    /// `None` when it is behind the eye.
    fn xf(&self, v: &VtxIn) -> Option<SVert> {
        let m = &self.r.pos_mtx[(self.r.cur_mtx / 3) as usize % 10];
        let eye = mtx::mult_vec(m, v.pos);
        let p = &self.r.proj;
        let clip: [f32; 4] = core::array::from_fn(|r| p[r][0] * eye[0] + p[r][1] * eye[1] + p[r][2] * eye[2] + p[r][3]);
        let w = clip[3];
        if w <= 1e-6 {
            return None;
        }
        let rw = 1.0 / w;
        let [vx, vy, vw, vh] = self.r.viewport;
        let x = vx + (1.0 + clip[0] * rw) * 0.5 * vw;
        let y = vy + (1.0 - clip[1] * rw) * 0.5 * vh;
        let z = 1.0 + clip[2] * rw;

        let nm = &self.r.nrm_mtx[(self.r.cur_mtx / 3) as usize % 10];
        let n = mtx::mult_vec_sr(nm, v.nrm);
        let color = if self.r.num_chans > 0 { self.light(0, eye, n, v.clr[0]) } else { [1.0; 4] };

        let mut uv = [[0.0; 2]; MAX_TC];
        for (g, out) in uv.iter_mut().enumerate().take(self.r.num_tex_gens as usize) {
            *out = self.tex_gen(g, v);
        }
        Some(SVert { x, y, z, rw, color, uv })
    }

    /// One color channel (0 or 1) for a vertex at `eye` with normal `n`.
    fn light(&self, chan: usize, eye: Vec3, n: Vec3, vtx: [f32; 4]) -> [f32; 4] {
        let mut out = [0.0; 4];
        for (ctl, range) in [(self.r.chans[chan], 0..3), (self.r.chans[chan + 2], 3..4)] {
            for i in range {
                let mat = if ctl.mat_src == GX_SRC_VTX { vtx[i] } else { self.r.mat_color[chan][i] as f32 / 255.0 };
                if !ctl.enable {
                    out[i] = mat;
                    continue;
                }
                let amb = if ctl.amb_src == GX_SRC_VTX { vtx[i] } else { self.r.amb_color[chan][i] as f32 / 255.0 };
                let mut sum = amb;
                for (l, light) in self.r.lights.iter().enumerate() {
                    if ctl.light_mask & (1 << l) != 0 {
                        sum += light.color[i] as f32 / 255.0 * Self::light_term(light, &ctl, eye, n);
                    }
                }
                out[i] = mat * sum.clamp(0.0, 1.0);
            }
        }
        out
    }

    /// Diffuse times attenuation of one light.
    fn light_term(l: &LightObj, ctl: &ChanCtrl, eye: Vec3, n: Vec3) -> f32 {
        let d = [l.pos[0] - eye[0], l.pos[1] - eye[1], l.pos[2] - eye[2]];
        let dist2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
        let dist = libm::sqrtf(dist2);
        let ld = if dist > 0.0 { [d[0] / dist, d[1] / dist, d[2] / dist] } else { [0.0; 3] };
        let dot = |a: Vec3, b: Vec3| a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        let ndl = dot(n, ld);
        let diffuse = match ctl.diff_fn {
            GX_DF_NONE => 1.0,
            GX_DF_SIGN => ndl,
            _ => ndl.max(0.0),
        };
        let attn = match ctl.attn_fn {
            GX_AF_SPOT => {
                // Cosine between the light's direction and its ray to the
                // vertex: `ld` points the other way, and so does the stored
                // (negated) direction.
                let c = dot(ld, l.dir);
                let ang = (l.a[0] + l.a[1] * c + l.a[2] * c * c).max(0.0);
                let dis = l.k[0] + l.k[1] * dist + l.k[2] * dist2;
                if dis > 0.0 { ang / dis } else { 0.0 }
            }
            GX_AF_SPEC => {
                // Specular: the light's direction is the half-angle vector.
                let c = dot(n, l.dir).max(0.0);
                let ang = (l.a[0] + l.a[1] * c + l.a[2] * c * c).max(0.0);
                let dis = l.k[0] + l.k[1] * c + l.k[2] * c * c;
                if ndl > 0.0 && dis > 0.0 { ang / dis } else { 0.0 }
            }
            _ => 1.0,
        };
        diffuse * attn
    }

    fn tex_gen(&self, g: usize, v: &VtxIn) -> [f32; 2] {
        let tg = self.r.tex_gens[g];
        let src = match tg.src {
            GX_TG_POS => [v.pos[0], v.pos[1], v.pos[2], 1.0],
            // The vertex's own normal, as sent: a normal-sourced texgen
            // gets its transform from the texture matrix alone.
            GX_TG_NRM => [v.nrm[0], v.nrm[1], v.nrm[2], 1.0],
            s if (GX_TG_TEX0..GX_TG_TEX0 + 8).contains(&s) => {
                let t = v.tex[(s - GX_TG_TEX0) as usize];
                [t[0], t[1], 1.0, 1.0]
            }
            _ => [0.0, 0.0, 1.0, 1.0],
        };
        if tg.mtx as u32 == GX_IDENTITY {
            return [src[0], src[1]];
        }
        let Some(m) = (tg.mtx as u32).checked_sub(GX_TEXMTX0).and_then(|i| self.r.tex_mtx.get((i / 3) as usize)) else {
            return [src[0], src[1]];
        };
        let row = |r: usize| m[r][0] * src[0] + m[r][1] * src[1] + m[r][2] * src[2] + m[r][3] * src[3];
        let (s, t) = (row(0), row(1));
        if tg.kind == GX_TG_MTX3X4 {
            let q = row(2);
            if q.abs() > 1e-6 {
                return [s / q, t / q];
            }
        }
        [s, t]
    }
}
