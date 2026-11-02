//! `backdrop.c` (0x8130AB14..0x8130B2F4): the submenus' background, a
//! grid of tiles (res 89) whose alpha is lit by GX spot lights, so it
//! only shows where the lights reach.
//!
//! `fn_8130AB14` (GX state for vertex-coloured 2D quads) has no caller
//! in the app yet and is not ported.

use super::menu::Work;
use super::mtx;
use super::shader::FastTex;
use crate::sdk::gx::*;
use crate::sdk::mtx::{self as psmtx, Mtx};

/// `Backdrop_SetupGX` (0x8130AD0C): GX state for the lit, textured
/// backdrop; `lights` light its alpha.
pub fn setup_gx(gx: &mut Gx, lights: u8) {
    gx.clear_vtx_desc();
    gx.set_vtx_attr_fmt(1, GX_VA_POS, 1, GX_S16, 4);
    gx.set_vtx_attr_fmt(1, GX_VA_NRM, 0, GX_S16, 4);
    gx.set_vtx_attr_fmt(1, GX_VA_TEX0, 1, GX_S16, 4);
    gx.set_num_chans(1);
    gx.set_chan_ctrl(GX_COLOR0, false, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    gx.set_chan_ctrl(GX_ALPHA0, true, GX_SRC_REG, GX_SRC_REG, lights, GX_DF_CLAMP, GX_AF_SPOT);
    gx.set_num_tex_gens(1);
    gx.set_tex_coord_gen(0, GX_TG_MTX3X4, GX_TG_TEX0, GX_IDENTITY);
    gx.set_num_tev_stages(1);
    gx.set_tev_order(0, 0, 0, GX_COLOR0A0);
    for s in 1..8 {
        gx.set_tev_order(s, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    }
    gx.set_tev_color_in(0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
    gx.set_tev_color_op(0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
    gx.set_tev_alpha_in(0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    gx.set_tev_alpha_op(0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
    gx.set_color_update(true);
    gx.set_alpha_update(true);
    gx.set_blend_mode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA);
    gx.set_z_mode(false, GX_LEQUAL, false);
}

/// `Backdrop_LoadSpot`: one spot light pointing down -z from `pos`, both
/// moved to view space by `mv`.
#[allow(clippy::too_many_arguments)]
fn load_spot(gx: &mut Gx, w: &Work, mv: &Mtx, pos: [f32; 3], bright: f32, cutoff: f32, alpha: u8, id: u8) {
    let mut light = LightObj::default();
    let pos = psmtx::mult_vec(mv, pos);
    let nrm = mtx::inverse33(mv).unwrap_or(psmtx::IDENTITY);
    let dir = psmtx::mult_vec(&nrm, [0.0, 0.0, -1.0]);
    light.init_pos(pos[0], pos[1], pos[2]);
    light.init_dir(dir[0], dir[1], dir[2]);
    light.init_spot(cutoff, SpotFn::Cos2);
    light.init_dist_attn(w.light_attn, bright, DistAttnFn::Gentle);
    light.init_color([255, 255, 255, alpha]);
    gx.load_light_obj_imm(&light, id);
}

/// `Backdrop_DrawLit` (0x8130AF54): the 584 x 448 plane in 32 x 32 tiles,
/// lit by GX_LIGHT3 at `lightPos` (light alpha `spot_alpha`) plus
/// `lights`, times `alpha`.
pub fn draw_lit<'a>(gx: &mut Gx<'a>, w: &Work, tex: &'a FastTex, mv: &Mtx, alpha: u8, lights: u8, spot_alpha: u8) {
    load_spot(gx, w, mv, w.light_pos, w.light_attn_k, w.light_cutoff, spot_alpha, GX_LIGHT3);
    gx.load_tex_obj(tex, 0);
    setup_gx(gx, lights | GX_LIGHT3);
    let mat = [255, 255, 255, alpha];
    gx.set_chan_mat_color(GX_COLOR0A0, mat);
    gx.set_chan_amb_color(GX_COLOR0A0, [0; 4]);
    for x in (0..584i16).step_by(32) {
        for y in (0..448i16).step_by(32) {
            gx.begin(GX_QUADS, 1, 4);
            gx.position3s16(x * 16, y * 16, 0);
            gx.normal3s16(0, 0, 16);
            gx.tex_coord2s16(0, 0);
            gx.position3s16(x * 16 + 512, y * 16, 0);
            gx.normal3s16(0, 0, 16);
            gx.tex_coord2s16(32, 0);
            gx.position3s16(x * 16 + 512, y * 16 + 512, 0);
            gx.normal3s16(0, 0, 16);
            gx.tex_coord2s16(32, 32);
            gx.position3s16(x * 16, y * 16 + 512, 0);
            gx.normal3s16(0, 0, 16);
            gx.tex_coord2s16(0, 32);
            gx.end();
        }
    }
    gx.set_chan_amb_color(GX_COLOR0A0, mat);
    gx.set_chan_ctrl(GX_COLOR0A0, true, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_SPOT);
}

/// `Backdrop_Draw` (0x8130B244).
pub fn draw<'a>(gx: &mut Gx<'a>, w: &Work, tex: &'a FastTex, mv: &Mtx, alpha: u8) {
    draw_lit(gx, w, tex, mv, alpha, GX_LIGHT3, 255);
}

/// `Backdrop_DrawSpots` (0x8130B26C): the backdrop with up to two extra
/// spot lights (GX_LIGHT4/5), the Memory Card screen's slot cursors.
#[allow(clippy::too_many_arguments)]
pub fn draw_spots<'a>(gx: &mut Gx<'a>, w: &Work, tex: &'a FastTex, mv: &Mtx, alpha: u8, pos1: [f32; 3], pos2: [f32; 3], alpha1: u8, alpha2: u8, spot_alpha: u8) {
    let mut lights = 0;
    if alpha1 != 0 {
        load_spot(gx, w, mv, pos1, 0.525, 28.0, alpha1, GX_LIGHT4);
        lights |= GX_LIGHT4;
    }
    if alpha2 != 0 {
        load_spot(gx, w, mv, pos2, 0.525, 28.0, alpha2, GX_LIGHT5);
        lights |= GX_LIGHT5;
    }
    draw_lit(gx, w, tex, mv, alpha, lights, spot_alpha);
}
