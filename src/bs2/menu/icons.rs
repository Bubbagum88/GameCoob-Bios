//! The icon cubes inside the glass cube (menu_icon_*.c): each submenu has a
//! small sculpture of cubes (model res 178) that builds itself while the
//! big cube shows that face, collapses when the submenu opens and fades
//! when the cube turns away. The mosaic of a submenu waits for its icon to
//! collapse (`*Icon_IsSettled`).
//!
//! Every icon has the same three updates, picked by Menu_UpdateIcons:
//! `face` while the cube shows its face, `enter` inside the submenu and
//! `hide` anywhere else.

use crate::bs2::gbase;
use crate::bs2::mtx::{self, cos_s, sin_s};
use crate::sdk::mtx::{self as psmtx, Mtx};
use super::super::raster::Prim;
use super::super::render::{self, DrawOpts, Prepared, View};
use super::super::shader::Shader;
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

/// The cube model and one copy of its shader per tint. The model's TEV
/// stage is `2 * (0.5 * tex + C0)`: TEVREG0 is a signed tint added to the
/// grey shading.
pub struct IconData {
    model: Prepared,
    shaders: Vec<Shader>,
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
const TINTS: [[i16; 3]; 5] = [[0, -90, 20], [-45, -40, 50], [-30, 20, -50], [40, 20, -50], [80, 0, -60]];

impl IconData {
    pub fn load(ipl: &Ipl) -> Option<IconData> {
        let model = Prepared::new(Model::parse(&ipl.resource(RES_CUBE).ok()?).ok()?);
        let base = model.shader(0)?.clone();
        let shaders = TINTS
            .iter()
            .map(|t| {
                let mut s = base.clone();
                s.set_reg(0, [t[0] as f32 / 255.0, t[1] as f32 / 255.0, t[2] as f32 / 255.0, 1.0]);
                s
            })
            .collect();
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
        Some(IconData { model, shaders, picture })
    }

    /// Queue one cube: `m` is its model-to-world matrix (scale included).
    fn cube<'a>(&'a self, list: &mut Vec<Prim<'a>>, tint: usize, m: &Mtx, view: &View, alpha: i16) {
        if alpha <= 0 {
            return;
        }
        let o = DrawOpts { alpha: alpha.min(255) as u8, ..Default::default() };
        render::draw_model_with(list, &self.model, self.shaders.get(tint), m, view, &o);
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
    pub fn new() -> GameIcon {
        let cubes = core::array::from_fn(|i| RingCube {
            start: i as i16 * 10,
            end: i as i16 * 10 + 20,
            angle: (i as i32) << 12,
            ..Default::default()
        });
        GameIcon { cubes, frame: 0, rx: 0, rz: 0, spin: 0, drx: 100, drz: 50 }
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

    /// GameIcon_CalcMtx + GameIcon_Draw: `cube · RotX(-90°) · R(rx, 0, rz)
    /// · T(x, y, 0) · S(0.65)`, in the plane of the top face.
    pub fn draw<'a>(&self, list: &mut Vec<Prim<'a>>, d: &'a IconData, view: &View, cube: &Mtx) {
        let base = psmtx::concat(cube, &mtx::rot_trans(-0x4000, 0, 0, [0.0; 3]));
        let base = psmtx::concat(&base, &mtx::rot_trans(self.rx as i32, 0, self.rz as i32, [0.0; 3]));
        for c in &self.cubes {
            let m = psmtx::concat(&base, &psmtx::concat(&psmtx::trans(c.x, c.y, 0.0), &psmtx::scale(GAME_SCALE, GAME_SCALE, GAME_SCALE)));
            d.cube(list, TINT_GAME, &m, view, c.alpha);
        }
    }
}

// ---------------------------------------------------------------------
// Memory Card: a 6x6 card of cubes with a 4x3 label (menu_icon_card.c)
// ---------------------------------------------------------------------

/// `CardParams`: cube scale, collapse speed, fade speed.
const CARD_SCALE: f32 = 0.7;
const CARD_SHRINK: f32 = 5.0;
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
const CARD_LABEL: [&[usize]; 6] = [&[7], &[8, 13], &[9, 14, 19], &[10, 15, 20], &[16, 21], &[22]];
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
    cubes: [CardCube; 36],
    frame: i32,
    yaw: i16,
    rand: u32,
}

impl CardIcon {
    /// CardIcon_Init + CardIcon_InitGrid.
    pub fn new() -> CardIcon {
        let cubes = core::array::from_fn(|i| CardCube {
            target: [(i % 6) as f32 * 20.0 - 50.0, 50.0 - (i / 6) as f32 * 20.0],
            ..Default::default()
        });
        let mut c = CardIcon { cubes, frame: 0, yaw: 0, rand: 1 };
        c.roll();
        c
    }

    /// Each cube flies out over 20 frames from a random start in 0..31.
    fn roll(&mut self) {
        for i in 0..36 {
            self.rand = self.rand.wrapping_mul(1103515245).wrapping_add(12345);
            let start = (self.rand >> 16) as i32 & 31;
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
        if f >= CARD_LABEL_FROM {
            let mut prev_ready = true;
            for group in CARD_LABEL {
                if prev_ready {
                    for &i in group {
                        self.cubes[i].alpha = (self.cubes[i].alpha + 30).clamp(0, 255);
                    }
                }
                prev_ready = prev_ready && group.iter().all(|&i| self.cubes[i].alpha > 100);
            }
        } else {
            for (i, c) in self.cubes.iter_mut().enumerate() {
                if card_inner(i) {
                    c.alpha = (c.alpha - 15).max(0);
                }
            }
        }
        if (CARD_LOOP.0..=CARD_LOOP.1).contains(&f) {
            let u = f - CARD_LOOP.0;
            self.yaw = match CARD_YAW.windows(2).find(|w| u < w[1].0) {
                Some(w) => mtx::to_s16(gbase::hermite(u as f32, w[0].0 as f32, w[0].1 as f32, 0.0, w[1].0 as f32, w[1].1 as f32, 0.0)),
                None => 0,
            };
        }
        self.frame = if f + 1 > CARD_LOOP.1 { CARD_LOOP.0 } else { f + 1 };
    }

    /// CardIcon_UpdateEnter: everything shrinks to the centre and fades.
    fn update_enter(&mut self) {
        for c in &mut self.cubes {
            c.alpha = (c.alpha - CARD_FADE).max(0);
            for p in &mut c.pos {
                *p = toward_zero(*p, CARD_SHRINK);
            }
        }
        self.frame = 0;
        self.yaw = 0;
    }

    /// CardIcon_UpdateHide: fade out where the cubes are.
    fn update_hide(&mut self) {
        for c in &mut self.cubes {
            c.alpha = (c.alpha - CARD_HIDE_FADE).max(0);
        }
        self.frame = 0;
        self.yaw = 0;
    }

    /// CardIcon_IsSettled: every cube invisible and at the centre.
    pub fn settled(&self) -> bool {
        self.cubes.iter().all(|c| c.alpha == 0 && c.pos == [0.0; 3])
    }

    /// CardIcon_CalcMtx + CardIcon_Draw: `cube · RotX(90°) · RotY(yaw) ·
    /// T(pos) · R(flip, flip, 0) · S`, on the bottom face. Label cubes are
    /// half size.
    pub fn draw<'a>(&self, list: &mut Vec<Prim<'a>>, d: &'a IconData, view: &View, cube: &Mtx) {
        let base = psmtx::concat(cube, &mtx::rot_trans(0x4000, 0, 0, [0.0; 3]));
        let base = psmtx::concat(&base, &mtx::rot_trans(0, self.yaw as i32, 0, [0.0; 3]));
        for (i, c) in self.cubes.iter().enumerate() {
            let k = if card_inner(i) { CARD_SCALE * 0.5 } else { CARD_SCALE };
            let a = c.flip as i32;
            let m = psmtx::concat(&base, &psmtx::concat(&mtx::rot_trans(a, a, 0, c.pos), &psmtx::scale(k, k, k)));
            d.cube(list, TINT_CARD, &m, view, c.alpha);
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
    pos: [[f32; 2]; 13],
    alpha: [i16; 13],
    frame: i16,
    settled: bool,
}

impl ClockIcon {
    pub fn new() -> ClockIcon {
        ClockIcon { pos: [[0.0; 2]; 13], alpha: [0; 13], frame: 0, settled: false }
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

    /// ClockIcon_CalcMtx + ClockIcon_Draw: `cube · RotY(-90°) · T(x, y, 0)
    /// · S`, on the right face. The dial and the seconds cube scale in with
    /// the frame count; the dial has its own tint.
    pub fn draw<'a>(&self, list: &mut Vec<Prim<'a>>, d: &'a IconData, view: &View, cube: &Mtx) {
        let base = psmtx::concat(cube, &mtx::rot_trans(0, -0x4000, 0, [0.0; 3]));
        let f = self.frame as f32;
        for i in 0..13 {
            let (k, tint) = match i {
                0..=4 => (CLOCK_DIAL_SCALE * f.min(CLOCK_DIAL_IN as f32) / CLOCK_DIAL_IN as f32, TINT_CLOCK_DIAL),
                CLOCK_SEC => (CLOCK_HAND_SCALE * (f - CLOCK_HANDS_FROM as f32).max(0.0) / 10.0, TINT_CLOCK),
                _ => (CLOCK_HAND_SCALE, TINT_CLOCK),
            };
            if k <= 0.0 {
                continue;
            }
            let m = psmtx::concat(&base, &psmtx::concat(&psmtx::trans(self.pos[i][0], self.pos[i][1], 0.0), &psmtx::scale(k, k, k)));
            d.cube(list, tint, &m, view, self.alpha[i]);
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
    f: i16,
    settled: bool,
}

fn option_slot(k: usize) -> [f32; 2] {
    [(k % 2) as f32 * 160.0 - 80.0, 70.0 - (k / 2) as f32 * 140.0]
}

impl OptionIcon {
    pub fn new() -> OptionIcon {
        OptionIcon { frame_cubes: [OptionCube::default(); 30], picture: Vec::new(), f: 0, settled: false }
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
                    c.alpha += 6;
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
                    c.alpha += 6;
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

    /// OptionIcon_CalcMtx + OptionIcon_Draw: frame cubes `face · T(cur) ·
    /// R(rot) · S`, picture cubes `face · T(cur) · S`, on the left face.
    pub fn draw<'a>(&self, list: &mut Vec<Prim<'a>>, d: &'a IconData, view: &View, cube: &Mtx) {
        let base = psmtx::concat(cube, &mtx::rot_trans(0, -0x4000, 0, [0.0; 3]));
        let k = psmtx::scale(OPTION_SCALE, OPTION_SCALE, OPTION_SCALE);
        for c in self.frame_cubes.iter().chain(&self.picture) {
            let r = mtx::rot_trans(c.rot[0] as i32, c.rot[1] as i32, 0, [c.cur[0], c.cur[1], 0.0]);
            let m = psmtx::concat(&base, &psmtx::concat(&r, &k));
            d.cube(list, TINT_OPTION, &m, view, c.alpha);
        }
    }
}
