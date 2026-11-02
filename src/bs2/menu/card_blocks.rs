//! The Memory Card blocks (`memcard_draw.c`, PAL 1.0 BS2
//! 0x8131EDBC..0x81321730): every directory entry of both cards is a small
//! glass block on a 4 x 4 window of the grid. Blocks fly in from the far
//! corner when the screen opens, zoom when chosen, fly off on Copy / Move
//! and shrink and burst into 12 pieces on Erase. The tint shows the card's
//! sector size.
//!
//! Positions are 12.4 layout units, as in BS2. The model matrices are
//! worked out at the start of each update ([`Blocks::update`] runs
//! `CardBlock_CalcMtx` before it moves anything), so the draw shows the
//! blocks where they were a frame earlier, like on the console.

use super::backend::DIR_ENTRIES;
use super::super::gmr::{self, Instance};
use super::super::render::{Prepared, View};
use super::super::shader::FastTex;
use super::Work;
use crate::bs2::gbase::hermite;
use crate::bs2::mtx::{self, cos_s, sin_s, to_s16, vec_hermite};
use crate::bs2::rnd;
use crate::sdk::gx::Gx;
use crate::sdk::mtx::{self as psmtx, Mtx, Vec3};
use alloc::vec;
use alloc::vec::Vec;

const N: usize = DIR_ENTRIES;
const BURST: usize = 12;

/// `CardParams` (CardParams_SetDefaults): block zoom, resting and chosen,
/// and the chosen block's sway.
const ICON_ZOOM: f32 = 2.0;
const ICON_ZOOM_SEL: f32 = 1.3;
const SWAY_X: f32 = 10.0;
const SWAY_Y: f32 = 5.0;

/// `CARD_CMD_*` kinds the blocks act on: Copy, Move (3) and Erase.
pub const CMD_COPY: u16 = 2;
pub const CMD_ERASE: u16 = 4;

/// Sector sizes a card can have, and the block colours for each
/// (`CardBlock_Color`): chosen save, save, chosen free entry, free entry.
const SIZES: [u32; 6] = [0x2000, 0x4000, 0x8000, 0x10000, 0x20000, 0x40000];
const COLORS: [[[i16; 3]; 4]; 6] = [
    [[100, 150, 255], [20, 50, 200], [30, 80, 100], [0, 20, 60]],
    [[100, 255, 110], [0, 105, 70], [50, 120, 80], [0, 37, 0]],
    [[200, 255, 75], [80, 120, 0], [90, 110, 0], [28, 34, 0]],
    [[255, 180, 50], [160, 100, 0], [100, 80, 0], [50, 30, 0]],
    [[255, 110, 100], [160, 45, 50], [150, 60, 50], [50, 20, 0]],
    [[200, 80, 190], [90, 30, 170], [100, 40, 90], [45, 0, 60]],
];

/// `CardBlock_Color(sectorSize, kind, slot)`: 0 the chosen save, 1 another
/// save, 2 the chosen empty block, 3 another empty block.
fn color(sector_size: u32, kind: usize) -> [i16; 4] {
    let s = SIZES.iter().position(|&s| s == sector_size).unwrap_or(0);
    let c = COLORS[s][kind];
    [c[0], c[1], c[2], 255]
}

/// The block models: a save (res 88) and an empty block (res 87).
pub struct Models {
    file: Prepared,
    empty: Prepared,
}

impl Models {
    pub fn new(file: Prepared, empty: Prepared) -> Models {
        Models { file, empty }
    }
}

/// `CardBlock` (0x5C bytes).
#[derive(Clone, Copy)]
struct Block {
    /// The frame the flight starts and ends.
    start: i16,
    frame: i16,
    end: i16,
    alpha: i16,
    /// 255 fades in, 0 fades out.
    alpha_target: i16,
    zoom_frame: i16,
    zoom_len: i16,
    zoom: f32,
    zoom_min: f32,
    zoom_max: f32,
    pos: Vec3,
    vel: Vec3,
    /// Flight: start velocity and position, end position.
    v0: Vec3,
    p0: Vec3,
    p1: Vec3,
}

impl Block {
    /// CardBlock_Init (0x8131EDBC).
    fn new() -> Block {
        let mut b = Block {
            start: 0,
            frame: 0,
            end: 0,
            alpha: 0,
            alpha_target: 0,
            zoom_frame: 0,
            zoom_len: 6,
            zoom: ICON_ZOOM_SEL,
            zoom_min: ICON_ZOOM_SEL,
            zoom_max: ICON_ZOOM,
            pos: [0.0; 3],
            vel: [0.0; 3],
            v0: [0.0; 3],
            p0: [0.0; 3],
            p1: [0.0; 3],
        };
        b.rewind();
        b
    }

    /// CardBlock_Rewind (0x8132005C).
    fn rewind(&mut self) {
        self.start = 0;
        self.frame = self.start;
        self.end = 40;
    }

    /// CardBlock_Finish (0x81320078).
    fn finish(&mut self) {
        self.start = 0;
        self.end = 40;
        self.frame = self.end;
    }

    /// CardBlock_Move: one frame of the Hermite flight.
    fn step(&mut self) {
        let p = if self.frame >= self.start {
            vec_hermite(self.p0, self.v0, self.p1, [0.0; 3], self.frame as f32, self.start as f32, self.end as f32)
        } else {
            self.p0
        };
        self.vel = core::array::from_fn(|k| p[k] - self.pos[k]);
        self.pos = p;
        self.frame += 1;
        if self.frame > self.end {
            self.frame = self.end;
        }
    }

    fn zoom_step(&mut self) {
        self.zoom = hermite(self.zoom_frame as f32, 0.0, self.zoom_min, 0.0, self.zoom_len as f32, self.zoom_max, 0.0);
    }

    fn zoom_clamp(&mut self) {
        if self.zoom_frame > self.zoom_len {
            self.zoom_frame = self.zoom_len;
        }
        if self.zoom_frame < 0 {
            self.zoom_frame = 0;
        }
    }

    /// How far the block has zoomed, 0..1.
    fn t(&self) -> f32 {
        (self.zoom - self.zoom_min) / (self.zoom_max - self.zoom_min)
    }

    /// CardBlock_IsHome.
    fn is_home(&self) -> bool {
        self.frame == self.end
    }
}

/// One piece of the Erase burst.
#[derive(Clone, Copy, Default)]
struct Burst {
    frame: i16,
    len: i16,
    delay: i16,
    angle: i16,
}

impl Burst {
    /// The progress the piece flies at (CardBlock_CalcBurst).
    fn t(&self) -> f32 {
        if self.frame > self.delay && self.frame - self.delay <= self.len {
            (self.frame - self.delay) as f32 / self.len as f32
        } else {
            0.0
        }
    }

    /// The piece's alpha out of `alpha` (CardBlock_DrawBursts).
    fn alpha(&self, alpha: u8) -> u8 {
        if self.frame > self.delay && self.frame - self.delay <= self.len {
            (alpha as i32 * (self.delay + (self.len - self.frame)) as i32 / self.len as i32) as u8
        } else {
            0
        }
    }
}

/// A model matrix and the scale `J3D_SetModelMtx` gets with it.
#[derive(Clone, Copy)]
struct Place {
    m: Mtx,
    scale: f32,
}

const NO_PLACE: Place = Place { m: psmtx::IDENTITY, scale: 1.0 };

/// What the blocks read from the card screen and the card thread.
pub struct Sel {
    /// Card_GetSlot, Card_GetSelectedFile.
    pub slot: usize,
    pub file: usize,
}

/// What a slot's draw reads (CardThread_GetFiles, CardThread_GetSlots and
/// the icons).
pub struct SlotInfo<'a, 'b> {
    pub used: &'b dyn Fn(usize) -> bool,
    /// The save's current icon frame (CardDraw_IconFrameCount and the
    /// texture CardDraw_SetIconFrame loads).
    pub icon: &'b dyn Fn(usize) -> Option<&'a FastTex>,
    pub sector_size: u32,
}

/// `memcard_draw.c`'s statics.
pub struct Blocks {
    blocks: [Vec<Block>; 2],
    /// The file / empty instances' matrices (sFileInst, sEmptyInst).
    place: [Vec<Place>; 2],
    fly: Block,
    /// sFlyInst.
    fly_place: Place,
    burst: [Burst; BURST],
    burst_place: [Place; BURST],
    fly_mtx: Mtx,
    /// Grid cells (12.4).
    grid_x: [[i16; 16]; 2],
    grid_y: [[i16; 16]; 2],
    op_slot: usize,
    op_dest: u16,
    op_file: usize,
    op_kind: u16,
    flying: bool,
    entering: bool,
    erasing: bool,
    bursting: bool,
    wobble: i16,
}

impl Blocks {
    /// fn_8131EEC4 (0x8131EEC4).
    pub fn new(grid_x: [[i16; 16]; 2], grid_y: [[i16; 16]; 2]) -> Blocks {
        Blocks {
            blocks: [vec![Block::new(); N], vec![Block::new(); N]],
            place: [vec![NO_PLACE; N], vec![NO_PLACE; N]],
            fly: Block::new(),
            fly_place: NO_PLACE,
            burst: [Burst::default(); BURST],
            burst_place: [NO_PLACE; BURST],
            fly_mtx: psmtx::IDENTITY,
            grid_x,
            grid_y,
            op_slot: 0,
            op_dest: 0,
            op_file: 0,
            op_kind: 0,
            flying: false,
            entering: true,
            erasing: false,
            bursting: false,
            wobble: 0,
        }
    }

    /// fn_81321714: the blocks have flown in.
    pub fn entered(&self) -> bool {
        !self.entering
    }

    /// The sway and wobble of a zoomed block at 12.4 `pos`, zoom `t`.
    fn place(&self, w: &Work, pos: Vec3, t: f32, zoom: f32) -> Place {
        let wb = self.wobble as i32;
        let pos = [pos[0] / 16.0, -pos[1] / 16.0, 2.0 * t];
        let rot = mtx::rot(
            to_s16(t * w.wobble_amp[0] as f32 * cos_s(wb * 70)) as i32,
            to_s16(t * w.wobble_amp[1] as f32 * cos_s(w.wobble_phase_ry as i32 + wb * 35)) as i32,
            to_s16(t * w.wobble_amp[2] as f32 * cos_s(wb * 35)) as i32,
        );
        let m = psmtx::trans(pos[0] + t * SWAY_X * sin_s(w.wobble_phase_tx as i32 + wb * 35), pos[1] - t * SWAY_Y * sin_s(wb * 70), pos[2]);
        // MtxFromSRT of the 2D plane's SRT: a move by (-292, 224).
        let m = psmtx::concat(&m, &psmtx::trans(-292.0, 224.0, 0.0));
        Place { m: psmtx::concat(&m, &rot), scale: zoom }
    }

    /// CardBlock_CalcMtx (0x8131F048).
    fn calc_mtx(&mut self, w: &Work) {
        // A block that is flying while an Erase runs keeps the previous
        // block's place, as the C's stale locals do.
        let (mut pos, mut t) = ([0.0; 3], 0.0);
        for slot in 0..2 {
            for i in 0..N {
                let zoom = self.blocks[slot][i].zoom;
                if slot == self.op_slot && i == self.op_file && self.flying {
                    if !self.erasing {
                        t = self.fly.t();
                        pos = self.fly.pos;
                    }
                } else {
                    t = self.blocks[slot][i].t();
                    pos = self.blocks[slot][i].pos;
                }
                self.place[slot][i] = self.place(w, pos, t, zoom);
            }
        }
        if self.erasing {
            let p = self.place(w, self.fly.pos, self.fly.t(), self.fly.zoom);
            if !self.bursting {
                self.fly_place = p;
            } else {
                self.fly_mtx = p.m;
            }
        }
    }

    /// CardBlock_CalcBurst (0x8131F770).
    fn calc_burst(&mut self, i: usize) {
        let b = &self.burst[i];
        let r = 48.0 * (0.25 + 0.75 * b.t());
        let m = psmtx::trans(r * sin_s(b.angle as i32), r * cos_s(b.angle as i32), 0.0);
        self.burst_place[i] = Place { m: psmtx::concat(&self.fly_mtx, &m), scale: 0.3 };
    }

    /// fn_8131F8F8 (0x8131F8F8): one frame, once the card icon has
    /// settled. `sel` is the cursor; `fade_done` Menu_CardFadeDone;
    /// `ready` each slot's `CARD_SLOT_READY`.
    pub fn update(&mut self, w: &Work, icon_settled: bool, sel: &Sel, fade_done: bool, ready: [bool; 2]) {
        if !icon_settled {
            return;
        }
        self.calc_mtx(w);
        if self.erasing {
            let f = &mut self.fly;
            f.zoom_step();
            f.zoom_frame -= 1;
            if self.flying {
                f.zoom_clamp();
            } else {
                // CardBlock_Shrink
                let gone = f.zoom_frame < 0;
                f.zoom_clamp();
                if gone {
                    if !self.bursting {
                        for b in self.burst.iter_mut() {
                            *b = Burst { frame: 0, len: 10, delay: (rnd::rand() & 3) as i16, angle: rnd::rand() as u16 as i16 };
                        }
                    }
                    self.bursting = true;
                    // CardBlock_StepBurst
                    let mut n = 0;
                    for i in 0..BURST {
                        let b = &mut self.burst[i];
                        b.frame += 1;
                        if b.frame > b.delay && b.frame - b.delay > b.len {
                            b.frame = b.len + b.delay;
                            n += 1;
                        }
                        self.calc_burst(i);
                    }
                    if n == BURST {
                        self.bursting = false;
                        self.erasing = false;
                    }
                }
            }
        } else {
            let step = if self.entering { 25 } else { 8 };
            for slot in 0..2 {
                for i in 0..N {
                    let b = &mut self.blocks[slot][i];
                    if b.frame >= b.start {
                        b.alpha += if b.alpha_target == 255 { step } else { -step };
                    }
                    b.alpha = b.alpha.clamp(0, 255);
                    b.step();
                    b.zoom_step();
                    b.zoom_frame += if slot == sel.slot && i == sel.file && !self.entering { 1 } else { -1 };
                    b.zoom_clamp();
                }
            }
            if self.flying {
                let f = &mut self.fly;
                f.step();
                if f.frame >= f.end - 5 {
                    // On around the point (4672, 3200), the screen's middle.
                    f.p0 = f.pos;
                    f.v0 = f.vel;
                    if f.p1[0] >= 4672.0 {
                        f.p1[0] -= 2.0 * (f.p1[0] - 4672.0);
                    } else {
                        f.p1[0] += 2.0 * (4672.0 - f.p1[0]);
                    }
                    if f.p1[1] >= 3200.0 {
                        f.p1[1] -= 2.0 * (f.p1[1] - 3200.0);
                    } else {
                        f.p1[1] += 2.0 * (3200.0 - f.p1[1]);
                    }
                    f.frame = 0;
                    f.end = 180;
                }
            }
        }
        if self.entering && self.settled(fade_done, ready) {
            for b in self.blocks.iter_mut().flatten() {
                b.finish();
            }
            self.entering = false;
        }
        self.wobble = self.wobble.wrapping_add(7);
    }

    /// fn_81320094 (0x81320094): Copy / Move send the chosen block off,
    /// Erase shrinks it. `dest` is its grid cell.
    pub fn start(&mut self, slot: usize, dest: u16, file: usize, kind: u16) {
        self.op_dest = dest;
        self.op_slot = slot;
        self.op_file = file;
        self.op_kind = kind;
        if !(CMD_COPY..=CMD_ERASE).contains(&kind) {
            return;
        }
        let src = &mut self.blocks[slot][file];
        self.fly = *src;
        src.alpha = 0;
        src.alpha_target = 0;
        if kind != CMD_ERASE {
            let f = &mut self.fly;
            f.p0 = f.pos;
            f.v0 = f.vel;
            let a = rnd::rand() as u16 as i32;
            f.p1 = [16.0 * (292.0 + 40.0 * sin_s(a)), 16.0 * (200.0 + 40.0 * cos_s(a)), 0.0];
            f.frame = 0;
            f.end = 40;
        } else {
            self.fly.zoom_frame = 20;
            self.fly.zoom_len = 20;
            self.fly.zoom_min = 0.0;
            self.erasing = true;
            self.bursting = false;
        }
        self.flying = true;
    }

    /// fn_8132030C (0x8132030C): the flying block lands on the other card
    /// as entry `file`.
    pub fn land(&mut self, file: usize) {
        if !(CMD_COPY..=CMD_ERASE).contains(&self.op_kind) {
            return;
        }
        self.flying = false;
        let (s, i) = (self.op_slot, self.op_file);
        if self.op_kind != CMD_ERASE {
            let pos = self.fly.pos;
            let d = &mut self.blocks[s ^ 1][file];
            d.pos = pos;
            d.vel = [0.0, if self.op_dest < 8 { 150.0 } else { -150.0 }, 0.0];
            d.zoom = d.zoom_max;
            d.zoom_frame = d.zoom_len;
            d.alpha = 255;
            d.alpha_target = 255;
            d.end = 60;
            if self.op_kind == CMD_COPY {
                self.blocks[s][i].pos = pos;
            }
        }
        if self.op_kind != CMD_COPY {
            self.blocks[s][i].alpha = 0;
            self.blocks[s][i].alpha_target = 0;
        }
    }

    /// fn_813204B0 (0x813204B0): the operation failed, the block goes
    /// back home.
    pub fn cancel(&mut self) {
        if !self.flying {
            return;
        }
        self.flying = false;
        if self.erasing {
            self.erasing = false;
            self.bursting = false;
        }
        let pos = self.fly.pos;
        let b = &mut self.blocks[self.op_slot][self.op_file];
        b.pos = pos;
        b.p0 = pos;
        b.frame = 0;
        b.end = 40;
        b.alpha = 255;
        b.alpha_target = 255;
    }

    /// fn_8132056C (0x8132056C): send every block to its cell, or just off
    /// the window (fading) when scrolled away. `order` is each slot's
    /// entries in grid order, `row` the first row shown.
    pub fn arrange(&mut self, order: &[Vec<usize>; 2], row: [i8; 2]) {
        for slot in 0..2 {
            let (gx, gy) = (self.grid_x[slot], self.grid_y[slot]);
            let first = row[slot] as i32 * 4;
            for i in 0..N {
                let b = &mut self.blocks[slot][order[slot][i]];
                b.rewind();
                let k = i as i32;
                b.p0 = b.pos;
                b.v0 = b.vel;
                if k >= first && k <= first + 15 {
                    b.alpha_target = 255;
                    b.p1[0] = gx[(k - first) as usize] as f32;
                    b.p1[1] = gy[(k - first) as usize] as f32;
                } else {
                    b.alpha_target = 0;
                    if k < first {
                        b.p1[0] = gx[i & 3] as f32;
                        b.p1[1] = gy[i & 3] as f32 - 512.0;
                    }
                    if k > first + 15 {
                        b.p1[0] = gx[(i & 3) + 12] as f32;
                        b.p1[1] = 512.0 + gy[(i & 3) + 12] as f32;
                    }
                }
                b.p1[2] = 0.0;
            }
        }
    }

    /// CardIcon_ResetFiles (0x813207C8): every block starts in the far
    /// corner, thrown in a random direction, and flies in after a random
    /// delay.
    pub fn reset_files(&mut self) {
        for b in self.blocks.iter_mut().flatten() {
            b.pos = [4672.0, 3584.0, 0.0];
            let a = rnd::rand() as u16 as i32;
            b.vel = [350.0 * sin_s(a), 350.0 * cos_s(a), 0.0];
            b.p0 = b.pos;
            b.v0 = b.vel;
            b.alpha = 0;
            b.frame = 0;
            let delay = (rnd::rand() & 15) as i16;
            b.start = delay;
            b.end = delay + 33;
        }
        self.entering = true;
    }

    /// fn_81320920 (0x81320920): a slot with no usable card fades its
    /// blocks out, then gathers them in the middle of its window.
    pub fn hide_slot(&mut self, slot: usize) {
        let mut n = 0;
        for b in self.blocks[slot].iter_mut() {
            b.alpha_target = 0;
            if b.alpha == 0 {
                n += 1;
            }
        }
        if n == N {
            let (gx, gy) = (&self.grid_x[slot], &self.grid_y[slot]);
            let c = [((gx[5] as i32 + gx[6] as i32) / 2) as f32, ((gy[5] as i32 + gy[9] as i32) / 2) as f32, 0.0];
            for b in self.blocks[slot].iter_mut() {
                b.pos = c;
                b.vel = [0.0; 3];
                b.p0 = b.pos;
                b.v0 = b.vel;
                b.alpha = 0;
                b.frame = 0;
            }
        }
    }

    /// fn_81320ABC (0x81320ABC): every block is home and the screen has
    /// faded in. Also makes each block's place the start of its next move.
    pub fn settled(&mut self, fade_done: bool, ready: [bool; 2]) -> bool {
        if !fade_done {
            return false;
        }
        let mut n = 0;
        for slot in 0..2 {
            n += if ready[slot] { self.blocks[slot].iter().filter(|b| b.is_home()).count() } else { N };
        }
        if n == N * 2 && !self.erasing {
            for slot in (0..2).filter(|&s| ready[s]) {
                for b in self.blocks[slot].iter_mut() {
                    b.p0 = b.pos;
                    b.v0 = b.vel;
                }
            }
            return true;
        }
        false
    }

    /// CardBlock_DrawSlot (0x81321378): one slot's blocks, then the chosen
    /// one or the flying one, then the erased one or its burst. The last
    /// three are drawn on every slot's call, as in BS2.
    #[allow(clippy::too_many_arguments)]
    pub fn draw_slot<'a>(&self, gx: &mut Gx<'a>, v: &View, models: &'a Models, slot: usize, alpha: u8, sel: &Sel, info: &[SlotInfo<'a, '_>; 2]) {
        let m = models;
        let me = &info[slot];
        for i in 0..N {
            let a = (alpha as i32 * self.blocks[slot][i].alpha as i32 / 255) as u8;
            let p = &self.place[slot][i];
            if (me.used)(i) {
                let hidden = (slot == self.op_slot && i == self.op_file && self.flying) || (slot == sel.slot && i == sel.file && !self.entering);
                if !hidden {
                    self.draw_file(gx, v, &m.file, p, a, slot, i, sel, me);
                }
            } else {
                let kind = if slot == sel.slot && i == sel.file && !self.entering { 2 } else { 3 };
                draw_empty(gx, v, &m.empty, p, a, color(me.sector_size, kind));
            }
        }
        if self.flying {
            if !self.erasing {
                let s = self.op_slot;
                if (info[s].used)(self.op_file) {
                    let a = (self.fly.alpha as i32 * alpha as i32 / 255) as u8;
                    self.draw_file(gx, v, &m.file, &self.place[s][self.op_file], a, s, self.op_file, sel, &info[s]);
                }
            }
        } else if !self.entering {
            let (s, i) = (sel.slot, sel.file);
            if (info[s].used)(i) {
                let a = (alpha as i32 * self.blocks[s][i].alpha as i32 / 255) as u8;
                self.draw_file(gx, v, &m.file, &self.place[s][i], a, s, i, sel, &info[s]);
            }
        }
        if self.erasing {
            let size = info[self.op_slot].sector_size;
            if !self.bursting {
                let a = (self.fly.alpha as i32 * alpha as i32 / 255) as u8;
                // File -1: no icon.
                draw_block(gx, v, &m.file, &self.fly_place, a, color(size, 1), None);
            } else {
                // CardBlock_DrawBursts
                for (b, p) in self.burst.iter().zip(self.burst_place.iter()) {
                    let a = b.alpha(alpha);
                    if a != 0 {
                        draw_block(gx, v, &m.file, p, a, color(size, 1), None);
                    }
                }
            }
        }
    }

    /// CardBlock_DrawFile (0x81320FD8).
    #[allow(clippy::too_many_arguments)]
    fn draw_file<'a>(&self, gx: &mut Gx<'a>, v: &View, p: &'a Prepared, at: &Place, alpha: u8, slot: usize, file: usize, sel: &Sel, info: &SlotInfo<'a, '_>) {
        let kind = if slot == sel.slot && file == sel.file && !self.entering { 0 } else { 1 };
        draw_block(gx, v, p, at, alpha, color(info.sector_size, kind), (info.icon)(file));
    }
}

/// An instance of a block model at `at` with the view without the
/// camera's height offset (`fn_81302F94`).
fn instance<'a>(p: &'a Prepared, at: &Place, v: &View, alpha: u8, color: [i16; 4], mats: [u16; 2]) -> Instance {
    let mut inst = Instance::new(&p.model);
    gmr::set_model_mtx(&mut inst, &at.m, [at.scale; 3]);
    gmr::set_view_mtx(&mut inst, &v.view);
    gmr::calc_joints(&p.model, &mut inst);
    inst.alpha = alpha;
    inst.tev_color = vec![(mats[0], 0, color), (mats[1], 0, color)];
    inst
}

/// A save's block in `color` (materials 0 and 2), packets 0x30 and 0xF0,
/// then its icon on the face (packet 0x90 with material 1 drawing
/// whatever TEXMAP0 holds).
fn draw_block<'a>(gx: &mut Gx<'a>, v: &View, p: &'a Prepared, at: &Place, alpha: u8, color: [i16; 4], icon: Option<&'a FastTex>) {
    if alpha == 0 {
        return;
    }
    let mut inst = instance(p, at, v, alpha, color, [0, 2]);
    gmr::draw_packet(gx, p, &mut inst, 0x30 / 0x18);
    gmr::draw_packet(gx, p, &mut inst, 0xF0 / 0x18);
    if let Some(tex) = icon {
        inst.tev_color.clear();
        inst.no_tex0 = Some(1);
        gx.load_tex_obj(tex, 0);
        gmr::draw_packet(gx, p, &mut inst, 0x90 / 0x18);
    }
}

/// CardBlock_DrawEmpty (0x81321168): an empty block in `color`
/// (materials 0 and 1).
fn draw_empty<'a>(gx: &mut Gx<'a>, v: &View, p: &'a Prepared, at: &Place, alpha: u8, color: [i16; 4]) {
    if alpha == 0 {
        return;
    }
    let mut inst = instance(p, at, v, alpha, color, [0, 1]);
    gmr::draw(gx, p, &mut inst);
}
