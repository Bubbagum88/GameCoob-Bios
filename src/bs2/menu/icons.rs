//! The icon cubes inside the glass cube (menu_icon_*.c): each submenu has a
//! small sculpture of cubes (model res 178) that builds itself while the
//! big cube shows that face, collapses when the submenu opens and fades
//! when the cube turns away. The mosaic of a submenu waits for its icon to
//! collapse (`*Icon_IsSettled`).
//!
//! Every icon has the same three updates, picked by Menu_UpdateIcons:
//! `face` while the cube shows its face, `enter` inside the submenu and
//! `hide` anywhere else.

use crate::bs2::{gbase, rnd};
use crate::bs2::mtx::{self, cos_s, sin_s};
use crate::sdk::mtx::{self as psmtx, Mtx};
use super::super::gmr::{self, Instance};
use super::super::render::Prepared;
use crate::sdk::gx::Gx;
use crate::ipl::j3d::Model;
use crate::ipl::Ipl;
use alloc::vec::Vec;

/// Small cube model.
const RES_CUBE: u16 = 178;

/// Which update an icon gets this frame.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Phase {
    Face,
    Enter,
    Hide,
}

/// The cube model. Its TEV stage is `2 * (0.5 * tex + C0)`: TEVREG0 is a
/// signed tint added to the grey shading.
pub struct IconData {
    model: Prepared,
    /// Options picture: one position per set cell of its bitmap (empty if
    /// the IPL is not PAL 1.0).
    picture: Vec<[f32; 2]>,
}

/// Tint ids in [`IconData`].
pub const TINT_GAME: usize = 0;
pub const TINT_CARD: usize = 1;
pub const TINT_OPTION: usize = 2;
pub const TINT_CLOCK: usize = 3;
pub const TINT_CLOCK_DIAL: usize = 4;

/// `Game_GetParams`, `Card_GetParams`, `Option_GetParams`,
/// `Clock_GetParams` tints, in the order of the `TINT_*` ids.
const TINTS: [[i16; 4]; 5] = [[0, -90, 20, 255], [-45, -40, 50, 255], [-30, 20, -50, 255], [40, 20, -50, 255], [80, 0, -60, 255]];

impl IconData {
    pub fn load(ipl: &Ipl) -> Option<IconData> {
        let model = Prepared::new(Model::parse(&ipl.resource(RES_CUBE).ok()?).ok()?);
        let cells = Ipl::ram_to_rom(OPTION_PICTURE_RAM)
            .and_then(|o| ipl.rom().get(o..o + 72))
            .filter(|b| b.iter().all(|&c| c <= 1))
            .unwrap_or(&[]);
        let picture = cells
            .iter()
            .enumerate()
            .filter(|&(_, &c)| c != 0)
            .map(|(k, _)| [(k % 9) as f32 * 20.0 - 80.0, 70.0 - (k / 9) as f32 * 20.0])
            .collect();
        Some(IconData { model, picture })
    }

    /// `J3D_InitModel` (or `J3D_CopyInstance`) of one cube.
    fn instance(&self) -> Instance {
        Instance::new(&self.model.model)
    }

    /// One cube's part of `*Icon_CalcMtx`: `J3D_SetModelMtx(m, scale)`,
    /// `J3D_SetViewMtx(Camera_GetView())`, `J3D_CalcJoints`.
    fn calc(&self, inst: &mut Instance, m: &Mtx, scale: f32, view: &Mtx) {
        gmr::set_model_mtx(inst, m, [scale; 3]);
        gmr::set_view_mtx(inst, view);
        gmr::calc_joints(&self.model.model, inst);
    }

    /// `J3D_Draw` of one cube with a tint (`model->material->tevColor`
    /// swapped for the draw). The Options and Calendar icons draw their
    /// cubes at alpha 0 too; that draws nothing (alpha below 255 also
    /// turns z off), so every icon skips them.
    fn draw<'a>(&'a self, gx: &mut Gx<'a>, inst: &Instance, tint: usize, alpha: i16) {
        if alpha <= 0 {
            return;
        }
        let mut inst = inst.clone();
        inst.alpha = alpha.min(255) as u8;
        inst.tev_color.extend(TINTS.get(tint).map(|&c| (0, 0, c)));
        gmr::draw(gx, &self.model, &mut inst);
    }
}

fn toward_zero(v: f32, step: f32) -> f32 {
    if v > 0.0 {
        (v - step).max(0.0)
    } else {
        (v + step).min(0.0)
    }
}

// ---------------------------------------------------------------------
// Game Play: a ring of 16 cubes (menu_icon_game.c)
// ---------------------------------------------------------------------

/// `GameParams`: ring radius, cube scale, collapse speed, fade speed.
const GAME_RADIUS: i32 = 75;
const GAME_SCALE: f32 = 0.65;
const GAME_SHRINK: f32 = 5.0;
const GAME_FADE: i16 = 10;
/// Frames until the ring is complete and starts to tumble.
const GAME_BUILD: i16 = 300;

#[derive(Clone, Copy, Default)]
struct RingCube {
    /// Frames between which this cube grows out to the ring.
    start: i16,
    end: i16,
    angle: i32,
    r: i32,
    x: f32,
    y: f32,
    alpha: i16,
}

pub struct GameIcon {
    /// `sGameIconInst`.
    inst: Vec<Instance>,
    cubes: [RingCube; 16],
    frame: i16,
    rx: i16,
    rz: i16,
    spin: i16,
    drx: i16,
    drz: i16,
}

impl GameIcon {
    /// GameIcon_Init + GameIcon_InitRing.
    pub fn new(d: &IconData) -> GameIcon {
        let cubes = core::array::from_fn(|i| RingCube {
            start: i as i16 * 10,
            end: i as i16 * 10 + 20,
            angle: (i as i32) << 12,
            ..Default::default()
        });
        GameIcon { inst: (0..16).map(|_| d.instance()).collect(), cubes, frame: 0, rx: 0, rz: 0, spin: 0, drx: 100, drz: 50 }
    }

    pub fn update(&mut self, phase: Phase) {
        match phase {
            Phase::Face => self.update_face(),
            Phase::Enter => self.update_enter(),
            Phase::Hide => self.update_hide(),
        }
    }

    /// GameIcon_UpdateFace: one cube after another grows out to the ring
    /// while the ring spins; once complete it also tumbles.
    fn update_face(&mut self) {
        let f = self.frame;
        for c in &mut self.cubes {
            if f > c.start && f < c.end {
                let span = (c.end - c.start) as i32;
                c.r = GAME_RADIUS * (f - c.start) as i32 / span;
                c.alpha = ((f - c.start) as i32 * 255 / span) as i16;
            } else if f <= c.start {
                c.r = 0;
                c.alpha = 0;
            } else {
                c.r = GAME_RADIUS;
                c.alpha = 255;
            }
            let a = c.angle - self.spin as i32;
            c.x = c.r as f32 * sin_s(a);
            c.y = c.r as f32 * cos_s(a);
        }
        self.frame += 1;
        self.spin = self.spin.wrapping_add(0x199);
        if self.frame > GAME_BUILD {
            self.frame = GAME_BUILD;
            self.rx = self.rx.wrapping_add(self.drx);
            self.rz = self.rz.wrapping_add(self.drz);
            if self.rx > 0x7D00 {
                self.drx = -100;
            }
            if self.rx < 1000 {
                self.drx = 100;
            }
            if self.rz > 0x2000 {
                self.drz = -50;
            }
            if self.rz < -0x2000 {
                self.drz = 50;
            }
            if self.spin > 0x1000 {
                self.spin %= 0x1000;
            }
        }
    }

    /// GameIcon_UpdateEnter: the ring collapses to its centre and fades.
    fn update_enter(&mut self) {
        let mut done = 0;
        for c in &mut self.cubes {
            c.alpha = (c.alpha - GAME_FADE).max(0);
            c.x = toward_zero(c.x, GAME_SHRINK);
            c.y = toward_zero(c.y, GAME_SHRINK);
            done += (c.x == 0.0) as u32 + (c.y == 0.0) as u32;
        }
        if done == 32 {
            self.rx = 0;
            self.rz = 0;
            self.spin = 0;
        }
        self.frame = 0;
    }

    /// GameIcon_UpdateHide: fade out where the cubes are.
    fn update_hide(&mut self) {
        let mut hidden = 0;
        for c in &mut self.cubes {
            c.alpha -= GAME_FADE;
            if c.alpha < 0 {
                c.alpha = 0;
                hidden += 1;
            }
        }
        self.frame = 0;
        if hidden == 16 {
            self.rx = 0;
            self.rz = 0;
            self.spin = 0;
        }
    }

    /// GameIcon_IsSettled: every cube has reached the centre.
    pub fn settled(&self) -> bool {
        self.cubes.iter().all(|c| c.x == 0.0 && c.y == 0.0)
    }

    /// GameIcon_CalcMtx (0x813235D4): `cube · RotX(-90°) · R(rx, 0, rz) ·
    /// T(x, y, 0)`, scaled 0.65, in the plane of the top face.
    pub fn calc_mtx(&mut self, d: &IconData, cube: &Mtx, view: &Mtx) {
        for (c, inst) in self.cubes.iter().zip(&mut self.inst) {
            let mut m = psmtx::trans(c.x, c.y, 0.0);
            let rot = mtx::rot_trans(self.rx as i32, 0, self.rz as i32, [0.0; 3]);
            m = psmtx::concat(&rot, &m);
            let rot = mtx::rot_trans(-0x4000, 0, 0, [0.0; 3]);
            m = psmtx::concat(&rot, &m);
            m = psmtx::concat(cube, &m);
            d.calc(inst, &m, GAME_SCALE, view);
        }
    }

    /// GameIcon_Draw (0x8132373C): once the ring is complete it is drawn
    /// back to front.
    pub fn draw<'a>(&self, gx: &mut Gx<'a>, d: &'a IconData) {
        let one = |gx: &mut Gx<'a>, i: usize| d.draw(gx, &self.inst[i], TINT_GAME, self.cubes[i].alpha);
        if self.frame < GAME_BUILD {
            for i in 0..16 {
                one(gx, i);
            }
        } else {
            for i in 0..8 {
                one(gx, 7 - i);
                one(gx, i + 8);
            }
        }
    }
}

// ---------------------------------------------------------------------
// Memory Card: a 6x6 card of cubes with a 4x3 label (menu_icon_card.c)
// ---------------------------------------------------------------------

/// `CardParams`: cube scale, collapse speed, fade speed.
const CARD_SCALE: f32 = 0.7;
const CARD_SHRINK: i16 = 5;
const CARD_FADE: i16 = 13;
/// Fade of every cube while another face shows.
const CARD_HIDE_FADE: i16 = 10;
/// Last frame of the fly-out.
const CARD_ASSEMBLE: i32 = 113;
/// The face animation loops over these frames once assembled.
const CARD_LOOP: (i32, i32) = (210, 490);
/// Flip waves: first frame, last frame, local time at the first frame.
const CARD_WAVES: [(i32, i32, i32); 3] = [(150, 195, 0), (300, 345, 0), (440, 485, 10)];
/// The label fades in from this frame, in diagonal steps.
const CARD_LABEL_FROM: i32 = 340;
/// Group yaw over the loop: frame (from 210) and angle at each segment end.
const CARD_YAW: [(i32, i32); 6] = [(0, 0), (50, -8000), (90, 32768), (140, 32768), (190, 40768), (230, 0)];

/// The 12 label cubes: rows 1..3, columns 1..4.
fn card_inner(i: usize) -> bool {
    (1..=3).contains(&(i / 6)) && (1..=4).contains(&(i % 6))
}

#[derive(Clone, Copy, Default)]
struct CardCube {
    pos: [f32; 3],
    target: [f32; 2],
    start: i32,
    end: i32,
    flip: i16,
    alpha: i16,
}

pub struct CardIcon {
    /// `sCardIconInst`.
    inst: Vec<Instance>,
    cubes: [CardCube; 36],
    frame: i32,
    yaw: i16,
    settled: bool,
}

impl CardIcon {
    /// CardIcon_Init + CardIcon_InitGrid.
    pub fn new(d: &IconData) -> CardIcon {
        let cubes = core::array::from_fn(|i| CardCube {
            target: [(i % 6) as f32 * 20.0 - 50.0, 50.0 - (i / 6) as f32 * 20.0],
            ..Default::default()
        });
        let mut c = CardIcon { inst: (0..36).map(|_| d.instance()).collect(), cubes, frame: 0, yaw: 0, settled: false };
        c.roll();
        c
    }

    /// CardIcon_RandomStarts: each cube flies out over 20 frames from a
    /// random start in 0..31.
    fn roll(&mut self) {
        for i in 0..36 {
            let start = (rnd::rand() & 0x1F) as i16 as i32;
            self.cubes[i].start = start;
            self.cubes[i].end = start + 20;
        }
    }

    pub fn update(&mut self, phase: Phase) {
        match phase {
            Phase::Face => self.update_face(),
            Phase::Enter => self.update_enter(),
            Phase::Hide => self.update_hide(),
        }
    }

    /// CardIcon_UpdateFace: the cubes fly out to the card, flip in
    /// diagonal waves, the label fades in and the card swings round.
    fn update_face(&mut self) {
        let f = self.frame;
        if f == 0 {
            self.roll();
        }
        if f <= CARD_ASSEMBLE {
            for (i, c) in self.cubes.iter_mut().enumerate() {
                let (t0, t1) = (c.start as f32, c.end as f32);
                for k in 0..2 {
                    c.pos[k] = if f <= c.start {
                        0.0
                    } else if f >= c.end {
                        c.target[k]
                    } else {
                        gbase::hermite(f as f32, t0, 0.0, c.target[k] / (t1 - t0), t1, c.target[k], 0.0)
                    };
                }
                c.pos[2] = 0.0;
                if !card_inner(i) {
                    c.alpha = if f <= c.start {
                        0
                    } else if f >= c.end {
                        255
                    } else {
                        ((f - c.start) * 255 / (c.end - c.start)) as i16
                    };
                }
            }
        }
        let wave = CARD_WAVES.iter().find(|w| (w.0..=w.1).contains(&f));
        for (i, c) in self.cubes.iter_mut().enumerate() {
            c.flip = match wave {
                Some(&(from, _, t0)) if !card_inner(i) => {
                    let t = f - from + t0;
                    let d = ((i / 6 + i % 6) * 3) as i32;
                    if t < d {
                        0
                    } else if t <= d + 15 {
                        ((t - d) * 0x4000 / 15) as i16
                    } else {
                        0x4000
                    }
                }
                _ => 0,
            };
        }
        self.fade_inner(f >= CARD_LABEL_FROM);
        if (CARD_LOOP.0..=CARD_LOOP.1).contains(&f) {
            let u = f - CARD_LOOP.0;
            self.yaw = match CARD_YAW.windows(2).find(|w| u < w[1].0) {
                Some(w) => mtx::to_s16(gbase::hermite(u as f32, w[0].0 as f32, w[0].1 as f32, 0.0, w[1].0 as f32, w[1].1 as f32, 0.0)),
                None => 0,
            };
        }
        self.frame = if f + 1 > CARD_LOOP.1 { CARD_LOOP.0 } else { f + 1 };
    }

    /// CardIcon_AddAlpha.
    fn add_alpha(&mut self, i: usize, step: i16) {
        let a = &mut self.cubes[i].alpha;
        *a = (*a + step).clamp(0, 255);
    }

    /// CardIcon_FadeInner (0x81321F60): the label fades in one diagonal
    /// after the other, and back out all at once.
    fn fade_inner(&mut self, fade_in: bool) {
        let a = |c: &Self, i: usize| c.cubes[i].alpha;
        if fade_in {
            self.add_alpha(7, 30);
            if a(self, 7) > 100 {
                self.add_alpha(8, 30);
                self.add_alpha(13, 30);
            }
            if a(self, 8) > 100 {
                self.add_alpha(9, 30);
                self.add_alpha(14, 30);
                self.add_alpha(19, 30);
            }
            if a(self, 9) > 100 {
                self.add_alpha(10, 30);
                self.add_alpha(15, 30);
                self.add_alpha(20, 30);
            }
            if a(self, 10) > 100 {
                self.add_alpha(16, 30);
                self.add_alpha(21, 30);
            }
            if a(self, 16) > 100 {
                self.add_alpha(22, 30);
            }
        } else {
            for i in [7, 8, 13, 9, 14, 19, 10, 15, 20, 16, 21, 22] {
                self.add_alpha(i, -15);
            }
        }
    }

    /// CardIcon_UpdateEnter (0x81322EE8): everything shrinks to the centre
    /// and fades. The positions go through an `s16` on the way, so they
    /// lose their fractions.
    fn update_enter(&mut self) {
        let mut settled = 0;
        for c in &mut self.cubes {
            c.alpha -= CARD_FADE;
            if c.alpha < 0 {
                c.alpha = 0;
            }
            for p in &mut c.pos {
                let mut v = *p as i16;
                if v > 0 {
                    v -= CARD_SHRINK;
                    if v < 0 {
                        v = 0;
                    }
                } else if v < 0 {
                    v += CARD_SHRINK;
                    if v > 0 {
                        v = 0;
                    }
                }
                *p = v as f32;
                if v == 0 && c.alpha == 0 {
                    settled += 1;
                }
            }
        }
        self.settled = settled == 36 * 3;
        self.frame = 0;
        self.yaw = 0;
    }

    /// CardIcon_UpdateHide: fade out where the cubes are.
    fn update_hide(&mut self) {
        for c in &mut self.cubes {
            c.alpha -= CARD_HIDE_FADE;
            if c.alpha < 0 {
                c.alpha = 0;
            }
        }
        self.frame = 0;
        self.yaw = 0;
    }

    /// CardIcon_IsSettled: whether the last UpdateEnter left every cube
    /// invisible and at the centre.
    pub fn settled(&self) -> bool {
        self.settled
    }

    /// CardIcon_CalcMtx (0x81322A18): `cube · RotX(90°) · RotY(yaw) ·
    /// T(pos) · R(flip, flip, 0)`, on the bottom face. Label cubes are half
    /// size.
    pub fn calc_mtx(&mut self, d: &IconData, cube: &Mtx, view: &Mtx) {
        let tilt = mtx::rot_trans(0x4000, 0, 0, [0.0; 3]);
        for (i, (c, inst)) in self.cubes.iter().zip(&mut self.inst).enumerate() {
            let a = c.flip as i32;
            let mut m = mtx::rot_trans(a, a, 0, c.pos);
            let rot = mtx::rot(0, self.yaw as i32, 0);
            m = psmtx::concat(&rot, &m);
            m = psmtx::concat(&tilt, &m);
            m = psmtx::concat(cube, &m);
            let k = if card_inner(i) { CARD_SCALE / 2.0 } else { CARD_SCALE };
            d.calc(inst, &m, k, view);
        }
    }

    /// CardIcon_Draw (0x81322C8C).
    pub fn draw<'a>(&self, gx: &mut Gx<'a>, d: &'a IconData) {
        for (c, inst) in self.cubes.iter().zip(&self.inst) {
            d.draw(gx, inst, TINT_CARD, c.alpha);
        }
    }
}

// ---------------------------------------------------------------------
// Calendar: a clock face showing the time (menu_icon_clock.c)
// ---------------------------------------------------------------------

/// `ClockParams`: collapse speed, fade speed.
const CLOCK_SHRINK: f32 = 6.0;
const CLOCK_FADE: i16 = 14;
const CLOCK_HIDE_FADE: i16 = 10;
/// Dial cube scale (it grows in) and hand cube scale.
const CLOCK_DIAL_SCALE: f32 = 0.7;
const CLOCK_HAND_SCALE: f32 = 0.55;
/// Radius of the four dial marks and of the seconds cube.
const CLOCK_RIM: f32 = 100.0;
/// The dial fades in over this many frames; the hands start after
/// `CLOCK_HANDS_FROM` and take 10 frames.
const CLOCK_DIAL_IN: i16 = 30;
const CLOCK_HANDS_FROM: i16 = 40;
const CLOCK_DONE: i16 = 50;

/// Cubes: 0..5 dial (centre and four marks), 5..9 minute hand, 9..12 hour
/// hand, 12 seconds cube.
const CLOCK_MIN: core::ops::Range<usize> = 5..9;
const CLOCK_HOUR: core::ops::Range<usize> = 9..12;
const CLOCK_SEC: usize = 12;

pub struct ClockIcon {
    /// `sClockDial`, `sClockMin`, `sClockHour`, `sClockSec`.
    inst: Vec<Instance>,
    pos: [[f32; 2]; 13],
    alpha: [i16; 13],
    frame: i16,
    settled: bool,
}

impl ClockIcon {
    pub fn new(d: &IconData) -> ClockIcon {
        ClockIcon { inst: (0..13).map(|_| d.instance()).collect(), pos: [[0.0; 2]; 13], alpha: [0; 13], frame: 0, settled: false }
    }

    /// `time` is year, month, day, hour, minute, second.
    pub fn update(&mut self, phase: Phase, time: &[i32; 6]) {
        match phase {
            Phase::Face => self.update_face(time),
            Phase::Enter => self.update_enter(),
            Phase::Hide => self.update_hide(),
        }
    }

    /// ClockIcon_UpdateFace: the dial fades in, then the hands grow out
    /// from the centre; the hands follow the clock. Angles are negative
    /// because the face is mirrored (it sits on the cube's side).
    fn update_face(&mut self, time: &[i32; 6]) {
        self.frame = (self.frame + 1).min(CLOCK_DONE);
        let f = self.frame as i32;
        let (hour, min, sec) = (time[3], time[4], time[5]);
        let min_a = (-min << 16) / 60 - (sec << 16) / 3600;
        let hour_a = (-(hour % 12) << 16) / 12 - (min << 16) / 720;
        // The original also subtracts a leftover loop counter (3).
        let sec_a = (-sec << 16) / 60 - 3;
        let dial = f.min(CLOCK_DIAL_IN as i32);
        let g = (f - CLOCK_HANDS_FROM as i32).max(0);
        for i in 0..5 {
            self.alpha[i] = (dial * 255 / CLOCK_DIAL_IN as i32) as u8 as i16;
            let (r, a) = if i == 0 { (0.0, 0) } else { (CLOCK_RIM, (i as i32 - 1) * 0x4000) };
            self.pos[i] = [r * sin_s(a), r * cos_s(a)];
        }
        let hand_alpha = (g * 255 / 10) as u8 as i16;
        for (range, a) in [(CLOCK_MIN, min_a), (CLOCK_HOUR, hour_a)] {
            for (j, i) in range.enumerate() {
                self.alpha[i] = hand_alpha;
                let r = (20 * (j as i32 + 1) * g) as f32;
                self.pos[i] = [r * sin_s(a) / 10.0, r * cos_s(a) / 10.0];
            }
        }
        self.alpha[CLOCK_SEC] = hand_alpha;
        self.pos[CLOCK_SEC] = [CLOCK_RIM * sin_s(sec_a), CLOCK_RIM * cos_s(sec_a)];
    }

    /// ClockIcon_UpdateEnter: everything shrinks to the centre and fades.
    fn update_enter(&mut self) {
        let mut done = 0;
        for (p, a) in self.pos.iter_mut().zip(&mut self.alpha) {
            *a = (*a - CLOCK_FADE).max(0);
            for v in p {
                *v = toward_zero(*v, CLOCK_SHRINK);
                done += (*v == 0.0) as u32;
            }
        }
        self.settled = done == 26;
        if self.settled {
            self.frame = 0;
        }
    }

    /// ClockIcon_UpdateHide: fade out where the cubes are.
    fn update_hide(&mut self) {
        let mut hidden = 0;
        for a in &mut self.alpha {
            *a -= CLOCK_HIDE_FADE;
            if *a < 0 {
                *a = 0;
                hidden += 1;
            }
        }
        if hidden == 13 {
            self.frame = 0;
        }
    }

    /// ClockIcon_IsSettled: every cube has reached the centre.
    pub fn settled(&self) -> bool {
        self.settled
    }

    /// ClockIcon_CalcMtx (0x81326808): `cube · RotY(-90°) · T(x, y, 0)`,
    /// on the right face. The dial and the seconds cube scale in with the
    /// frame count.
    pub fn calc_mtx(&mut self, d: &IconData, cube: &Mtx, view: &Mtx) {
        let rot = mtx::rot_trans(0, -0x4000, 0, [0.0; 3]);
        let base = psmtx::concat(cube, &rot);
        let n = self.frame;
        let hand_t = if n > CLOCK_HANDS_FROM { n - CLOCK_HANDS_FROM } else { 0 };
        for i in 0..13 {
            let t = psmtx::trans(self.pos[i][0], self.pos[i][1], 0.0);
            let m = psmtx::concat(&base, &t);
            let k = match i {
                0..=4 => CLOCK_DIAL_SCALE * n.min(CLOCK_DIAL_IN) as f32 / CLOCK_DIAL_IN as f32,
                CLOCK_SEC => CLOCK_HAND_SCALE * hand_t as f32 / 10.0,
                _ => CLOCK_HAND_SCALE,
            };
            d.calc(&mut self.inst[i], &m, k, view);
        }
    }

    /// ClockIcon_Draw (0x81326CA4): the dial has its own tint.
    pub fn draw<'a>(&self, gx: &mut Gx<'a>, d: &'a IconData) {
        for i in 0..13 {
            let tint = if i < 5 { TINT_CLOCK_DIAL } else { TINT_CLOCK };
            d.draw(gx, &self.inst[i], tint, self.alpha[i]);
        }
    }
}

// ---------------------------------------------------------------------
// Options: a square frame that turns into a picture (menu_icon_option.c)
// ---------------------------------------------------------------------

/// The picture's 9x8 bitmap (one byte per cell, top row first), PAL 1.0.
const OPTION_PICTURE_RAM: u32 = 0x8138_1450;
const OPTION_SCALE: f32 = 0.67;
const OPTION_SHRINK: f32 = 5.0;
const OPTION_FADE: i16 = 12;
const OPTION_HIDE_FADE: i16 = 10;
/// The face animation loops back to the rotation wave after this frame.
const OPTION_LOOP: (i16, i16) = (70, 599);
/// Corner cubes of the frame.
const OPTION_CORNERS: [usize; 4] = [0, 7, 22, 15];
/// Which corner cube sits in slot k (TL, TR, BL, BR) after the slide, and
/// the slot it slides from (each moves one corner clockwise).
const OPTION_SLIDE: [usize; 4] = [22, 0, 15, 7];
const OPTION_SLIDE_FROM: [usize; 4] = [2, 0, 3, 1];

#[derive(Clone, Copy, Default)]
struct OptionCube {
    base: [f32; 2],
    cur: [f32; 2],
    rot: [i16; 2],
    alpha: i16,
}

pub struct OptionIcon {
    /// 30 frame cubes, clockwise from the top right; the corners are 0, 7,
    /// 15 and 22.
    frame_cubes: [OptionCube; 30],
    /// One cube per set cell of the picture.
    picture: Vec<OptionCube>,
    /// `sOptRing` then `sOptShape`.
    inst: Vec<Instance>,
    f: i16,
    settled: bool,
}

fn option_slot(k: usize) -> [f32; 2] {
    [(k % 2) as f32 * 160.0 - 80.0, 70.0 - (k / 2) as f32 * 140.0]
}

impl OptionIcon {
    pub fn new(d: &IconData) -> OptionIcon {
        let inst = (0..30 + d.picture.len()).map(|_| d.instance()).collect();
        OptionIcon { frame_cubes: [OptionCube::default(); 30], picture: Vec::new(), inst, f: 0, settled: false }
    }

    /// OptionIcon_ResetShape: the frame edges at their places, corners in
    /// the centre, everything invisible.
    fn reset_shape(&mut self, d: &IconData) {
        if self.picture.len() != d.picture.len() {
            self.picture = d.picture.iter().map(|&p| OptionCube { base: p, cur: p, ..Default::default() }).collect();
        }
        for (i, c) in self.frame_cubes.iter_mut().enumerate() {
            let i = i as f32;
            c.base = match i as usize {
                1..=6 => [80.0, 70.0 - 20.0 * i],
                8..=14 => [(15.0 - i) * 20.0 - 80.0, -70.0],
                16..=21 => [-80.0, 70.0 - (22.0 - i) * 20.0],
                23..=29 => [(i - 22.0) * 20.0 - 80.0, 70.0],
                _ => [0.0; 2],
            };
            c.rot = [0; 2];
            c.alpha = 0;
        }
        for c in &mut self.picture {
            c.rot = [0; 2];
            c.alpha = 0;
        }
    }

    fn snap(&mut self) {
        for c in self.frame_cubes.iter_mut().chain(&mut self.picture) {
            c.cur = c.base;
        }
    }

    fn all(&mut self) -> impl Iterator<Item = &mut OptionCube> {
        self.frame_cubes.iter_mut().chain(self.picture.iter_mut())
    }

    pub fn update(&mut self, phase: Phase, d: &IconData) {
        match phase {
            Phase::Face => self.update_face(d),
            Phase::Enter => self.update_enter(),
            Phase::Hide => self.update_hide(),
        }
    }

    /// OptionIcon_UpdateFace: the corners fly out and slide round drawing
    /// the frame; the frame cubes turn in a wave, the frame squeezes into
    /// the picture, the picture ripples twice and squeezes back into the
    /// frame. Loops from the wave.
    fn update_face(&mut self, d: &IconData) {
        let f = self.f;
        if f == 0 {
            self.reset_shape(d);
        }
        let h = |t: i16, p0: f32, p1: f32, d: f32| gbase::hermite(t as f32, 0.0, p0, 0.0, d, p1, 0.0);
        match f {
            0..=19 => {
                for i in OPTION_CORNERS {
                    let c = &mut self.frame_cubes[i];
                    c.base = [0.0; 2];
                    c.alpha = (c.alpha + 6).clamp(0, 255);
                }
                self.snap();
            }
            20..=39 => {
                let t = (f - 20) as f32;
                for (k, &i) in OPTION_CORNERS.iter().enumerate() {
                    let to = option_slot(k);
                    let c = &mut self.frame_cubes[i];
                    for a in 0..2 {
                        c.base[a] = gbase::hermite(t, 0.0, 0.0, (to[a] as i32 / 20) as f32, 20.0, to[a], 0.0);
                    }
                    c.alpha = (c.alpha + 6).clamp(0, 255);
                }
                self.snap();
            }
            40..=69 => {
                let t = (f - 40) as f32;
                for (k, &i) in OPTION_SLIDE.iter().enumerate() {
                    let (from, to) = (option_slot(OPTION_SLIDE_FROM[k]), option_slot(k));
                    let c = &mut self.frame_cubes[i];
                    for a in 0..2 {
                        let v = ((to[a] - from[a]) as i32 / 30) as f32;
                        c.base[a] = gbase::hermite(t, 0.0, from[a], v, 30.0, to[a], 0.0);
                    }
                    c.alpha = (c.alpha + 6).min(255);
                    let [x, y] = c.base;
                    // Reveal the edge behind the moving corner.
                    let edge: &mut dyn Iterator<Item = usize> = match k {
                        0 => &mut ((((70.0 - y) / 20.0) as i32 + 1)..=7).map(|j| (22 - j) as usize),
                        1 => &mut (0..((x + 80.0) / 20.0) as i32).map(|j| (23 + j) as usize),
                        2 => &mut ((((x + 80.0) / 20.0) as i32 + 1)..=8).map(|j| (15 - j) as usize),
                        _ => &mut (0..((70.0 - y) / 20.0) as i32).map(|j| (1 + j) as usize),
                    };
                    for e in edge {
                        if let Some(c) = self.frame_cubes.get_mut(e) {
                            c.alpha = 255;
                        }
                    }
                }
                self.snap();
            }
            70..=224 => {
                let t = (f - 70) as i32;
                for (i, c) in self.frame_cubes.iter_mut().enumerate() {
                    let s = 5 * i as i32;
                    if (s..=s + 10).contains(&t) {
                        let a = ((t - s) * 0x4000 / 10) as i16;
                        c.rot = match i {
                            0..=6 => [a, 0],
                            7..=14 => [0, -a],
                            15..=21 => [-a, 0],
                            _ => [0, a],
                        };
                    }
                }
                self.snap();
            }
            225..=264 => {
                let c = mtx::to_s16(h(f - 225, 80.0, 0.0, 40.0)) as f32;
                for q in &mut self.frame_cubes {
                    q.cur[0] = q.base[0].clamp(-c, c);
                }
            }
            265..=304 => {
                let t = f - 265;
                if t == 0 {
                    self.all().for_each(|q| q.alpha ^= 0xFF);
                }
                let c = mtx::to_s16(h(t, 0.0, 80.0, 40.0)) as f32;
                for q in &mut self.picture {
                    q.cur = [q.base[0].clamp(-c, c), q.base[1]];
                }
            }
            305..=518 => {
                let t = if f < 397 { f - 305 } else { f - 397 } as i32;
                for q in &mut self.picture {
                    let r = (7.0 - (70.0 - q.base[1]) / 20.0) as i32;
                    let u = t - 4 * r;
                    let hop = if (0..64).contains(&u) {
                        let v = u % 32;
                        if v < 16 {
                            h(v as i16, 0.0, 10.0, 16.0)
                        } else {
                            h((v - 16) as i16, 10.0, 0.0, 16.0)
                        }
                    } else {
                        0.0
                    };
                    q.cur[1] = q.base[1] + hop;
                }
            }
            519..=558 => {
                let c = mtx::to_s16(h(f - 519, 70.0, 0.0, 40.0)) as f32;
                for q in &mut self.picture {
                    q.cur = [q.base[0], q.base[1].clamp(-c, c)];
                }
            }
            559..=598 => {
                let t = f - 559;
                if t == 0 {
                    self.all().for_each(|q| q.alpha ^= 0xFF);
                }
                let c = mtx::to_s16(h(t, 0.0, 80.0, 40.0)) as f32;
                for q in &mut self.frame_cubes {
                    q.cur = [q.base[0], q.base[1].clamp(-c, c)];
                }
            }
            _ => {}
        }
        self.f = if f + 1 > OPTION_LOOP.1 { OPTION_LOOP.0 } else { f + 1 };
    }

    /// OptionIcon_UpdateEnter: everything shrinks to the centre and fades.
    fn update_enter(&mut self) {
        let mut done = 0;
        let n = 2 * (30 + self.picture.len());
        for c in self.all() {
            c.alpha = (c.alpha - OPTION_FADE).clamp(0, 255);
            for v in &mut c.cur {
                *v = toward_zero(*v, OPTION_SHRINK);
                done += (*v == 0.0) as usize;
            }
        }
        self.settled = done == n;
        self.f = 0;
    }

    /// OptionIcon_UpdateHide: fade out where the cubes are.
    fn update_hide(&mut self) {
        for c in self.all() {
            c.alpha = (c.alpha - OPTION_HIDE_FADE).max(0);
        }
        self.f = 0;
    }

    /// OptionIcon_IsSettled: every cube has reached the centre.
    pub fn settled(&self) -> bool {
        self.settled
    }

    /// OptionIcon_CalcMtx (0x81325960): frame cubes `cube · RotY(-90°) ·
    /// T(cur) · R(rot)`, picture cubes `cube · RotY(-90°) · T(cur)`, scaled
    /// 0.67, on the left face.
    pub fn calc_mtx(&mut self, d: &IconData, cube: &Mtx, view: &Mtx) {
        let rot = mtx::rot_trans(0, -0x4000, 0, [0.0; 3]);
        let base = psmtx::concat(cube, &rot);
        let (ring, shape) = self.inst.split_at_mut(30);
        for (c, inst) in self.frame_cubes.iter().zip(ring) {
            let t = mtx::rot_trans(c.rot[0] as i32, c.rot[1] as i32, 0, [c.cur[0], c.cur[1], 0.0]);
            let m = psmtx::concat(&base, &t);
            d.calc(inst, &m, OPTION_SCALE, view);
        }
        for (c, inst) in self.picture.iter().zip(shape) {
            let t = psmtx::trans(c.cur[0], c.cur[1], 0.0);
            let m = psmtx::concat(&base, &t);
            d.calc(inst, &m, OPTION_SCALE, view);
        }
    }

    /// OptionIcon_Draw (0x81325B48).
    pub fn draw<'a>(&self, gx: &mut Gx<'a>, d: &'a IconData) {
        for (c, inst) in self.frame_cubes.iter().chain(&self.picture).zip(&self.inst) {
            d.draw(gx, inst, TINT_OPTION, c.alpha);
        }
    }
}
