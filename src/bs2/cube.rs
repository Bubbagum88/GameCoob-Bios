//! The rolling cube that draws the "G": a clean reimplementation of the BS2
//! intro state machine. Every number here is a frame count or distance taken
//! from the IPL's own defaults (see the decomp's docs/intro.md); the path the
//! cube rolls is read from the user's ipl.bin at runtime.

use super::mtx;
use crate::sdk::mtx::{self as psmtx, Mtx};
use super::scene::Scene;
use alloc::vec::Vec;

pub const GRID: usize = 32;
pub const FACES: usize = 3;

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

#[derive(Clone, Copy, Debug)]
pub struct PathStep {
    pub dir: u8,
    pub group: u8,
    pub life: u8,
}

#[derive(Clone, Copy, Default)]
pub struct Tile {
    pub life: u8,
    pub group: u8,
    /// 0 idle, 1 fading in, 2 fading out
    pub state: u8,
}

pub struct Cube {
    pub path: Vec<PathStep>,
    pub tiles: Vec<Tile>,
    pub x: i32,
    pub y: i32,
    pub dir: u8,
    pub face: u8,
    pub alpha: u8,
    pub step_frame: i32,
    pub side_frames: i32,
    pub corner_frames: i32,
    pub step_delay: i32,
    pub step_done_frame: i32,
    pub step: usize,
    pub tile_inset: i32,
    pub done: bool,
    fall_frame: i32,
    fall_frames: i32,
    fall_delay: i32,
    fall_vel: f32,
    fall_accel: f32,
    pub fall_height: f32,
    pub rise_frame: i32,
    pub rise_frames: i32,
    rise_delay: i32,
    pub rise_turns: i32,
    pub rise_height: f32,
    drop_frame: i32,
    drop_frames: i32,
    drop_delay: i32,
    drop_vel: f32,
    drop_accel: f32,
    pub drop_height: f32,
    logo_frames: i32,
    logo_frame: i32,
    pub text_frame: i32,
    pub text_frames: i32,
    pub text_delay: i32,
    pub size: i32,
    pub tile_rgb: [u8; 3],
    jingle_frame: i32,
    jingle_delay: [i32; 3],
    pub phase: Phase,
    next_phase: Phase,
    /// TRUE until the cube lifts off; the A-button spin only works meanwhile.
    pub running: bool,
    land_pending: bool,
    drop_pending: bool,
    jingle_pending: bool,
}

/// Which boot jingle to play (the Z-button easter eggs).
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Jingle {
    Normal = 0,
    AllPadsZ = 1,
    Pad1Z = 2,
}

impl Cube {
    pub fn new(path: Vec<PathStep>) -> Self {
        let mut c = Cube {
            path,
            tiles: alloc::vec![Tile::default(); FACES * GRID * GRID],
            x: 16,
            y: 18,
            dir: 0,
            face: 0,
            alpha: 0xFF,
            step_frame: 0,
            side_frames: 8,
            corner_frames: 15,
            step_delay: 0,
            step_done_frame: 0,
            step: 0,
            tile_inset: 0,
            done: false,
            fall_frame: 0,
            fall_frames: 4,
            fall_delay: 13,
            fall_vel: 0.0,
            fall_accel: 0.0,
            fall_height: 100.0,
            rise_frame: 0,
            rise_frames: 15,
            rise_delay: 6,
            rise_turns: 6,
            rise_height: 130.0,
            drop_frame: 0,
            drop_frames: 8,
            drop_delay: 17,
            drop_vel: 30.0,
            drop_accel: 0.0,
            drop_height: 130.0,
            logo_frames: 33,
            logo_frame: 0,
            text_frame: 0,
            text_frames: 50,
            text_delay: 4,
            size: 54,
            tile_rgb: [100, 80, 190],
            jingle_frame: 0,
            jingle_delay: [0, 6, 5],
            phase: Phase::Fall,
            next_phase: Phase::Fall,
            running: true,
            land_pending: true,
            drop_pending: true,
            jingle_pending: true,
        };
        c.reset();
        c
    }

    fn reset(&mut self) {
        let fall_h0 = 100.0;
        let drop_v0 = 30.0;
        self.x = 16;
        self.y = 18;
        self.face = 0;
        self.step = 0;
        self.dir = self.path.first().map_or(DIR_END, |s| s.dir);
        self.done = false;
        self.phase = Phase::Fall;
        self.next_phase = Phase::Fall;
        self.fall_vel = 0.0;
        self.fall_accel = 2.0 * fall_h0 / (self.fall_frames * self.fall_frames) as f32;
        self.fall_height = fall_h0;
        let h = self.rise_height;
        self.drop_vel = drop_v0;
        self.drop_accel =
            (2.0 * h / (self.drop_frames * self.drop_frames) as f32 - drop_v0 / self.drop_frames as f32).max(0.0);
        self.drop_height = h;
        self.fall_frame = 0;
        self.drop_frame = 0;
        self.step_frame = 0;
        self.rise_frame = 0;
        self.logo_frame = 0;
        self.text_frame = 0;
        self.jingle_frame = 0;
        self.jingle_pending = true;
        self.running = true;
        self.land_pending = true;
        self.drop_pending = true;
        self.tiles.fill(Tile::default());
    }

    #[inline]
    pub fn tile(&self, f: usize, x: usize, y: usize) -> &Tile {
        &self.tiles[(f * GRID + x) * GRID + y]
    }

    fn path_step(&self) -> PathStep {
        self.path.get(self.step).copied().unwrap_or(PathStep { dir: DIR_END, group: 0, life: 0 })
    }

    /// Advance tile lifetimes by one path step.
    fn tick_tiles(&mut self) {
        for t in self.tiles.iter_mut() {
            if t.state == 2 {
                *t = Tile::default();
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

    /// Move one cell, or over an edge onto another face, and lay a tile.
    fn step_grid(&mut self) {
        match self.dir {
            0 => self.x = (self.x + 1).min(32),
            1 => self.y = (self.y + 1).min(32),
            2 => self.x = (self.x - 1).max(0),
            3 => self.y = (self.y - 1).max(0),
            d => {
                let face = d as i32 - 4;
                if (face + 3 - self.face as i32) % 3 == 1 && self.y == 16 {
                    self.y = self.x;
                    self.x = 16;
                    self.face = face as u8;
                }
                if (face + 3 - self.face as i32) % 3 == 2 && self.x == 16 {
                    self.x = self.y;
                    self.y = 16;
                    self.face = face as u8;
                }
            }
        }
        self.tick_tiles();
        let s = self.path_step();
        let (f, x, y) = (self.face as usize, self.x as usize, self.y as usize);
        if f < FACES && x < GRID && y < GRID {
            let t = &mut self.tiles[(f * GRID + x) * GRID + y];
            t.life = s.life;
            t.group = s.group;
            t.state = 1;
        }
    }

    /// Roll angle so far and its maximum (90 degrees flat, 180 over an edge).
    pub fn roll_angle(&self) -> (i32, i32) {
        if self.dir >= 4 {
            ((self.step_frame << 15) / (self.corner_frames - self.step_delay), 0x8000)
        } else {
            ((self.step_frame << 14) / (self.side_frames - self.step_delay), 0x4000)
        }
    }

    fn is_step_done(&mut self) -> bool {
        let (a, max) = self.roll_angle();
        if a > max && self.step_done_frame == 0 {
            self.step_done_frame = self.step_frame;
        }
        self.step_done_frame > 0 && self.step_frame >= self.step_done_frame + self.step_delay
    }

    fn update_fall(&mut self, scene: &mut Scene) -> bool {
        self.fall_height -= self.fall_vel;
        if self.fall_height < 0.0 {
            self.fall_height = 0.0;
            if self.land_pending {
                scene.land_impulse = scene.land_impulse0;
                scene.shake_phase = 0x7FFF;
                self.land_pending = false;
            }
        }
        self.fall_vel += self.fall_accel;
        if self.fall_frame > self.fall_frames + self.fall_delay {
            self.land_pending = true;
            return true;
        }
        self.fall_frame += 1;
        false
    }

    /// Returns the jingle to start this frame, if any.
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

    /// One logic frame. Returns a jingle to start, if this frame starts one.
    pub fn update(&mut self, scene: &mut Scene, egg: Jingle) -> Option<Jingle> {
        self.phase = self.next_phase;
        let jingle = self.update_jingle(egg);
        match self.phase {
            Phase::Fall => {
                if self.update_fall(scene) {
                    self.next_phase = Phase::Path;
                }
            }
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
                        self.dir = self.path_step().dir;
                    }
                }
                if self.dir == DIR_RISE {
                    self.next_phase = Phase::Rise;
                } else {
                    self.step_frame += 1;
                }
            }
            Phase::Rise => {
                self.running = false;
                self.rise_frame += 1;
                if self.rise_frame > self.rise_frames + self.rise_delay {
                    self.next_phase = Phase::Drop;
                }
            }
            Phase::Drop => {
                self.drop_height -= self.drop_vel;
                if self.drop_height < 0.0 {
                    self.drop_height = 0.0;
                    if self.drop_pending {
                        scene.drop_impulse = scene.drop_impulse0;
                        scene.shake_phase = 0x7FFF;
                        self.drop_pending = false;
                    }
                }
                self.drop_vel += self.drop_accel;
                if self.drop_frame > self.drop_frames + self.drop_delay {
                    self.drop_pending = true;
                    self.next_phase = Phase::Logo;
                } else {
                    self.drop_frame += 1;
                    self.text_frame = (self.text_frame + 1).min(self.text_frames + self.text_delay);
                }
            }
            Phase::Logo => {
                self.logo_frame += 1;
                if self.logo_frame > self.logo_frames {
                    self.logo_frame = self.logo_frames;
                    self.done = true;
                }
                scene.logo_alpha = (self.logo_frame * 0xFF / self.logo_frames) as u8;
                self.text_frame += 1;
                let mut text_done = false;
                if self.text_frame >= self.text_frames + self.text_delay {
                    self.text_frame = self.text_frames + self.text_delay;
                    text_done = true;
                }
                if !text_done || scene.drop_impulse != 0 {
                    self.done = false;
                }
            }
        }
        jingle
    }

    /// Model matrix of the small cube (before the world matrix), plus the
    /// world-space height to lift it by.
    pub fn small_cube_mtx(&self, world: &Mtx) -> Mtx {
        let half = (self.size / 2) as f32;
        let c3 = (self.size * 3 / 2) as f32;
        let mut m;
        match self.phase {
            Phase::Fall | Phase::Path => {
                // Roll about the leading bottom edge.
                let (mut a, max) = self.roll_angle();
                if a > max {
                    a = max;
                }
                m = psmtx::concat(&psmtx::IDENTITY, &psmtx::trans(-half, half, -half));
                m = psmtx::concat(&mtx::rot_axis('x', a), &m);
                // Point the roll along the travel direction and place on the grid.
                let yaw = match self.dir {
                    0 => -0x4000,
                    1 => 0x7FFF,
                    2 => 0x4000,
                    _ => 0,
                };
                m = psmtx::concat(&psmtx::trans(half, 0.0, half), &m);
                m = psmtx::concat(&mtx::rot_trans(0, yaw, 0, [-half, 0.0, -half]), &m);
                let sz = self.size as f32;
                m = psmtx::concat(
                    &psmtx::trans(-(self.x - 16) as f32 * sz, 0.0, -(self.y - 16) as f32 * sz),
                    &m,
                );
                if self.face == 1 {
                    m = psmtx::concat(&mtx::rot_trans(0x4000, 0, 0x4000, [0.0; 3]), &m);
                }
                if self.face == 2 {
                    m = psmtx::concat(&mtx::rot_trans(0, -0x4000, -0x4000, [0.0; 3]), &m);
                }
                m = psmtx::concat(&psmtx::trans(c3, c3, c3), &m);
                m = psmtx::concat(world, &m);
                if self.phase == Phase::Fall {
                    m = psmtx::concat(&psmtx::trans(0.0, self.fall_height, 0.0), &m);
                }
            }
            _ => {
                // Grow to 1.5x while flipping, then rise and drop.
                let rf = self.rise_frame as f32;
                let n = self.rise_frames as f32;
                let scale = (1.0 + 0.5 * rf / n).min(1.5);
                let max_a = self.rise_turns << 14;
                let a = (max_a * self.rise_frame / self.rise_frames).min(max_a);
                let sz = self.size as f32;
                let lift = (half - (rf * ((1.0 + scale) * sz) / n) * 0.5).max(-(scale * sz * 0.5));
                m = psmtx::scale(scale, scale, scale);
                m = psmtx::concat(&mtx::rot_trans(a, 0, 0, [scale * -sz * 0.5, lift, scale * -sz * 0.5]), &m);
                m = psmtx::concat(&mtx::rot_trans(0, -0x4000, -0x4000, [0.0; 3]), &m);
                m = psmtx::concat(&psmtx::trans(c3, c3, c3), &m);
                m = psmtx::concat(world, &m);
                let y = if self.phase == Phase::Rise {
                    let r = (self.rise_frames - self.rise_frame).max(0) as f32;
                    let h = self.rise_height;
                    (h - r * r * r * (h * r) / (n * n * n * n)).min(h)
                } else {
                    self.drop_height
                };
                m = psmtx::concat(&psmtx::trans(0.0, y, 0.0), &m);
            }
        }
        m
    }

    /// The wordmark animation frame (0..60).
    pub fn text_anim_frame(&self) -> f32 {
        if self.text_frame > self.text_delay {
            60.0 * ((self.text_frame - self.text_delay) as f32 / self.text_frames as f32)
        } else {
            0.0
        }
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
