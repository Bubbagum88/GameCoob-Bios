//! The menu mosaic ("fx", `menu_fx.c`, PAL 1.0 BS2 0x81329B34..0x81330F30):
//! the words and digits of Game Play, Options and Calendar drawn with small
//! cubes. Every cube flies along a Hermite curve from the shape file (res
//! 37, [`shape`]) to its place; the per-language glyph tables (res
//! 179..184, [`anim`]) say which cubes make up each word.
//!
//! The submenus call one `Fx*_Update` per frame ([`Fx::option_update`],
//! [`Fx::clock_update`], [`Fx::game_update`]); each lays the shape out,
//! marks the cells to show, tint and sway in the cube pool, and appends the
//! cubes of the effect units ([`morph`], [`grid`], [`wipe`]). [`Fx::draw`]
//! (`menu_fx_draw.c`) draws them all.
//!
//! The pool holds five animated values per cube, `v[2k]` stepping one per
//! frame toward `v[2k+1]` (fn_8132BF20):
//!
//! | pair       | set by      | read as                                      |
//! |------------|-------------|----------------------------------------------|
//! | `v[0..1]`  | fn_8132CDD4 | `v[1]`: weight of colour pair 0; `v[0]`: how |
//! |            |             | much a line's lit cells grow                 |
//! | `v[2..3]`  | fn_8132CF7C | shown (alpha = 255 * progress * `v[2]`)      |
//! | `v[4..5]`  | fn_8132CE3C | (nothing reads it)                           |
//! | `v[6..7]`  | fn_8132CF14 | pair 1 instead of pair 2                     |
//! | `v[8..9]`  | fn_8132CEA4 | how much of the spark sway the cube follows  |

pub mod anim;
pub mod draw;
pub mod grid;
pub mod morph;
pub mod shape;
pub mod spark;
pub mod wipe;

use self::anim as e;
use super::gbase::hermite;
use super::mtx::{self, to_s16};
use super::render::Prepared;
use super::rnd::rand;
use crate::ipl::fx::{Glyphs, Shapes};
use crate::ipl::j3d::Model;
use crate::ipl::Ipl;
use crate::sdk::mtx as psmtx;
use alloc::vec;
use alloc::vec::Vec;

type Vec3 = [f32; 3];
pub type Rgba = [u8; 4];

/// `FX_MAX`: cubes in the pool.
pub const FX_MAX: usize = 0x861;

/// Mosaic cube model.
const RES_MODEL: u16 = 104;
/// CAFD flight paths.
const RES_SHAPES: u16 = 37;
/// Lit-cell tables in SRAM language order (En, De, Fr, Es, It, Nl).
const RES_GLYPHS: [u16; 6] = [180, 182, 181, 184, 183, 179];

/// A placed cube, as handed to the renderer (`FxCube`).
#[derive(Clone, Copy, Debug, Default)]
pub struct FxCube {
    pub scale: Vec3,
    pub rot: [i16; 3],
    pub pos: Vec3,
}

/// `FxTextLine`: offset, scale and how much lit cells grow.
#[derive(Clone, Copy)]
pub struct TextLine {
    pub x: f32,
    pub y: f32,
    pub scale: f32,
    pub spacing: f32,
}

const fn line(x: f32, y: f32, scale: f32, spacing: f32) -> TextLine {
    TextLine { x, y, scale, spacing }
}

/// The mosaic's fields of `MenuWork`.
/// `unk00` and `fxDepth` (where Fx_Draw splits the cubes between its two
/// passes) are left out: [`Fx::draw`] draws every cube each frame.
pub struct Params {
    pub title: [TextLine; 3],
    pub line_ofs: [i32; 6],
    pub item: [TextLine; 2],
    pub cursor_x: f32,
    pub cursor_y: f32,
    pub cursor_scale: f32,
    pub fade_in: i16,
    pub fade_out: i16,
    pub cube_scale: f32,
    pub b4: f32,
    pub b8: f32,
    pub bc: f32,
    pub c0: f32,
    pub c4: f32,
    pub c8: i16,
    pub cc: f32,
    pub d0: f32,
    pub d4: i16,
    pub ready_count: i16,
    pub ready_ratio: f32,
    pub blink_period: i16,
    pub f0: f32,
    pub sel: [f32; 8],
    pub sel2: [f32; 4],
    pub x124: i16,
    pub x126: i16,
    pub count_override: i16,
    pub game_color: [i16; 6],
    pub option_color: [i16; 18],
    pub clock_color: [i16; 18],
    pub x17e: u16,
}

/// Menu_SetDefaults (0x81311464) with Menu_SetPalTextLayout (0x813113EC).
pub const P: Params = Params {
    title: [line(10.0, -84.0, 0.55, 0.2), line(0.0, -35.0, 0.64, 0.2), line(0.0, 10.0, 0.66, 0.2)],
    line_ofs: [-2, -2, -2, -2, -1, -1],
    item: [line(0.0, -46.0, 0.74, 0.2), line(0.0, -20.0, 0.74, 0.2)],
    cursor_x: -8.0,
    cursor_y: -8.0,
    cursor_scale: 0.66,
    fade_in: 10,
    fade_out: 10,
    cube_scale: 1.0,
    b4: 2.0,
    b8: 5.0,
    bc: 30.0,
    c0: 3.0,
    c4: 0.45,
    c8: 120,
    cc: 0.4,
    d0: 2.0,
    d4: 10,
    ready_count: 58,
    ready_ratio: 0.9,
    blink_period: 25,
    f0: 10.0,
    sel: [-10.0, 31.0, -30.0, 31.0, -15.0, 13.0, 15.0, 13.0],
    sel2: [-50.0, 13.0, 50.0, 13.0],
    x124: 30,
    x126: 10,
    count_override: -1,
    game_color: [130, 50, 50, 20, 160, 70],
    option_color: [180, 20, 255, 100, 0, 50, 120, 0, 220, 80, 35, 25, 40, 0, 120, 40, 0, 10],
    clock_color: [240, 100, 255, 50, 0, 20, 235, 70, 185, 40, 0, 0, 160, 50, 90, 40, 0, 0],
    x17e: 1,
};

/// What the mosaic reads from the rest of BS2 each frame.
#[derive(Clone, Copy, Default)]
pub struct Env {
    /// Lang_Get: the menu language (0..5, SRAM order).
    pub lang: usize,
    /// Option_GetLang: the language the Options list is on.
    pub opt_lang: usize,
    /// Option_GetLangRow per language: y of the `lan1`..`lan6` anchors.
    pub lang_y: [i16; 6],
    /// OSTicksToCalendarTime(OSGetTime()): year, month 0..11, day, hour,
    /// minute, second.
    pub now: [i32; 6],
}

/// The mosaic's resources, read once from the dump (Fx_Init,
/// Fx_InitCubeModel).
pub struct FxData {
    pub shapes: Shapes,
    glyphs: Vec<Glyphs>,
    model: Prepared,
}

impl FxData {
    pub fn load(ipl: &Ipl) -> Option<FxData> {
        let shapes = Shapes::parse(&ipl.resource(RES_SHAPES).ok()?).ok()?;
        let glyphs = RES_GLYPHS.iter().map(|&r| Glyphs::parse(&ipl.resource(r).ok()?).ok()).collect::<Option<Vec<_>>>()?;
        let model = Prepared::new(Model::parse(&ipl.resource(RES_MODEL).ok()?).ok()?);
        Some(FxData { shapes, glyphs, model })
    }

    /// `sFxGlyphRes[lang]`.
    pub fn glyphs(&self, lang: usize) -> &Glyphs {
        &self.glyphs[lang.min(self.glyphs.len() - 1)]
    }
}

/// Per-cube animated values (`FxPool`).
struct Pool {
    v: [Vec<i16>; 10],
    /// Frames since the screen opened (`0xA794`).
    frame: i16,
    /// The frame count it steps to (`0xA796`).
    ready: i16,
}

impl Pool {
    fn new() -> Pool {
        Pool { v: core::array::from_fn(|_| vec![0; FX_MAX]), frame: 0, ready: 0 }
    }

    fn clear(&mut self) {
        self.v.iter_mut().for_each(|a| a.fill(0));
        self.frame = 0;
        self.ready = 0;
    }
}

/// `FxState`: the screen (1 Options, 2 Calendar, 3 Game Play) and its
/// sub-state, copied every frame. Options: `v[2]` sub, `v[3]` row, `v[4]`
/// offset, `v[5]` sound. Calendar: `v[6]` sub, `v[7]` row, `v[8]` date
/// field, `v[9]` time field, `v[10..13]` the values being edited. Game
/// Play: `v[0]` starting, `v[1]` disc message.
#[derive(Clone, Copy, Default)]
pub struct State {
    pub screen: i16,
    pub v: [i16; 13],
}

/// `FxSlide`: the language list position, eased over 10 frames.
#[derive(Clone, Copy, Default)]
struct Slide {
    t: i16,
    from: f32,
    to: f32,
}

/// `FxCursor`: the language word cross-fade.
struct Cursor {
    v: [i16; 3],
    f: [f32; 3],
    delay: [u8; 0x280],
}

/// The mosaic.
pub struct Fx {
    pool: Pool,
    state: State,
    prev: State,
    cube: Vec<FxCube>,
    alpha: Vec<u8>,
    light: Vec<Rgba>,
    dark: Vec<Rgba>,
    cursor: Cursor,
    slide: Slide,
    blink: u16,
    /// `sFxRand`: drawn when Game Play starts its fly-out; nothing reads it.
    #[allow(dead_code)]
    rand: i32,
    /// `sFxLight0..2`, `sFxDark0..2`.
    lights: [Rgba; 3],
    darks: [Rgba; 3],
    count: u32,
    lang: i32,
    /// `sFxUiLang`: FxOption_ChangeUiLang is empty, so nothing reads it.
    #[allow(dead_code)]
    ui_lang: i32,
    spark: spark::Spark,
    grid: grid::Grid,
    wipe: wipe::Wipe,
    morph: morph::Morph,
    env: Env,
}

/// fn_8132BFF8: one step toward a target.
fn step(v: &mut i16, target: i16) {
    if *v > target {
        *v -= 1;
    } else if *v < target {
        *v += 1;
    }
}

/// fn_8132C660: split 0..9999 into decimal digits, ones first.
pub fn digits(value: i32) -> [i16; 4] {
    let mut d = [0i16; 4];
    let mut v = value;
    if v > 999 {
        d[3] = (v / 1000) as i16;
        v -= d[3] as i32 * 1000;
    }
    if v > 99 {
        d[2] = (v / 100) as i16;
        v -= d[2] as i32 * 100;
    }
    if v > 9 {
        d[1] = (v / 10) as i16;
        v -= d[1] as i32 * 10;
    }
    d[0] = v as i16;
    d
}

/// fn_8132C830: centre of `n` cubes (the ones `idx` names).
pub fn centre(cubes: &[FxCube], idx: impl Iterator<Item = usize>, n: u32) -> Vec3 {
    let (mut x, mut y, mut z) = (0.0f32, 0.0f32, 0.0f32);
    for i in idx {
        let c = cubes.get(i).copied().unwrap_or_default();
        x += c.pos[0];
        y += c.pos[1];
        z += c.pos[2];
    }
    [x / n as f32, y / n as f32, z / n as f32]
}

/// fn_8132C750 (Fx_RotAbout): rotate a cube about `o`; the result holds the
/// position change and the angles.
pub fn rot_about(r: [i16; 3], c: &FxCube, o: Vec3) -> FxCube {
    let m = mtx::rot(r[0] as i32, r[1] as i32, r[2] as i32);
    let v = [c.pos[0] - o[0], c.pos[1] - o[1], c.pos[2] - o[2]];
    let q = psmtx::mult_vec(&m, v);
    FxCube { scale: [0.0; 3], rot: r, pos: [(q[0] + o[0]) - c.pos[0], (q[1] + o[1]) - c.pos[1], (q[2] + o[2]) - c.pos[2]] }
}

/// Fx_ScaleAbout: scale a cube's offset from `o` and its size.
fn scale_about(o: Vec3, s: f32, c: &mut FxCube) {
    c.pos[0] = o[0] + s * (c.pos[0] - o[0]);
    c.pos[1] = o[1] + s * (c.pos[1] - o[1]);
    c.pos[2] = o[2] + s * (c.pos[2] - o[2]);
    c.scale = c.scale.map(|v| v * s);
}

/// fn_8132CB60: degrees to a 16-bit angle.
pub fn deg(d: f32) -> i16 {
    (65535.0 * d / 360.0) as i32 as i16
}

/// fn_81330518: bilinear colour mix, `(a*u + b*(1-u))*(1-v) + c*v`.
fn mix(a: &Rgba, b: &Rgba, c: &Rgba, u: f32, v: f32) -> Rgba {
    let (iu, iv) = (1.0 - u, 1.0 - v);
    core::array::from_fn(|k| (v * c[k] as f32 + iv * (u * a[k] as f32 + iu * b[k] as f32)) as u8)
}

/// Fx_Squash: the fade-out's squash over its first tenth.
fn squash(t: f32) -> f32 {
    if t < 0.1 {
        1.0 - 0.05 * hermite(t, 0.0, 0.0, 0.0, 0.1, 1.0, 0.0)
    } else {
        1.0 - 0.05 * hermite(t, 0.1, 1.0, 0.0, 1.0, 0.0, 0.0)
    }
}

/// `sFxFlyDelay`: per-cube fly-out delay (0xFF = first to go).
#[rustfmt::skip]
static FLY_DELAY: [u8; 0x2C8] = [
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF1, 0xED, 0xE9, 0xE6, 0xE1,
    0xDD, 0xD9, 0xD4, 0xCF, 0xCB, 0xC5, 0xC1, 0xBC, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9A, 0x95, 0x8A,
    0x84, 0x7E, 0x79, 0x73, 0x68, 0x63, 0x53, 0x48, 0x44, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF1, 0xED, 0xE9, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC5,
    0xC0, 0xBB, 0xB5, 0xB0, 0xAB, 0xA6, 0xA0, 0x9A, 0x95, 0x8F, 0x89, 0x84, 0x7E, 0x79, 0x62, 0x5D,
    0x4E, 0x43, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF0, 0xED,
    0xEA, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC5, 0xC1, 0xBB, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0,
    0x9A, 0x95, 0x8F, 0x89, 0x84, 0x7F, 0x79, 0x73, 0x5D, 0x58, 0x49, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF0, 0xED, 0xEA, 0xE5, 0xE2, 0xDE, 0xD8, 0xD4, 0xCF, 0xCB, 0xC6,
    0xC1, 0xBB, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9B, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x78, 0x73, 0x6D,
    0x68, 0x62, 0x58, 0x52, 0x44, 0x3A, 0x36, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7,
    0xF4, 0xF1, 0xED, 0xE9, 0xE6, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC5, 0xC1, 0xBB, 0xB6, 0xB0,
    0xAB, 0xA5, 0xA0, 0x9B, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6E, 0x68, 0x53, 0x4E, 0x44,
    0x3F, 0x36, 0x32, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF0, 0xED, 0xEA, 0xE5, 0xE1,
    0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC5, 0xC1, 0xBC, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9A, 0x95, 0x8F,
    0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6D, 0x68, 0x62, 0x4E, 0x49, 0x44, 0x3B, 0x32, 0x2E, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF1, 0xED, 0xEA, 0xE6, 0xE1, 0xDD, 0xD8, 0xD4, 0xCF, 0xCB,
    0xC6, 0xC1, 0xBB, 0xB5, 0xB0, 0xAB, 0xA5, 0xA0, 0x9A, 0x95, 0x8F, 0x89, 0x84, 0x7E, 0x78, 0x73,
    0x6E, 0x68, 0x62, 0x5E, 0x58, 0x53, 0x49, 0x44, 0x36, 0x2E, 0x2A, 0xD9, 0xD4, 0xCF, 0xCB, 0xC5,
    0xB0, 0xAB, 0xA5, 0xA0, 0x9A, 0x95, 0x8F, 0x8A, 0x84, 0x7F, 0xCA, 0xC6, 0xC0, 0xBB, 0xA6, 0xA0,
    0x9A, 0x95, 0x90, 0x8A, 0x84, 0x7E, 0x78, 0x73, 0x6E, 0x58, 0x52, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFC, 0xFA, 0xF7, 0xF4, 0xED, 0xE9, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC6, 0xC0, 0xBB,
    0xB6, 0xB0, 0xAB, 0xA5, 0xA0, 0x9B, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6E, 0x68, 0x62,
    0x5D, 0x58, 0x52, 0x4E, 0x48, 0x44, 0x3F, 0x3A, 0x36, 0x32, 0x2E, 0x29, 0x26, 0x22, 0x1F, 0x1C,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF1, 0xEA, 0xE5, 0xE1, 0xDD, 0xD8, 0xD4, 0xCF,
    0xCA, 0xC5, 0xC1, 0xBB, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9A, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x78,
    0x73, 0x6D, 0x68, 0x62, 0x5D, 0x58, 0x53, 0x4E, 0x48, 0x44, 0x3F, 0x3B, 0x36, 0x32, 0x2E, 0x2A,
    0x26, 0x23, 0x1F, 0x1C, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF0, 0xED, 0xE5, 0xE1, 0xDD,
    0xD8, 0xD4, 0xCF, 0xCA, 0xC6, 0xC0, 0xBB, 0xB6, 0xB0, 0xAB, 0xA5, 0xA0, 0x9A, 0x95, 0x8F, 0x8A,
    0x84, 0x7F, 0x79, 0x73, 0x6D, 0x68, 0x62, 0x5D, 0x58, 0x53, 0x4E, 0x49, 0x43, 0x3F, 0x3A, 0x36,
    0x32, 0x2E, 0x29, 0x26, 0x22, 0x1F, 0x1C, 0x19, 0xFF, 0xFF, 0xFC, 0xFA, 0xF7, 0xF4, 0xF0, 0xED,
    0xE9, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xCF, 0xCA, 0xC5, 0xC1, 0xBB, 0xB6, 0xB1, 0xAB, 0xA6, 0xA0,
    0x9A, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6E, 0x67, 0x63, 0x5D, 0x58, 0x52, 0x4D, 0x49,
    0x43, 0x3F, 0x3B, 0x36, 0x32, 0x2E, 0x2A, 0x26, 0x22, 0x1F, 0x1C, 0x19, 0x16, 0x14, 0xFF, 0xFF,
    0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF0, 0xED, 0xE6, 0xE1, 0xDD, 0xD9, 0xD4, 0xD0, 0xCA, 0xC5, 0xC0,
    0xBB, 0xB5, 0xB0, 0xAB, 0xA5, 0xA0, 0x9B, 0x95, 0x8F, 0x8A, 0x84, 0x7E, 0x79, 0x73, 0x6D, 0x68,
    0x63, 0x5D, 0x58, 0x52, 0x4D, 0x49, 0x44, 0x3F, 0x3B, 0x36, 0x32, 0x2E, 0x2A, 0x26, 0x23, 0x1F,
    0x1C, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFA, 0xF7, 0xF4, 0xF1, 0xEA, 0xE6, 0xE2, 0xDD, 0xD9,
    0xD4, 0xCF, 0xCA, 0xC5, 0xC0, 0xBB, 0xB6, 0xB0, 0xAB, 0xA6, 0xA0, 0x9B, 0x94, 0x8F, 0x8A, 0x84,
    0x7E, 0x78, 0x73, 0x6D, 0x68, 0x63, 0x5D, 0x58, 0x53, 0x4D, 0x49, 0x44, 0x3F, 0x3B, 0x36, 0x32,
    0x2D, 0x29, 0x26, 0x22, 0x1F, 0x1C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFA, 0xF8, 0xF4, 0xF1,
    0xED, 0xE9, 0xE5, 0xE1, 0xDD, 0xD9, 0xD4, 0xD0, 0xCA, 0xC6, 0xC0, 0xBB, 0xB6, 0xB0, 0xAB, 0xA5,
    0xA0, 0x9A, 0x95, 0x8F, 0x89, 0x84, 0x7F, 0x79, 0x73, 0x6D, 0x68, 0x62, 0x5D, 0x58, 0x52, 0x4D,
    0x49, 0x44, 0x3F, 0x3A, 0x36, 0x32, 0x2D, 0x2A, 0x26, 0x22, 0x1F, 0x1C, 0xED, 0xE9, 0xE5, 0xE1,
    0xA0, 0x9B, 0xED, 0xE2, 0x00, 0x00, 0x00, 0x00,
];

type Setter = fn(&mut Fx, usize, bool);

/// FX_APPEND: add an effect's cubes and alphas to the pool.
fn append(cube: &mut [FxCube], alpha: &mut [u8], count: &mut u32, src: &[FxCube], a: &[u8], n: usize) {
    let at = *count as usize;
    let n = n.min(FX_MAX.saturating_sub(at)).min(src.len()).min(a.len());
    cube[at..at + n].copy_from_slice(&src[..n]);
    alpha[at..at + n].copy_from_slice(&a[..n]);
    *count += n as u32;
}

impl Fx {
    /// Fx_Init's state (the resources are [`FxData`]).
    pub fn new() -> Fx {
        Fx {
            pool: Pool::new(),
            state: State::default(),
            prev: State::default(),
            cube: vec![FxCube::default(); FX_MAX],
            alpha: vec![0; FX_MAX],
            light: vec![[0; 4]; FX_MAX],
            dark: vec![[0; 4]; FX_MAX],
            cursor: Cursor { v: [0; 3], f: [0.0; 3], delay: [0; 0x280] },
            slide: Slide::default(),
            blink: 0,
            rand: 0,
            lights: [[0; 4]; 3],
            darks: [[0; 4]; 3],
            count: 0,
            lang: 0,
            ui_lang: 0,
            spark: spark::Spark::new(),
            grid: grid::Grid::new(),
            wipe: wipe::Wipe::new(),
            morph: morph::Morph::new(),
            env: Env::default(),
        }
    }

    /// Fx_IsReady (0x81329B6C): true once at most `margin` frames of the
    /// fly-in are left.
    pub fn is_ready(&self, margin: i32) -> bool {
        self.pool.frame as u32 >= (P.ready_count as i32 - margin) as u32
    }

    /// Fx_Clear (0x81329C98): close the mosaic; the next Fx*_Update starts
    /// its screen again. What was last placed stays drawable.
    pub fn clear(&mut self) {
        self.close();
        self.pool.clear();
        self.slide = Slide::default();
    }

    fn set_colors(&mut self, c: &[i16; 18]) {
        for k in 0..3 {
            self.lights[k] = [c[6 * k] as u8, c[6 * k + 2] as u8, c[6 * k + 4] as u8, 0xFF];
            self.darks[k] = [c[6 * k + 1] as u8, c[6 * k + 3] as u8, c[6 * k + 5] as u8, 0xFF];
        }
    }

    fn cells(&mut self, g: &Glyphs, entry: usize, f: Setter, on: bool) {
        for c in g.list(entry) {
            f(self, c as usize, on);
        }
    }

    /// FX_DIGIT: show digit `d` in a digit slot entry.
    fn digit(&mut self, g: &Glyphs, slot: usize, d: i16) {
        let shape = e::digit(d);
        for i in 0..g.count(slot) as usize {
            let c = g.at(slot, g.at(shape, i) as usize);
            self.cf7c(c as usize, true);
        }
    }

    /// FxOption_Update (0x81329CE4): one frame of the Options mosaic.
    pub fn option_update(&mut self, d: &FxData, env: Env, sub: i16, row: i16, offset: i16, sound: i16) {
        self.env = env;
        let opt_lang = env.opt_lang as i32;
        if self.lang != opt_lang {
            self.change_lang(self.lang, opt_lang);
        }
        self.lang = opt_lang;
        // FxOption_ChangeUiLang does nothing.
        self.ui_lang = env.lang as i32;
        self.set_option_state(sub, row, offset, sound);
        if self.prev.screen != 1 {
            self.morph.reset();
            self.spark.reset();
            self.pool.clear();
            self.slide = Slide::default();
            self.pool.ready = P.ready_count;
            self.blink = 0;
            self.lang = opt_lang;
            self.change_lang(self.lang, self.lang);
            self.cursor_park(opt_lang);
        }
        self.set_colors(&P.option_color);
        self.reset_colors();
        self.option_rows(d);
        self.option_show(d);
        self.option_light(d);
        self.option_sway(d);
        let t = self.progress();
        self.count = shape::eval(&d.shapes, 2, &mut self.cube, t);
        self.option_bars(d);
        self.option_lines(d);
        if self.progress() > P.ready_ratio {
            self.spark.options(&self.cube, d.glyphs(env.lang));
        }
        self.tint();
        self.morph.update(&self.state, &self.prev);
        let (cubes, n, a) = self.morph.cubes(&d.shapes);
        let alpha = vec![a; n];
        append(&mut self.cube, &mut self.alpha, &mut self.count, &cubes, &alpha, n);
        self.grid.options(&self.cube, d.glyphs(env.lang), &self.state, &self.prev);
        let (c, a, n) = self.grid.out();
        append(&mut self.cube, &mut self.alpha, &mut self.count, c, a, n);
        if self.state.v[2] == 2 && self.prev.v[2] != 2 {
            self.grid.finish();
        }
        self.wipe.options(&self.cube, d.glyphs(env.lang), &self.state, &self.prev);
        let (c, a, n) = self.wipe.out();
        append(&mut self.cube, &mut self.alpha, &mut self.count, c, a, n);
        if self.state.v[2] == 1 && self.prev.v[2] != 1 {
            self.wipe.finish();
        }
        if self.state.v[2] == 3 {
            self.cursor_fade(d);
        }
        self.step_all();
        self.blink += 1;
        if self.blink as i32 >= P.blink_period as i32 {
            self.blink = 0;
        }
    }

    /// FxClock_Update (0x8132A14C): one frame of the Calendar mosaic.
    /// `edit` is `ClockEdit.value` (year, month 0..11, day or hour, minute,
    /// second).
    pub fn clock_update(&mut self, d: &FxData, env: Env, sub: i16, row: i16, date_field: i16, time_field: i16, edit: [i16; 3]) {
        self.env = env;
        self.set_clock_state(sub, row, date_field, time_field, edit);
        if self.prev.screen != 2 {
            self.pool.clear();
            self.slide = Slide::default();
            self.pool.ready = P.ready_count;
            self.spark.reset();
        }
        self.set_colors(&P.clock_color);
        self.reset_colors();
        self.clock_digits(d);
        self.clock_light(d);
        let t = self.progress();
        self.count = shape::eval(&d.shapes, 0, &mut self.cube, t);
        self.clock_lines(d);
        if self.progress() > P.ready_ratio {
            self.spark.clock(&self.cube, d.glyphs(env.lang));
        }
        self.tint();
        self.grid.clock(&self.cube, d.glyphs(env.lang), &self.state, &self.prev, env.now);
        let (c, a, n) = self.grid.out();
        append(&mut self.cube, &mut self.alpha, &mut self.count, c, a, n);
        if self.state.v[6] != 0 && self.prev.v[6] == 0 {
            self.grid.finish();
        }
        self.step_all();
    }

    /// FxGame_Update (0x8132A3D4): one frame of the Game Play mosaic.
    /// `starting` flies the text away; `msg` is Game_FxMessage.
    pub fn game_update(&mut self, d: &FxData, env: Env, starting: bool, msg: i16) {
        self.env = env;
        self.set_game_state(starting as i16, msg);
        if self.prev.screen != 3 {
            self.pool.clear();
            self.slide = Slide::default();
            for i in 0..FX_MAX {
                self.alpha[i] = 0xFF;
                self.cea4(i, true);
                self.ce3c(i, true);
                self.cf7c(i, true);
            }
            self.spark.reset();
            self.pool.ready = P.ready_count;
        }
        let (l, k) = (&P.game_color, 0xFF);
        for c in 0..3 {
            self.lights[c][..3].copy_from_slice(&[l[0] as u8, l[2] as u8, l[4] as u8]);
            self.darks[c][..3].copy_from_slice(&[l[1] as u8, l[3] as u8, l[5] as u8]);
        }
        self.lights[0][3] = k;
        self.darks[0][3] = k;
        if P.count_override >= 0 {
            self.pool.frame = P.count_override;
        }
        if self.state.v[0] == 0 {
            let t = self.progress();
        self.count = shape::eval(&d.shapes, 1, &mut self.cube, t);
            self.game_cursor();
        } else {
            if self.prev.v[0] == 0 {
                self.pool.frame = 0;
                self.pool.ready = P.ready_count;
                self.rand = (rand() % 5) as i32;
            }
            self.fly_out(d, self.progress());
            self.game_cursor();
        }
        self.reset_colors();
        self.game_cells();
        if self.progress() > P.ready_ratio {
            self.spark.game(&mut self.cube, self.count);
        }
        self.tint();
        self.wipe.game(&self.cube, d.glyphs(env.lang), &self.state, &self.prev);
        let (c, a, n) = self.wipe.out();
        append(&mut self.cube, &mut self.alpha, &mut self.count, c, a, n);
        self.step_all();
    }

    /// fn_8132A690: the date and time shown, with the group being edited
    /// taken from `e`.
    fn shown_time(&self, field: i16, e: [i16; 3]) -> [i16; 6] {
        let [mut year, mut mon, mut mday, mut hour, mut min, mut sec] = self.env.now;
        if field == 1 {
            year = e[0] as i32;
            mon = e[1] as i32;
            mday = e[2] as i32;
        } else if field == 2 {
            hour = e[0] as i32;
            min = e[1] as i32;
            sec = e[2] as i32;
        }
        [year as i16, (mon + 1) as i16, mday as i16, hour as i16, min as i16, sec as i16]
    }

    /// fn_8132A75C: show or hide the Options lines and their arrows.
    fn option_show(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        let (title, item, lang) = (e::SOUND_ROW, e::OFFSET_ROW, e::LANG_LIST);
        match self.state.v[3] {
            0 => self.cells(g, title, Fx::cf14, true),
            1 => self.cells(g, item, Fx::cf14, true),
            2 => self.cells(g, lang, Fx::cf14, true),
            _ => {}
        }
        match self.state.v[2] {
            0 => {
                self.cells(g, title, Fx::ce3c, true);
                self.cells(g, item, Fx::ce3c, true);
                self.cells(g, lang, Fx::ce3c, true);
                self.bars(g, [false; 6]);
            }
            1 => {
                self.cells(g, title, Fx::ce3c, true);
                self.cells(g, item, Fx::ce3c, false);
                self.cells(g, lang, Fx::ce3c, false);
                match self.state.v[5] {
                    0 => self.bars(g, [false, true, false, false, false, false]),
                    1 => self.bars(g, [true, false, false, false, false, false]),
                    _ => {}
                }
                if self.prev.v[2] == 1 {
                    self.cells(g, title, Fx::cf7c, false);
                }
            }
            2 => {
                self.cells(g, title, Fx::ce3c, false);
                self.cells(g, item, Fx::ce3c, true);
                self.cells(g, lang, Fx::ce3c, false);
                if self.prev.v[2] == 2 {
                    self.cells(g, e::OFFSET_DIGITS[0], Fx::cf7c, false);
                    self.cells(g, e::OFFSET_DIGITS[1], Fx::cf7c, false);
                }
                let o = self.state.v[4];
                self.bars(g, [false, false, o > -0x20, o < 0x20, false, false]);
            }
            3 => {
                self.cells(g, title, Fx::ce3c, false);
                self.cells(g, item, Fx::ce3c, false);
                self.cells(g, lang, Fx::ce3c, true);
                let l = self.env.opt_lang;
                self.bars(g, [false, false, false, false, l != 0, l != 5]);
            }
            _ => {}
        }
    }

    /// fn_8132EEB0 for each of the six arrow bars in turn.
    fn bars(&mut self, g: &Glyphs, on: [bool; 6]) {
        for (k, &o) in on.iter().enumerate() {
            self.bar(g, k, o);
        }
    }

    /// fn_8132ADCC: the cells of the sound and offset rows.
    fn option_rows(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        self.cells(g, e::OFFSET_ROW, Fx::cf7c, false);
        self.cells(g, e::SOUND_ROW, Fx::cf7c, false);
        self.cells(g, e::LANG_LIST, Fx::cf7c, true);
        match self.state.v[5] {
            0 => {
                self.cells(g, e::MONO, Fx::cf7c, true);
                for (k, on) in [false, true, false, false].into_iter().enumerate() {
                    self.bar(g, k, on);
                }
            }
            1 => {
                self.cells(g, e::STEREO, Fx::cf7c, true);
                for (k, on) in [true, false, false, false].into_iter().enumerate() {
                    self.bar(g, k, on);
                }
            }
            _ => {}
        }
        let o = self.state.v[4];
        if o < 0 {
            self.cells(g, e::OFFSET_MINUS, Fx::cf7c, true);
        } else if o > 0 {
            self.cells(g, e::OFFSET_PLUS, Fx::cf7c, true);
        }
        let n = digits(libm::fabs(o as f64) as i16 as i32);
        self.digit(g, e::OFFSET_DIGITS[0], n[1]);
        self.digit(g, e::OFFSET_DIGITS[1], n[0]);
    }

    /// fn_8132B114: the Calendar date and time read-out.
    fn clock_digits(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        let v = &self.state.v;
        let dt = self.shown_time(v[6], [v[10], v[11], v[12]]);
        self.cells(g, e::DATE_ROW, Fx::cf7c, false);
        self.cells(g, e::TIME_ROW, Fx::cf7c, false);
        let n = digits(dt[0] as i32);
        for (k, &s) in e::YEAR_DIGITS.iter().enumerate() {
            self.digit(g, s, n[3 - k]);
        }
        for (field, slots) in [(1, e::MONTH_DIGITS), (2, e::DAY_DIGITS), (3, e::HOUR_DIGITS), (4, e::MINUTE_DIGITS), (5, e::SECOND_DIGITS)] {
            let n = digits(dt[field] as i32);
            self.digit(g, slots[0], n[1]);
            self.digit(g, slots[1], n[0]);
        }
        self.cells(g, e::SEPARATORS, Fx::cf7c, true);
    }

    /// FX_EDIT_CELLS: the cells of the field being edited: lit, shown (the
    /// cursor flight draws them once it runs), not swaying.
    fn edit_cells(&mut self, g: &Glyphs, entry: usize, prev_field: i16) {
        for c in g.list(entry) {
            let c = c as usize;
            if self.prev.v[6] == prev_field {
                self.cf7c(c, false);
            }
            self.ce3c(c, true);
            self.cdd4(c, true);
            self.cea4(c, false);
        }
    }

    /// fn_8132B678: highlight the Calendar row and the field being edited.
    fn clock_light(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        let (on, off) = if self.state.v[7] != 0 { (e::TIME_ROW, e::DATE_ROW) } else { (e::DATE_ROW, e::TIME_ROW) };
        if self.state.v[7] != 0 {
            for c in g.list(off) {
                self.cdd4(c as usize, false);
                self.cea4(c as usize, false);
            }
            for c in g.list(on) {
                self.cdd4(c as usize, false);
                self.cea4(c as usize, true);
                self.cf14(c as usize, true);
            }
        } else {
            for c in g.list(on) {
                self.cdd4(c as usize, false);
                self.cea4(c as usize, true);
                self.cf14(c as usize, true);
            }
            for c in g.list(off) {
                self.cdd4(c as usize, false);
                self.cea4(c as usize, false);
            }
        }
        match self.state.v[6] {
            1 => {
                if P.x17e == 0 {
                    self.cells(g, e::DATE_ROW, Fx::cea4, false);
                }
                match self.state.v[8] {
                    0 => {
                        for s in [e::YEAR_DIGITS[0], e::YEAR_DIGITS[1]] {
                            for c in g.list(s) {
                                self.ce3c(c as usize, true);
                                self.cdd4(c as usize, true);
                                self.cea4(c as usize, false);
                            }
                        }
                        self.edit_cells(g, e::YEAR_DIGITS[2], 1);
                        self.edit_cells(g, e::YEAR_DIGITS[3], 1);
                    }
                    1 => self.edit_cells(g, e::MONTH, 1),
                    2 => self.edit_cells(g, e::DAY, 1),
                    _ => {}
                }
            }
            2 => {
                if P.x17e == 0 {
                    self.cells(g, e::TIME_ROW, Fx::cea4, false);
                }
                match self.state.v[9] {
                    0 => self.edit_cells(g, e::HOUR, 2),
                    1 => self.edit_cells(g, e::MINUTE, 2),
                    2 => self.edit_cells(g, e::SECOND, 2),
                    _ => {}
                }
            }
            _ => {
                self.cells(g, e::DATE_ROW, Fx::ce3c, true);
                self.cells(g, e::TIME_ROW, Fx::ce3c, true);
            }
        }
    }

    /// fn_8132BD00: fade, sway and tint every placed cube.
    fn tint(&mut self) {
        let t = self.progress();
        for i in 0..(self.count as usize).min(FX_MAX) {
            self.alpha[i] = (255.0 * (t * self.cc00(i))) as u8;
            let o = self.spark.cube[i];
            let w = self.cc78(i);
            let c = &mut self.cube[i];
            c.pos[0] += o.pos[0] * w;
            c.pos[1] += o.pos[1] * w;
            c.pos[2] += o.pos[2] * w;
            c.rot = o.rot.map(|r| to_s16(r as f32 * w));
            let (u, v) = (self.cd5c(i), self.cb88(i));
            self.light[i] = mix(&self.lights[1], &self.lights[2], &self.lights[0], u, v);
            self.dark[i] = mix(&self.darks[1], &self.darks[2], &self.darks[0], u, v);
        }
    }

    /// fn_8132BF20: step every animated value one frame toward its target.
    fn step_all(&mut self) {
        let v = &mut self.pool.v;
        for i in 0..(self.count as usize).min(FX_MAX) {
            for k in [0, 2, 4, 8, 6] {
                let target = v[k + 1][i];
                step(&mut v[k][i], target);
            }
        }
        step(&mut self.pool.frame, self.pool.ready);
        step(&mut self.slide.t, 10);
        step(&mut self.cursor.v[2], 10);
    }

    /// fn_8132C024: light the Options line being edited.
    fn option_light(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        let on = match self.state.v[2] {
            1 => [true, false, false],
            2 => [false, true, false],
            3 => [false, false, true],
            _ => [false; 3],
        };
        for (entry, on) in [e::SOUND_ROW, e::OFFSET_ROW, e::LANG_LIST].into_iter().zip(on) {
            self.cells(g, entry, Fx::cdd4, on);
        }
    }

    /// fn_8132C2FC: sway the Options line that has the cursor.
    fn option_sway(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        let on = if self.state.v[2] == 0 {
            match self.state.v[3] {
                0 => [true, false, false],
                1 => [false, true, false],
                2 => [false, false, true],
                _ => return,
            }
        } else {
            [false; 3]
        };
        for (entry, on) in [e::SOUND_ROW, e::OFFSET_ROW, e::LANG_LIST].into_iter().zip(on) {
            self.cells(g, entry, Fx::cea4, on);
        }
    }

    /// fn_8132C5E0: Game Play's own cells sway and stay hidden (its text is
    /// the wipe's copy).
    fn game_cells(&mut self) {
        for i in 0..FX_MAX {
            self.cea4(i, true);
            self.ce3c(i, true);
            self.cdd4(i, false);
            self.cf7c(i, false);
        }
        for i in 0..FX_MAX {
            self.cf7c(i, false);
        }
    }

    /// fn_8132CB88: weight of colour pair 0 (the target, not the eased value).
    fn cb88(&self, i: usize) -> f32 {
        self.pool.v[1][i] as f32 / P.fade_out as f32
    }

    /// fn_8132CC00: how far the cube is shown.
    fn cc00(&self, i: usize) -> f32 {
        self.pool.v[2][i] as f32 / P.fade_in as f32
    }

    /// fn_8132CC78: how much of the sway the cube follows.
    fn cc78(&self, i: usize) -> f32 {
        self.pool.v[8][i] as f32 / P.fade_out as f32
    }

    /// fn_8132CCF4: how far the screen has come in (0..1).
    fn progress(&self) -> f32 {
        self.pool.frame as f32 / P.ready_count as f32
    }

    /// fn_8132CD5C: pair 1 rather than pair 2.
    fn cd5c(&self, i: usize) -> f32 {
        self.pool.v[6][i] as f32 / P.fade_in as f32
    }

    /// Fx_Lit: how much a lit cell has grown.
    fn lit(&self, i: usize) -> f32 {
        self.pool.v[0].get(i).copied().unwrap_or(0) as f32 / P.fade_out as f32
    }

    fn set(&mut self, k: usize, i: usize, v: i16) {
        if let Some(x) = self.pool.v[k].get_mut(i) {
            *x = v;
        }
    }

    /// fn_8132CDD4
    fn cdd4(&mut self, i: usize, on: bool) {
        self.set(1, i, if on { P.fade_out } else { 0 });
    }

    /// fn_8132CE3C
    fn ce3c(&mut self, i: usize, on: bool) {
        self.set(5, i, if on { P.fade_in } else { 0 });
    }

    /// fn_8132CEA4
    fn cea4(&mut self, i: usize, on: bool) {
        self.set(9, i, if on { P.fade_out } else { 0 });
    }

    /// fn_8132CF14
    fn cf14(&mut self, i: usize, on: bool) {
        self.set(7, i, if on { P.fade_in } else { 0 });
    }

    /// fn_8132CF7C: show or hide now.
    fn cf7c(&mut self, i: usize, on: bool) {
        let v = if on { P.fade_in } else { 0 };
        self.set(2, i, v);
        self.set(3, i, v);
    }

    /// fn_8132D000: close the mosaic.
    fn close(&mut self) {
        self.prev = self.state;
        self.state.screen = 0;
        self.state.v[1..].fill(0x7F);
    }

    /// fn_8132D084
    fn set_game_state(&mut self, starting: i16, msg: i16) {
        self.prev = self.state;
        self.state.screen = 3;
        self.state.v = [0x7F; 13];
        self.state.v[0] = starting;
        self.state.v[1] = msg;
    }

    /// FxOption_SetState (0x8132D10C)
    fn set_option_state(&mut self, sub: i16, row: i16, offset: i16, sound: i16) {
        self.prev = self.state;
        self.state.screen = 1;
        self.state.v = [0x7F; 13];
        self.state.v[2..6].copy_from_slice(&[sub, row, offset, sound]);
    }

    /// fn_8132D194
    fn set_clock_state(&mut self, sub: i16, row: i16, date_field: i16, time_field: i16, e: [i16; 3]) {
        self.prev = self.state;
        self.state.screen = 2;
        self.state.v = [0x7F; 13];
        self.state.v[6..13].copy_from_slice(&[sub, row, date_field, time_field, e[0], e[1], e[2]]);
    }

    /// FxSlideOff: the language list's sideways offset as it comes in.
    fn slide_off(&self) -> f32 {
        let mut t = (self.progress() - 0.9) / (1.0 - 0.9);
        if t < 0.0 {
            t = 0.0;
        } else if t > 1.0 {
            t = 1.0;
        }
        (t as f64 * (-271.0 + (271.0 / 296.0) * self.slide_pos() as f64)) as f32
    }

    /// FX_PLACE_LINE: move a line by its title offset as the screen comes
    /// in and scale it about its centre.
    fn place_line(&mut self, g: &Glyphs, entry: usize, l: usize, o: Option<[f32; 2]>) {
        let n = g.count(entry) as u32;
        let mut c = centre(&self.cube, g.list(entry).map(|c| c as usize), n);
        if let Some([x, y]) = o {
            c[0] = x;
            c[1] = y;
        }
        let (t, p) = (&P.title[l], self.progress());
        for i in g.list(entry) {
            if let Some(q) = self.cube.get_mut(i as usize) {
                q.pos[0] += t.x * p;
                q.pos[1] += t.y * p;
                scale_about(c, t.scale, q);
            }
        }
    }

    /// FX_GROW_LINE / FX_GROW_ITEM: grow the lit cells of a line.
    /// With `slide`, the cells also take the language list's sideways offset.
    fn grow_line(&mut self, g: &Glyphs, entry: usize, spacing: f32, slide: bool) {
        let o = centre(&self.cube, g.list(entry).map(|c| c as usize), g.count(entry) as u32);
        for i in g.list(entry) {
            let s = 1.0 + self.lit(i as usize) * spacing;
            let t = if slide { Some(self.slide_off()) } else { None };
            if let Some(q) = self.cube.get_mut(i as usize) {
                scale_about(o, s, q);
                if let Some(t) = t {
                    q.pos[0] += t;
                }
            }
        }
    }

    /// fn_8132D234: lay out the three Options lines.
    fn option_lines(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        self.place_line(g, e::SOUND_ROW, 0, None);
        self.place_line(g, e::OFFSET_ROW, 1, None);
        // The language list scales about a fixed point.
        self.place_line(g, e::LANG_LIST, 2, Some([5.0, -135.0]));
        self.grow_line(g, e::SOUND_ROW, P.title[0].spacing, false);
        self.grow_line(g, e::OFFSET_ROW, P.title[1].spacing, false);
        self.grow_line(g, e::LANG_LIST, P.title[2].spacing, true);
    }

    /// FxOption_ChangeLang (0x8132DD84): slide the language list to the row
    /// of the new language.
    fn change_lang(&mut self, from: i32, to: i32) {
        let ofs = P.line_ofs.get(to as usize).copied().unwrap_or(0);
        let row = self.env.lang_y.get(self.env.opt_lang).copied().unwrap_or(0);
        self.slide.from = self.slide_pos();
        self.slide.to = (ofs + row as i32) as f32;
        self.slide.t = 0;
        self.cursor_start(from, to);
    }

    /// fn_8132DE98: the language list's position.
    fn slide_pos(&self) -> f32 {
        hermite(self.slide.t as f32, 0.0, self.slide.from, 0.0, 10.0, self.slide.to, 0.0)
    }

    /// FX_PLACE_ITEM
    fn place_item(&mut self, g: &Glyphs, entry: usize, l: usize) {
        let o = centre(&self.cube, g.list(entry).map(|c| c as usize), g.count(entry) as u32);
        let t = &P.item[l];
        for i in g.list(entry) {
            if let Some(q) = self.cube.get_mut(i as usize) {
                q.pos[0] += t.x;
                q.pos[1] += t.y;
                scale_about(o, t.scale, q);
            }
        }
    }

    /// fn_8132DEFC: lay out the Calendar date and time lines.
    fn clock_lines(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        self.place_item(g, e::DATE_ROW, 0);
        self.place_item(g, e::TIME_ROW, 1);
        let grow = match (self.state.v[6], self.state.v[8], self.state.v[9]) {
            (1, 0, _) => Some((e::YEAR, 0)),
            (1, 1, _) => Some((e::MONTH, 0)),
            (1, 2, _) => Some((e::DAY, 0)),
            (2, _, 0) => Some((e::HOUR, 1)),
            (2, _, 1) => Some((e::MINUTE, 1)),
            (2, _, 2) => Some((e::SECOND, 1)),
            _ => None,
        };
        if let Some((entry, l)) = grow {
            self.grow_line(g, entry, P.item[l].spacing, false);
        }
    }

    /// fn_8132EC54: Game Play's per-language offset and scale.
    fn game_cursor(&mut self) {
        let y = P.cursor_y;
        let (x, s) = match self.env.opt_lang {
            1 => (-36.0, 0.65),
            2 => (-36.0, 0.6),
            _ => (P.cursor_x, P.cursor_scale),
        };
        let n = (self.count as usize).min(FX_MAX);
        let o = centre(&self.cube, 0..n, self.count);
        for c in &mut self.cube[..n] {
            c.pos[0] += x;
            c.pos[1] += y;
            c.pos[0] = o[0] + s * (c.pos[0] - o[0]);
            c.pos[1] = o[1] + s * (c.pos[1] - o[1]);
            c.pos[2] = o[2] + s * (c.pos[2] - o[2]);
            c.scale = c.scale.map(|v| v * s);
        }
    }

    /// fn_8132EEB0: start fading an arrow bar in (on) or out.
    fn bar(&mut self, g: &Glyphs, k: usize, on: bool) {
        for c in g.list(e::BARS[k]) {
            let c = c as usize;
            self.set(5, c, if on { P.fade_in } else { 0 });
            self.set(3, c, if on { P.fade_in } else { 0 });
            self.set(7, c, P.fade_in);
            self.set(1, c, P.fade_out);
        }
    }

    /// FX_SEL_MOVE + FX_SEL_GROW for one bar: move it with its line, grow
    /// it, swing it by the blink and move it to its end of the line.
    fn place_bar(&mut self, g: &Glyphs, k: usize, o: Vec3, l: usize, sign: f32, end: [f32; 2]) {
        let t = &P.title[l];
        let swing = P.f0 * hermite(self.blink_wave(), 0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
        for i in g.list(e::BARS[k]) {
            if let Some(q) = self.cube.get_mut(i as usize) {
                q.pos[0] += t.x;
                q.pos[1] += t.y;
                scale_about(o, t.scale, q);
            }
        }
        for i in g.list(e::BARS[k]) {
            if let Some(q) = self.cube.get_mut(i as usize) {
                scale_about(o, 1.0 + t.spacing, q);
                q.pos[0] += sign * swing;
                q.pos[0] += end[0];
                q.pos[1] += end[1];
            }
        }
    }

    /// fn_8132F010: lay out the arrow bars of the three lines.
    fn option_bars(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        let mut o = [[0.0f32; 3]; 3];
        for (k, entry) in [e::SOUND_ROW, e::OFFSET_ROW, e::LANG_LIST].into_iter().enumerate() {
            o[k] = centre(&self.cube, g.list(entry).map(|c| c as usize), g.count(entry) as u32);
            o[k][0] += P.title[k].x;
            o[k][1] += P.title[k].y;
        }
        let (s, s2) = (&P.sel, &P.sel2);
        let right0 = if self.env.opt_lang == 4 { -40.0 } else { s[2] };
        self.place_bar(g, 0, o[0], 0, -1.0, [s[0], s[1]]);
        self.place_bar(g, 1, o[0], 0, 1.0, [right0, s[3]]);
        self.place_bar(g, 2, o[1], 1, -1.0, [s[4], s[5]]);
        self.place_bar(g, 3, o[1], 1, 1.0, [s[6], s[7]]);
        self.place_bar(g, 4, o[2], 2, -1.0, [s2[0], s2[1]]);
        self.place_bar(g, 5, o[2], 2, 1.0, [s2[2], s2[3]]);
    }

    /// fn_8132FF6C: blink triangle wave, 0..1..0 over fxBlinkPeriod frames.
    fn blink_wave(&self) -> f32 {
        let t = self.blink as f32 / P.blink_period as f32;
        if (t as f64) < 0.5 {
            2.0 * t
        } else {
            2.0 * (1.0 - t)
        }
    }

    /// fn_8132FFF0: give every cell the base colours and stop its fades.
    fn reset_colors(&mut self) {
        self.light.fill(self.lights[0]);
        self.dark.fill(self.darks[0]);
        self.pool.v[7].fill(0);
        self.pool.v[5].fill(0);
    }

    /// fn_81330128: fly the cubes away, staggered by `sFxFlyDelay`.
    fn fly(&mut self, t: f32) {
        let mut a = to_s16(255.0 * (1.0 - 1.4 * t));
        if a < 0 {
            a = 0;
        }
        self.lights[0][3] = a as u8;
        for i in 0..(self.count as usize).min(FX_MAX) {
            let mut k = t - 0.4 * (1.0 - FLY_DELAY.get(i).copied().unwrap_or(0) as f32 / 255.0);
            if k < 0.0 {
                k = 0.0;
            }
            let c = &mut self.cube[i];
            c.pos[0] += -585.0 * k;
            c.pos[1] += 200.0 * k;
            c.pos[2] -= 500.0 * k;
            let r = to_s16(65535.0 * k);
            let d = rot_about([r, 0, r], c, [0.0; 3]);
            c.pos[0] += d.pos[0];
            c.pos[1] += d.pos[1];
            c.pos[2] += d.pos[2];
            c.rot[0] = c.rot[0].wrapping_add(d.rot[0]);
            c.rot[2] = c.rot[2].wrapping_add(d.rot[2]);
        }
    }

    /// fn_81330374: the fade-out: a short squash, then the fly-away.
    fn fly_out(&mut self, d: &FxData, t: f32) {
        self.count = shape::eval(&d.shapes, 1, &mut self.cube, 1.0);
        if t < 0.1 {
            let n = (self.count as usize).min(FX_MAX);
            let o = centre(&self.cube, 0..n, self.count);
            let s = squash(10.0 * t);
            for c in &mut self.cube[..n] {
                scale_about(o, s, c);
                c.scale = c.scale.map(|v| v * s);
            }
        } else {
            self.fly(t - 0.1);
        }
    }

    /// fn_813306C8: park the cursor on a language.
    fn cursor_park(&mut self, lang: i32) {
        self.cursor.v = [lang as i16, lang as i16, 10];
        self.cursor.f = [0.0; 3];
    }

    /// fn_813306FC: start the language word cross-fade, with random delays.
    fn cursor_start(&mut self, from: i32, to: i32) {
        self.cursor.f[0] = if from < to { -1.0 } else { 1.0 };
        self.cursor.v[2] = 0;
        self.cursor.v[0] = from as i16;
        self.cursor.v[1] = to as i16;
        for d in self.cursor.delay.iter_mut() {
            *d = (rand() % 60 + 20) as u8;
        }
    }

    /// FxCursorT
    fn cursor_t(&self) -> f32 {
        self.cursor.v[2] as f32 / 10.0
    }

    /// One cursor copy: cube `src` moved by `k` delays and faded to `a`.
    fn cursor_copy(&mut self, at: usize, src: usize, delay: usize, k: f32, a: f32, light: Rgba, dark: Rgba) {
        if at >= FX_MAX {
            return;
        }
        let mut c = self.cube.get(src).copied().unwrap_or_default();
        let dl = self.cursor.delay.get(delay).copied().unwrap_or(0) as f32;
        c.pos[0] = (c.pos[0] as f64 - 0.2 * (self.cursor.f[0] * (dl * k)) as f64) as f32;
        self.cube[at] = c;
        self.alpha[at] = (255.0 * a) as u8;
        self.light[at] = light;
        self.dark[at] = dark;
    }

    /// fn_813307B4: cross-fade the sound word between the old and the new
    /// language's spelling.
    fn cursor_fade(&mut self, d: &FxData) {
        let entry = match self.state.v[5] {
            0 => Some(e::MONO),
            1 => Some(e::STEREO),
            _ => None,
        };
        let from = d.glyphs(self.cursor.v[0] as usize);
        let to = d.glyphs(self.cursor.v[1] as usize);
        let (nf, nt) = entry.map(|x| (from.count(x) as usize, to.count(x) as usize)).unwrap_or((0, 0));
        let g = d.glyphs(self.env.lang);
        for c in g.list(e::SOUND_ROW) {
            if let Some(a) = self.alpha.get_mut(c as usize) {
                *a = 0;
            }
        }
        let t = self.cursor_t();
        let (l2, d2) = (self.lights[2], self.darks[2]);
        let base = self.count as usize;
        for i in 0..nf {
            let src = from.at(entry.unwrap_or(0), i) as usize;
            self.cursor_copy(base + i, src, i, t, 1.0 - t, l2, d2);
        }
        self.count += nf as u32;
        let base = self.count as usize;
        for i in 0..nt {
            let src = to.at(entry.unwrap_or(0), i) as usize;
            // The new spelling comes in from the other side.
            self.cursor_copy(base + i, src, nf + i, -(1.0 - t), t, l2, d2);
        }
        self.count += nt as u32;
        self.count = self.count.min(FX_MAX as u32);
        self.cursor_line(d);
    }

    /// fn_81330B7C: fade the offset line across to a moving copy.
    fn cursor_line(&mut self, d: &FxData) {
        let g = d.glyphs(self.env.lang);
        let n = g.count(e::OFFSET_ROW) as usize;
        let t = self.cursor_t();
        let base = self.count as usize;
        for i in 0..n {
            let src = g.at(e::OFFSET_ROW, i) as usize;
            let at = base + i;
            if at >= FX_MAX {
                break;
            }
            let (a, l, k) = (
                self.alpha.get(src).copied().unwrap_or(0),
                self.light.get(src).copied().unwrap_or_default(),
                self.dark.get(src).copied().unwrap_or_default(),
            );
            self.cursor_copy(at, src, i, t, 0.0, l, k);
            self.alpha[at] = (a as f32 * (1.0 - t)) as u8;
        }
        self.count = (self.count + n as u32).min(FX_MAX as u32);
        for i in 0..n {
            let src = g.at(e::OFFSET_ROW, i) as usize;
            let dl = self.cursor.delay.get(i).copied().unwrap_or(0) as f32;
            let f0 = self.cursor.f[0];
            if let Some(c) = self.cube.get_mut(src) {
                c.pos[0] = (c.pos[0] as f64 + 0.2 * (f0 * (dl * (1.0 - t))) as f64) as f32;
            }
            if let Some(a) = self.alpha.get_mut(src) {
                *a = (*a as f32 * t) as u8;
            }
        }
    }
}
