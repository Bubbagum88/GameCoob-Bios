//! The scene director around the cube: holding A to spin, the idle wobble,
//! the impact shake, and the world matrix every intro model hangs from.
//! Reimplemented from the IPL's behavior; constants are its own defaults.

use super::gbase;
use super::mtx::{self, sin_s, to_s16};
use crate::sdk::mtx::{self as psmtx, Mtx};

pub struct Scene {
    pub spin: i16,
    pub spin_threshold: f32,
    pub spin_speed: f32,
    pub spin_accel: f32,
    spin_accel_step: f32,
    pub wobble_phase: i16,
    pub wobble_amp: i16,
    spin_tilt: f32,
    pub squash_xz: f32,
    pub stretch_y: f32,
    pub logo_alpha: u8,
    pub land_impulse: i16,
    pub drop_impulse: i16,
    pub land_impulse0: i16,
    pub drop_impulse0: i16,
    pub shake_phase: i16,
    land_decay: f32,
    drop_decay: f32,
    land_lift: f32,
    drop_lift: f32,
    shake_step: i16,
    iso: [i32; 3],
    ease_speed: f32,
    ease_frame: i32,
    ease_from: i16,
    whoosh_min: f32,
}

impl Scene {
    pub fn new() -> Self {
        Scene {
            spin: 0,
            spin_threshold: 5000.0,
            spin_speed: 0.0,
            spin_accel: 0.0,
            spin_accel_step: 1.5,
            wobble_phase: 2000,
            wobble_amp: 0,
            // 0.7 by default, but Scene_Reset clears it before the intro.
            spin_tilt: 0.0,
            squash_xz: 0.5,
            stretch_y: 1.0,
            logo_alpha: 0,
            land_impulse: 0,
            drop_impulse: 0,
            land_impulse0: 4000,
            drop_impulse0: 12000,
            shake_phase: 0,
            land_decay: 0.93,
            drop_decay: 0.925,
            land_lift: 0.015,
            drop_lift: 0.005,
            shake_step: 1000,
            iso: [0x2000, -0x1914, -0x1555],
            ease_speed: 0.0,
            ease_frame: 60,
            ease_from: 0,
            whoosh_min: 0.4,
        }
    }

    /// Hold A (while `can_spin`) to accelerate. Returns a whoosh volume when
    /// the spin wraps past the front.
    pub fn update_spin(&mut self, held: bool, can_spin: bool, force: bool) -> Option<f32> {
        let prev = self.spin;
        if force || (held && can_spin) {
            self.spin_accel += self.spin_accel_step;
            self.spin_speed += self.spin_accel;
            self.spin = to_s16(self.spin as f32 + self.spin_speed);
            if !force {
                self.ease_from = self.spin;
                self.ease_speed = self.spin_speed;
                self.ease_frame = 0;
            }
        } else {
            self.spin_accel *= 0.76;
            self.spin_speed *= 0.95;
            if self.spin_accel < 0.1 {
                self.spin_accel = 0.0;
            }
            if self.spin_speed < 0.01 {
                self.spin_speed = 0.0;
            }
            self.spin = to_s16(gbase::hermite(
                self.ease_frame as f32,
                0.0,
                self.ease_from as f32,
                self.ease_speed,
                50.0,
                0.0,
                0.0,
            ));
            self.ease_frame = (self.ease_frame + 1).min(50);
        }
        let level = (self.spin_speed / 5000.0).min(1.0);
        let level = if level < 0.01 { 0.0 } else { level };
        let wrapped = (prev < 0 && self.spin >= 0) || (prev > 0 && self.spin < 0);
        if (can_spin || force) && wrapped && level > self.whoosh_min {
            return Some(level);
        }
        None
    }

    fn decay_shake(&mut self) {
        let mut land = self.land_impulse as f32 * self.land_decay;
        let mut drop = self.drop_impulse as f32 * self.drop_decay;
        if land < 10.0 {
            land = 0.0;
        }
        if drop < 80.0 {
            drop = 0.0;
        }
        if land > 0.0 || drop > 0.0 {
            self.shake_phase = self.shake_phase.wrapping_add(self.shake_step);
        }
        self.land_impulse = land as i16;
        self.drop_impulse = drop as i16;
    }

    pub fn update_wobble(&mut self) {
        let mut amp = self.wobble_amp as f32;
        amp = if self.spin_accel > 0.0 { amp + 36.0 } else { amp * 0.945 };
        if amp < 1.0 {
            amp = 0.0;
        }
        if amp > 0.0 {
            self.wobble_phase = self.wobble_phase.wrapping_add(7);
        }
        self.wobble_amp = amp.min(5000.0) as i16;
        self.decay_shake();
    }

    /// `RotY(spin) · Shake · Wobble · Iso · T(15,15,15)`, then squash/stretch.
    pub fn world_mtx(&self) -> Mtx {
        let f = self.spin_tilt * self.spin_speed / self.spin_threshold;
        let amp = self.wobble_amp as f32;
        let ph = self.wobble_phase as i32;
        let base = 1000.0 * f;
        let w = |k: i32| to_s16(base + (1.0 - f) * amp * sin_s(ph * k)) as i32;

        let mut m = psmtx::concat(&mtx::rot_trans(self.iso[0], self.iso[1], self.iso[2], [0.0; 3]), &psmtx::trans(15.0, 15.0, 15.0));
        m = psmtx::concat(&mtx::rot_trans(w(0x5A), w(0x3C), w(0x46), [0.0; 3]), &m);

        let land = self.land_impulse as f32;
        let drop = self.drop_impulse as f32;
        let sn = sin_s(self.shake_phase as i32);
        let shake = mtx::rot_trans(
            to_s16(-drop * sn) as i32,
            0,
            to_s16(land * sn) as i32,
            [0.0, self.land_lift * land * sn + self.drop_lift * drop * sn, 0.0],
        );
        m = psmtx::concat(&shake, &m);
        m = psmtx::concat(&mtx::rot_trans(0, self.spin as i32, 0, [0.0; 3]), &m);

        let k = self.spin_speed / self.spin_threshold;
        mtx::scale_cols(&mut m, [1.0 - (1.0 - self.squash_xz) * k, 1.0 + self.stretch_y * k, 1.0 - (1.0 - self.squash_xz) * k]);
        m
    }

    /// The spin tilt factor (0.7).
    pub fn spin_tilt_k(&self) -> f32 {
        self.spin_tilt
    }
}
