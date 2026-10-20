//! The boot intro: a small cube tumbles around a 3x3 grid, leaving tiles that
//! trace a "C", hops into the middle, then the tiles rise into a solid block
//! and the wordmark fades in. Original choreography; no Nintendo assets.
//!
//! Easter egg in spirit of the console's held-button variants: press Z during
//! the intro to switch to the "odd" palette. Any other key skips.

use alloc::vec::Vec;
use uefi::proto::console::gop::GraphicsOutput;
use uefi::proto::console::text::Key;
use uefi::system;

use crate::clock::Clock;
use crate::gfx::{Camera, Canvas, Rgb, draw_box, mix, rgb};
use crate::math::*;

/// Grid cells the small cube rolls through. The first two are the approach
/// from off the board and leave no tile.
const PATH: [(i32, i32); 9] = [(3, -1), (2, -1), (1, -1), (0, -1), (-1, -1), (-1, 0), (-1, 1), (0, 1), (1, 1)];
const APPROACH: usize = 2;

const ROLL_START: f32 = 0.5;
const ROLL_STEP: f32 = 0.30;
const HOP_LEN: f32 = 0.7;
const RISE_LEN: f32 = 1.1;
const TEXT_DELAY: f32 = 0.9;
const HOLD: f32 = 2.4;
const FADE_OUT: f32 = 0.6;

#[derive(Clone, Copy)]
pub struct Palette {
    pub bg_top: Rgb,
    pub bg_bottom: Rgb,
    pub tile_top: Rgb,
    pub tile_side: Rgb,
    pub cube: [Rgb; 6],
    pub text: Rgb,
}

pub const NORMAL: Palette = Palette {
    bg_top: rgb(4, 2, 14),
    bg_bottom: rgb(24, 16, 58),
    tile_top: rgb(132, 112, 246),
    tile_side: rgb(78, 60, 188),
    cube: [
        rgb(196, 186, 255),
        rgb(172, 160, 244),
        rgb(236, 232, 255),
        rgb(150, 138, 226),
        rgb(214, 206, 255),
        rgb(160, 150, 236),
    ],
    text: rgb(226, 220, 255),
};

pub const ODD: Palette = Palette {
    bg_top: rgb(2, 10, 6),
    bg_bottom: rgb(10, 40, 24),
    tile_top: rgb(110, 220, 120),
    tile_side: rgb(50, 150, 70),
    cube: [
        rgb(230, 255, 200),
        rgb(200, 245, 170),
        rgb(250, 255, 235),
        rgb(170, 225, 140),
        rgb(215, 250, 190),
        rgb(185, 235, 155),
    ],
    text: rgb(220, 255, 210),
};

fn roll_end() -> f32 {
    ROLL_START + ROLL_STEP * (PATH.len() - 1) as f32
}

fn cell(c: (i32, i32)) -> Vec3 {
    v3(c.0 as f32, 0.5, c.1 as f32)
}

/// Position and orientation of the rolling cube at time `t`.
fn rolling_cube(t: f32) -> (Vec3, Mat3) {
    let steps = PATH.len() - 1;
    let mut orient = Mat3::IDENTITY;
    let up = v3(0.0, 1.0, 0.0);
    let k = ((t - ROLL_START) / ROLL_STEP).max(0.0);
    for i in 0..steps {
        let a = cell(PATH[i]);
        let b = cell(PATH[i + 1]);
        let dir = b - a;
        let axis = up.cross(dir);
        let pivot = a + dir * 0.5 - v3(0.0, 0.5, 0.0);
        let step_t = k - i as f32;
        if step_t < 1.0 {
            let ang = ease(step_t.max(0.0)) * PI * 0.5;
            let r = Mat3::axis_angle(axis, ang);
            return (pivot + r.apply(a - pivot), r.mul(&orient));
        }
        orient = Mat3::axis_angle(axis, PI * 0.5).mul(&orient).snapped();
    }
    (cell(PATH[steps]), orient)
}

pub fn play(gop: &mut GraphicsOutput, cv: &mut Canvas, clock: &Clock) -> Palette {
    let mut pal = NORMAL;
    let ring: Vec<(i32, i32)> = PATH[APPROACH..].to_vec();
    let hop_start = roll_end();
    let rise_start = hop_start + HOP_LEN;
    let text_start = rise_start + TEXT_DELAY;
    let end = rise_start + RISE_LEN + HOLD;
    let t0 = clock.now();

    loop {
        let t = clock.now() - t0;
        if t > end + FADE_OUT {
            break;
        }
        match system::with_stdin(|i| i.read_key()) {
            Ok(Some(Key::Printable(c))) if matches!(char::from(c), 'z' | 'Z') => pal = ODD,
            Ok(Some(_)) => break,
            _ => {}
        }

        cv.gradient(pal.bg_top, pal.bg_bottom);

        // Camera: low orbit while rolling, swings up to a near-overhead
        // three-quarter view once the block forms.
        let swing = ease((t - hop_start) / (RISE_LEN + HOP_LEN));
        let yaw = lerp(0.55 - t * 0.08, 0.0, swing);
        let el = lerp(0.62, 1.02, swing);
        let dist = lerp(7.0, 9.5, swing);
        let target = v3(0.0, lerp(0.3, -0.1, swing), lerp(0.0, 0.5, swing));
        let eye = target + v3(sin(yaw) * cos(el), sin(el), cos(yaw) * cos(el)) * dist;
        let cam = Camera::look_at(eye, target, 42.0, cv);

        // Tiles: appear as the cube leaves each ring cell, then rise.
        let tile_colors = |top: Rgb| [pal.tile_side, pal.tile_side, top, pal.tile_side, pal.tile_side, pal.tile_side];
        for (i, &c) in ring.iter().enumerate() {
            let left = if i + 1 == ring.len() {
                hop_start + HOP_LEN * 0.3
            } else {
                ROLL_START + ROLL_STEP * (APPROACH + i) as f32 + ROLL_STEP * 0.5
            };
            if t < left {
                continue;
            }
            let pop = ease_back((t - left) / 0.25);
            let rise = ease_back((t - rise_start - i as f32 * 0.06) / (RISE_LEN * 0.6));
            let h = lerp(0.08 * pop, 1.0, rise);
            let flash = clamp01(1.0 - (t - left) / 0.4);
            let top = mix(pal.tile_top, rgb(255, 255, 255), flash * 0.6);
            draw_box(
                cv,
                &cam,
                v3(c.0 as f32, h * 0.5, c.1 as f32),
                v3(0.47, h * 0.5, 0.47),
                &Mat3::IDENTITY,
                &tile_colors(top),
            );
        }

        // The small cube: roll, then hop into the center with a spin.
        let (pos, rot) = if t < hop_start {
            rolling_cube(t)
        } else {
            let (from, orient) = rolling_cube(hop_start);
            let k = clamp01((t - hop_start) / HOP_LEN);
            let e = ease(k);
            let mut p = from.lerp(cell((0, 0)), e);
            p.y += sin(k * PI) * 1.8;
            let spin = Mat3::rot_y(e * PI * 2.0).mul(&Mat3::rot_x(e * PI * 2.0));
            (p, spin.mul(&orient))
        };
        draw_box(cv, &cam, pos, v3(0.5, 0.5, 0.5), &rot, &pal.cube);

        // Wordmark.
        let ta = clamp01((t - text_start) / 0.8);
        if ta > 0.0 {
            let s = (cv.h as i32 / 70).max(3);
            let cx = cv.w as i32 / 2;
            let y = cv.h as i32 * 70 / 100;
            cv.text_centered(cx, y, s, "GAMECOOB", pal.text, ta);
            let s2 = (s / 3).max(1);
            cv.text_centered(cx, y + s * 11, s2, "BOOT ENVIRONMENT", pal.tile_top, ta * 0.85);
        }

        let fade_in = clamp01(t / 0.5);
        let fade_out = 1.0 - clamp01((t - end) / FADE_OUT);
        cv.fade(fade_in * fade_out);
        cv.present(gop);
    }
    pal
}
