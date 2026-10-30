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
use crate::sdk::gx::{Gx, GxArena, GxRegs, GX_COLOR0A0};
use crate::sdk::mtx::{self as psmtx, Mtx};
use raster::Target;
use gmr::Instance;
use render::{Prepared, View};
use ginit::{Disc, DiscState};
use scene::{Scene, SceneState, SHAKE_MAX};

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

/// The scene (`scene.c`, `scene_intro.c`, `scene_spinout.c`): the scene
/// state machine and the models it draws, the rolling cube, and the bits of
/// BS2Main's frame it depends on (the camera, the projection flag).
pub struct Intro {
    pub cube: Cube,
    pub scene: Scene,
    /// `sState`: the state `Scene_Draw2D` draws.
    pub state: SceneState,
    /// `sNextState`: the state the next `Scene_Update` runs.
    next_state: SceneState,
    pub disc: Disc,
    /// What is left of [`HOLD_FRAMES`].
    hold: i32,
    pub frame: u32,
    /// `gMenuWork`, made by `Scene_Init` (Menu_Init): the spin-out runs
    /// the menu's idle-wobble clock, and the menu takes it over.
    pub menu: menu::Work,
    /// `disp->ortho` (the flag at 0x1E): set by `Scene_Init`, cleared by the
    /// spin-out and the menu.
    ortho: bool,
    /// The camera height `fn_81302DF0` picked at the start of the frame.
    cam_y: f32,
    /// `sWorldMtx`, from the last `Scene_DrawWorld`.
    world: Mtx,
    /// `sSpinOutMtx`.
    spin_out_mtx: Mtx,
    /// GX registers as the last rendered frame left them.
    gx_regs: core::cell::Cell<Option<GxRegs>>,
}

impl Intro {
    /// `Scene_Init(TRUE)` on a boot with no disc.
    pub fn new(a: &Assets) -> Intro {
        mtx::sin_table_init(12);
        let disp = bs2main::disp_settings();
        Intro {
            cube: Cube::new(a.path.clone()),
            scene: Scene::new(),
            state: SceneState::NoDisc,
            next_state: SceneState::NoDisc,
            disc: Disc::new(),
            hold: HOLD_FRAMES,
            frame: 0,
            menu: menu::Work::new(),
            ortho: true,
            cam_y: camera::cam_y(disp, camera::CamState::Intro),
            world: psmtx::IDENTITY,
            spin_out_mtx: psmtx::IDENTITY,
            gx_regs: core::cell::Cell::new(None),
        }
    }

    /// One 1/60 s logic frame: `fn_81302DF0`'s camera for the state the last
    /// frame left, then `Scene_Update`. Once the menu has taken over this
    /// only latches the state; `Menu_Update` is the menu's own.
    pub fn update(&mut self, input: Input) -> Option<Jingle> {
        self.frame += 1;
        let s = &self.scene;
        let cam = match self.state {
            SceneState::SpinOut => camera::CamState::SpinOut { frame: s.out_frame, fade: s.out_fade_frames as i32 },
            SceneState::Menu => camera::CamState::Menu,
            _ => camera::CamState::Intro,
        };
        self.cam_y = camera::cam_y(bs2main::disp_settings(), cam);

        self.state = self.next_state;
        let mut jingle = None;
        match self.state {
            SceneState::NoDisc => {
                let (next, j) = self.update_no_disc(input);
                self.next_state = next;
                jingle = j;
                self.draw_world();
            }
            SceneState::SpinOut => self.next_state = self.spin_out(),
            SceneState::Menu => self.ortho = false,
            // The disc boot, the SRAM-lost language select and the cover
            // open state need a drive.
            SceneState::Mode0 | SceneState::Disc | SceneState::Mode3 => {}
        }
        jingle
    }

    /// `Scene_UpdateNoDisc` (0x8130CB74).
    fn update_no_disc(&mut self, input: Input) -> (SceneState, Option<Jingle>) {
        let s = &mut self.scene;
        let cube = &mut self.cube;
        if !cube.done {
            let _whoosh = s.update_spin(input.a, cube.running, false);
        }
        s.update_wobble();
        let jingle = cube.update(s, Some(input.egg.unwrap_or(Jingle::Normal)));
        if s.spin_speed > s.spin_threshold as f32 {
            s.spin = s.spin_rest;
            return (SceneState::SpinOut, jingle);
        }

        if s.fatal {
            s.fatal_fade = (s.fatal_fade + 1).min(s.fatal_fade_frames);
            return (SceneState::NoDisc, jingle);
        }

        let disc = &mut self.disc;
        if matches!(disc.state(), DiscState::Wrong | DiscState::Retry) {
            s.err_fade = (s.err_fade + 1).min(s.err_fade_frames);
        } else {
            s.err_fade = (s.err_fade - 1).max(0);
        }

        if s.cover_opened {
            if cube.done {
                if self.hold > 0 {
                    self.hold -= 1;
                    return (SceneState::NoDisc, jingle);
                }
                disc.fade_out_start();
                if disc.fade_alpha() == 0 {
                    let _whoosh = s.update_spin(false, cube.running, true);
                    s.update_wobble();
                    if s.spin_speed > s.spin_threshold as f32 {
                        s.spin = s.spin_rest;
                        return (SceneState::SpinOut, jingle);
                    }
                }
            }
            return (SceneState::NoDisc, jingle);
        }

        match disc.state() {
            // fn_8130213C boots the disc once the cube is done.
            DiscState::Ready => {}
            DiscState::Fatal => s.fatal = true,
            DiscState::None | DiscState::CoverOpen => s.cover_opened = true,
            DiscState::Reading | DiscState::Wrong | DiscState::Retry => {}
        }
        (SceneState::NoDisc, jingle)
    }

    /// `Scene_DrawWorld` (0x8130C90C): the world matrix and `Cube_Draw`'s
    /// cube matrix. The models' joints follow from these and the camera,
    /// so [`Intro::draw`] works them out.
    fn draw_world(&mut self) {
        self.world = self.scene.world_mtx();
        self.cube.calc_mtx(&self.world);
    }

    /// `Scene_SpinOut` (0x8130ED30): state 4, until the spin has settled.
    fn spin_out(&mut self) -> SceneState {
        let s = &mut self.scene;
        if s.out_first {
            s.out_start = s.spin;
            s.out_first = false;
        }
        let _whoosh = s.update_spin_out();
        s.out_frame += 1;
        if s.spin_speed <= 0.0 && s.spin == 0 {
            s.out_first = true;
        }
        self.ortho = false;
        let rot = mtx::rot_trans(0, s.spin as i32, 0, [0.0; 3]);
        let wobble = s.calc_spin_out_mtx(&mut self.menu, true);
        self.spin_out_mtx = psmtx::concat(&rot, &wobble);
        if s.out_frame <= s.out_fade_frames as i32 {
            self.draw_world();
        }
        let s = &self.scene;
        if s.spin_speed <= 0.0 && s.spin == 0 {
            return SceneState::Menu;
        }
        SceneState::SpinOut
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
            self.draw(&mut gx, a, None, vp);
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
        self.draw(&mut gx, a, None, vp);
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
            Some(m) if self.state == SceneState::Menu => {
                // fn_81302FA0's light, then Scene_Draw2DMenu.
                let view = self.view(vp);
                camera::load_light(&mut gx, &view.view);
                m.draw(&mut gx, a, ma, &view)
            }
            _ => self.draw(&mut gx, a, Some(ma), vp),
        }
        self.gx_regs.set(Some(gx.regs()));
        t.clear([0.0; 3]);
        let job = raster::Job::new(t, &gx.list, 16);
        par(&job);
    }

    /// The frame's camera (`Camera_GetView`), light and projection
    /// (`fn_81301100`).
    pub fn view(&self, vp: [f32; 2]) -> View {
        let mut view = View::intro(self.cam_y, self.ortho);
        view.vp = vp;
        view
    }

    /// A model placed by `J3D_SetModelMtx` and `J3D_SetViewMtx`, joints
    /// from `J3D_CalcJoints`.
    fn place(p: &Prepared, m: &Mtx, s: f32, view: &Mtx) -> Instance {
        let mut inst = Instance::new(&p.model);
        gmr::set_model_mtx(&mut inst, m, [s; 3]);
        gmr::set_view_mtx(&mut inst, view);
        gmr::calc_joints(&p.model, &mut inst);
        inst
    }

    /// `Cube_DrawText`: the wordmark, its joint (`.bck`) and material
    /// color (`.ipk`) animations on the text clock.
    fn text(&self, a: &Assets, view: &Mtx) -> Instance {
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
        inst
    }

    /// `Scene_Draw2D` (0x8130BE78) for the intro states, after
    /// `fn_81302FA0`'s light and `fn_81301100`'s projection. `ma` draws the
    /// menu face during the spin-out.
    pub fn draw<'a>(&self, gx: &mut Gx<'a>, a: &'a Assets, ma: Option<&'a menu::MenuAssets>, vp: [f32; 2]) {
        let view = self.view(vp);
        gx.set_projection(&view.projection());
        gx.set_viewport(0.0, 0.0, vp[0], vp[1]);
        camera::load_light(gx, &view.view);
        match self.state {
            SceneState::SpinOut | SceneState::Menu => self.draw_2d_spin_out(gx, a, ma, &view),
            _ => self.draw_2d_no_disc(gx, a, &view.view),
        }
    }

    /// `Scene_Draw2DNoDisc` (0x8130D73C): the cube intro. The disc error
    /// texts (dialog strings 4 and 5) need a drive and are not drawn.
    fn draw_2d_no_disc<'a>(&self, gx: &mut Gx<'a>, a: &'a Assets, view: &Mtx) {
        let s = &self.scene;
        let fade0 = (255 - s.unk_c0[2] as i32 * 150 / 30) as u8;
        let fade1 = (255 - s.unk_c0[7] as i32 * 150 / 30) as u8;
        let fade = fade0.min(fade1) as i32;
        let shake = (s.land_impulse as i32 + s.drop_impulse as i32).min(SHAKE_MAX);

        // The "NINTENDO GAMECUBE" wordmark, fixed to the screen, drawn over
        // the state the last frame left; then fn_81304CE8.
        let mut text = self.text(a, view);
        text.alpha = (fade * 255 / 255) as u8;
        gmr::draw(gx, &a.text, &mut text);
        gmr::setup(gx);

        // The inner background cube, visible while the impacts shake it.
        let world = &self.world;
        let mut inner = Self::place(&a.bg_inner, world, s.model_scale, view);
        inner.alpha = (fade * (s.shake_alpha as i32 * shake / SHAKE_MAX) / 255) as u8;
        if inner.alpha > 0 {
            gmr::draw(gx, &a.bg_inner, &mut inner);
        }

        // The outer background's inside faces, visible while wobbling.
        let wobble = (s.wobble_amp as i32).min(SHAKE_MAX);
        let mut outer = Self::place(&a.bg_outer, world, 1.01, view);
        outer.alpha = (fade * (s.shake_alpha as i32 * wobble / SHAKE_MAX) / 255) as u8;
        if outer.alpha > 0 {
            gmr::draw_packet(gx, &a.bg_outer, &mut outer, BG_OUTER_BACK);
        }

        // Cube_DrawBig: the trail tiles and the small cube, fading out as
        // the logo fades in.
        let cube_a = ((255 - s.logo_alpha as i32) * fade / 255) as u8;
        cube_draw::draw_big_tiles(gx, &self.cube, world, view, cube_a, &a.tile);
        let mut small = Self::place(&a.small_cube, &self.cube.mtx, s.model_scale, view);
        small.alpha = (self.cube.alpha as i32 * cube_a as i32 / 255) as u8;
        gmr::draw(gx, &a.small_cube, &mut small);

        // The logo, then the outer background's outside faces.
        let mut logo = Self::place(&a.logo, world, 1.0, view);
        logo.alpha = (s.logo_alpha as i32 * fade / 255) as u8;
        if logo.alpha > 0 {
            gmr::draw(gx, &a.logo, &mut logo);
        }
        if outer.alpha > 0 {
            gmr::draw_packet(gx, &a.bg_outer, &mut outer, BG_OUTER_FRONT);
        }
    }

    /// `Scene_Draw2DSpinOut` (0x8130E3C8): the logo scene fading out, the
    /// glass menu cube in, and the menu's front face over the last frames.
    fn draw_2d_spin_out<'a>(&self, gx: &mut Gx<'a>, a: &'a Assets, ma: Option<&'a menu::MenuAssets>, view: &View) {
        let s = &self.scene;
        let (of, fade) = (s.out_frame, s.out_fade_frames as i32);
        let big_alpha;
        if of <= fade {
            let shake = (s.land_impulse as i32 + s.drop_impulse as i32 + s.wobble_amp as i32).min(SHAKE_MAX);
            let mut inner = Self::place(&a.bg_inner, &self.world, s.model_scale, &view.view);
            big_alpha = (s.bg_alpha as i32 * of / fade) as u8;
            inner.alpha = ((fade - of) * (s.shake_alpha as i32 * shake) / (fade * SHAKE_MAX)) as u8;
            let mut text = self.text(a, &view.view);
            text.alpha = (inner.alpha as i32 * s.m0_logo_fade as i32 / s.m0_logo_frames as i32) as u8;
            gmr::draw(gx, &a.bg_inner, &mut inner);
            gmr::draw(gx, &a.text, &mut text);
        } else {
            big_alpha = s.bg_alpha;
        }
        if big_alpha > 0 {
            // Scene_SetBigCubeMtx, then Scene_SetAmbDark and Scene_SetAmbFull
            // around the draw.
            let mut cube = Instance::new(&a.big_cube.model);
            gmr::set_model_mtx(&mut cube, &self.spin_out_mtx, s.big_cube_scale());
            gmr::set_view_mtx(&mut cube, &view.view);
            gmr::calc_joints(&a.big_cube.model, &mut cube);
            cube.alpha = big_alpha;
            gx.set_chan_amb_color(GX_COLOR0A0, [0x28, 0x28, 0x28, 0xFF]);
            gmr::draw(gx, &a.big_cube, &mut cube);
            gx.set_chan_amb_color(GX_COLOR0A0, [0xFF; 4]);
        }
        let end = s.out_turn_frames + s.out_settle_frames + s.out_coast_frames;
        let start = end - s.out_face_frames;
        let face = if of >= start && of <= end {
            Some(((of - start) * 255 / (end - start)) as i16 as u8)
        } else if of > end {
            Some(255)
        } else {
            None
        };
        if let (Some(alpha), Some(ma)) = (face, ma) {
            menu::Menu::draw_spin_out_face(gx, ma, view, &self.spin_out_mtx, alpha);
        }
    }

    /// `sNextState` is the menu: the spin-out has settled.
    pub fn finished(&self) -> bool {
        self.next_state == SceneState::Menu
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
