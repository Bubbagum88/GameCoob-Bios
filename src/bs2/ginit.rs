//! BS2 graphics init and per-frame GX helpers (`gInit.c`,
//! 0x813012D0..0x81302660). The GX setup helpers and the disc state the
//! scene reads ([`Disc`], with no drive behind it yet) are ported; DEMOInit
//! and the frame begin/end still live in `boot_intro.rs`.

use crate::sdk::gx::*;

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
}

impl Disc {
    pub fn new() -> Disc {
        Disc { state: DiscState::None, fade: 0, reset_fade: false }
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
}
