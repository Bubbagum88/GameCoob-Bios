//! The menu mosaic ("fx"): words and digits built from small cubes that fly
//! in along the IPL's own Hermite paths, used inside Game Play, Options and
//! Calendar. Rebuilt from the decompiled BS2 `menu_fx_*` code.
//!
//! Each screen owns one [`fx::Set`](crate::ipl::fx::Set) of cubes. Every
//! frame a builder says which lit-cell entries are shown and how they are
//! highlighted ([`Build`]); [`Mosaic::frame`] then
//!
//! 1. moves every cube along its path at the screen's progress (it counts
//!    up one per frame to [`READY`] frames after the screen opens),
//! 2. sways the groups the builder named, once the progress passes 0.9,
//! 3. mixes each cube's light and dark colour from the screen's three
//!    colour pairs, and
//! 4. steps every per-cube ramp one frame toward its target.
//!
//! [`Mosaic::draw`] queues the visible cubes with the mosaic cube model
//! (res 104). Its one TEV stage blends TEVREG1 (dark) to TEVREG0 (light) by
//! the shading texture; Fx_Draw points those two registers at each cube's
//! own colours.
//!
//! Per-cube state, as in the decomp (all ramps step by one per frame):
//!
//! | ramp      | frames | meaning                                           |
//! |-----------|--------|---------------------------------------------------|
//! | `vis`     | 10     | shown (alpha = 255 * progress * vis)              |
//! | `bright`  | 10     | weight of colour pair 0 (target clears each frame)|
//! | `sel`     | 10     | pair 1 instead of pair 2 under `bright`; also how |
//! |           |        | much a calendar field being edited is enlarged    |
//! | `sway`    | 10     | how much of its group's sway the cube follows     |

use super::mtx::{self, SinTable};
use super::raster::Prim;
use super::render::{self, DrawOpts, Prepared, View};
use super::shader::Shader;
use crate::ipl::fx::{Glyphs, Paths};
use crate::ipl::j3d::Model;
use crate::ipl::Ipl;
use alloc::vec::Vec;

pub use crate::ipl::fx::{glyph, set};

/// Mosaic cube model.
const RES_MODEL: u16 = 104;
/// CAFD flight paths.
const RES_PATHS: u16 = 37;
/// Lit-cell tables in SRAM language order (En, De, Fr, Es, It, Nl).
const RES_GLYPHS: [u16; 6] = [180, 182, 181, 184, 183, 179];

/// MenuWork defaults the mosaic reads.
/// `fxReadyCount`: frames from opening a screen until every cube has landed.
pub const READY: i16 = 58;
/// `fxReadyRatio`: progress after which the groups start to sway.
const SWAY_FROM: f32 = 0.9;
/// `fxFadeIn` / `fxFadeOut`: length of the per-cube ramps.
const RAMP: i16 = 10;
/// `fxCubeScale`.
const CUBE_SCALE: f32 = 1.0;
/// `fxC8`: frames per sway cycle.
const SWAY_PERIOD: u16 = 120;
/// `fxB4, fxB8, fxBC, fxC0, fxC4`: sway x and y (units), yaw and roll
/// (degrees), and where the x swing turns.
const SWAY_X: f32 = 2.0;
const SWAY_Y: f32 = 5.0;
const SWAY_YAW: f32 = 30.0;
const SWAY_ROLL: f32 = 3.0;
const SWAY_X_SPLIT: f32 = 0.45;

pub type Rgb = [u8; 3];

/// A light/dark colour pair.
#[derive(Clone, Copy, Debug)]
pub struct Pair {
    pub light: Rgb,
    pub dark: Rgb,
}

pub const fn pair(light: Rgb, dark: Rgb) -> Pair {
    Pair { light, dark }
}

/// The MenuWork colour pairs of each screen. Game Play uses one pair for
/// all three slots.
pub const GAME_COLORS: [Pair; 3] = [pair([130, 50, 160], [50, 20, 70]); 3];
pub const OPTION_COLORS: [Pair; 3] =
    [pair([180, 255, 0], [20, 100, 50]), pair([120, 220, 35], [0, 80, 25]), pair([40, 120, 0], [0, 40, 10])];
pub const CLOCK_COLORS: [Pair; 3] =
    [pair([240, 255, 0], [100, 50, 20]), pair([235, 185, 0], [70, 40, 0]), pair([160, 90, 0], [50, 40, 0])];

/// fn_8132BFF8: one step toward a target.
#[derive(Clone, Copy, Default)]
struct Ramp {
    cur: i16,
    target: i16,
}

impl Ramp {
    fn step(&mut self) {
        self.cur += (self.target - self.cur).signum();
    }
    fn set(&mut self, on: bool) {
        self.target = if on { RAMP } else { 0 };
    }
    /// Show or hide now; a cell that is already shown keeps its ramp.
    fn force(&mut self, on: bool) {
        if !on || self.target == 0 {
            self.cur = if on { RAMP } else { 0 };
        }
        self.set(on);
    }
    fn k(&self) -> f32 {
        self.cur as f32 / RAMP as f32
    }
}

#[derive(Clone, Copy, Default)]
struct Cell {
    vis: Ramp,
    bright: Ramp,
    sel: Ramp,
    sway: Ramp,
    pos: [f32; 3],
    /// Cube size (1 unless a layout step scaled its group).
    scale: f32,
    rot: [i32; 3],
    light: [f32; 4],
    dark: [f32; 4],
    alpha: u8,
}

/// A layout step applied to a glyph entry once the cubes are on their
/// paths (fn_8132DEFC).
#[derive(Clone, Copy)]
enum Place {
    /// Move by (x, y) and scale by `k` about the entry's centre.
    Move(f32, f32, f32),
    /// Enlarge each cube by `1 + k * sel` about the entry's centre.
    Pulse(f32),
}

/// The builder's view of one frame: which cells are shown and how.
pub struct Build<'a> {
    glyphs: &'a Glyphs,
    cells: &'a mut [Cell],
    groups: Vec<&'a [u16]>,
    places: Vec<(&'a [u16], Place)>,
    all: &'a [u16],
}

impl<'a> Build<'a> {
    fn each(&mut self, entry: usize, f: impl Fn(&mut Cell)) {
        for &c in self.glyphs.get(entry) {
            if let Some(cell) = self.cells.get_mut(c as usize) {
                f(cell);
            }
        }
    }

    /// fn_8132CF7C: show or hide an entry now.
    pub fn show(&mut self, entry: usize, on: bool) {
        self.each(entry, |c| c.vis.force(on));
    }

    /// Fade an entry's shown cells in again from nothing (call after the
    /// cells are shown for the frame).
    pub fn flash(&mut self, entry: usize) {
        self.each(entry, |c| {
            if c.vis.target > 0 {
                c.vis.cur = 0;
            }
        });
    }

    /// Show digit `d` in a digit slot entry (the slot's other cells hide).
    pub fn digit(&mut self, slot: usize, d: u8) {
        let shape = self.glyphs.get(glyph::DIGIT0 + (d as usize % 10));
        for (k, &c) in self.glyphs.get(slot).iter().enumerate() {
            if let Some(cell) = self.cells.get_mut(c as usize) {
                cell.vis.force(shape.contains(&(k as u16)));
            }
        }
    }

    /// Show `n` in consecutive digit slots, most significant first.
    pub fn number(&mut self, slots: &[usize], n: u32) {
        let mut v = n;
        for &s in slots.iter().rev() {
            self.digit(s, (v % 10) as u8);
            v /= 10;
        }
    }

    /// fn_8132CF14: draw in colour pair 0 this frame (ramps in and out).
    pub fn bright(&mut self, entry: usize) {
        self.each(entry, |c| c.bright.set(true));
    }

    /// fn_8132CDD4: pair 1 (on) or pair 2 (off) where not bright.
    pub fn select(&mut self, entry: usize, on: bool) {
        self.each(entry, |c| c.sel.set(on));
    }

    /// Lay a whole line out: move it by (`x`, `y`) and scale it by `k`
    /// about its centre (the calendar rows, fxItem in MenuWork).
    pub fn place(&mut self, entry: usize, x: f32, y: f32, k: f32) {
        self.places.push((self.glyphs.get(entry), Place::Move(x, y, k)));
    }

    /// Enlarge the entry's selected cubes by up to `k` about its centre
    /// (the calendar field being edited).
    pub fn pulse(&mut self, entry: usize, k: f32) {
        self.places.push((self.glyphs.get(entry), Place::Pulse(k)));
    }

    /// fn_8132CEA4: follow (on) or leave (off) the group sway.
    pub fn sway(&mut self, entry: usize, on: bool) {
        self.each(entry, |c| c.sway.set(on));
    }

    /// Sway the cubes of `entry` as one rigid group (around their centre).
    pub fn group(&mut self, entry: usize) {
        self.groups.push(self.glyphs.get(entry));
    }

    /// Sway every cube of the screen as one group.
    pub fn group_all(&mut self) {
        self.groups.push(self.all);
    }
}

/// The mosaic's resources, read once from the dump.
pub struct MosaicData {
    pub paths: Paths,
    glyphs: Vec<Glyphs>,
    model: Prepared,
    all: Vec<u16>,
}

impl MosaicData {
    pub fn load(ipl: &Ipl) -> Option<MosaicData> {
        let paths = Paths::parse(&ipl.resource(RES_PATHS).ok()?).ok()?;
        let glyphs = RES_GLYPHS.iter().map(|&r| Glyphs::parse(&ipl.resource(r).ok()?).ok()).collect::<Option<Vec<_>>>()?;
        let model = Prepared::new(Model::parse(&ipl.resource(RES_MODEL).ok()?).ok()?);
        let max = paths.sets.iter().map(|s| s.paths.len()).max().unwrap_or(0);
        Some(MosaicData { paths, glyphs, model, all: (0..max as u16).collect() })
    }

    /// Language `lang`'s lit-cell table.
    pub fn glyphs(&self, lang: usize) -> &Glyphs {
        &self.glyphs[lang.min(self.glyphs.len() - 1)]
    }
}

/// Mosaic state: one screen at a time.
#[derive(Default)]
pub struct Mosaic {
    screen: Option<usize>,
    /// Frames since the screen opened, up to [`READY`] (`pool+0xA794`).
    progress: i16,
    cells: Vec<Cell>,
    /// Each cube's copy of the cube shader, with its colours loaded.
    shaders: Vec<Shader>,
    wave: u16,
    restart: bool,
}

impl Mosaic {
    pub fn new() -> Mosaic {
        Mosaic::default()
    }

    /// Fx_Clear (while the menu is on the cube): the next frame of any
    /// screen starts its fly-in again. The last frame stays drawable for
    /// the submenu's fade-out.
    pub fn clear(&mut self) {
        self.restart = true;
    }

    /// The set of the screen being shown.
    pub fn screen(&self) -> Option<usize> {
        self.screen
    }

    /// Fx_IsReady: true once at most `margin` frames of the fly-in are left.
    pub fn ready(&self, margin: i16) -> bool {
        self.progress >= READY - margin
    }

    /// One frame of screen `set` (a [`set`] id) in language `lang`:
    /// `build` marks the cells, then the cubes move, sway and get coloured.
    pub fn frame(&mut self, data: &MosaicData, set: usize, lang: usize, colors: &[Pair; 3], s: &SinTable, build: impl FnOnce(&mut Build)) {
        let Some(paths) = data.paths.sets.get(set) else { return };
        let n = paths.paths.len();
        if self.cells.len() < n {
            self.cells.resize(n, Cell::default());
        }
        if self.shaders.len() < n {
            let Some(sh) = data.model.shader(0) else { return };
            self.shaders.resize(n, sh.clone());
        }
        if self.screen != Some(set) || self.restart {
            self.screen = Some(set);
            self.restart = false;
            self.progress = 0;
            self.wave = 0;
            self.cells.iter_mut().for_each(|c| *c = Cell::default());
        }
        // fn_8132FFF0: the bright targets clear every frame.
        for c in &mut self.cells[..n] {
            c.bright.target = 0;
        }
        let mut b = Build { glyphs: data.glyphs(lang), cells: &mut self.cells[..n], groups: Vec::new(), places: Vec::new(), all: &data.all[..n] };
        build(&mut b);
        let groups: Vec<Vec<u16>> = b.groups.iter().map(|g| g.to_vec()).collect();
        let places: Vec<(Vec<u16>, Place)> = b.places.iter().map(|(g, p)| (g.to_vec(), *p)).collect();

        let t = self.progress as f32 / READY as f32;
        for (i, c) in self.cells[..n].iter_mut().enumerate() {
            c.pos = data.paths.eval(set, i, t);
            c.rot = [0; 3];
            c.scale = 1.0;
        }
        self.place(&places, n);
        if t > SWAY_FROM {
            self.sway(&groups, n, s);
        }
        for c in &mut self.cells[..n] {
            c.alpha = (255.0 * t * c.vis.k()) as u8;
            let (f, g) = (c.sel.k(), c.bright.k());
            let mix = |p: fn(&Pair) -> Rgb| -> [f32; 4] {
                let (a, b0, b1) = (p(&colors[0]), p(&colors[1]), p(&colors[2]));
                let ch = |k: usize| g * a[k] as f32 + (1.0 - g) * (f * b0[k] as f32 + (1.0 - f) * b1[k] as f32);
                [ch(0), ch(1), ch(2), 255.0]
            };
            c.light = mix(|p| p.light);
            c.dark = mix(|p| p.dark);
        }
        for (c, sh) in self.cells[..n].iter().zip(&mut self.shaders) {
            sh.set_reg(0, c.light.map(|v| v / 255.0));
            sh.set_reg(1, c.dark.map(|v| v / 255.0));
        }
        // fn_8132BF20
        for c in &mut self.cells[..n] {
            c.vis.step();
            c.bright.step();
            c.sel.step();
            c.sway.step();
        }
        if self.progress < READY {
            self.progress += 1;
        }
        self.wave = (self.wave + 1) % SWAY_PERIOD;
    }

    /// fn_8132DEFC: the layout steps, each about the centre of its entry
    /// (fn_8132C830) taken before the step.
    fn place(&mut self, places: &[(Vec<u16>, Place)], n: usize) {
        for (g, p) in places {
            let ids = || g.iter().map(|&i| i as usize).filter(|&i| i < n);
            let k = ids().count();
            if k == 0 {
                continue;
            }
            let mut ctr = [0.0f32; 3];
            for i in ids() {
                (0..3).for_each(|a| ctr[a] += self.cells[i].pos[a]);
            }
            ctr = ctr.map(|v| v / k as f32);
            for i in ids() {
                let c = &mut self.cells[i];
                let (d, s) = match *p {
                    Place::Move(x, y, s) => ([x, y], s),
                    Place::Pulse(s) => ([0.0; 2], 1.0 + s * c.sel.k()),
                };
                c.pos[0] = ctr[0] + s * (c.pos[0] + d[0] - ctr[0]);
                c.pos[1] = ctr[1] + s * (c.pos[1] + d[1] - ctr[1]);
                c.pos[2] = ctr[2] + s * (c.pos[2] - ctr[2]);
                c.scale *= s;
            }
        }
    }

    /// fn_8133498C + fn_8132C750: each group turns about its centre and
    /// drifts; a cube follows by its `sway` ramp.
    fn sway(&mut self, groups: &[Vec<u16>], n: usize, s: &SinTable) {
        let u = self.wave as f32 / SWAY_PERIOD as f32;
        let dx = swing(SWAY_X_SPLIT, fract(u + SWAY_X_SPLIT * 0.5), SWAY_X);
        let dy = swing(0.5, fract(0.25 + 2.0 * u), SWAY_Y);
        let ph = fract(0.25 + u);
        let yaw = deg(swing(0.5, ph, SWAY_YAW));
        let roll = deg(swing(0.5, ph, SWAY_ROLL));
        let r = mtx::rot_trans(s, 0, yaw, roll, [0.0; 3]);
        for g in groups {
            let mut ctr = [0.0f32; 3];
            let mut k = 0.0;
            for &i in g.iter().filter(|&&i| (i as usize) < n) {
                let p = self.cells[i as usize].pos;
                (0..3).for_each(|a| ctr[a] += p[a]);
                k += 1.0;
            }
            if k == 0.0 {
                continue;
            }
            ctr = ctr.map(|v| v / k);
            for &i in g.iter().filter(|&&i| (i as usize) < n) {
                let c = &mut self.cells[i as usize];
                let w = c.sway.k();
                if w <= 0.0 {
                    continue;
                }
                let d = [c.pos[0] - ctr[0], c.pos[1] - ctr[1], c.pos[2] - ctr[2]];
                let q = mtx::apply(&r, d);
                c.pos[0] += (q[0] - d[0] + dx) * w;
                c.pos[1] += (q[1] - d[1] + dy) * w;
                c.pos[2] += (q[2] - d[2]) * w;
                c.rot = [0, (yaw as f32 * w) as i32, (roll as f32 * w) as i32];
            }
        }
    }

    /// Fx_Draw: queue the visible cubes. `view` is the submenu camera
    /// (one unit per pixel at z = 0), `alpha` the submenu's fx fader.
    pub fn draw<'a>(&'a self, list: &mut Vec<Prim<'a>>, data: &'a MosaicData, view: &View, alpha: u8, s: &SinTable) {
        let Some(set) = self.screen else { return };
        let n = data.paths.sets[set].paths.len().min(self.shaders.len());
        for (c, sh) in self.cells[..n].iter().zip(&self.shaders) {
            let a = (c.alpha as u32 * alpha as u32 / 255) as u8;
            if a <= 10 {
                continue;
            }
            let k = CUBE_SCALE * c.scale;
            let m = mtx::concat(&mtx::rot_trans(s, c.rot[0], c.rot[1], c.rot[2], c.pos), &mtx::scale(k, k, k));
            let o = DrawOpts { alpha: a, ..Default::default() };
            render::draw_model_with(list, &data.model, Some(sh), &m, view, &o);
        }
    }
}

fn fract(x: f32) -> f32 {
    x - libm::floorf(x)
}

fn deg(d: f32) -> i32 {
    (65535.0 * d / 360.0) as i32
}

/// fn_81334B40: a smooth swing -amp..amp over [0, split], back over
/// [split, 1].
fn swing(split: f32, x: f32, amp: f32) -> f32 {
    if x < split {
        mtx::hermite(x, 0.0, -amp, 0.0, split, amp, 0.0)
    } else {
        mtx::hermite(x, split, amp, 0.0, 1.0, -amp, 0.0)
    }
}

/// The screen builders of the IPL (FxClock_Update, FxGame_Update,
/// FxOption_Update), in terms of [`glyph`] entries.
pub mod screen {
    use super::{glyph as g, Build};

    /// Calendar state, as Clock_Update passes it.
    #[derive(Clone, Copy, Default)]
    pub struct Clock {
        /// 0 menu, 1 editing the date, 2 editing the time.
        pub sub: u8,
        /// The sub-state of the previous frame.
        pub prev_sub: u8,
        /// 0 date, 1 time.
        pub row: u8,
        /// Date field being edited: 0 year, 1 month, 2 day.
        pub date_field: u8,
        /// Time field being edited: 0 hour, 1 minute, 2 second.
        pub time_field: u8,
        /// Year, month 1..12, day, hour, minute, second.
        pub time: [i32; 6],
        /// `MenuWork.fx17E`: the other fields keep swaying while one is edited.
        pub keep_sway: bool,
    }

    /// fn_8132B114 + fn_8132B678.
    pub fn clock(b: &mut Build, c: &Clock) {
        let [y, mo, d, h, mi, s] = c.time.map(|v| v.max(0) as u32);
        // The digits (the slots and separators cover both rows).
        b.number(&g::YEAR_DIGITS, y);
        b.number(&g::MONTH_DIGITS, mo);
        b.number(&g::DAY_DIGITS, d);
        b.number(&g::HOUR_DIGITS, h);
        b.number(&g::MINUTE_DIGITS, mi);
        b.number(&g::SECOND_DIGITS, s);
        b.show(g::SEPARATORS, true);
        // The selected row sways and is drawn bright.
        let (on, off) = if c.row != 0 { (g::TIME_ROW, g::DATE_ROW) } else { (g::DATE_ROW, g::TIME_ROW) };
        b.select(off, false);
        b.sway(off, false);
        b.select(on, false);
        b.sway(on, true);
        b.bright(on);
        // The field being edited stands still in pair 1 and flashes in.
        let field = match c.sub {
            1 => Some((g::DATE_ROW, [&g::YEAR_DIGITS[..], &[g::MONTH], &[g::DAY]][c.date_field.min(2) as usize])),
            2 => Some((g::TIME_ROW, [&[g::HOUR][..], &[g::MINUTE], &[g::SECOND]][c.time_field.min(2) as usize])),
            _ => None,
        };
        if let Some((row, entries)) = field {
            if !c.keep_sway {
                b.sway(row, false);
            }
            for &e in entries {
                if c.prev_sub != c.sub {
                    b.flash(e);
                }
                b.select(e, true);
                b.sway(e, false);
            }
        }
        b.group(g::DATE_ROW);
        b.group(g::TIME_ROW);
        // fn_8132DEFC: the rows move down under their labels at 0.74 size
        // (MenuWork fxItem, PAL), and the field being edited grows by up to
        // a fifth as it is selected.
        b.place(g::DATE_ROW, ITEM[0].0, ITEM[0].1, ITEM[0].2);
        b.place(g::TIME_ROW, ITEM[1].0, ITEM[1].1, ITEM[1].2);
        match c.sub {
            1 => b.pulse([g::YEAR, g::MONTH, g::DAY][c.date_field.min(2) as usize], ITEM[0].3),
            2 => b.pulse([g::HOUR, g::MINUTE, g::SECOND][c.time_field.min(2) as usize], ITEM[1].3),
            _ => {}
        }
    }

    /// MenuWork fxItem[0..2] after the PAL overrides: x, y, scale, spacing.
    const ITEM: [(f32, f32, f32, f32); 2] = [(0.0, -46.0, 0.74, 0.2), (0.0, -20.0, 0.74, 0.2)];

    /// Game Play's message: 0 "PRESS START", 1 "NO DISC", 2 unknown disc.
    pub fn game(b: &mut Build, msg: u8) {
        for e in [g::NO_DISC, g::PRESS_START, g::UNKNOWN_DISC] {
            b.show(e, false);
        }
        let e = match msg {
            0 => g::PRESS_START,
            1 => g::NO_DISC,
            _ => g::UNKNOWN_DISC,
        };
        b.show(e, true);
        b.bright(e);
        b.sway(e, true);
        b.group_all();
    }

    /// Options state (FxOption_Update arguments).
    #[derive(Clone, Copy, Default)]
    pub struct Options {
        /// 0 menu, 1 sound, 2 screen position, 3 language.
        pub sub: u8,
        /// Row 0..2 (sound, screen position, language); `None` hides it.
        pub row: u8,
        /// Sound: `Some(stereo)`, or `None` to leave the row empty.
        pub stereo: Option<bool>,
        /// Screen position -32..32, or any two-digit value to show.
        pub offset: Option<i32>,
        /// Show the language row.
        pub lang: bool,
    }

    /// The three option rows.
    pub fn options(b: &mut Build, o: &Options) {
        b.show(g::SOUND_ROW, false);
        b.show(g::OFFSET_ROW, false);
        for e in [g::OFFSET_MINUS, g::OFFSET_PLUS, g::LANG_DASH, g::MONO, g::STEREO] {
            b.show(e, false);
        }
        for e in g::SOUND_ARROWS.iter().chain(&g::OFFSET_ARROWS).chain(&g::LANG_ARROWS) {
            b.show(*e, false);
        }
        let mut rows: [&[usize]; 3] = [&[], &[], &[]];
        if let Some(st) = o.stereo {
            b.show(if st { g::STEREO } else { g::MONO }, true);
            rows[0] = if st { &[g::STEREO] } else { &[g::MONO] };
        }
        if let Some(v) = o.offset {
            // Like the IPL: an arrow only when the offset is not 0.
            match v.signum() {
                -1 => b.show(g::OFFSET_MINUS, true),
                1 => b.show(g::OFFSET_PLUS, true),
                _ => {}
            }
            b.number(&g::OFFSET_DIGITS, v.unsigned_abs().min(99));
            rows[1] = &[g::OFFSET_ROW];
        }
        if o.lang {
            b.show(g::LANG_DASH, true);
            rows[2] = &[g::LANG_DASH];
        }
        let arrows = [g::SOUND_ARROWS, g::OFFSET_ARROWS, g::LANG_ARROWS];
        for (i, es) in rows.iter().enumerate() {
            let sel = i == o.row as usize;
            let editing = o.sub as usize == i + 1;
            for &e in *es {
                b.sway(e, sel && !editing);
                b.select(e, editing);
                if sel {
                    b.bright(e);
                }
                b.group(e);
            }
            if editing {
                for &a in &arrows[i] {
                    b.show(a, true);
                    b.bright(a);
                }
            }
        }
    }
}
