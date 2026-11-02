//! The camera and projection of a frame ([`View`]) and IPL models with
//! their textures decoded once ([`Prepared`]). The models are drawn by
//! [`super::gmr`] through `sdk::gx`.

use crate::sdk::mtx::{self as psmtx, Mtx, Mtx44};
use super::bs2main::{self, Projection};
use super::camera;
use super::shader::FastTex;
use crate::ipl::j3d::Model;
use alloc::vec::Vec;

pub const EFB_W: usize = 640;
pub const EFB_H: usize = 448;

/// Camera and projection for one frame.
pub struct View {
    pub view: Mtx,
    pub ortho: Option<[f32; 4]>, // top, bottom, left, right
    pub fovy: f32,
    pub aspect: f32,
    pub near: f32,
    pub far: f32,
    /// Viewport size in pixels (the GX EFB is 640x448).
    pub vp: [f32; 2],
}

impl View {
    /// The BS2 camera raised by `y` (`fn_81302DF0`, see [`camera::cam_y`])
    /// and the scene projection (`fn_81301100`), orthographic for the
    /// intro.
    pub fn intro(y: f32, ortho: bool) -> View {
        let disp = bs2main::disp_settings();
        let view = camera::view(disp, y);
        let (ortho, fovy, aspect) = match bs2main::projection(disp, ortho) {
            Projection::Ortho { t, b, l, r } => (Some([t, b, l, r]), disp.fovy, bs2main::ASPECT),
            Projection::Perspective { fovy, aspect } => (None, fovy, aspect),
        };
        View {
            view,
            ortho,
            fovy,
            aspect,
            near: disp.near,
            far: disp.far,
            vp: [EFB_W as f32, EFB_H as f32],
        }
    }

    /// The projection as `GXSetProjection` takes it.
    pub fn projection(&self) -> Mtx44 {
        match self.ortho {
            Some([t, b, l, r]) => psmtx::ortho(t, b, l, r, self.near, self.far),
            None => psmtx::perspective(self.fovy, self.aspect, self.near, self.far),
        }
    }
}

/// A model with its textures converted once.
pub struct Prepared {
    pub model: Model,
    texs: Vec<FastTex>,
}

impl Prepared {
    /// Texture `i` of the model, decoded.
    pub fn tex(&self, i: u16) -> Option<&FastTex> {
        self.texs.get(i as usize)
    }

    pub fn new(model: Model) -> Prepared {
        let texs = model.textures.iter().map(FastTex::new).collect();
        Prepared { model, texs }
    }
}
