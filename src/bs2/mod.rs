//! The GameCube boot intro, rebuilt from the decompiled BS2 and drawn with
//! the models and textures of the user's own `ipl.bin`.
//!
//! * [`cube`] and [`scene`] are the frame-exact state machines.
//! * [`render`] and [`raster`] draw J3D models like GX does, on the CPU.
//! * [`Assets::load`] pulls every model, texture and animation out of the dump.

pub mod anim;
pub mod cube;
pub mod mtx;
pub mod raster;
pub mod render;
pub mod scene;
pub mod shader;

use crate::ipl::{bti, j3d::Model, Ipl};
use alloc::vec::Vec;
use anim::{Bck, Ipk};
use cube::{Cube, Jingle, Phase, GRID};
use mtx::{Mtx, SinTable};
use raster::{BlendMode, Prim, SVert, Sh, State, Target};
use render::{DrawOpts, Prepared, View};
use scene::Scene;

/// Resource ids in the IPL archive.
mod res {
    pub const BG_INNER: u16 = 0x23;
    pub const BG_OUTER: u16 = 0x24;
    pub const TILE_TEX: u16 = 0x65;
    pub const LOGO: u16 = 0x66;
    pub const BIG_CUBE: u16 = 0x67;
    pub const TEXT_BCK: u16 = 0x69;
    pub const TEXT_IPK: u16 = 0x6A;
    pub const TEXT: u16 = 0x6B;
    pub const SMALL_CUBE: u16 = 0xA8;
}

pub struct Assets {
    pub bg_inner: Prepared,
    pub bg_outer: Prepared,
    pub logo: Prepared,
    pub big_cube: Prepared,
    pub text: Prepared,
    pub small_cube: Prepared,
    pub tile: shader::FastTex,
    pub text_bck: Bck,
    pub text_ipk: Ipk,
    pub path: Vec<cube::PathStep>,
}

impl Assets {
    pub fn load(ipl: &Ipl) -> Option<Assets> {
        let model = |id| Model::parse(&ipl.resource(id).ok()?).ok().map(Prepared::new);
        let bs2 = ipl.bs2();
        let path = cube::read_path(|a| bs2.get(a.checked_sub(crate::ipl::BS2_RAM)? as usize).copied())?;
        Some(Assets {
            bg_inner: model(res::BG_INNER)?,
            bg_outer: model(res::BG_OUTER)?,
            logo: model(res::LOGO)?,
            big_cube: model(res::BIG_CUBE)?,
            text: model(res::TEXT)?,
            small_cube: model(res::SMALL_CUBE)?,
            tile: shader::FastTex::new(&bti::parse(&ipl.resource(res::TILE_TEX).ok()?).ok()?),
            text_bck: Bck::parse(&ipl.resource(res::TEXT_BCK).ok()?)?,
            text_ipk: Ipk::parse(&ipl.resource(res::TEXT_IPK).ok()?)?,
            path,
        })
    }
}

/// What the player pressed this logic frame.
#[derive(Clone, Copy, Default)]
pub struct Input {
    /// A held (spins the cube).
    pub a: bool,
    /// Easter egg selection, latched at boot.
    pub egg: Option<Jingle>,
}

/// Frames to hold the finished logo before the IPL spins to the menu on its
/// own when no disc is present (measured from a capture of the real IPL).
pub const HOLD_FRAMES: i32 = 0;
/// Spin-out timing (Scene defaults): keep spinning, decelerate with three
/// extra turns, settle, and how long the logo scene keeps fading.
pub const SPINOUT_KEEP: i32 = 15;
pub const SPINOUT_BRAKE: i32 = 80;
pub const SPINOUT_SETTLE: i32 = 50;
pub const SPINOUT_TURNS: i32 = 3;
pub const SPINOUT_FADE: i32 = 30;
pub const SPINOUT_END: i32 = SPINOUT_KEEP + SPINOUT_BRAKE + SPINOUT_SETTLE;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum State0 {
    Intro,
    SpinOut,
    Done,
}

pub struct Intro {
    pub sin: SinTable,
    pub cube: Cube,
    pub scene: Scene,
    pub state: State0,
    hold: i32,
    pub spin_frame: i32,
    spin_start: i16,
    pub frame: u32,
}

impl Intro {
    pub fn new(a: &Assets) -> Intro {
        Intro {
            sin: SinTable::new(),
            cube: Cube::new(a.path.clone()),
            scene: Scene::new(),
            state: State0::Intro,
            hold: 0,
            spin_frame: 0,
            spin_start: 0,
            frame: 0,
        }
    }

    /// One 1/60 s logic frame (Scene_UpdateNoDisc / the spin-out).
    pub fn update(&mut self, input: Input) -> Option<Jingle> {
        self.frame += 1;
        let mut jingle = None;
        match self.state {
            State0::Intro => {
                if self.cube.done {
                    self.hold += 1;
                }
                let force = self.hold > HOLD_FRAMES;
                if !self.cube.done || force {
                    let _whoosh = self.scene.update_spin(input.a, self.cube.running, force);
                }
                self.scene.update_wobble();
                jingle = self.cube.update(&mut self.scene, input.egg.unwrap_or(Jingle::Normal));
                if self.scene.spin_speed > self.scene.spin_threshold {
                    self.state = State0::SpinOut;
                    self.spin_start = self.scene.spin;
                }
            }
            State0::SpinOut => {
                self.update_spin_out();
                self.scene.update_wobble();
                if self.spin_frame > SPINOUT_END {
                    self.state = State0::Done;
                }
            }
            State0::Done => {}
        }
        jingle
    }

    /// Scene_UpdateSpinOut: brake the spin with Hermite curves so the menu
    /// cube lands facing front.
    fn update_spin_out(&mut self) {
        let sc = &mut self.scene;
        let t = self.spin_frame;
        let prev = sc.spin;
        let (k0, k1, k2) = (SPINOUT_KEEP, SPINOUT_KEEP + SPINOUT_BRAKE, SPINOUT_END);
        if t < k0 {
            sc.spin = mtx::to_s16(sc.spin as f32 + sc.spin_speed);
            self.spin_start = sc.spin;
        } else if t < k1 {
            let from = -(self.spin_start as i32 + (SPINOUT_TURNS << 16)) as f32;
            sc.spin = mtx::to_s16(mtx::hermite(t as f32, k0 as f32, from, sc.spin_threshold, k1 as f32, 5000.0, 0.0));
            if t > k0 {
                sc.spin_speed = sc.spin.wrapping_sub(prev) as f32;
            }
        } else if t < k2 {
            sc.spin = mtx::to_s16(mtx::hermite(t as f32, k1 as f32, 5000.0, 0.0, k2 as f32, 0.0, 0.0));
            sc.spin_speed = sc.spin.wrapping_sub(prev) as f32;
        } else if t == k2 {
            sc.spin = 0;
            sc.spin_speed = 0.0;
        }
        self.spin_frame += 1;
    }

    /// Render the current frame into `t`; `par` runs the rasterizer job on
    /// as many processors as the caller has (it must call `job.work()` on
    /// each and return once `job.finished()`).
    pub fn render(&self, t: &mut Target, a: &Assets, par: &dyn Fn(&raster::Job)) {
        let mut list = Vec::new();
        self.draw(&mut list, a, [t.w as f32, t.h as f32]);
        t.clear([0.0; 3]);
        let job = raster::Job::new(t, &list, 16);
        par(&job);
    }

    /// Build the frame's display list (Scene_DrawWorld + Scene_Draw2DNoDisc).
    fn draw<'a>(&self, t: &mut Vec<Prim<'a>>, a: &'a Assets, vp: [f32; 2]) {
        if self.state != State0::Intro {
            return self.draw_spin_out(t, a, vp);
        }
        let s = &self.sin;
        let sc = &self.scene;
        let mut view = View::intro(-55.0, true);
        view.vp = vp;
        let world = sc.world_mtx(s);
        let master: i32 = 255;
        let scaled = |m: &Mtx, k: f32| mtx::concat(m, &mtx::scale(k, k, k));

        // 1. The "NINTENDO GAMECUBE" wordmark, fixed to the screen.
        self.draw_text(t, a, &view, master as u8);

        // 2. Inner background cube, visible while the impacts shake it.
        let shake = (sc.land_impulse as i32 + sc.drop_impulse as i32).min(3000);
        let inner_a = master * (255 * shake / 3000) / 255;
        render::draw_model(t, &a.bg_inner, &world, &view, &DrawOpts { alpha: inner_a as u8, ..Default::default() });

        // 3. Outer background, back half, visible while wobbling.
        let outer_a = (master * (255 * (sc.wobble_amp as i32).min(3000) / 3000) / 255) as u8;
        let outer = scaled(&world, 1.01);
        let (back, front) = split_draws(&a.bg_outer.model);
        render::draw_model(t, &a.bg_outer, &outer, &view, &DrawOpts { alpha: outer_a, only: Some(back), ..Default::default() });

        // 4. Trail tiles and the small cube, fading out as the logo fades in.
        let cube_a = ((255 - sc.logo_alpha as i32) * master / 255) as u8;
        self.draw_tiles(t, a, &world, &view, cube_a);
        let small = self.cube.small_cube_mtx(s, &world);
        let small_a = (self.cube.alpha as i32 * cube_a as i32 / 255) as u8;
        render::draw_model(t, &a.small_cube, &small, &view, &DrawOpts { alpha: small_a, ..Default::default() });

        // 5. The logo, 6. the outer background's front half.
        let logo_a = (sc.logo_alpha as i32 * master / 255) as u8;
        render::draw_model(t, &a.logo, &world, &view, &DrawOpts { alpha: logo_a, ..Default::default() });
        render::draw_model(t, &a.bg_outer, &outer, &view, &DrawOpts { alpha: outer_a, only: Some(front), ..Default::default() });
    }

    fn draw_text<'a>(&self, t: &mut Vec<Prim<'a>>, a: &'a Assets, view: &View, alpha: u8) {
        let tf = self.cube.text_anim_frame();
        let joints = render::pose(&a.text.model, |j| {
            let ja = a.text_bck.joints.get(j)?;
            let v = |tr: &[anim::Track; 3]| [tr[0].sample(tf), tr[1].sample(tf), tr[2].sample(tf)];
            let r = v(&ja.r);
            Some((v(&ja.s), [r[0] as i32 as i16, r[1] as i32 as i16, r[2] as i32 as i16], v(&ja.t)))
        });
        let colors: Vec<(u16, [f32; 4])> = a
            .text_ipk
            .entries
            .iter()
            .map(|(m, tr)| (*m, [tr[0].sample(tf), tr[1].sample(tf), tr[2].sample(tf), tr[3].sample(tf)]))
            .collect();
        render::draw_model(
            t,
            &a.text,
            &mtx::IDENTITY,
            view,
            &DrawOpts { alpha, joints: Some(&joints), mat_colors: &colors, only: None },
        );

    }

    /// Scene_SpinOut + Scene_Draw2DSpinOut: perspective camera rising to the
    /// menu height, the logo scene fading out and the glass menu cube in.
    fn draw_spin_out<'a>(&self, t: &mut Vec<Prim<'a>>, a: &'a Assets, vp: [f32; 2]) {
        let s = &self.sin;
        let sc = &self.scene;
        let f = self.spin_frame.min(SPINOUT_END);
        let fade = SPINOUT_FADE;
        let yoff = if f <= fade { -55.0 + 45.0 * f as f32 / fade as f32 } else { -10.0 };
        let mut view = View::intro(yoff, false);
        view.vp = vp;
        let mut text_view = View::intro(-55.0, true);
        text_view.vp = view.vp;
        let k = sc.spin_speed / sc.spin_threshold;
        if f <= fade {
            // The logo scene (inner background and wordmark) fades out.
            let shake = (sc.land_impulse as i32 + sc.drop_impulse as i32 + sc.wobble_amp as i32).min(3000);
            let a_in = ((fade - f) * 255 * shake / (fade * 3000)) as u8;
            let world = sc.world_mtx(s);
            render::draw_model(t, &a.bg_inner, &world, &view, &DrawOpts { alpha: a_in, ..Default::default() });
            self.draw_text(t, a, &text_view, a_in);
        }
        // The glass cube: faces the camera once the spin settles.
        let tilt = (1.0 - sc.spin_tilt_k()) * k;
        let wob = sc.wobble_amp as f32 * s.sin(90 * sc.wobble_phase as i32);
        let rot = mtx::rot_trans(
            s,
            mtx::to_s16(tilt * (8191.0 + wob)) as i32,
            mtx::to_s16(tilt * -6371.0) as i32,
            mtx::to_s16(tilt * -5461.0) as i32,
            [0.0; 3],
        );
        let m = mtx::concat(&mtx::rot_trans(s, 0, sc.spin as i32, 0, [0.0; 3]), &rot);
        let base = 1.1 * (1.0 - k);
        let sxz = base + sc.squash_xz * k;
        let sy = base + (1.0 + sc.stretch_y) * k;
        let m = mtx::concat(&m, &mtx::scale(sxz, sy, sxz));
        let cube_a = if f <= fade { (255 * f / fade) as u8 } else { 255 };
        view.ambient = [40.0 / 255.0, 40.0 / 255.0, 40.0 / 255.0, 1.0];
        render::draw_model(t, &a.big_cube, &m, &view, &DrawOpts { alpha: cube_a, ..Default::default() });
    }

    /// Cube_DrawTiles: one textured quad per live cell on the three faces.
    fn draw_tiles<'a>(&self, t: &mut Vec<Prim<'a>>, a: &'a Assets, world: &Mtx, view: &View, alpha: u8) {
        if alpha == 0 {
            return;
        }
        let c = &self.cube;
        let c3 = (c.size * 3 / 2) as f32;
        let mv = mtx::concat(&view.view, &mtx::concat(world, &mtx::trans(c3, c3, c3)));
        let size = c.size as f32;
        let inset = c.tile_inset as f32 / 16.0;
        let rgb = c.tile_rgb.map(|x| x as f32 / 255.0);
        let st = State { z_compare: true, z_update: false, blend: BlendMode::Alpha, cull: 0 };
        for f in 0..cube::FACES {
            for x in 0..GRID {
                for y in 0..GRID {
                    let tile = c.tile(f, x, y);
                    if tile.group == 0 {
                        continue;
                    }
                    let ta = c.tile_alpha(tile) * alpha as i32 / 255;
                    let px = (x as f32 - 16.0) * size;
                    let py = (y as f32 - 16.0) * size;
                    let corners = [
                        (-(px + inset), -(py + inset), [0.0, 0.0]),
                        (-(px + size), -(py + inset), [0.0, 2.0]),
                        (-(px + size), -(py + size), [2.0, 2.0]),
                        (-(px + inset), -(py + size), [2.0, 0.0]),
                    ];
                    let mut sv = [SVert::default(); 4];
                    let mut ok = true;
                    for (k, &(u, v, uv)) in corners.iter().enumerate() {
                        let p = match f {
                            0 => [u, 0.0, v],
                            1 => [v, u, 0.0],
                            _ => [0.0, v, u],
                        };
                        let Some(s) = view.project(mtx::apply(&mv, p)) else {
                            ok = false;
                            break;
                        };
                        sv[k] = SVert { x: s[0], y: s[1], z: s[2], rw: s[3], color: [rgb[0], rgb[1], rgb[2], ta as f32 / 255.0], uv: [uv, [0.0; 2]] };
                    }
                    if !ok {
                        continue;
                    }
                    let sh = Sh::Modulate { tex: &a.tile };
                    t.push(Prim { v: [sv[0], sv[1], sv[2]], st, sh });
                    t.push(Prim { v: [sv[0], sv[2], sv[3]], st, sh });
                }
            }
        }
    }

    pub fn finished(&self) -> bool {
        self.state == State0::Done
    }

    #[allow(dead_code)]
    pub fn phase(&self) -> Phase {
        self.cube.phase
    }
}

/// The outer background is two passes: inside faces (drawn behind the
/// cube) then outside faces (drawn over the logo).
fn split_draws(m: &Model) -> (core::ops::Range<usize>, core::ops::Range<usize>) {
    let n = m.draws.len();
    let half = n / 2;
    (0..half.max(1).min(n), half.max(1).min(n)..n)
}
