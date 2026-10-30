//! BS2 main loop (`bs2main.c`, 0x81300F50..0x81301190): the display
//! settings every camera and projection reads, and the projection setup.
//!
//! The loop itself (`BS2Main`) still lives in `boot_intro.rs`; it moves here
//! once gInit's frame begin/end are ported.

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
