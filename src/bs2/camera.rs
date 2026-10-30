//! BS2 camera, light and 2D view setup (`camera.c`, 0x81302DF0..0x81303204).

use super::bs2main::DispSettings;
use super::mtx::{self, Srt};
use crate::sdk::gx::{Gx, LightObj};
use crate::sdk::mtx::{self as psmtx, Mtx, Vec3};

/// Eye distance at which 448 units fill the screen height: 224 / tan(fovy / 2).
pub fn eye_dist(disp: &DispSettings) -> f32 {
    224.0 / libm::tanf(3.141_592 * disp.fovy / 360.0)
}

/// The scene states `fn_81302DF0` looks at.
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum CamState {
    /// Every state but the two below: the intro height.
    Intro,
    /// State 4, the spin-out: rises from `cam_y_far` to `cam_y_near` over
    /// `fade` frames.
    SpinOut { frame: i32, fade: i32 },
    /// State 5, the menu.
    Menu,
}

/// The camera height offset `fn_81302DF0` picks for the scene state.
pub fn cam_y(disp: &DispSettings, state: CamState) -> f32 {
    match state {
        CamState::SpinOut { frame, fade } if frame <= fade => {
            disp.cam_y_far - frame as f32 * (disp.cam_y_far - disp.cam_y_near) / fade as f32
        }
        CamState::SpinOut { .. } | CamState::Menu => disp.cam_y_near,
        CamState::Intro => disp.cam_y_far,
    }
}

/// `fn_81302DF0`: the camera view (`Camera_GetView`): on +Z at
/// [`eye_dist`], looking at the origin, both raised by `y`. With `y` 0 this
/// is the view without the height offset (`fn_81302F94`).
pub fn view(disp: &DispSettings, y: f32) -> Mtx {
    psmtx::look_at([0.0, y, eye_dist(disp)], [0.0, 1.0, 0.0], [0.0, y, 0.0])
}

/// The view-space position of GX_LIGHT0, at (400, 500, 200) in world
/// space.
pub fn light_pos(view: &Mtx) -> Vec3 {
    psmtx::mult_vec(view, [400.0, 500.0, 200.0])
}

/// `fn_81302FA0`: load GX_LIGHT0, white, at [`light_pos`], its direction
/// the view's origin (unused: BS2's channels have no spot attenuation).
pub fn load_light(gx: &mut Gx, view: &Mtx) {
    let mut light = LightObj::default();
    let pos = light_pos(view);
    let dir = psmtx::mult_vec(view, [0.0; 3]);
    light.init_pos(pos[0], pos[1], pos[2]);
    light.init_dir(dir[0], dir[1], dir[2]);
    light.init_color([255; 4]);
    gx.load_light_obj_imm(&light, 1);
}

/// `GX_Setup2DView` (0x813030B0): the 2D model-view, 584 x 448 layout units
/// with y down at z 0, in front of the un-raised camera.
pub fn setup_2d_view(disp: &DispSettings) -> Mtx {
    let srt = Srt { s: [1.0, -1.0, 1.0], r: [0; 3], t: [-292.0, 224.0, 0.0] };
    psmtx::concat(&view(disp, 0.0), &mtx::from_srt(&srt))
}
