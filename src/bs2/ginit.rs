//! BS2 graphics init and per-frame GX helpers (`gInit.c`,
//! 0x813012D0..0x81302660): the GX setup helpers, the screen dim, the disc
//! state the scene reads ([`Disc`], with no drive behind it yet) and the
//! reset switch. DEMOInit and the frame begin/end are the UEFI glue's
//! (`boot_intro.rs`): the app has no VI or EFB copy to set up.

use crate::sdk::gx::*;
use crate::sdk::mtx as psmtx;

/// `GX_Setup2D` (0x8130180C): flat 2D drawing. Positions are s16 with 4
/// fraction bits; `color` adds a per-vertex color (else the channel's
/// material color), `tex` a texture in TEXMAP0 with s16 coordinates of 14
/// fraction bits, either multiplied by the color (`modulate`) or only
/// supplying alpha. Alpha blended, no z, back faces culled.
pub fn setup_2d(gx: &mut Gx, tex: bool, color: bool, modulate: bool) {
    if !tex && !color {
        return;
    }
    let mut src = GX_SRC_REG;
    gx.clear_vtx_desc();
    gx.set_vtx_attr_fmt(1, GX_VA_POS, 1, GX_S16, 4);
    if color {
        src = GX_SRC_VTX;
    }
    if tex {
        gx.set_vtx_attr_fmt(1, GX_VA_TEX0, 1, GX_S16, 14);
    }
    gx.set_num_chans(1);
    gx.set_chan_ctrl(GX_COLOR0A0, false, src, src, 0, GX_DF_CLAMP, GX_AF_NONE);

    if tex {
        gx.set_num_tex_gens(1);
        // The C passes 1 here, which is GX_TG_MTX2x4.
        gx.set_tex_coord_gen(0, GX_TG_MTX2X4, GX_TG_TEX0, GX_IDENTITY);
        gx.set_num_tev_stages(1);
        gx.set_tev_order(0, 0, 0, GX_COLOR0A0);
        for s in 1..8 {
            gx.set_tev_order(s, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        }
        if modulate {
            gx.set_tev_color_in(0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
        } else {
            gx.set_tev_color_in(0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
        }
        gx.set_tev_color_op(0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
        gx.set_tev_alpha_in(0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
        gx.set_tev_alpha_op(0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
    } else {
        gx.set_num_tex_gens(0);
        gx.set_num_tev_stages(1);
        gx.set_tev_order(0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        for s in 1..8 {
            gx.set_tev_order(s, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        }
        gx.set_tev_color_in(0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
        gx.set_tev_color_op(0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
        gx.set_tev_alpha_in(0, GX_CA_ZERO, GX_CA_KONST, GX_CA_RASA, GX_CA_ZERO);
        gx.set_tev_alpha_op(0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
    }
    gx.set_color_update(true);
    gx.set_alpha_update(true);
    gx.set_blend_mode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA);
    gx.set_z_mode(false, GX_LEQUAL, false);
    gx.set_cull_mode(GX_CULL_BACK);
}

/// `Screen_Dim` (0x81301BEC): a black quad over the whole screen at
/// `alpha`, seen from z 1700 under the frame's projection.
pub fn screen_dim(gx: &mut Gx, alpha: u8) {
    let m = psmtx::look_at([0.0, 0.0, 1700.0], [0.0, 1.0, 0.0], [0.0, 0.0, 0.0]);
    gx.load_pos_mtx_imm(&m, GX_PNMTX0);
    gx.set_current_mtx(GX_PNMTX0);
    gx.clear_vtx_desc();
    gx.set_vtx_attr_fmt(1, GX_VA_POS, 1, GX_S16, 0);
    gx.set_num_chans(1);
    gx.set_chan_ctrl(GX_COLOR0A0, false, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    gx.set_num_tex_gens(0);
    gx.set_num_tev_stages(1);
    gx.set_tev_order(0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    for s in 1..8 {
        gx.set_tev_order(s, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    }
    gx.set_tev_op(0, TevMode::PassClr);
    gx.set_alpha_update(true);
    gx.set_color_update(true);
    gx.set_blend_mode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA);
    gx.set_z_mode(false, GX_ALWAYS, false);
    gx.set_cull_mode(GX_CULL_NONE);

    let color = alpha as u32;
    gx.begin(GX_QUADS, 1, 4);
    for (x, y) in [(-400, 400), (400, 400), (400, -400), (-400, -400)] {
        gx.position3s16(x, y, 10);
        gx.color1u32(color);
    }
    gx.end();
}

/// The disc states `Disc_GetState` reports (`DVDStep`'s result).
#[allow(dead_code)] // the dummy drive only reports `None`
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum DiscState {
    /// "Press START to begin!"
    Ready = 0x10,
    /// "Reading disc..."
    Reading = 0x11,
    /// "Please insert a NINTENDO GAMECUBE DISC."
    None = 0x12,
    CoverOpen = 0x13,
    /// "The disc could not be read."
    Wrong = 0x16,
    /// "An error has occurred."
    Fatal = 0x17,
    /// "The disc could not be read."
    Retry = 0x18,
}

/// gInit.c's disc globals: the state (`lbl_814AD2EC`) and the screen fade
/// (`lbl_814AD2E8`, `lbl_814AD2E4`). There is no drive yet, so the state is
/// always [`DiscState::None`].
pub struct Disc {
    state: DiscState,
    fade: i16,
    reset_fade: bool,
    /// `lbl_814AD2FC`: the reset switch was down last frame.
    reset_held: bool,
}

impl Disc {
    pub fn new() -> Disc {
        Disc { state: DiscState::None, fade: 0, reset_fade: false, reset_held: false }
    }

    /// `InitDVDLoader` (0x813020C8) and `Disc_GetState` (0x813020EC).
    pub fn state(&self) -> DiscState {
        self.state
    }

    /// `Disc_FadeOutStart` (0x81302100).
    pub fn fade_out_start(&mut self) {
        if !self.reset_fade {
            self.fade -= 6;
            if self.fade < 0 {
                self.fade = 0;
            }
        }
    }

    /// `Disc_GetFadeAlpha` (0x81302130).
    pub fn fade_alpha(&self) -> u8 {
        self.fade as u8
    }

    /// `fn_813022C4` (0x813022C4): dim the frame by the fade alpha.
    pub fn draw_dim(&self, gx: &mut Gx) {
        if self.fade > 0 {
            screen_dim(gx, self.fade as u8);
        }
    }

    /// `fn_81302368` (0x81302368): the reset switch, `pressed` this frame.
    /// Letting go of it (unless the disc failed for good, `fatal`) fades
    /// the screen to black, 8 a frame; returns true once it is black and
    /// the scene should restart (`Scene_Init(FALSE)`).
    ///
    /// GameCoob: the stand-in memory cards need no unmounting and the
    /// stand-in drive no re-reading, so the restart is only the scene's.
    pub fn reset_switch(&mut self, pressed: bool, fatal: bool) -> bool {
        if self.reset_fade {
            self.fade += 8;
            if self.fade > 0xFF {
                self.reset_fade = false;
                self.fade = 0;
                self.reset_held = false;
                return true;
            }
        } else if self.reset_held && !pressed && !fatal {
            self.reset_fade = true;
        }
        self.reset_held = pressed;
        false
    }
}
