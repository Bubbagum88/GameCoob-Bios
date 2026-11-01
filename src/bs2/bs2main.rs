//! BS2 main loop (`bs2main.c`, 0x81300F50..0x81301190): the display
//! settings every camera and projection reads, the projection setup, and
//! one pass of the `BS2Main` loop ([`Bs2Main`]).
//!
//! The UEFI glue (`boot_intro.rs`) and tools/menu-render own the things
//! the C gets from the SDK: the pad (keys), the 60 Hz pacing (VI retrace),
//! the rasterizer cores and the present (DEMODoneRender's copy out).

use super::menu::backend::{DiscDrive, MemCards};
use super::menu::{Action, Menu, MenuAssets, Pad, SysInfo};
use super::{raster, Assets, Input, Intro};
use alloc::boxed::Box;

/// The display settings record (`DispSettings`, 0x50 bytes) with the fields
/// the ported units read.
#[derive(Clone, Copy)]
pub struct DispSettings {
    /// 0x00, 0x04: clip planes.
    pub near: f32,
    pub far: f32,
    /// 0x08: vertical field of view in degrees.
    pub fovy: f32,
    /// 0x14: camera height offset of the intro (`camYFar`).
    pub cam_y_far: f32,
    /// 0x18: camera height offset of the menu (`camYNear`).
    pub cam_y_near: f32,
    /// 0x1E: orthographic projection (the intro) instead of perspective.
    #[allow(dead_code)] // the scene passes it to `projection` itself for now
    pub ortho: bool,
    /// 0x2B: frame padding around dialog text, pixels.
    pub text_pad: u8,
    /// 0x2C: how far a line-end SJIS comma or period hangs out.
    pub punct_hang: i16,
    /// 0x32, 0x34: text glow alpha range.
    pub glow_max: i16,
    pub glow_min: i16,
    /// 0x38: length of the glow pulse (`pulse` runs 0..this and back).
    pub pulse_len: i16,
}

/// `fn_81300F50`: the defaults BS2Main installs.
pub const DISP_DEFAULTS: DispSettings = DispSettings {
    near: 50.0,
    far: 10000.0,
    fovy: 20.0,
    cam_y_far: -55.0,
    cam_y_near: -10.0,
    ortho: true,
    text_pad: 8,
    punct_hang: 6,
    glow_max: 255,
    glow_min: 70,
    pulse_len: 35,
};

/// `Disp_GetSettings` (0x813010F8). The app never changes them yet.
pub fn disp_settings() -> &'static DispSettings {
    &DISP_DEFAULTS
}

/// What `fn_81301100` hands to `GXSetProjection`.
#[derive(Clone, Copy)]
pub enum Projection {
    /// `C_MTXPerspective(fovy, aspect, near, far)`
    Perspective { fovy: f32, aspect: f32 },
    /// `C_MTXOrtho(top, bottom, left, right, near, far)`
    Ortho { t: f32, b: f32, l: f32, r: f32 },
}

/// The 592-wide safe area over the 448-line EFB.
pub const ASPECT: f32 = 592.0 / 448.0;

/// `fn_81301100`: the scene projection.
pub fn projection(disp: &DispSettings, ortho: bool) -> Projection {
    if ortho {
        Projection::Ortho { t: 224.0, b: -224.0, l: -296.0, r: 292.0 }
    } else {
        Projection::Perspective { fovy: disp.fovy, aspect: ASPECT }
    }
}

/// What the pad gave this frame (DEMOPadRead's result).
#[derive(Clone, Copy, Default)]
pub struct FrameInput {
    /// The intro's buttons (A held, the boot easter egg).
    pub intro: Input,
    /// The menu's presses.
    pub pad: Pad,
    /// The reset switch is down (`OSGetResetSwitchState`).
    pub reset: bool,
}

/// The state the `BS2Main` loop runs: the scene (with the cube and the
/// menu work) and, once the intro has spun out, the menu.
///
/// GameCoob: the menu is made when the intro finishes rather than by
/// `Scene_Init`, because it reads this PC's boot entries ([`SysInfo`]).
pub struct Bs2Main {
    pub intro: Intro,
    pub menu: Option<Menu>,
    /// The disc drive and memory cards while no menu holds them (the C's
    /// DVD and card threads outlive a reset).
    backends: Option<(Box<dyn DiscDrive>, Box<dyn MemCards>)>,
}

impl Bs2Main {
    /// `BS2Main` up to the loop: the scene with `Scene_Init(TRUE)`.
    pub fn new(a: &Assets, drive: Box<dyn DiscDrive>, cards: Box<dyn MemCards>) -> Bs2Main {
        Bs2Main { intro: Intro::new(a), menu: None, backends: Some((drive, cards)) }
    }

    /// Run the intro to its end at once (a skip).
    pub fn skip_intro(&mut self) {
        while !self.intro.finished() {
            self.intro.update(Input::default());
        }
    }

    /// The logic of one loop pass: `fn_81302DF0` and `Scene_Update` (the
    /// intro, or Menu_Update with the guide and the mosaic once the menu
    /// is up), then the reset switch (`fn_81302368`). `info` gives the
    /// menu's view of the PC when it starts. Returns what the menu asks
    /// for; the caller carries it out.
    pub fn update(&mut self, a: &Assets, ma: &MenuAssets, input: &FrameInput, info: impl FnOnce() -> SysInfo) -> Option<Action> {
        let mut action = None;
        match &mut self.menu {
            Some(m) => {
                // Scene_Update in state 5 runs Menu_Update.
                self.intro.update(Input::default());
                action = m.update(ma, &self.intro.camera_view(), &input.pad);
                m.tick_guide();
                m.update_fx(ma);
            }
            None => {
                self.intro.update(input.intro);
                if self.intro.finished() {
                    if let Some((drive, cards)) = self.backends.take() {
                        self.menu = Some(Menu::new(info(), ma, drive, cards, self.intro.menu.clone()));
                    }
                }
            }
        }
        let fatal = self.intro.scene.fatal;
        if self.intro.disc.reset_switch(input.reset, fatal) {
            if let Some(m) = self.menu.take() {
                self.backends = Some((m.drive, m.cards));
            }
            self.intro.restart(a);
        }
        action
    }

    /// The draw of one loop pass: `fn_81301100`'s projection,
    /// `Scene_Draw2D` and `fn_813022C4`'s dim, rasterized into `t`.
    pub fn render(&self, t: &mut raster::Target, a: &Assets, ma: &MenuAssets, par: &dyn Fn(&raster::Job)) {
        self.intro.render_with_menu(t, a, ma, self.menu.as_ref(), par);
    }
}
