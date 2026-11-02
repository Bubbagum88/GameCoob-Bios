//! Rolling-cube intro (`cube.c`, 0x8130EF48..0x813108B8): the small cube
//! falls, rolls a path over three faces leaving tiles behind, rises, drops
//! into the big cube and the logo fades in. The path table is read from the
//! user's ipl.bin at runtime.
//!
//! `CubeState` and the file's statics (`sPhase`, `sRunning`, the grid, the
//! pending flags) are one [`Cube`]; `Scene_Get()` is the `scene` argument.

use super::mtx;
use crate::sdk::mtx::{self as psmtx, Mtx};
use super::scene::Scene;
use alloc::vec::Vec;

pub const GRID: usize = 32;
pub const FACES: usize = 3;

pub const DIR_FACE0: i8 = 4;
pub const DIR_RISE: u8 = 7;
pub const DIR_END: u8 = 8;

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Phase {
    Fall,
    Path,
    Rise,
    Drop,
    Logo,
}

/// One entry of the path table at 0x81381160.
#[derive(Clone, Copy, Debug)]
pub struct PathStep {
    pub dir: u8,
    pub group: u8,
    pub life: u8,
}

/// One cell of the tile grid (`CubeTile`), `[face][x][y]`.
#[derive(Clone, Copy, Default)]
pub struct Tile {
    pub life: u8,
    pub group: u8,
    /// 0 idle, 1 fading in, 2 fading out
    pub state: u8,
    pub color: u32,
}

/// Which boot jingle to play (the Z-button easter eggs).
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Jingle {
    Normal = 0,
    AllPadsZ = 1,
    Pad1Z = 2,
}

pub struct Cube {
    // CubeState (0x814B5D58)
    pub x: i32,
    pub y: i32,
    pub dir: i8,
    pub face: i8,
    pub alpha: u8,
    pub color: u32,
    pub flags: u16,
    pub step_frame: i16,
    pub side_frames: i16,
    pub corner_frames: i16,
    pub step_delay: i16,
    pub step_done_frame: i16,
    pub unk1c: u8,
    pub step: u8,
    pub tile_inset: u8,
    pub done: bool,
    pub hold_fall: bool,
    fall_frame: u8,
    fall_frames: u8,
    fall_delay: u8,
    fall_vel: f32,
    fall_accel: f32,
    fall_height0: f32,
    pub fall_height: f32,
    pub rise_frame: u8,
    pub rise_frames: u8,
    rise_delay: u8,
    pub rise_turns: u8,
    pub rise_height: f32,
    drop_frame: u8,
    drop_frames: u8,
    drop_delay: u8,
    drop_vel: f32,
    drop_accel: f32,
    drop_height0: f32,
    pub drop_height: f32,
    drop_vel0: f32,
    logo_frames: u8,
    logo_frame: u8,
    pub text_frame: u8,
    pub text_frames: u8,
    pub text_delay: u8,
    pub size: u8,
    pub tile_r: u8,
    pub tile_g: u8,
    pub tile_b: u8,
    jingle_frame: i16,
    jingle_delay: [i16; 3],

    // cube.c statics
    pub path: Vec<PathStep>,
    pub grid: Vec<Tile>,
    pub phase: Phase,
    next_phase: Phase,
    /// `sRunning`: TRUE until the cube lifts off; the A-button spin only
    /// works meanwhile.
    pub running: bool,
    land_pending: bool,
    drop_pending: bool,
    jingle_pending: bool,
    /// `sCubeMtx`, from [`Cube::calc_mtx`].
    pub mtx: Mtx,
}

impl Cube {
    /// `Cube_Init(TRUE)`: defaults, then a reset.
    pub fn new(path: Vec<PathStep>) -> Self {
        let mut c = Cube {
            x: 0,
            y: 0,
            dir: 0,
            face: 0,
            alpha: 0,
            color: 0,
            flags: 0,
            step_frame: 0,
            side_frames: 0,
            corner_frames: 0,
            step_delay: 0,
            step_done_frame: 0,
            unk1c: 0,
            step: 0,
            tile_inset: 0,
            done: false,
            hold_fall: false,
            fall_frame: 0,
            fall_frames: 0,
            fall_delay: 0,
            fall_vel: 0.0,
            fall_accel: 0.0,
            fall_height0: 0.0,
            fall_height: 0.0,
            rise_frame: 0,
            rise_frames: 0,
            rise_delay: 0,
            rise_turns: 0,
            rise_height: 0.0,
            drop_frame: 0,
            drop_frames: 0,
            drop_delay: 0,
            drop_vel: 0.0,
            drop_accel: 0.0,
            drop_height0: 0.0,
            drop_height: 0.0,
            drop_vel0: 0.0,
            logo_frames: 0,
            logo_frame: 0,
            text_frame: 0,
            text_frames: 0,
            text_delay: 0,
            size: 0,
            tile_r: 0,
            tile_g: 0,
            tile_b: 0,
            jingle_frame: 0,
            jingle_delay: [0; 3],
            path,
            grid: alloc::vec![Tile::default(); FACES * GRID * GRID],
            phase: Phase::Fall,
            next_phase: Phase::Fall,
            running: false,
            land_pending: true,
            drop_pending: true,
            jingle_pending: true,
            mtx: psmtx::IDENTITY,
        };
        c.set_defaults();
        c.init();
        c
    }

    /// `Cube_Init` after the first call: clear the grid and reset.
    pub fn init(&mut self) {
        self.clear_grid();
        self.reset();
        self.running = true;
        self.land_pending = true;
        self.drop_pending = true;
    }

    /// `Cube_ClearGrid` (0x8130EF5C).
    fn clear_grid(&mut self) {
        for t in self.grid.iter_mut() {
            t.life = 0;
            t.group = 0;
            t.state = 0;
        }
    }

    /// `Cube_Reset` (0x8130F06C).
    fn reset(&mut self) {
        self.x = 16;
        self.y = 18;
        self.face = 0;
        self.step = 0;
        self.dir = self.path_step().dir as i8;
        self.tile_inset = 0;
        self.done = false;
        if self.flags & 1 == 0 {
            self.color = 0x9664FF00;
        }
        self.phase = Phase::Fall;
        self.next_phase = Phase::Fall;

        // Constant acceleration so the fall covers fallHeight0 in fallFrames.
        self.fall_vel = 0.0;
        let n = self.fall_frames as i32;
        self.fall_accel = 2.0 * self.fall_height0 / (n * n) as f32;
        self.fall_height = self.fall_height0;

        // The drop starts at dropVel0 and accelerates to land in dropFrames.
        self.drop_height0 = self.rise_height;
        self.drop_vel = self.drop_vel0;
        let n = self.drop_frames as i32;
        self.drop_accel = 2.0 * self.drop_height0 / (n * n) as f32 - self.drop_vel / n as f32;
        if self.drop_accel < 0.0 {
            self.drop_accel = 0.0;
        }
        self.drop_height = self.drop_height0;

        self.fall_frame = 0;
        self.drop_frame = 0;
        self.step_frame = 0;
        self.rise_frame = 0;
        self.logo_frame = 0;
        self.text_frame = 0;
        self.jingle_frame = 0;
        self.jingle_pending = true;
    }

    /// `Cube_SetDefaults` (0x8130F1C0).
    fn set_defaults(&mut self) {
        self.step_done_frame = 0;
        self.flags = 0;
        self.hold_fall = false;
        self.jingle_delay = [0, 6, 5];
        self.fall_height0 = 100.0;
        self.fall_frames = 4;
        self.fall_delay = 13;
        self.side_frames = 8;
        self.corner_frames = 15;
        self.step_delay = 0;
        self.alpha = 255;
        self.unk1c = 5;
        self.rise_turns = 6;
        self.rise_frames = 15;
        self.rise_delay = 6;
        self.rise_height = 130.0;
        self.drop_frames = 8;
        self.drop_delay = 17;
        self.drop_vel0 = 30.0;
        self.logo_frames = 33;
        self.text_frames = 50;
        self.text_delay = 4;
        self.size = 54;
        self.tile_r = 100;
        self.tile_g = 80;
        self.tile_b = 190;
    }

    #[inline]
    pub fn tile(&self, f: usize, x: usize, y: usize) -> &Tile {
        &self.grid[(f * GRID + x) * GRID + y]
    }

    fn path_step(&self) -> PathStep {
        self.path.get(self.step as usize).copied().unwrap_or(PathStep { dir: DIR_END, group: 0, life: 0 })
    }

    /// `Cube_TickTiles` (0x8130F2F0): advance tile lifetimes by one path step.
    fn tick_tiles(&mut self) {
        for t in self.grid.iter_mut() {
            if t.state == 2 {
                t.state = 0;
                t.group = 0;
                t.life = 0;
            } else if t.state == 1 {
                t.state = 0;
            } else if t.group != 0 && t.life != 0xFF {
                t.life -= 1;
                if t.life == 0 {
                    t.state = 2;
                }
            }
        }
    }

    /// `Cube_StepGrid` (0x8130F418): move one cell (or over an edge onto
    /// another face) and leave a tile there.
    fn step_grid(&mut self) {
        let g = GRID as i32;
        match self.dir {
            0 => self.x = if self.x < g { self.x + 1 } else { g },
            1 => self.y = if self.y < g { self.y + 1 } else { g },
            2 => self.x = if self.x > 0 { self.x - 1 } else { 0 },
            3 => self.y = if self.y > 0 { self.y - 1 } else { 0 },
            4..=6 => {
                let face = self.dir - DIR_FACE0;
                if (face as i32 + 3 - self.face as i32) % 3 == 1 && self.y == 16 {
                    self.y = self.x;
                    self.x = 16;
                    self.face = face;
                }
                if (face as i32 + 3 - self.face as i32) % 3 == 2 && self.x == 16 {
                    self.x = self.y;
                    self.y = 16;
                    self.face = face;
                }
            }
            _ => {}
        }
        self.tick_tiles();
        let s = self.path_step();
        let (f, x, y) = (self.face as usize, self.x as usize, self.y as usize);
        // The C indexes past the grid when x or y reaches 32; the path
        // never gets there.
        if f < FACES && x < GRID && y < GRID {
            let color = self.color;
            let t = &mut self.grid[(f * GRID + x) * GRID + y];
            t.life = s.life;
            t.group = s.group;
            t.state = 1;
            t.color = color;
        }
    }

    /// The roll angle so far and its maximum (90 degrees flat, 180 over an
    /// edge), as `Cube_CalcRollMtx` and `Cube_IsStepDone` work it out.
    fn roll_angle(&self) -> (i32, i32) {
        let f = self.step_frame as i32;
        if self.dir >= DIR_FACE0 {
            ((f << 15) / (self.corner_frames - self.step_delay) as i32, 0x8000)
        } else {
            ((f << 14) / (self.side_frames - self.step_delay) as i32, 0x4000)
        }
    }

    /// `Cube_CalcRollMtx` (0x8130F6B8): roll about the leading bottom edge.
    fn calc_roll_mtx(&self, m: &mut Mtx) {
        let (mut angle, max) = self.roll_angle();
        if angle > max {
            angle = max;
        }
        let h = self.size as i32 / 2;
        let t = mtx::rot_trans(0, 0, 0, [-h as f32, h as f32, -h as f32]);
        *m = psmtx::concat(m, &t);
        *m = psmtx::concat(&mtx::rot_axis('x', angle), m);
    }

    /// `Cube_CalcGridMtx` (0x8130F7F0): turn to the step direction, move to
    /// the grid cell, and onto the current face.
    fn calc_grid_mtx(&self, m: &mut Mtx) {
        let yaw = match self.dir {
            0 => -0x4000,
            1 => 0x7FFF,
            2 => 0x4000,
            _ => 0,
        };
        let h = (self.size as i32 / 2) as f32;
        *m = psmtx::concat(&psmtx::trans(h, 0.0, h), m);
        *m = psmtx::concat(&mtx::rot_trans(0, yaw, 0, [-h, 0.0, -h]), m);
        let sz = self.size as i32;
        *m = psmtx::concat(&psmtx::trans((-(self.x - 16) * sz) as f32, 0.0, (-(self.y - 16) * sz) as f32), m);
        if self.face == 1 {
            *m = psmtx::concat(&mtx::rot_trans(0x4000, 0, 0x4000, [0.0; 3]), m);
        }
        if self.face == 2 {
            *m = psmtx::concat(&mtx::rot_trans(0, -0x4000, -0x4000, [0.0; 3]), m);
        }
    }

    /// `Cube_IsStepDone` (0x8130FA48).
    fn is_step_done(&mut self) -> bool {
        let (angle, max) = self.roll_angle();
        if angle > max && self.step_done_frame == 0 {
            self.step_done_frame = self.step_frame;
        }
        self.step_done_frame > 0 && self.step_frame >= self.step_done_frame + self.step_delay
    }

    /// `Cube_UpdateFall` (0x8130FAF0).
    fn update_fall(&mut self, s: &mut Scene) -> Phase {
        if self.hold_fall {
            return Phase::Fall;
        }
        self.fall_height -= self.fall_vel;
        if self.fall_height < 0.0 {
            self.fall_height = 0.0;
            if self.land_pending {
                s.land_impulse = s.land_impulse0;
                s.shake_phase = 0x7FFF;
                self.land_pending = false;
            }
        }
        self.fall_vel += self.fall_accel;
        if self.fall_frame as i32 > self.fall_frames as i32 + self.fall_delay as i32 {
            self.land_pending = true;
            return Phase::Path;
        }
        self.fall_frame += 1;
        Phase::Fall
    }

    /// `Cube_TextFrame`: the wordmark animation frame (0..60), on the text
    /// clock that starts with the drop.
    pub fn text_anim_frame(&self) -> f32 {
        if self.text_frame > self.text_delay {
            60.0 * ((self.text_frame - self.text_delay) as f32 / self.text_frames as f32)
        } else {
            0.0
        }
    }

    /// `Cube_CalcRiseMtx` (0x8130FBC4): grow to 1.5x while making riseTurns
    /// quarter turns.
    fn calc_rise_mtx(&self, m: &mut Mtx) {
        let rf = self.rise_frame as i32;
        let n = self.rise_frames as i32;
        let mut scale = 1.0 + 0.5 * rf as f32 / n as f32;
        if scale > 1.5 {
            scale = 1.5;
        }
        let max = (self.rise_turns as i32) << 14;
        let mut angle = max * rf / n;
        if angle > max {
            angle = max;
        }
        *m = psmtx::scale(scale, scale, scale);
        let sz = self.size as i32;
        let mut y = (sz / 2) as f32 - rf as f32 * ((1.0 + scale) * sz as f32) / n as f32 / 2.0;
        let lo = -(scale * sz as f32 / 2.0);
        if y < lo {
            y = lo;
        }
        let t = mtx::rot_trans(angle, 0, 0, [scale * -sz as f32 / 2.0, y, scale * -sz as f32 / 2.0]);
        *m = psmtx::concat(&t, m);
        *m = psmtx::concat(&mtx::rot_trans(0, -0x4000, -0x4000, [0.0; 3]), m);
    }

    /// `sCubeMtx` as `Cube_Draw` (0x81310060) builds it: `Cube_DrawPath`
    /// for the fall and the path, `Cube_DrawRise` for the rest. `world` is
    /// `Scene_GetWorldMtx`.
    pub fn calc_mtx(&mut self, world: &Mtx) {
        let c3 = (self.size as i32 * 3 / 2) as f32;
        let mut m = psmtx::IDENTITY;
        match self.phase {
            Phase::Fall | Phase::Path => {
                self.calc_roll_mtx(&mut m);
                self.calc_grid_mtx(&mut m);
                m = psmtx::concat(&mtx::rot_trans(0, 0, 0, [c3; 3]), &m);
                m = psmtx::concat(world, &m);
                if self.phase == Phase::Fall {
                    m = psmtx::concat(&psmtx::trans(0.0, self.fall_height, 0.0), &m);
                }
            }
            _ => {
                let n = self.rise_frames as i32;
                let mut r = n - self.rise_frame as i32;
                self.calc_rise_mtx(&mut m);
                m = psmtx::concat(&mtx::rot_trans(0, 0, 0, [c3; 3]), &m);
                m = psmtx::concat(world, &m);
                let t = if self.phase == Phase::Rise {
                    // Ease out: h * (1 - r^4 / n^4)
                    if r < 0 {
                        r = 0;
                    }
                    let h = self.rise_height;
                    let rf = r as f32;
                    let mut y = h - rf * (rf * (rf * (h * rf))) / (n * (n * (n * n))) as f32;
                    if y > h {
                        y = h;
                    }
                    psmtx::trans(0.0, y, 0.0)
                } else {
                    psmtx::trans(0.0, self.drop_height, 0.0)
                };
                m = psmtx::concat(&t, &m);
            }
        }
        self.mtx = m;
    }

    /// `Cube_UpdateJingle` (0x81310494): the boot jingle, with the Z easter
    /// eggs. Returns the jingle `Sound_Play` starts this frame.
    fn update_jingle(&mut self, egg: Jingle) -> Option<Jingle> {
        let mut play = None;
        if self.jingle_pending && self.jingle_frame > self.jingle_delay[egg as usize] {
            play = Some(egg);
            self.jingle_pending = false;
        }
        if self.jingle_pending {
            self.jingle_frame += 1;
        }
        play
    }

    /// `Cube_TickText`.
    fn tick_text(&mut self) -> bool {
        let mut done = false;
        self.text_frame += 1;
        if self.text_frame as i32 >= self.text_frames as i32 + self.text_delay as i32 {
            self.text_frame = self.text_frames + self.text_delay;
            done = true;
        }
        done
    }

    /// `Cube_Update(playJingle)` (0x81310584): one logic frame. Returns the
    /// jingle to start, if this frame starts one.
    pub fn update(&mut self, s: &mut Scene, play_jingle: Option<Jingle>) -> Option<Jingle> {
        self.phase = self.next_phase;
        let jingle = play_jingle.and_then(|egg| self.update_jingle(egg));
        match self.phase {
            Phase::Fall => self.next_phase = self.update_fall(s),
            Phase::Path => {
                if self.is_step_done() {
                    self.step_frame = 0;
                    self.step_done_frame = 0;
                    self.step += 1;
                    if self.path_step().dir == DIR_END {
                        self.done = true;
                        self.tick_tiles();
                    } else {
                        self.step_grid();
                        self.dir = self.path_step().dir as i8;
                    }
                }
                self.next_phase = if self.dir == DIR_RISE as i8 {
                    Phase::Rise
                } else {
                    self.step_frame += 1;
                    Phase::Path
                };
            }
            Phase::Rise => {
                self.running = false;
                self.rise_frame += 1;
                self.next_phase = if self.rise_frame as i32 > self.rise_frames as i32 + self.rise_delay as i32 {
                    Phase::Drop
                } else {
                    Phase::Rise
                };
            }
            Phase::Drop => {
                self.drop_height -= self.drop_vel;
                if self.drop_height < 0.0 {
                    self.drop_height = 0.0;
                    if self.drop_pending {
                        s.drop_impulse = s.drop_impulse0;
                        s.shake_phase = 0x7FFF;
                        self.drop_pending = false;
                    }
                }
                self.drop_vel += self.drop_accel;
                self.next_phase = if self.drop_frame as i32 > self.drop_frames as i32 + self.drop_delay as i32 {
                    self.drop_pending = true;
                    Phase::Logo
                } else {
                    self.drop_frame += 1;
                    self.text_frame += 1;
                    if self.text_frame as i32 >= self.text_frames as i32 + self.text_delay as i32 {
                        self.text_frame = self.text_frames + self.text_delay;
                    }
                    Phase::Drop
                };
            }
            Phase::Logo => {
                self.logo_frame += 1;
                if self.logo_frame > self.logo_frames {
                    self.logo_frame = self.logo_frames;
                    self.done = true;
                }
                s.logo_alpha = (self.logo_frame as i32 * 255 / self.logo_frames as i32) as u8;
                if !self.tick_text() {
                    self.done = false;
                }
                if s.drop_impulse as f32 != 0.0 {
                    self.done = false;
                }
            }
        }
        jingle
    }
}

/// Read the path table (4 bytes per step, ends at dir 8) from BS2.
pub fn read_path(bs2_at: impl Fn(u32) -> Option<u8>) -> Option<Vec<PathStep>> {
    const PATH_ADDR: u32 = 0x8138_1160;
    let mut v = Vec::new();
    for i in 0..64u32 {
        let a = PATH_ADDR + i * 4;
        let dir = bs2_at(a)?;
        v.push(PathStep { dir, group: bs2_at(a + 1)?, life: bs2_at(a + 2)? });
        if dir == DIR_END {
            return Some(v);
        }
    }
    None
}
