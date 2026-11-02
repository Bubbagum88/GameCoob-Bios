//! The intro scene state (`Scene`, scene.h) and the parts of `scene.c`,
//! `scene_intro.c` and `scene_spinout.c` that work on it alone: holding A
//! to spin, the idle wobble, the impact shake, the world matrix every
//! intro model hangs from, and the spin-out into the menu. The state
//! machine around them is [`super::Intro`].

use super::gbase;
use super::menu::Work;
use super::mtx::{self, cos_s, sin_s, to_s16};
use crate::sdk::mtx::{self as psmtx, Mtx};

/// `SceneState`.
#[allow(dead_code)] // the states that need a drive are not ported yet
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum SceneState {
    /// SRAM was lost: language select, then the logo.
    Mode0 = 0,
    /// Default boot: no disc inserted.
    NoDisc = 1,
    /// Disc present, boot after the logo.
    Disc = 2,
    /// Cover open or drive error.
    Mode3 = 3,
    /// The cube spun fast enough: transition to the menu.
    SpinOut = 4,
    Menu = 5,
}

/// `SCENE_SHAKE_MAX`: impulse (or wobble) at which the backgrounds are
/// fully visible.
pub const SHAKE_MAX: i32 = 3000;

/// `Scene` (0x814AF878, 0xD8 bytes). The comments give the defaults from
/// [`Scene::set_defaults`].
#[allow(dead_code)] // the whole struct, for the states still to port
#[derive(Clone, Default)]
pub struct Scene {
    pub unk0: i16,
    pub unk2: i16,
    pub model_scale: f32,
    /// Y rotation of the whole scene.
    pub spin: i16,
    pub spin_rest: i16,
    /// (5000) spin speed that starts the menu.
    pub spin_threshold: i32,
    pub spin_speed: f32,
    pub spin_accel: f32,
    pub spin_accel_step: f32,
    pub unk1c: f32,
    pub wobble_phase: i16,
    pub wobble_amp: i16,
    /// (0.7), but Scene_Reset clears it before the intro.
    pub spin_tilt: f32,
    /// (30) spin-out: frames the intro models fade.
    pub out_fade_frames: i16,
    /// (3) spin-out: extra full turns.
    pub out_turns: i16,
    /// (5000) spin-out: angle reached after the turns.
    pub out_settle: i16,
    /// Spin when the spin-out began.
    pub out_start: i16,
    pub out_coast_frames: i32,
    pub out_turn_frames: i32,
    pub out_settle_frames: i32,
    pub out_frame: i32,
    /// (100) the menu face fades in over the last N frames.
    pub out_face_frames: i32,
    pub out_speed: f32,
    pub squash_xz: f32,
    pub stretch_y: f32,
    pub unk50: f32,
    pub unk54: f32,
    pub unk58: u8,
    /// (255) inner background alpha at full shake.
    pub shake_alpha: u8,
    pub bg_alpha: u8,
    pub logo_alpha: u8,
    pub land_impulse: i16,
    pub drop_impulse: i16,
    pub land_impulse0: i16,
    pub drop_impulse0: i16,
    pub shake_phase: i16,
    pub m0_logo_fade: i16,
    pub m0_logo_frames: i16,
    pub m0_text_fade: i16,
    pub unk6c: i16,
    pub m0_text_frames: i16,
    pub unk70: i16,
    pub m0_state: i16,
    pub disc_fade: i16,
    pub disc_fade_frames: i16,
    pub disc_logo_dim: i16,
    pub ask_fade: i16,
    pub ask_fade_frames: i16,
    pub disc_state: i16,
    pub ask_choice: i16,
    pub ask_gap: i16,
    /// "Disc could not be read" fade.
    pub err_fade: i16,
    pub err_fade_frames: i16,
    /// "An error has occurred" fade.
    pub fatal_fade: i16,
    pub fatal_fade_frames: i16,
    pub land_decay: f32,
    pub drop_decay: f32,
    pub land_lift: f32,
    pub drop_lift: f32,
    pub shake_step: i16,
    pub iso: [i16; 3],
    pub ease_speed: f32,
    pub ease_frame: i16,
    pub last_spin: i16,
    pub ease_from: i16,
    /// Set until the spin-out has started.
    pub out_first: bool,
    /// The disc reported DISC_FATAL.
    pub fatal: bool,
    pub cover_opened: bool,
    pub disc_wait: i16,
    pub m3_wait: i16,
    pub whoosh_min: f32,
    pub out_whoosh_min: f32,
    pub unk_c0: [i16; 8],
    /// Mode 0: the language list is up.
    pub lang_select: i16,
    /// Language under the cursor.
    pub lang: i16,
    pub lang_fade: i16,
}

impl Scene {
    /// `Scene_Init(TRUE)`'s defaults and reset.
    pub fn new() -> Self {
        let mut s = Scene::default();
        s.set_defaults();
        s.reset();
        s
    }

    /// `Scene_SetDefaults` (0x8130BFF0).
    fn set_defaults(&mut self) {
        self.unk0 = 0;
        self.unk2 = 100;
        self.spin_threshold = 5000;
        self.spin_tilt = 0.7;
        self.out_fade_frames = 30;
        self.squash_xz = 0.5;
        self.stretch_y = 1.0;
        self.spin_accel_step = 1.5;
        self.unk1c = 100.0;
        self.shake_alpha = 255;
        self.unk58 = 255;
        self.land_decay = 0.93;
        self.land_lift = 0.015;
        self.land_impulse0 = 4000;
        self.drop_decay = 0.925;
        self.drop_lift = 0.005;
        self.drop_impulse0 = 12000;
        self.shake_step = 1000;
        self.m0_logo_frames = 30;
        self.m0_text_frames = 30;
        self.unk70 = 30;
        self.disc_fade_frames = 30;
        self.ask_fade_frames = 30;
        self.ask_gap = 8;
        self.disc_logo_dim = 150;
        self.err_fade_frames = 30;
        self.fatal_fade_frames = 30;
        self.out_coast_frames = 15;
        self.out_turn_frames = 80;
        self.out_settle_frames = 50;
        self.out_settle = 5000;
        self.out_turns = 3;
        self.out_speed = self.spin_threshold as f32;
        self.out_face_frames = 100;
        self.iso = [0x2000, -0x1914, -0x1555];
        self.fatal = false;
        self.whoosh_min = 0.4;
        self.out_whoosh_min = 0.5;
    }

    /// `Scene_Reset` (0x8130C130). `Lang_Get()` is English (0) for now.
    pub fn reset(&mut self) {
        self.cover_opened = false;
        self.model_scale = 1.0;
        self.spin = 0;
        self.spin_speed = 0.0;
        self.spin_accel = 0.0;
        self.ease_speed = 0.0;
        self.ease_from = 0;
        self.last_spin = 0;
        self.out_frame = 1;
        self.out_first = true;
        self.spin_rest = 0;
        self.ease_frame = 60;
        self.wobble_phase = 2000;
        self.wobble_amp = 0;
        self.spin_tilt = 0.0;
        self.unk50 = 0.1;
        self.unk54 = self.squash_xz;
        self.bg_alpha = 255;
        self.logo_alpha = 0;
        self.land_impulse = 0;
        self.drop_impulse = 0;
        self.shake_phase = 0;
        self.m0_logo_fade = self.m0_logo_frames;
        self.m0_text_fade = 0;
        self.unk6c = 0;
        self.m0_state = 0;
        self.disc_fade = 0;
        self.disc_state = 0;
        self.ask_fade = 0;
        self.ask_choice = 1;
        self.err_fade = 0;
        self.fatal_fade = 0;
        self.m3_wait = 120;
        self.unk_c0 = [0, 1, 0, 0, 0, 1, 0, 0];
        self.lang_select = 1;
        self.lang = 0;
        self.lang_fade = 0;
    }

    /// `Scene_UpdateSpin` (0x8130C250): hold A while the cube runs to spin
    /// the scene; released, the spin eases back to rest over 50 frames.
    /// Returns the level `Sound_UpdateSpin(0, level)` gets when the spin
    /// wraps past the front.
    pub fn update_spin(&mut self, a_held: bool, cube_running: bool, force: bool) -> Option<f32> {
        let mut ease_from = self.ease_from;
        let mut ease_speed = self.ease_speed;
        let prev = self.spin;
        if force {
            self.spin_accel += self.spin_accel_step;
            self.spin_speed += self.spin_accel;
            self.spin = to_s16(self.spin as f32 + self.spin_speed);
        } else if a_held && cube_running {
            self.spin_accel += self.spin_accel_step;
            self.spin_speed += self.spin_accel;
            self.spin = to_s16(self.spin as f32 + self.spin_speed);
            ease_from = self.spin;
            ease_speed = self.spin_speed;
            self.ease_frame = 0;
        } else {
            self.spin_accel *= 0.76;
            self.spin_speed *= 0.95;
            if (self.spin_accel as f64) < 0.1 {
                self.spin_accel = 0.0;
            }
            if (self.spin_speed as f64) < 0.01 {
                self.spin_speed = 0.0;
            }
            self.spin = to_s16(gbase::hermite(self.ease_frame as f32, 0.0, ease_from as f32, ease_speed, 50.0, 0.0, 0.0));
            self.ease_frame += 1;
            if self.ease_frame > 50 {
                self.ease_frame = 50;
            }
        }

        let mut level = self.spin_speed / 5000.0;
        if level > 1.0 {
            level = 1.0;
        }
        if level < 0.01 {
            level = 0.0;
        }
        let mut whoosh = None;
        if cube_running || force {
            let wrapped = (prev < 0 && self.spin >= 0) || (prev > 0 && self.spin < 0);
            if wrapped && level > self.whoosh_min {
                whoosh = Some(level);
            }
        }
        self.last_spin = prev;
        self.ease_from = ease_from;
        self.ease_speed = ease_speed;
        whoosh
    }

    /// `Scene_DecayShake` (0x8130C518).
    fn decay_shake(&mut self) {
        let mut phase = self.shake_phase;
        let mut land = self.land_impulse as f32;
        let mut drop = self.drop_impulse as f32;
        land *= self.land_decay;
        drop *= self.drop_decay;
        if land < 10.0 {
            land = 0.0;
        }
        if drop < 80.0 {
            drop = 0.0;
        }
        if land > 0.0 || drop > 0.0 {
            phase = phase.wrapping_add(self.shake_step);
        }
        self.land_impulse = to_s16(land);
        self.drop_impulse = to_s16(drop);
        self.shake_phase = phase;
    }

    /// `Scene_UpdateWobble` (0x8130C5E4).
    pub fn update_wobble(&mut self) {
        let mut phase = self.wobble_phase;
        let mut amp = self.wobble_amp as f32;
        if self.spin_accel > 0.0 {
            amp = 36.0 + amp;
        } else {
            amp *= 0.945;
        }
        if amp < 1.0 {
            amp = 0.0;
        }
        if amp > 0.0 {
            phase = phase.wrapping_add(7);
        }
        if amp > 5000.0 {
            amp = 5000.0;
        }
        self.wobble_amp = to_s16(amp);
        self.wobble_phase = phase;
        self.decay_shake();
    }

    /// `Scene_CalcShakeMtx`: the impact shake rolls about X (drop) and Z
    /// (landing) and lifts the scene.
    fn calc_shake_mtx(&self) -> Mtx {
        let phase = self.shake_phase as i32;
        let land = self.land_impulse as f32;
        let drop = self.drop_impulse as f32;
        let ty = self.land_lift * land * sin_s(phase) + self.drop_lift * drop * sin_s(phase);
        mtx::rot_trans(to_s16(-1.0 * drop * sin_s(phase)) as i32, 0, to_s16(land * sin_s(phase)) as i32, [0.0, ty, 0.0])
    }

    /// `Scene_CalcWorldMtx` (0x8130C694):
    /// `RotY(spin) · Shake · Wobble · Iso · T(15, 15, 15)`.
    fn calc_world_mtx(&self) -> Mtx {
        let phase = self.wobble_phase as i32;
        let amp = self.wobble_amp as f32;
        let k = self.spin_tilt * self.spin_speed / self.spin_threshold as f32;
        let mut t = psmtx::trans(15.0, 15.0, 15.0);
        let mut m = mtx::rot_trans(self.iso[0] as i32, self.iso[1] as i32, self.iso[2] as i32, [0.0; 3]);
        t = psmtx::concat(&m, &t);
        let w = |speed: i32| to_s16(1000.0 * k + (1.0 - k) * (amp * sin_s(phase * speed))) as i32;
        m = mtx::rot_trans(w(0x5A), w(0x3C), w(0x46), [0.0; 3]);
        m = psmtx::concat(&m, &t);
        m = psmtx::concat(&self.calc_shake_mtx(), &m);
        t = mtx::rot_trans(0, self.spin as i32, 0, [0.0; 3]);
        psmtx::concat(&t, &m)
    }

    /// `sWorldMtx` as `Scene_DrawWorld` (0x8130C90C) builds it: the world
    /// matrix, squashed along XZ and stretched along Y as the spin speeds
    /// up (`Scene_SquashWorld`).
    pub fn world_mtx(&self) -> Mtx {
        let mut w = psmtx::concat(&self.calc_world_mtx(), &psmtx::IDENTITY);
        let xz = 1.0 - self.squash_xz;
        let th = self.spin_threshold as f32;
        let squash = [
            1.0 - xz * self.spin_speed / th,
            1.0 + self.stretch_y * self.spin_speed / th,
            1.0 - xz * self.spin_speed / th,
        ];
        mtx::scale_cols(&mut w, squash);
        w
    }

    /// `Scene_UpdateSpinOut` (0x8130E70C): coast, then 3 full turns plus
    /// outSettle (Hermite), then settle back to 0. Returns the
    /// `Sound_UpdateSpin(slow, level)` arguments when the spin wraps past
    /// the front.
    pub fn update_spin_out(&mut self) -> Option<(bool, f32)> {
        let prev = self.spin;
        let f = self.out_frame;
        let (c, t, st) = (self.out_coast_frames, self.out_turn_frames, self.out_settle_frames);
        let speed_since = |spin: i16| (spin as u16).wrapping_sub(prev as u16) as i16 as f32;
        if f >= 0 && f < c {
            self.spin = to_s16(self.spin as f32 + self.spin_speed);
            self.out_start = self.spin;
        } else if f >= c && f < t + c {
            let from = -(self.out_start as i32 + ((self.out_turns as i32) << 16));
            self.spin = gbase::hermite(f as f32, c as f32, from as f32, self.out_speed, (c + t) as f32, self.out_settle as f32, 0.0) as i32 as i16;
            if f > c {
                self.spin_speed = speed_since(self.spin);
            }
        } else if f >= t + c && f < c + (t + st) {
            self.spin = gbase::hermite(f as f32, (t + c) as f32, self.out_settle as f32, 0.0, (c + (t + st)) as f32, 0.0, 0.0) as i32 as i16;
            self.spin_speed = speed_since(self.spin);
        } else if f == c + (t + st) {
            self.spin = 0;
            self.spin_speed = 0.0;
        }

        let slow = self.spin_speed <= 5000.0;
        let mut level = self.spin_speed / 5000.0;
        if level > 1.0 {
            level = 1.0;
        }
        if level < 0.01 {
            level = 0.0;
        }
        let wrapped = (prev < 0 && self.spin >= 0) || (prev > 0 && self.spin < 0);
        (wrapped && level > self.out_whoosh_min).then_some((slow, level))
    }

    /// `Scene_CalcSpinOutMtx` (0x8130EA34): the spin-out cube matrix, the
    /// intro wobble blended into the menu cube's idle wobble as the spin
    /// slows down. Advances `menu.wobble_frame` by 7 when `advance` is set.
    pub fn calc_spin_out_mtx(&self, menu: &mut Work, advance: bool) -> Mtx {
        let mut frame = menu.wobble_frame;
        let phase = self.wobble_phase as i32;
        let amp = self.wobble_amp as f32;
        let k = self.spin_speed / self.spin_threshold as f32;
        let tilt = 1.0 - self.spin_tilt;
        if self.out_frame == 1 {
            frame = 0;
        }
        let a = tilt * k;
        let b = 1.0 - a;
        let f = frame as i32;
        let ty = menu.wobble_amp_ty as f32 * sin_s(menu.wobble_speed_ty as i32 * f);
        let tx = menu.wobble_amp_tx as f32 * sin_s(menu.wobble_phase_tx as i32 + menu.wobble_speed_tx as i32 * f);
        let s16 = |x: f32| x as i32 as i16 as i32;
        let m = mtx::rot_trans(
            to_s16(a * (8191.0 + amp * sin_s(s16(90.0 * phase as f32))) + b * (menu.wobble_amp[0] as f32 * cos_s(menu.wobble_speed[0] as i32 * f))) as i32,
            to_s16(a * (-6371.0 + amp * sin_s(s16(60.0 * phase as f32))) + b * (menu.wobble_amp[1] as f32 * cos_s(menu.wobble_phase_ry as i32 + menu.wobble_speed[1] as i32 * f))) as i32,
            to_s16(a * (-5461.0 + amp * sin_s(s16(70.0 * phase as f32))) + b * (menu.wobble_amp[2] as f32 * cos_s(menu.wobble_speed[2] as i32 * f))) as i32,
            [tx, ty, 0.0],
        );
        if advance {
            frame = frame.wrapping_add(7);
        }
        menu.wobble_frame = frame;
        m
    }

    /// `Scene_SetBigCubeMtx`'s scale: 1.1x at rest, squashed and stretched
    /// like the world while spinning.
    pub fn big_cube_scale(&self) -> [f32; 3] {
        let k = self.spin_speed / self.spin_threshold as f32;
        let rest = 1.1 * (1.0 - k);
        let scale = self.model_scale;
        let xz = scale * (rest + (1.0 - (1.0 - self.squash_xz)) * k);
        [xz, scale * (rest + (1.0 + self.stretch_y) * k), xz]
    }
}
