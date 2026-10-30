//! The GameCube boot intro, rebuilt from the decompiled BS2 and drawn with
//! the models and textures of the user's own `ipl.bin`.
//!
//! * [`cube`] and [`scene`] are the frame-exact state machines.
//! * [`render`] and [`raster`] draw J3D models like GX does, on the CPU.
//! * [`Assets::load`] pulls every model, texture and animation out of the dump.

pub mod anim;
pub mod bs2main;
pub mod camera;
pub mod cube;
pub mod cube_draw;
pub mod fader;
pub mod font;
pub mod fx;
pub mod gbase;
pub mod glh;
pub mod ginit;
pub mod gmr;
pub mod menu;
pub mod mtx;
pub mod raster;
pub mod render;
pub mod scene;
pub mod shader;
pub mod ui;

use crate::ipl::{bti, j3d::Model, Ipl};
use alloc::vec::Vec;
use anim::{Bck, Ipk};
use cube::{Cube, Jingle, Phase};
use mtx::sin_s;
use crate::sdk::gx::{Gx, GxArena, GxRegs, GX_COLOR0A0};
use crate::sdk::mtx::{self as psmtx, Mtx};
use raster::Target;
use gmr::Instance;
use render::{Prepared, View};
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

/// Frames to hold the finished logo before it spins to the menu on its own
/// when no disc is present. The real IPL holds none; GameCoob lingers one
/// second (60 frames) on the logo.
pub const HOLD_FRAMES: i32 = 60;
/// Spin-out timing (Scene defaults): keep spinning, decelerate with three
/// extra turns, settle, and how long the logo scene keeps fading.
pub const SPINOUT_KEEP: i32 = 15;
pub const SPINOUT_BRAKE: i32 = 80;
pub const SPINOUT_SETTLE: i32 = 50;
pub const SPINOUT_TURNS: i32 = 3;
pub const SPINOUT_FADE: i32 = 30;
pub const SPINOUT_END: i32 = SPINOUT_KEEP + SPINOUT_BRAKE + SPINOUT_SETTLE;
/// Frames before the end of the spin-out that the front face starts fading in.
pub const SPINOUT_FACE: i32 = 100;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum State0 {
    Intro,
    SpinOut,
    Done,
}

pub struct Intro {
    pub cube: Cube,
    pub scene: Scene,
    pub state: State0,
    hold: i32,
    pub spin_frame: i32,
    spin_start: i16,
    pub frame: u32,
    /// The menu's idle-wobble clock (MenuWork.wobbleFrame): restarts with
    /// the spin-out and keeps running into the menu.
    pub wobble_frame: i16,
    /// GX registers as the last rendered frame left them.
    gx_regs: core::cell::Cell<Option<GxRegs>>,
}

impl Intro {
    pub fn new(a: &Assets) -> Intro {
        mtx::sin_table_init(12);
        Intro {
            cube: Cube::new(a.path.clone()),
            scene: Scene::new(),
            state: State0::Intro,
            hold: 0,
            spin_frame: 1,
            spin_start: 0,
            frame: 0,
            wobble_frame: 0,
            gx_regs: core::cell::Cell::new(None),
        }
    }

    /// One 1/60 s logic frame (Scene_UpdateNoDisc / the spin-out).
    pub fn update(&mut self, input: Input) -> Option<Jingle> {
        self.frame += 1;
        let mut jingle = None;
        match self.state {
            State0::Intro => {
                // Scene_UpdateNoDisc: A spins the cube while it runs; once
                // it is done (and the disc text has faded) the scene spins
                // itself, with a second spin and wobble step that frame.
                if !self.cube.done {
                    let _whoosh = self.scene.update_spin(input.a, self.cube.running, false);
                }
                self.scene.update_wobble();
                jingle = self.cube.update(&mut self.scene, input.egg.unwrap_or(Jingle::Normal));
                if self.scene.spin_speed > self.scene.spin_threshold {
                    self.start_spin_out();
                } else if self.cube.done {
                    self.hold += 1;
                    if self.hold > HOLD_FRAMES {
                        let _whoosh = self.scene.update_spin(false, self.cube.running, true);
                        self.scene.update_wobble();
                        if self.scene.spin_speed > self.scene.spin_threshold {
                            self.start_spin_out();
                        }
                    }
                }
            }
            State0::SpinOut => {
                // Scene_SpinOut: the spin-out counter starts at 1 (Scene_Reset).
                // Scene_CalcSpinOutMtx drives the menu's wobble clock (still 0
                // from Menu_Init) by 7 per frame.
                if self.spin_frame == 1 {
                    self.wobble_frame = 0;
                }
                self.update_spin_out();
                // The scene wobble is frozen during the spin-out.
                self.wobble_frame = self.wobble_frame.wrapping_add(7);
                if self.spin_frame > SPINOUT_END {
                    self.state = State0::Done;
                }
            }
            State0::Done => {}
        }
        jingle
    }

    /// Entering Scene state 4 snaps the scene spin to spinRest (0).
    fn start_spin_out(&mut self) {
        self.state = State0::SpinOut;
        self.scene.spin = 0;
        self.spin_start = 0;
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
            sc.spin = mtx::to_s16(gbase::hermite(t as f32, k0 as f32, from, sc.spin_threshold, k1 as f32, 5000.0, 0.0));
            if t > k0 {
                sc.spin_speed = sc.spin.wrapping_sub(prev) as f32;
            }
        } else if t < k2 {
            sc.spin = mtx::to_s16(gbase::hermite(t as f32, k1 as f32, 5000.0, 0.0, k2 as f32, 0.0, 0.0));
            sc.spin_speed = sc.spin.wrapping_sub(prev) as f32;
        } else if t == k2 {
            sc.spin = 0;
            sc.spin_speed = 0.0;
        }
        self.spin_frame += 1;
    }

    /// GX for this frame, starting from the registers the last frame left.
    /// BS2 draws every frame twice, once per half of the antialiased EFB
    /// (`fn_813015A4`), so even on the first frame the wordmark, in the
    /// lower half, is drawn over the state the first pass left; a first
    /// frame here does that pass once to get it.
    fn frame_gx<'a>(&self, arena: &'a GxArena<'a>, a: &'a Assets, vp: [f32; 2]) -> Gx<'a> {
        let regs = self.gx_regs.get().unwrap_or_else(|| {
            let scratch = GxArena::new();
            let mut gx = Gx::new(&scratch, vp[0], vp[1]);
            self.draw(&mut gx, a, vp);
            gx.regs()
        });
        Gx::with_regs(arena, regs)
    }

    /// Render the current frame into `t`; `par` runs the rasterizer job on
    /// as many processors as the caller has (it must call `job.work()` on
    /// each and return once `job.finished()`).
    #[allow(dead_code)] // used by tools/intro-render
    pub fn render(&self, t: &mut Target, a: &Assets, par: &dyn Fn(&raster::Job)) {
        let arena = GxArena::new();
        let vp = [t.w as f32, t.h as f32];
        let mut gx = self.frame_gx(&arena, a, vp);
        self.draw(&mut gx, a, vp);
        self.gx_regs.set(Some(gx.regs()));
        t.clear([0.0; 3]);
        let job = raster::Job::new(t, &gx.list, 16);
        par(&job);
    }

    /// Render the intro with the menu cube's front face fading in during the
    /// spin-out, or the menu once it has taken over.
    pub fn render_with_menu(&self, t: &mut Target, a: &Assets, ma: &menu::MenuAssets, m: Option<&menu::Menu>, par: &dyn Fn(&raster::Job)) {
        let arena = GxArena::new();
        let vp = [t.w as f32, t.h as f32];
        let mut gx = self.frame_gx(&arena, a, vp);
        match m {
            Some(m) => m.draw(&mut gx, a, ma, &self.spin_out_view(vp)),
            None => {
                self.draw(&mut gx, a, vp);
                // Scene_Draw2DSpinOut: the front face fades in over the
                // last 100 frames of the spin-out.
                let f = self.spin_frame - (SPINOUT_END - SPINOUT_FACE);
                if self.state != State0::Intro && f >= 0 {
                    let alpha = (255 * f.min(SPINOUT_FACE) / SPINOUT_FACE) as u8;
                    menu::Menu::draw_spin_out_face(&mut gx, ma, &self.spin_out_view(vp), &self.spin_out_mtx(), alpha);
                }
            }
        }
        self.gx_regs.set(Some(gx.regs()));
        t.clear([0.0; 3]);
        let job = raster::Job::new(t, &gx.list, 16);
        par(&job);
    }

    /// Build the frame's display list (Scene_DrawWorld + Scene_Draw2DNoDisc).
    pub fn draw<'a>(&self, gx: &mut Gx<'a>, a: &'a Assets, vp: [f32; 2]) {
        if self.state != State0::Intro {
            return self.draw_spin_out(gx, a, vp);
        }
        let sc = &self.scene;
        let mut view = View::intro(camera::cam_y(bs2main::disp_settings(), camera::CamState::Intro), true);
        view.vp = vp;
        gx.set_projection(&view.projection());
        gx.set_viewport(0.0, 0.0, vp[0], vp[1]);
        camera::load_light(gx, &view.view);

        // Scene_DrawWorld: the models' joint matrices.
        let world = sc.world_mtx();
        let placed = |p: &Prepared, m: &Mtx, s: f32| {
            let mut inst = Instance::new(&p.model);
            gmr::set_model_mtx(&mut inst, m, [s; 3]);
            gmr::set_view_mtx(&mut inst, &view.view);
            gmr::calc_joints(&p.model, &mut inst);
            inst
        };
        let mut inner = placed(&a.bg_inner, &world, 1.0);
        let mut logo = placed(&a.logo, &world, 1.0);
        let mut outer = placed(&a.bg_outer, &world, 1.01);
        let mut small = placed(&a.small_cube, &self.cube.small_cube_mtx(&world), 1.0);

        // Scene_Draw2DNoDisc.
        let master: i32 = 255;
        // 1. The "NINTENDO GAMECUBE" wordmark, fixed to the screen, drawn
        //    over the state the last frame left.
        self.draw_text(gx, a, &view.view, master as u8);
        gmr::setup(gx);

        // 2. Inner background cube, visible while the impacts shake it.
        let shake = (sc.land_impulse as i32 + sc.drop_impulse as i32).min(3000);
        inner.alpha = (master * (255 * shake / 3000) / 255) as u8;
        if inner.alpha > 0 {
            gmr::draw(gx, &a.bg_inner, &mut inner);
        }

        // 3. Outer background, inside faces, visible while wobbling.
        outer.alpha = (master * (255 * (sc.wobble_amp as i32).min(3000) / 3000) / 255) as u8;
        if outer.alpha > 0 {
            gmr::draw_packet(gx, &a.bg_outer, &mut outer, BG_OUTER_BACK);
        }

        // 4. Cube_DrawBig: trail tiles and the small cube, fading out as the
        //    logo fades in.
        let cube_a = ((255 - sc.logo_alpha as i32) * master / 255) as u8;
        cube_draw::draw_big_tiles(gx, &self.cube, &world, &view.view, cube_a, &a.tile);
        small.alpha = (self.cube.alpha as i32 * cube_a as i32 / 255) as u8;
        gmr::draw(gx, &a.small_cube, &mut small);

        // 5. The logo, 6. the outer background's outside faces.
        logo.alpha = (sc.logo_alpha as i32 * master / 255) as u8;
        if logo.alpha > 0 {
            gmr::draw(gx, &a.logo, &mut logo);
        }
        if outer.alpha > 0 {
            gmr::draw_packet(gx, &a.bg_outer, &mut outer, BG_OUTER_FRONT);
        }
    }

    /// Cube_DrawText + `J3D_Draw(&sText)`: the wordmark with its joint
    /// (`.bck`) and material color (`.ipk`) animations.
    fn draw_text<'a>(&self, gx: &mut Gx<'a>, a: &'a Assets, view: &Mtx, alpha: u8) {
        let tf = self.cube.text_anim_frame();
        let mut inst = Instance::new(&a.text.model);
        gmr::set_model_mtx(&mut inst, &psmtx::IDENTITY, [1.0; 3]);
        gmr::set_view_mtx(&mut inst, view);
        let joint = |j: u16| {
            let ja = a.text_bck.joints.get(j as usize)?;
            let v = |tr: &[anim::Track; 3]| [tr[0].sample(tf), tr[1].sample(tf), tr[2].sample(tf)];
            let r = v(&ja.r);
            Some(mtx::Srt { s: v(&ja.s), r: [r[0] as i32 as i16, r[1] as i32 as i16, r[2] as i32 as i16], t: v(&ja.t) })
        };
        let color = |m: u16| {
            let (_, tr) = a.text_ipk.entries.iter().find(|(i, _)| *i == m)?;
            Some([0, 1, 2, 3].map(|k| tr[k].sample(tf).clamp(0.0, 255.0) as u8))
        };
        gmr::animate_joints(&a.text.model, &mut inst, &joint, Some(&color));
        inst.alpha = alpha;
        gmr::draw(gx, &a.text, &mut inst);
    }

    /// Scene_SpinOut + Scene_Draw2DSpinOut: perspective camera rising to the
    /// menu height, the logo scene fading out and the glass menu cube in.
    fn draw_spin_out<'a>(&self, gx: &mut Gx<'a>, a: &'a Assets, vp: [f32; 2]) {
        let sc = &self.scene;
        let f = self.spin_frame.min(SPINOUT_END);
        let fade = SPINOUT_FADE;
        let view = self.spin_out_view(vp);
        gx.set_viewport(0.0, 0.0, vp[0], vp[1]);
        camera::load_light(gx, &view.view);
        let k = sc.spin_speed / sc.spin_threshold;
        if f < fade {
            // The logo scene (inner background and wordmark) fades out.
            let shake = (sc.land_impulse as i32 + sc.drop_impulse as i32 + sc.wobble_amp as i32).min(3000);
            let a_in = ((fade - f) * 255 * shake / (fade * 3000)) as u8;
            gx.set_projection(&view.projection());
            let mut inner = Instance::new(&a.bg_inner.model);
            gmr::set_model_mtx(&mut inner, &sc.world_mtx(), [1.0; 3]);
            gmr::set_view_mtx(&mut inner, &view.view);
            gmr::calc_joints(&a.bg_inner.model, &mut inner);
            inner.alpha = a_in;
            gmr::draw(gx, &a.bg_inner, &mut inner);
            // The wordmark stays where the intro camera had it.
            let text_view = View::intro(bs2main::disp_settings().cam_y_far, true);
            gx.set_projection(&text_view.projection());
            self.draw_text(gx, a, &text_view.view, a_in);
        }
        gx.set_projection(&view.projection());
        // The glass cube (Scene_SpinOut): faces the camera once the spin
        // settles, already wobbling like the menu cube it becomes.
        let m = self.spin_out_mtx();
        let base = 1.1 * (1.0 - k);
        let sxz = base + sc.squash_xz * k;
        let sy = base + (1.0 + sc.stretch_y) * k;
        let mut cube = Instance::new(&a.big_cube.model);
        gmr::set_model_mtx(&mut cube, &m, [sxz, sy, sxz]);
        gmr::set_view_mtx(&mut cube, &view.view);
        gmr::calc_joints(&a.big_cube.model, &mut cube);
        cube.alpha = if f < fade { (255 * f / fade) as u8 } else { 255 };
        if cube.alpha > 0 {
            // Scene_SetAmbDark, then Scene_SetAmbFull, which stays set.
            gx.set_chan_amb_color(GX_COLOR0A0, [0x28, 0x28, 0x28, 0xFF]);
            gmr::draw(gx, &a.big_cube, &mut cube);
            gx.set_chan_amb_color(GX_COLOR0A0, [0xFF; 4]);
        }
    }

    /// The camera of the spin-out and the menu (perspective, rising to y -10).
    pub fn spin_out_view(&self, vp: [f32; 2]) -> View {
        let state = camera::CamState::SpinOut { frame: self.spin_frame.min(SPINOUT_END), fade: SPINOUT_FADE };
        let mut view = View::intro(camera::cam_y(bs2main::disp_settings(), state), false);
        view.vp = vp;
        view
    }

    /// Scene_SpinOut's cube matrix without the model scale: `RotY(spin) ·
    /// Scene_CalcSpinOutMtx`. The latter blends the scene's spin tilt (and
    /// its wobble) with the menu's idle wobble (MenuWork defaults) by
    /// `f = (1 - spinTilt) · spinSpeed / threshold`, so as the spin brakes
    /// the cube settles into exactly the matrix the menu starts from.
    pub fn spin_out_mtx(&self) -> Mtx {
        let sc = &self.scene;
        let k = sc.spin_speed / sc.spin_threshold;
        let f = (1.0 - sc.spin_tilt_k()) * k;
        let amp = sc.wobble_amp as f32;
        let ph = sc.wobble_phase as i32;
        // Drawn from the matrix computed before this frame's +7.
        let w = menu::Wobble::at(self.wobble_frame.wrapping_sub(7));
        let blend = |tilt: f32, speed: i32, menu: f32| mtx::to_s16(f * (tilt + amp * sin_s(ph * speed)) + (1.0 - f) * menu) as i32;
        let rot = mtx::rot_trans(
            blend(8191.0, 90, w.rx),
            blend(-6371.0, 60, w.ry),
            blend(-5461.0, 70, w.rz),
            [w.tx, w.ty, 0.0],
        );
        psmtx::concat(&mtx::rot_trans(0, sc.spin as i32, 0, [0.0; 3]), &rot)
    }

    /// Cube_DrawTiles: one textured quad per live cell on the three faces.
    pub fn finished(&self) -> bool {
        self.state == State0::Done
    }

    #[allow(dead_code)]
    pub fn phase(&self) -> Phase {
        self.cube.phase
    }
}

/// `sBgOuter`'s packets: the inside faces (`packets + 0x30`, drawn behind
/// the cube) and the outside faces (`packets + 0x78`, over the logo).
const BG_OUTER_BACK: u16 = 0x30 / 0x18;
const BG_OUTER_FRONT: u16 = 0x78 / 0x18;
