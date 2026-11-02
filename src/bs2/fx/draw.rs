//! Mosaic drawing (`menu_fx_draw.c`, PAL 1.0 BS2 0x81332FB8..0x81333344):
//! the cube model (res 104) drawn once per placed cube.
//!
//! BS2 draws half the cubes per anti-aliasing pass (`sFxPass`: those behind
//! the menu plane, then those in front) and Fx_Update computes every cube's
//! matrices up front; here each frame draws every cube and computes the
//! matrices of the ones it draws. The cubes do not move between the two.

use super::Fx;
use crate::bs2::gmr::{self, Instance};
use crate::bs2::mtx;
use crate::bs2::render::View;
use crate::sdk::gx::Gx;

impl Fx {
    /// Fx_Draw (0x81333030): every cube above alpha 10, with its light and
    /// dark colours in TEVREG0 and TEVREG1. `view` is the submenu camera,
    /// `alpha` the submenu's fx fader.
    pub fn draw<'a>(&self, gx: &mut Gx<'a>, data: &'a super::FxData, view: &View, alpha: u8) {
        let n = (self.count as usize).min(super::FX_MAX);
        let s10 = |c: super::Rgba| [c[0] as i16, c[1] as i16, c[2] as i16, 0xFF];
        for i in 0..n {
            let a = (self.light[i][3] as u32 * (alpha as u32 * self.alpha[i] as u32) / 65025) as u8;
            if a <= 10 {
                continue;
            }
            // Fx_Update (0x81333250)
            let c = &self.cube[i];
            let m = mtx::rot_trans(c.rot[0] as i32, c.rot[1] as i32, c.rot[2] as i32, c.pos);
            let mut inst = Instance::new(&data.model.model);
            gmr::set_model_mtx(&mut inst, &m, c.scale);
            gmr::set_view_mtx(&mut inst, &view.view);
            gmr::calc_joints(&data.model.model, &mut inst);
            inst.alpha = a;
            inst.tev_color = alloc::vec![(0, 0, s10(self.light[i])), (0, 1, s10(self.dark[i]))];
            gmr::draw_packet(gx, &data.model, &mut inst, 1);
        }
    }
}
