//! Rolling-cube intro drawing (`cube_draw.c`, 0x813108B8..0x81310F58): the
//! trail tiles the cube leaves on the three faces of the big cube.
//!
//! `Cube_DrawBig` also draws the small cube model, through [`super::gmr`];
//! the caller does that part.

use super::cube::{self, Cube, GRID};
use super::ginit;
use super::mtx;
use super::shader::FastTex;
use crate::sdk::gx::*;
use crate::sdk::mtx::{self as psmtx, Mtx};

/// The first half of `Cube_DrawBig` (0x81310E24): 2D state with a texture,
/// TEX0 in s16 with 13 fraction bits (so 0x4000 is 2.0), no culling, then
/// the tiles. `world` is `Scene_GetWorldMtx`, `view` `Camera_GetView`.
pub fn draw_big_tiles<'a>(gx: &mut Gx<'a>, c: &Cube, world: &Mtx, view: &Mtx, alpha: u8, tile_tex: &'a FastTex) {
    ginit::setup_2d(gx, true, false, false);
    gx.set_vtx_attr_fmt(1, GX_VA_TEX0, 1, GX_S16, 13);
    gx.set_cull_mode(GX_CULL_NONE);
    draw_tiles(gx, c, world, view, alpha, tile_tex);
}

/// `Cube_DrawTiles` (0x813108B8): every lit tile of the grid, fading in
/// (state 1) or out (state 2) over a side step.
fn draw_tiles<'a>(gx: &mut Gx<'a>, c: &Cube, world: &Mtx, view: &Mtx, alpha: u8, tile_tex: &'a FastTex) {
    let center = (c.size * 3 / 2) as f32;
    let mut m = mtx::rot_trans(0, 0, 0, [center; 3]);
    m = psmtx::concat(world, &m);
    m = psmtx::concat(view, &m);
    // The tiles are unlit, so the normal matrix never shows.
    let nrm = mtx::inverse33(&m).unwrap_or(psmtx::IDENTITY);
    gx.load_pos_mtx_imm(&m, GX_PNMTX0);
    gx.load_nrm_mtx_imm(&nrm, GX_PNMTX0);
    gx.set_current_mtx(GX_PNMTX0);
    for f in 0..cube::FACES {
        for x in 0..GRID {
            for y in 0..GRID {
                let tile = c.tile(f, x, y);
                if tile.group == 0 {
                    continue;
                }
                let mut a = 255;
                if tile.state == 1 {
                    a = c.step_frame * 255 / c.side_frames;
                }
                if tile.state == 2 {
                    a = (c.side_frames - c.step_frame) * 255 / c.side_frames;
                }
                let a = a.clamp(0, 255);
                draw_tile(gx, c, x as i32, y as i32, f, (a * alpha as i32 / 255) as u8, tile_tex);
            }
        }
    }
}

/// `Cube_TileVtx`: face 0 is the plane (a, 0, b), face 1 (b, a, 0), face 2
/// (0, b, a).
fn tile_vtx(gx: &mut Gx, face: usize, a: i32, b: i32, s: u16, t: u16) {
    let (a, b) = (a as u16, b as u16);
    match face {
        0 => gx.position3u16(a, 0, b),
        1 => gx.position3u16(b, a, 0),
        _ => gx.position3u16(0, b, a),
    }
    gx.tex_coord2u16(s, t);
}

/// `Cube_DrawTile` (0x81310AD0): one textured quad on face plane `face`, in
/// 1/16 units, colored by the material color.
fn draw_tile<'a>(gx: &mut Gx<'a>, c: &Cube, x: i32, y: i32, face: usize, alpha: u8, tile_tex: &'a FastTex) {
    let inset = c.tile_inset;
    let full = c.size * 16;
    let px = (x - 16) * c.size * 16;
    let py = (y - 16) * c.size * 16;
    let [r, g, b] = c.tile_rgb;
    gx.set_chan_mat_color(GX_COLOR0A0, [r, g, b, alpha]);
    gx.load_tex_obj(tile_tex, 0);
    gx.begin(GX_QUADS, 1, 4);
    tile_vtx(gx, face, -(px + inset), -(py + inset), 0, 0);
    tile_vtx(gx, face, -(px + full), -(py + inset), 0, 0x4000);
    tile_vtx(gx, face, -(px + full), -(py + full), 0x4000, 0x4000);
    tile_vtx(gx, face, -(px + inset), -(py + full), 0x4000, 0);
    gx.end();
}
