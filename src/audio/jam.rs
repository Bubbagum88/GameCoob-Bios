//! The Jam sequence player (`jaudio/jammain_2.c`, `seqsetup.c`,
//! `noteon.c`): runs `iplrom.com`, a tree of tracks that play notes, wait,
//! and react to the ports BS2 writes. Ported call for call; the C names are
//! kept in the comments so each part can be checked against the decomp.
//!
//! Track 0 is the root the IPL starts (`lbl_815D3C68`, not from the pool);
//! the rest is the pool of 256 `seqp_` the sequence opens children from.

use super::Engine;
use super::bank::Osc;
use super::voice::{Ctl, DAC_RATE, SUBFRAME};

#[derive(Clone, Copy, Default)]
struct Move {
    cur: f32,
    target: f32,
    dur: f32,
    step: f32,
}

#[derive(Clone, Copy, Default)]
struct Port {
    import: bool,
    export: bool,
    value: u16,
}

/// `seqp_`.
pub struct Track {
    /// 0 free, 1 running, 2 set up, 3 stopping.
    pub state: u8,
    allocated: bool,
    pc: usize,
    stack: [usize; 8],
    loops: [u16; 8],
    depth: usize,
    flags: u8,
    pub parent: Option<usize>,
    children: [Option<usize>; 16],
    id: u32,
    conn: u32,
    wait: i64,
    note_played: bool,
    notes: [u8; 8],
    chans: [Option<usize>; 8],
    chan_ids: [u16; 8],
    gate: u8,
    note_dur: i64,
    note_flags: u8,
    last_note: u8,
    gate_mode: bool,
    pub ctl: Ctl,
    mv: [Move; 18],
    reg: [u16; 32],
    ports: [Port; 16],
    tempo_acc: f32,
    tempo_factor: f32,
    time_base: u16,
    tempo: u16,
    time_rel: u8,
    pub osc: [Osc; 2],
    pub route: [u8; 2],
    transpose: i8,
    final_transpose: u8,
    muted: u8,
    child_mute: u16,
    int_active: u8,
    int_pending: u8,
    int_enable: u8,
    timer_count: u8,
    int_addr: [usize; 8],
    saved_pc: usize,
    saved_wait: i64,
    timer: u32,
    max_time: u32,
    update_flags: u32,
    pan_calc: [u8; 3],
    parent_pan_calc: [u8; 3],
    registered: bool,
    tempo_sync: bool,
}

impl Track {
    pub fn new() -> Track {
        Track {
            state: 0,
            allocated: false,
            pc: 0,
            stack: [0; 8],
            loops: [0; 8],
            depth: 0,
            flags: 0,
            parent: None,
            children: [None; 16],
            id: 0,
            conn: 0,
            wait: 0,
            note_played: false,
            notes: [0xFF; 8],
            chans: [None; 8],
            chan_ids: [0; 8],
            gate: 0,
            note_dur: 0,
            note_flags: 0,
            last_note: 0,
            gate_mode: false,
            ctl: Ctl::new(),
            mv: [Move::default(); 18],
            reg: [0; 32],
            ports: [Port::default(); 16],
            tempo_acc: 0.0,
            tempo_factor: 1.0,
            time_base: 48,
            tempo: 120,
            time_rel: 1,
            osc: [Osc::default(), Osc::default()],
            route: [15, 15],
            transpose: 0,
            final_transpose: 0,
            muted: 0,
            child_mute: 0,
            int_active: 0,
            int_pending: 0,
            int_enable: 0,
            timer_count: 0,
            int_addr: [0; 8],
            saved_pc: 0,
            saved_wait: 0,
            timer: 0,
            max_time: 0,
            update_flags: 0,
            pan_calc: [2; 3],
            parent_pan_calc: [2; 3],
            registered: false,
            tempo_sync: false,
        }
    }

    fn args(&self, i: usize) -> u16 {
        self.reg[8 + i]
    }
}

/// Track id `Sound_*` write to, from `ConnectName`.
pub const POOL: usize = 256;

/// `Arglist`: argument count and 2-bit types (0 byte, 1 word, 2 24-bit,
/// 3 register) of commands 0xC0..0xFF.
const ARGS: [(u8, u16); 64] = [
    (0, 0),
    (2, 0x8),
    (2, 0x8),
    (1, 0x2),
    (0, 0),
    (0, 0),
    (1, 0),
    (1, 0x2),
    (0, 0),
    (1, 0x1),
    (0, 0),
    (2, 0),
    (2, 0xc),
    (1, 0),
    (1, 0),
    (1, 0x3),
    (2, 0x5),
    (2, 0xc),
    (2, 0xc),
    (2, 0xf),
    (1, 0),
    (1, 0),
    (1, 0),
    (2, 0x8),
    (5, 0x155),
    (1, 0),
    (1, 0),
    (1, 0),
    (1, 0x1),
    (2, 0x4),
    (1, 0),
    (2, 0x8),
    (1, 0),
    (0, 0),
    (0, 0),
    (0, 0),
    (2, 0x4),
    (0, 0),
    (0, 0),
    (1, 0x1),
    (0, 0),
    (0, 0),
    (1, 0x2),
    (5, 0),
    (4, 0x55),
    (1, 0x2),
    (1, 0x2),
    (3, 0),
    (1, 0),
    (1, 0),
    (3, 0x28),
    (0, 0),
    (0, 0),
    (0, 0),
    (0, 0),
    (0, 0),
    (0, 0),
    (0, 0),
    (1, 0x1),
    (0, 0),
    (0, 0),
    (1, 0x1),
    (1, 0x1),
    (0, 0),
];

/// What a command tells the note loop (`Cmd_Process` results).
#[derive(PartialEq)]
enum Flow {
    Next,
    Wait,
    Retry,
    Finish,
}

/// `__PanCalc`.
fn blend(from: f32, to: f32, ratio: f32, kind: u8) -> f32 {
    match kind {
        0 => from,
        1 => to,
        _ => from * (1.0 - ratio) + to * ratio,
    }
}

/// `C5BASE_PITCHTABLE[i]`: 2^((i - 60) / 12).
pub fn key_pitch(i: i32) -> f32 {
    libm::exp2f((i.clamp(0, 127) - 60) as f32 / 12.0)
}

/// `Jam_PitchToCent`: a pitch parameter (-1..1) times the bend range in
/// semitones, as a frequency ratio.
fn pitch_to_cent(p: f32, scale: f32) -> f32 {
    let s = 4.0 * p * scale;
    let mut idx = s as i32;
    let mut frac = s - idx as f32;
    if s < 0.0 && frac != 0.0 {
        frac += 1.0;
        idx -= 1;
    }
    if frac >= 1.0 {
        frac -= 1.0;
        idx += 1;
    }
    // KEY_TABLE: 64 steps across one semitone.
    let fine = libm::exp2f(((frac * 64.0) as u16 as f32 / 64.0) / 12.0);
    key_pitch(idx + 60) * fine
}

fn ext8(v: u8) -> u16 {
    v as i8 as i16 as u16
}

impl Engine {
    fn byte(&self, ofs: usize) -> u8 {
        self.archive.seq.get(ofs).copied().unwrap_or(0xFF)
    }

    fn rd8(&mut self, t: usize) -> u8 {
        let pc = self.tracks[t].pc;
        self.tracks[t].pc += 1;
        self.byte(pc)
    }

    fn rd16(&mut self, t: usize) -> u16 {
        (self.rd8(t) as u16) << 8 | self.rd8(t) as u16
    }

    fn rd24(&mut self, t: usize) -> u32 {
        (self.rd8(t) as u32) << 16 | (self.rd8(t) as u32) << 8 | self.rd8(t) as u32
    }

    fn rd_ofs(&self, ofs: usize, n: usize) -> u32 {
        (0..n).fold(0, |a, i| a << 8 | self.byte(ofs + i) as u32)
    }

    // ---- registers ----

    /// `Jam_WriteRegDirect`.
    fn write_reg_direct(&mut self, t: usize, idx: u8, value: u16) {
        let (idx, value, shown) = match idx {
            0..=2 => {
                let v = value & 0xff;
                (idx, v, ext8(v as u8))
            }
            32 | 33 => return,
            34 => {
                self.write_reg_direct(t, 0, value >> 8);
                (1, value & 0xff, value)
            }
            _ => (idx, value, value),
        };
        let tr = &mut self.tracks[t];
        if let Some(r) = tr.reg.get_mut(idx as usize) {
            *r = value;
        }
        tr.reg[3] = shown;
    }

    /// `Jam_ReadRegDirect`.
    fn read_reg_direct(&mut self, t: usize, idx: u8) -> u16 {
        let r = match idx {
            0x20 | 0x21 => self.tracks[t].reg[6],
            0x22 => {
                let lo = self.read_reg_direct(t, 1);
                let hi = self.read_reg_direct(t, 0);
                hi << 8 | lo
            }
            0x2C => {
                let mut r = 0u16;
                for i in (0..16).rev() {
                    r <<= 1;
                    if let Some(c) = self.tracks[t].children[i] {
                        if self.tracks[c].state != 0 {
                            r |= 1;
                        }
                    }
                }
                r
            }
            0x2D => {
                let mut r = 0u16;
                for i in (0..8).rev() {
                    r <<= 1;
                    r |= self.check_note_stop(t, i) as u16;
                }
                r
            }
            0x30 => {
                let tr = &self.tracks[t];
                if tr.depth == 0 { 0 } else { tr.loops[tr.depth - 1] }
            }
            _ => self.tracks[t].reg.get(idx as usize).copied().unwrap_or(0),
        };
        match idx {
            0 | 1 | 2 | 0x21 => r & 0xff,
            0x20 => r >> 8,
            _ => r,
        }
    }

    fn ext_reg(&self, t: usize, i: usize) -> u32 {
        let r = &self.tracks[t].reg;
        (r[16 + 2 * i] as u32) << 16 | r[17 + 2 * i] as u32
    }

    /// `Jam_ReadReg32`.
    fn read_reg32(&mut self, t: usize, idx: u8) -> u32 {
        match idx {
            40..=43 => self.ext_reg(t, idx as usize - 40),
            35 => (self.read_reg_direct(t, 4) as u32) << 16 | self.read_reg_direct(t, 5) as u32,
            _ => self.read_reg_direct(t, idx) as u32,
        }
    }

    /// `__ExchangeRegisterValue`: a register, or a port from 64 up.
    fn exch(&mut self, t: usize, r: u8) -> u32 {
        if r < 64 { self.read_reg32(t, r) } else { self.tracks[t].ports[(r - 64) as usize & 15].value as u32 }
    }

    /// `Jam_WriteTimeParam`: start moving a parameter to a value.
    fn write_time_param(&mut self, t: usize, cb: u8) {
        let idx = self.rd8(t) as usize;
        let target = match cb & 0x0C {
            0x00 => {
                let r = self.rd8(t) as usize;
                self.tracks[t].reg.get(r).copied().unwrap_or(0) as i16
            }
            0x04 => self.rd8(t) as i16,
            0x08 => ((self.rd8(t) as u16) << 8) as i16,
            _ => self.rd16(t) as i16,
        };
        let dur: i64 = match cb & 3 {
            0 => -1,
            1 => {
                let r = self.rd8(t) as usize;
                self.tracks[t].reg.get(r).copied().unwrap_or(0) as i64
            }
            2 => self.rd8(t) as i64,
            _ => self.rd16(t) as i64,
        };
        let Some(m) = self.tracks[t].mv.get_mut(idx) else { return };
        m.target = target as f32 / 32768.0;
        if dur >= 0 {
            m.dur = dur as f32;
        }
        if m.dur <= 0.0 {
            m.cur = m.target;
            m.step = 0.0;
            m.dur = 1.0;
        } else {
            m.step = (m.target - m.cur) / m.dur;
        }
    }

    /// `LoadTbl`.
    fn load_tbl(&self, ofs: u32, idx: u32, kind: u32) -> u32 {
        let o = ofs as usize;
        let i = idx as usize;
        match kind {
            4 => self.rd_ofs(o + i, 1),
            5 => self.rd_ofs(o + i * 2, 2),
            6 => self.rd_ofs(o + i * 3, 3),
            7 => self.rd_ofs(o + i * 4, 4),
            _ => self.rd_ofs(o + i, 4),
        }
    }

    /// `Jam_WriteRegParam`: register arithmetic.
    fn write_reg_param(&mut self, t: usize, cb: u8) {
        let mut cb = cb;
        let mut src = (cb & 0xc) as u32;
        let mut op = (cb & 3) as u32;
        let mut tbl_kind = 0u32;
        let mut r27 = 0u32;
        if cb & 0x0F == 0x0B {
            src = 0;
            op = 0xb;
        }
        if cb & 0x0F == 0x0A {
            cb = self.rd8(t);
            src = (cb & 0x0C) as u32;
            op = 0xa;
            tbl_kind = (cb >> 4) as u32 + 4;
        }
        if cb & 0x0F == 0x09 {
            cb = self.rd8(t);
            src = (cb & 0x0c) as u32;
            op = (cb & 0xf0) as u32;
            if src == 8 {
                src = 0x10;
            }
        }
        let mut ri = self.rd8(t);
        if op == 0xA {
            let r = self.rd8(t);
            r27 = self.read_reg32(t, r);
        }
        let mut v: i16 = match src {
            0 => {
                let r = self.rd8(t);
                self.read_reg_direct(t, r) as i16
            }
            4 => self.rd8(t) as i16,
            8 => ((self.rd8(t) as u16) << 8) as i16,
            12 => self.rd16(t) as i16,
            _ => -1,
        };
        let old = self.read_reg_direct(t, ri) as i16;
        match op {
            0 => {}
            1 => {
                if src == 4 {
                    v = ext8(v as u8) as i16;
                }
                v = old.wrapping_add(v);
            }
            2 => {
                let xy = (old as i32).wrapping_mul(v as i32) as u32;
                self.write_reg_direct(t, 4, (xy >> 16) as u16);
                self.write_reg_direct(t, 5, xy as u16);
                return;
            }
            3 => {
                self.tracks[t].reg[3] = old.wrapping_sub(v) as u16;
                return;
            }
            0xB => v = old.wrapping_sub(v),
            0x10 | 0x20 => {
                if src == 4 {
                    v = ext8(v as u8) as i16;
                }
                let sh = (v as i32).unsigned_abs().min(31);
                v = match (op, v < 0) {
                    (0x10, true) => ((old as u16 as u32) >> sh) as i16,
                    (0x10, false) => ((old as u16 as u32) << sh) as i16,
                    (_, true) => ((old as i32) >> sh) as i16,
                    (_, false) => ((old as i32) << sh) as i16,
                };
            }
            0x30 => v &= old,
            0x40 => v |= old,
            0x50 => v ^= old,
            0x60 => v = old.wrapping_neg(),
            0x90 => {
                let m = v as u16 as i32;
                let r = self.random();
                v = if m == 0 { 0 } else { (r % m) as i16 };
            }
            0xA => {
                r27 = self.load_tbl(r27, v as u16 as u32, tbl_kind);
                v = r27 as u16 as i16;
            }
            _ => {}
        }
        let shown: u16;
        let tr = &mut self.tracks[t];
        match ri {
            0..=2 => {
                v &= 0xff;
                shown = ext8(v as u8);
            }
            0x21 => {
                ri = 6;
                v = ((tr.reg[6] & 0xff00) | (v as u16 & 0xff)) as i16;
                shown = v as u16;
            }
            0x20 => {
                ri = 6;
                v = ((tr.reg[6] & 0x00ff) | ((v as u16) << 8)) as i16;
                shown = v as u16;
            }
            0x2E => {
                ri = 0xd;
                v = ((tr.reg[13] & 0xff00) | (v as u16 & 0xff)) as i16;
                shown = v as u16;
            }
            0x2F => {
                ri = 0xd;
                v = ((tr.reg[13] & 0x00ff) | ((v as u16) << 8)) as i16;
                shown = v as u16;
            }
            0x22 => {
                self.write_reg_direct(t, 0, (v as u16) >> 8);
                v &= 0xff;
                shown = v as u16;
                ri = 1;
            }
            0x28..=0x2B => {
                let i = (ri - 0x28) as usize;
                tr.reg[16 + 2 * i] = (r27 >> 16) as u16;
                tr.reg[17 + 2 * i] = r27 as u16;
                return;
            }
            _ => shown = v as u16,
        }
        let tr = &mut self.tracks[t];
        if let Some(r) = tr.reg.get_mut(ri as usize) {
            *r = v as u16;
        }
        tr.reg[3] = shown;
        match ri {
            // Osc_Clear_Overwrite.
            6 => tr.route = [15, 15],
            7 => tr.update_flags |= 2,
            0xd => {
                tr.ctl.prio = tr.reg[13] as u32 | 0x10000;
                tr.ctl.release_time = 0;
            }
            _ => {}
        }
    }

    /// `__ConditionCheck` on register 3.
    fn condition(&mut self, t: usize, c: u8) -> bool {
        let r = self.read_reg_direct(t, 3);
        match c & 0x0f {
            0 => true,
            1 => r == 0,
            2 => r != 0,
            3 => r == 1,
            4 => r >= 0x8000,
            5 => r < 0x8000,
            _ => false,
        }
    }

    // ---- ports and the track registry ----

    /// `Jam_SetInterrupt`.
    fn set_interrupt(&mut self, t: usize, n: u8) {
        let tr = &mut self.tracks[t];
        if tr.int_enable & (1 << n) != 0 {
            tr.int_pending |= 1 << n;
        }
    }

    /// `Jam_TryInterrupt`.
    fn try_interrupt(&mut self, t: usize) {
        let tr = &mut self.tracks[t];
        if tr.int_active != 0 {
            return;
        }
        for i in 0..8 {
            let m = 1u8 << i;
            if tr.int_enable & m != 0 && tr.int_pending & m != 0 {
                tr.saved_pc = tr.pc;
                tr.pc = tr.int_addr[i];
                tr.int_active = m;
                tr.saved_wait = tr.wait;
                tr.wait = 0;
                tr.int_pending ^= m;
                return;
            }
        }
    }

    /// `Jam_WritePortAppDirect`: what `Sound_*` and `ChildWritePort` use.
    pub(super) fn write_port(&mut self, t: Option<usize>, port: usize, value: u16) {
        let Some(t) = t else { return };
        let p = &mut self.tracks[t].ports[port & 15];
        p.value = value;
        p.import = true;
        match port {
            0 => self.set_interrupt(t, 3),
            1 => self.set_interrupt(t, 4),
            _ => {}
        }
    }

    /// `Jam_GetTrackHandle`.
    pub(super) fn track_by_id(&self, id: u32) -> Option<usize> {
        self.registry.iter().find(|r| r.0 == id).map(|r| r.1)
    }

    fn regist(&mut self, t: usize, id: u32) {
        if self.registry.iter().any(|r| r.0 == id) || self.registry.len() >= 32 {
            return;
        }
        self.registry.push((id, t));
        self.tracks[t].registered = true;
    }

    fn unregist(&mut self, t: usize) {
        if self.tracks[t].registered {
            self.registry.retain(|r| r.1 != t);
            self.tracks[t].registered = false;
        }
    }

    // ---- track setup (seqsetup.c) ----

    /// `Init_Track`.
    fn init_track(&mut self, t: usize, pc: usize, parent: Option<usize>) {
        let (tempo, factor, base, rel) = match parent {
            Some(p) => {
                let p = &self.tracks[p];
                (p.tempo, p.tempo_factor, p.time_base, p.time_rel)
            }
            None => (120, 1.0, 48, 1),
        };
        let inherit = parent.map(|p| {
            let p = &self.tracks[p];
            (p.reg[6], p.reg[7], p.reg[13], [p.args(0), p.args(1), p.args(2), p.args(3), p.args(4)], p.pan_calc, p.parent_pan_calc)
        });
        let tr = &mut self.tracks[t];
        tr.pc = pc;
        tr.tempo = tempo;
        tr.time_base = base;
        tr.time_rel = rel;
        tr.child_mute = 0;
        if parent.is_some() {
            tr.tempo_sync = false;
            tr.tempo_factor = factor;
        } else {
            tr.muted = 0;
        }
        tr.depth = 0;
        tr.note_dur = 0;
        tr.wait = 0;
        tr.state = 1;
        tr.parent = parent;
        tr.int_enable = 0;
        tr.int_active = 0;
        tr.timer = 0;
        for m in tr.mv.iter_mut() {
            *m = Move { cur: 1.0, target: 1.0, dur: 0.0, step: m.step };
        }
        for i in [1, 2, 4, 5, 13, 14, 15, 17] {
            tr.mv[i].cur = 0.0;
            tr.mv[i].target = 0.0;
        }
        tr.mv[3].cur = 0.5;
        tr.mv[3].target = 0.5;
        tr.mv[16].cur = 0.5;
        tr.mv[16].target = 0.5;
        tr.reg = [0; 32];
        match inherit {
            Some((bank, scale, prio, args, pc_, ppc)) if tr.flags & 2 == 0 => {
                tr.reg[6] = bank;
                tr.reg[7] = scale;
                tr.reg[13] = prio;
                tr.reg[8..13].copy_from_slice(&args);
                tr.pan_calc = pc_;
                tr.parent_pan_calc = ppc;
            }
            _ => {
                tr.reg[8..13].copy_from_slice(&[0, 1, 1, 0x7fff, 0x4000]);
                tr.pan_calc = [2; 3];
                tr.parent_pan_calc = [2; 3];
                tr.reg[6] = 0xf0;
                tr.reg[7] = 0x0c;
                tr.reg[13] = 0x40;
            }
        }
        tr.children = [None; 16];
        tr.notes = [0xFF; 8];
        tr.chans = [None; 8];
        tr.chan_ids = [0; 8];
        tr.note_flags = 0;
        tr.last_note = 0;
        tr.note_played = false;
        tr.gate_mode = false;
        // Osc_Init_Env: ENVELOPE_DEF, routing cleared.
        tr.osc[0] = Osc { mode: 0, rate: 1.0, attack: None, release: Some(self.tabs.rel.clone()), width: 1.0, vertex: 0.0 };
        tr.route = [15, 15];
        tr.transpose = 0;
        tr.final_transpose = 0;
        tr.ports = [Port::default(); 16];
        tr.registered = false;
        // Init_1shot: InitJcs.
        tr.ctl = Ctl::new();
    }

    /// `Jaq_SetSeqData` + `Jaq_StartSeq` for the root track.
    pub(super) fn start_root(&mut self) {
        let r = &mut self.tracks[0];
        r.id = 0;
        r.flags = 3;
        r.allocated = false;
        self.init_track(0, 0, None);
        let r = &mut self.tracks[0];
        r.tempo_acc = 0.0;
        r.tempo_factor = 1.0;
        self.update_track_all(0);
        self.tracks[0].state = 1;
    }

    /// `Jaq_OpenTrack`.
    fn open_track(&mut self, t: usize, flags: u32, src: u32) {
        let mut child = (flags & 0x0F) as usize;
        let mut tflags = ((flags & 0xC0) >> 6) as u8;
        if flags & 0x20 != 0 {
            tflags = 4;
            child = self.read_reg_direct(t, child as u8) as usize;
        }
        if child >= 16 {
            return;
        }
        if let Some(old) = self.tracks[t].children[child].take() {
            self.close_track(old);
        }
        let Some(n) = self.free.pop_front() else { return };
        self.tracks[n].state = 2;
        self.tracks[n].allocated = true;
        self.tracks[t].children[child] = Some(n);
        let pid = self.tracks[t].id;
        let depth = pid & 0xF000_0000;
        self.tracks[n].id = ((pid << 4 | child as u32) & 0x0FFF_FFFF) | depth.wrapping_add(0x1000_0000);
        self.tracks[n].conn = 0;
        self.tracks[n].flags = tflags;
        self.init_track(n, src as usize, Some(t));
        let p = &self.tracks[t];
        let m = p.muted | ((p.child_mute >> child) & 1) as u8;
        self.tracks[n].muted = m;
        self.update_track_all(n);
    }

    /// `Jaq_CloseTrack`.
    pub(super) fn close_track(&mut self, t: usize) {
        if self.tracks[t].state == 0 {
            return;
        }
        // __AllNoteOff
        let root = self.tracks[t].parent.is_none();
        for i in 0..8 {
            self.note_off(t, i, if root { 10 } else { 0 });
            self.tracks[t].notes[i] = 0xFF;
        }
        // BackTrack
        self.tracks[t].state = 0;
        if self.tracks[t].allocated {
            self.free.push_back(t);
        }
        for i in 0..16 {
            if let Some(c) = self.tracks[t].children[i].take() {
                self.close_track(c);
            }
        }
        self.tracks[t].muted = 0;
        self.tracks[t].child_mute = 0;
        // FixMoveChannelAll / FixReleaseChannelAll: the voices go up to the
        // parent's manager and keep the values they last had.
        let parent = self.tracks[t].parent;
        for v in self.voices.iter_mut().flatten() {
            if v.mgr == Some(t) {
                v.mgr = parent;
                v.follow = false;
            }
        }
        self.unregist(t);
    }

    // ---- parameters to the channel manager ----

    /// `Jam_UpdateTrackAll`.
    fn update_track_all(&mut self, t: usize) {
        if self.tracks[t].flags == 4 {
            return;
        }
        let pctl = self.tracks[t].parent.map(|p| self.tracks[p].ctl);
        let tr = &mut self.tracks[t];
        tr.ctl.prio = tr.reg[13] as u32 | 0x10000;
        tr.ctl.release_time = 0;
        let vol = if tr.muted > 0 { 0.0 } else { tr.mv[0].cur };
        let pitch = pitch_to_cent(tr.mv[1].cur, tr.reg[7] as f32);
        let (pan, fx, dolby) = (tr.mv[3].cur, tr.mv[2].cur, tr.mv[4].cur);
        match pctl {
            Some(p) if tr.flags & 1 == 0 => {
                let r = tr.args(4) as f32 / 32767.0;
                tr.ctl.volume = p.volume * vol;
                tr.ctl.pitch = p.pitch * pitch;
                tr.ctl.pan = blend(pan, p.pan, r, tr.parent_pan_calc[0]);
                tr.ctl.fxmix = blend(fx, p.fxmix, r, tr.parent_pan_calc[1]);
                tr.ctl.dolby = blend(dolby, p.dolby, r, tr.parent_pan_calc[2]);
            }
            _ => {
                tr.ctl.volume = vol;
                tr.ctl.pitch = pitch;
                tr.ctl.pan = pan;
                tr.ctl.fxmix = fx;
                tr.ctl.dolby = dolby;
            }
        }
    }

    /// `Jam_UpdateTrack`: the parameters `flags` names.
    fn update_track(&mut self, t: usize, flags: u32) {
        if self.tracks[t].flags == 4 {
            return;
        }
        if flags & 0x40 != 0 && self.tracks[t].parent.is_none() {
            self.update_tempo(t);
        }
        let pctl = self.tracks[t].parent.map(|p| self.tracks[p].ctl);
        let tr = &mut self.tracks[t];
        let vol = if tr.muted != 0 { 0.0 } else { tr.mv[0].cur };
        let pitch = pitch_to_cent(tr.mv[1].cur, tr.reg[7] as f32);
        let (pan, fx, dolby) = (tr.mv[3].cur, tr.mv[2].cur, tr.mv[4].cur);
        match pctl {
            Some(p) if tr.flags & 1 == 0 => {
                let r = tr.args(4) as f32 / 32767.0;
                if flags & 1 != 0 {
                    tr.ctl.volume = p.volume * vol;
                }
                if flags & 2 != 0 {
                    tr.ctl.pitch = p.pitch * pitch;
                }
                if flags & 8 != 0 {
                    tr.ctl.pan = blend(pan, p.pan, r, tr.parent_pan_calc[0]);
                }
                if flags & 4 != 0 {
                    tr.ctl.fxmix = blend(fx, p.fxmix, r, tr.parent_pan_calc[1]);
                }
                if flags & 0x10 != 0 {
                    tr.ctl.dolby = blend(dolby, p.dolby, r, tr.parent_pan_calc[2]);
                }
            }
            _ => {
                if flags & 1 != 0 {
                    tr.ctl.volume = vol;
                }
                if flags & 2 != 0 {
                    tr.ctl.pitch = pitch;
                }
                if flags & 8 != 0 {
                    tr.ctl.pan = pan;
                }
                if flags & 4 != 0 {
                    tr.ctl.fxmix = fx;
                }
                if flags & 0x10 != 0 {
                    tr.ctl.dolby = dolby;
                }
            }
        }
    }

    /// `Jam_UpdateTempo`: ticks per subframe from time base and tempo.
    fn update_tempo(&mut self, t: usize) {
        match self.tracks[t].parent {
            None => {
                let tr = &mut self.tracks[t];
                tr.tempo_factor = tr.time_base as f32 * tr.tempo as f32 / (DAC_RATE * 60.0 / SUBFRAME);
            }
            Some(p) => {
                let (f, b) = (self.tracks[p].tempo_factor, self.tracks[p].time_base);
                self.tracks[t].tempo_factor = f;
                self.tracks[t].time_base = b;
            }
        }
        for i in 0..16 {
            if let Some(c) = self.tracks[t].children[i] {
                if self.tracks[c].state != 0 {
                    self.update_tempo(c);
                }
            }
        }
    }

    /// `SeqUpdate`.
    fn seq_update(&mut self, t: usize, flags: u32) {
        let f = flags | self.tracks[t].update_flags;
        self.tracks[t].update_flags = 0;
        if f != 0 {
            self.update_track(t, f);
        }
        for i in 0..16 {
            if let Some(c) = self.tracks[t].children[i] {
                if self.tracks[c].state != 0 {
                    self.seq_update(c, f);
                }
            }
        }
    }

    /// `Jaq_RootCallback`: once per subframe.
    pub(super) fn root_callback(&mut self) {
        if self.tracks[0].state == 0 {
            return;
        }
        let r = &mut self.tracks[0];
        r.tempo_acc += r.tempo_factor;
        while self.tracks[0].tempo_acc >= 1.0 {
            self.tracks[0].tempo_acc -= 1.0;
            if self.seqmain_note(0, 0) == -1 {
                self.close_track(0);
                return;
            }
        }
        self.seq_update(0, 0);
    }

    /// `Jam_SEQtimeToDSPtime`: ticks to subframes.
    fn seq_to_dsp(&self, t: usize, dur: i64, gate: u8) -> i32 {
        let tr = &self.tracks[t];
        let d = dur as f32 * gate as f32 / 100.0;
        (if tr.time_rel == 1 { d / tr.tempo_factor } else { d * 120.0 / tr.time_base as f32 }) as i32
    }

    // ---- notes (noteon.c) ----

    /// `CheckNoteStop`.
    fn check_note_stop(&mut self, t: usize, i: usize) -> bool {
        let Some(slot) = self.tracks[t].chans[i] else { return true };
        let id = self.tracks[t].chan_ids[i];
        match &self.voices[slot] {
            Some(v) if v.id == id => v.note == 0xFF,
            _ => {
                self.tracks[t].chans[i] = None;
                self.tracks[t].chan_ids[i] = 0;
                true
            }
        }
    }

    /// `NoteOFF_R` (`rel` 0: `NoteOFF`).
    fn note_off(&mut self, t: usize, i: usize, rel: u16) {
        let Some(slot) = self.tracks[t].chans[i].take() else { return };
        let id = core::mem::take(&mut self.tracks[t].chan_ids[i]);
        if self.voices[slot].as_ref().is_some_and(|v| v.id == id) {
            if rel != 0 {
                if let Some(v) = self.voices[slot].as_mut() {
                    v.buf[0].release = rel;
                }
            }
            self.release_voice(slot);
        }
    }

    /// `NoteON`: play `key` on the track's program into note slot `ch`.
    fn note_on(&mut self, t: usize, ch: usize, key: u8, vel: u8, dur: i32) -> i32 {
        // A muted track still plays (at volume 0): its pause status (10)
        // never has bit 0x40 set.
        if self.tracks[t].chans[ch].is_some() {
            self.note_off(t, ch, 0);
        }
        let reg6 = self.read_reg_direct(t, 6);
        // Bank 0 is the only bank (Jac_BnkVirtualToPhysical).
        if reg6 >> 8 != 0 {
            return -1;
        }
        let prog = reg6 & 0xff;
        // 0xE4.. are drum sets and 0xF0.. the DSP's own oscillators: the
        // IPL's sequence never sounds either.
        if prog >= 0xe4 {
            return -1;
        }
        let args = [self.tracks[t].args(0), self.tracks[t].args(1), self.tracks[t].args(2)];
        let Some(slot) = self.play_1shot(t, prog as usize, key, vel, dur) else { return -1 };
        self.tracks[t].chans[ch] = Some(slot);
        let route = self.tracks[t].route;
        let tosc = self.tracks[t].osc.clone();
        let v = self.voices[slot].as_mut().unwrap();
        self.tracks[t].chan_ids[ch] = v.id;
        // UpdatePanPower_1Shot
        let sum = args[0] as f32 + args[1] as f32 + args[2] as f32;
        if sum != 0.0 {
            v.pan[0] = [args[0] as f32 / sum, args[1] as f32 / sum, args[2] as f32 / sum];
        }
        for i in 0..2 {
            let flag = route[i] as usize;
            if flag == 15 || flag == 14 {
                continue;
            }
            let k = flag % 4;
            if flag >= 8 {
                if let Some(o) = &v.osc[k] {
                    self.tracks[t].osc[i] = o.clone();
                }
            } else if flag >= 4 {
                if let Some(o) = &v.osc[k] {
                    let rel = self.tracks[t].osc[i].release.clone();
                    self.tracks[t].osc[i] = Osc { release: rel, ..o.clone() };
                }
            }
            // Effecter_Overwrite_1ShotD (only two envelope buffers exist).
            if k < 2 {
                let o = if flag >= 4 { self.tracks[t].osc[i].clone() } else { tosc[i].clone() };
                v.buf[k].state = 1;
                let x = super::voice::osc_to_ofs(&o, &mut v.buf[k], &self.tabs.force_release);
                v.do_effect_osc(o.mode, x);
                v.osc[k] = Some(o);
            }
        }
        self.update_track(t, 3);
        // ResetInitialVolume
        let ctl = self.tracks[t].ctl;
        let v = self.voices[slot].as_mut().unwrap();
        v.update_effecter(Some(&ctl), self.stereo);
        v.lv0 = v.lv1;
        0
    }

    // ---- the note loop ----

    /// `Jam_SeqmainNote`: one tick of track `t` and its children. -1 when
    /// the track finished.
    fn seqmain_note(&mut self, t: usize, muted: u8) -> i32 {
        let mut flags = 0u32;
        if let Some(p) = self.tracks[t].parent {
            if self.tracks[t].tempo_sync {
                let r = (self.tracks[t].tempo as f32 / self.tracks[p].tempo.max(1) as f32).min(1.0);
                self.tracks[t].tempo_acc += r;
                if self.tracks[t].tempo_acc < 1.0 {
                    return 0;
                }
                self.tracks[t].tempo_acc -= 1.0;
            }
            self.tracks[t].muted = muted;
            let pt = self.tracks[p].final_transpose;
            let tr = &mut self.tracks[t];
            tr.final_transpose = (tr.transpose as u8).wrapping_add(pt);
        } else {
            let tr = &mut self.tracks[t];
            tr.final_transpose = tr.transpose as u8;
        }
        self.set_interrupt(t, 7);
        {
            let tr = &mut self.tracks[t];
            if tr.timer != 0 {
                tr.timer -= 1;
                if tr.timer == 0 {
                    let en = tr.int_enable & (1 << 6) != 0;
                    if en {
                        tr.int_pending |= 1 << 6;
                    }
                    if tr.timer_count != 0 {
                        tr.timer_count -= 1;
                        if tr.timer_count != 0 {
                            tr.timer = tr.max_time;
                        }
                    } else {
                        tr.timer = tr.max_time;
                    }
                }
            }
        }
        let mut budget = 4096;
        'retry: loop {
            self.try_interrupt(t);
            if self.tracks[t].wait == -1 {
                if self.check_note_stop(t, 0) {
                    self.tracks[t].wait = 0;
                } else {
                    break 'retry;
                }
            }
            if self.tracks[t].wait > 0 {
                self.tracks[t].wait -= 1;
                if self.tracks[t].wait != 0 {
                    break 'retry;
                }
                let tr = &mut self.tracks[t];
                if tr.note_dur != -1 && tr.note_flags == 0 && tr.note_played {
                    tr.notes[0] = 0xFF;
                    tr.chans[0] = None;
                }
            }
            loop {
                budget -= 1;
                if budget == 0 {
                    // A sequence that loops without waiting: give up this tick.
                    break 'retry;
                }
                let op = self.rd8(t);
                if op < 0x80 {
                    if self.note_cmd(t, op) {
                        break 'retry;
                    }
                } else if op & 0xf0 == 0x80 || op == 0xf9 {
                    if self.note_off_cmd(t, op) {
                        break 'retry;
                    }
                } else {
                    let flow = match op & 0xf0 {
                        0x90 => {
                            self.write_time_param(t, op & 0x0f);
                            Flow::Next
                        }
                        0xa0 => {
                            self.write_reg_param(t, op & 0x0f);
                            Flow::Next
                        }
                        0xb0 => {
                            let from_reg = op & 8 != 0;
                            let count = (op & 7) as usize;
                            let mut cmd = self.rd8(t);
                            if from_reg {
                                cmd = self.exch(t, cmd) as u8;
                            }
                            // The C leaves the mask uninitialised; the IPL's
                            // sequence never uses these.
                            let mut mask = 0u16;
                            if !from_reg || count != 0 {
                                let mut bits = self.rd8(t);
                                let mut pair = 3u16;
                                for _ in 0..count + 1 {
                                    if bits & 0x80 != 0 {
                                        mask |= pair;
                                    }
                                    bits <<= 1;
                                    pair = pair.wrapping_shl(2);
                                }
                            }
                            self.cmd(t, cmd, mask)
                        }
                        _ => self.cmd(t, op, 0),
                    };
                    match flow {
                        Flow::Next => {}
                        Flow::Wait => break 'retry,
                        Flow::Retry => continue 'retry,
                        Flow::Finish => return -1,
                    }
                }
            }
        }
        // timed:
        for i in 0..18 {
            let m = &mut self.tracks[t].mv[i];
            if m.dur > 0.0 {
                m.cur += m.step;
                m.dur -= 1.0;
                let cur = m.cur;
                if i <= 5 || i >= 11 {
                    flags |= 1 << i;
                } else {
                    self.osc_update_param(t, i, cur);
                }
            }
        }
        self.tracks[t].update_flags |= flags;
        for i in 0..16 {
            let Some(c) = self.tracks[t].children[i] else { continue };
            if self.tracks[c].state == 0 {
                continue;
            }
            let cm = self.tracks[t].muted | ((self.tracks[t].child_mute >> i) & 1) as u8;
            if self.seqmain_note(c, cm) == -1 {
                self.close_track(c);
                self.tracks[t].children[i] = None;
            }
        }
        0
    }

    /// A note command. True when it ends the tick (a timed note waits).
    fn note_cmd(&mut self, t: usize, op: u8) -> bool {
        let ft = self.tracks[t].final_transpose;
        let mut key = op.wrapping_add(ft);
        let fl = self.rd8(t);
        if fl & 0x80 != 0 {
            key = self.exch(t, key) as u8;
            key = key.wrapping_add(ft);
        }
        let mut bend_to = key;
        if fl >> 5 & 2 != 0 {
            bend_to = key;
            key = self.tracks[t].last_note;
        }
        let mut vel = self.rd8(t);
        if vel >= 0x80 {
            vel = self.exch(t, vel - 0x80) as u8;
        }
        let (ch, gate, dur): (usize, u8, i64);
        if fl & 7 == 0 {
            let mut g = self.rd8(t);
            if g >= 0x80 {
                g = self.exch(t, g - 0x80) as u8;
            }
            let n = fl >> 3 & 3;
            let mut d = 0u32;
            for _ in 0..n {
                d = d << 8 | self.rd8(t) as u32;
            }
            if n == 1 && d >= 0x80 {
                d = self.exch(t, (d - 0x80) as u8);
            }
            (ch, gate, dur) = (0, g, d as i64);
        } else {
            let mut r = (fl & 7) as u32;
            if fl >> 3 & 3 != 0 {
                r = self.exch(t, (r - 1) as u8) as u8 as u32;
                if r >= 8 {
                    return true;
                }
            }
            (ch, gate, dur) = (r as usize, 100, -1);
        }
        let nf = fl >> 5 & 3;
        self.tracks[t].note_flags = nf;
        let mut played = false;
        let mut d = if dur != -1 { self.seq_to_dsp(t, dur, gate) } else { -1 };
        if nf & 1 != 0 {
            d = -1;
        }
        let res = if self.tracks[t].gate_mode {
            // Gate_1Shot: retune the held voice.
            self.gate_on(t, ch, key, vel, d)
        } else {
            self.note_on(t, ch, key, vel, d)
        };
        if res != -1 {
            self.tracks[t].notes[ch] = key;
            played = true;
        }
        let tr = &mut self.tracks[t];
        tr.note_played = played;
        tr.gate = gate;
        tr.note_dur = dur;
        tr.gate_mode = nf & 1 != 0;
        if nf & 2 != 0 {
            let steps = if d == -1 { self.seq_to_dsp(t, dur, gate) } else { d };
            self.key_target(t, bend_to.wrapping_add(ft), steps);
            key = bend_to;
        }
        self.tracks[t].last_note = key;
        if dur != -1 {
            self.tracks[t].wait = if dur == 0 { -1 } else { dur };
            return true;
        }
        false
    }

    /// A note off or wait. True when it ends the tick.
    fn note_off_cmd(&mut self, t: usize, op: u8) -> bool {
        let mut op = op;
        let mut bytes = 1;
        let mut rel = 0u16;
        if op == 0xf9 {
            let cb = self.rd8(t);
            let v = self.exch(t, cb & 7);
            if v > 7 || v == 0 {
                if cb & 0x80 != 0 {
                    self.rd8(t);
                }
                return false;
            }
            op = 0x80 + v as u8 + if cb & 0x80 != 0 { 8 } else { 0 };
        }
        let mut v = op & 0x0f;
        if v == 8 {
            bytes = 2;
            v = 0;
        }
        if v > 8 {
            v -= 8;
            rel = self.rd8(t) as u16;
            if rel > 100 {
                rel = (rel - 98) * 20;
            }
        }
        if v == 0 {
            let mut w = 0i64;
            for _ in 0..bytes {
                w = w << 8 | self.rd8(t) as i64;
            }
            self.tracks[t].wait = w;
            return w != 0;
        }
        self.note_off(t, v as usize, rel);
        false
    }

    /// `GateON`.
    fn gate_on(&mut self, t: usize, ch: usize, key: u8, vel: u8, dur: i32) -> i32 {
        let Some(slot) = self.tracks[t].chans[ch] else { return -1 };
        let Some(v) = self.voices[slot].as_mut() else { return -1 };
        if v.note_id == -1 {
            v.note_id = if dur == 0 { -1 } else { dur };
            v.countdown = v.note_id;
            let base = self.archive.waves.get(v.wave).map_or(60, |w| w.base_key as i32);
            v.velocity = vel;
            v.note = key;
            v.cur_pitch = v.base_pitch * key_pitch(key as i32 + 60 - base);
            let x = vel as f32 / 127.0;
            v.cur_vol = x * x * v.base_vol;
        }
        0
    }

    /// `SetKeyTarget_1Shot` on the track's first note slot.
    fn key_target(&mut self, t: usize, key: u8, steps: i32) {
        let Some(slot) = self.tracks[t].chans[0] else { return };
        let Some(v) = self.voices[slot].as_mut() else { return };
        let base = self.archive.waves.get(v.wave).map_or(60, |w| w.base_key as i32);
        let p = v.base_pitch * key_pitch(key as i32 + 60 - base);
        if steps <= 0 {
            v.cur_pitch = p;
            v.sweep = 0;
        } else {
            v.target_pitch = p;
            v.sweep = steps.min(0xffff) as u16;
        }
    }

    /// `Osc_Update_Param`.
    fn osc_update_param(&mut self, t: usize, i: usize, val: f32) {
        let o = &mut self.tracks[t].osc;
        match i {
            6 => o[0].width = val,
            7 => o[0].rate = val,
            8 => o[0].vertex = val,
            9 => o[1].width = val,
            10 => o[1].rate = val,
            11 => o[1].vertex = val,
            _ => {}
        }
    }

    // ---- commands (Cmd_*) ----

    fn cmd(&mut self, t: usize, op: u8, mask: u16) -> Flow {
        let n = op.wrapping_sub(0xC0) as usize & 63;
        let (count, types) = ARGS[n];
        let mut types = types | mask;
        let mut a = [0u32; 8];
        for x in a.iter_mut().take(count as usize) {
            *x = match types & 3 {
                0 => self.rd8(t) as u32,
                1 => self.rd16(t) as u32,
                2 => self.rd24(t),
                _ => {
                    let r = self.rd8(t);
                    self.exch(t, r)
                }
            };
            types >>= 2;
        }
        let tr = &mut self.tracks[t];
        match n {
            1 => self.open_track(t, a[0], a[1]),
            2 => match tr.parent {
                Some(p) => self.open_track(p, a[0], a[1]),
                None => return Flow::Next,
            },
            3 => {
                if tr.depth < 8 {
                    tr.stack[tr.depth] = tr.pc;
                    tr.depth += 1;
                }
                tr.pc = a[0] as usize;
            }
            4 | 8 => {
                let fl = self.rd8(t);
                let target;
                if fl & 0x80 != 0 {
                    let r = self.rd8(t);
                    let idx = self.read_reg_direct(t, r) as usize;
                    if fl & 0x40 != 0 {
                        let base = if fl & 0x20 != 0 {
                            let r = self.rd8(t);
                            self.read_reg_direct(t, r) as usize
                        } else {
                            self.rd24(t) as usize
                        };
                        target = self.rd_ofs(base + idx * 3, 3) as usize;
                    } else {
                        target = idx;
                    }
                } else {
                    target = self.rd24(t) as usize;
                }
                if self.condition(t, fl & 0x0f) {
                    let tr = &mut self.tracks[t];
                    if n == 4 && tr.depth < 8 {
                        tr.stack[tr.depth] = tr.pc;
                        tr.depth += 1;
                    }
                    tr.pc = target;
                }
            }
            5 => {
                if tr.depth > 0 {
                    tr.depth -= 1;
                    tr.pc = tr.stack[tr.depth];
                }
            }
            6 => {
                if self.condition(t, a[0] as u8) {
                    let tr = &mut self.tracks[t];
                    if tr.depth > 0 {
                        tr.depth -= 1;
                        tr.pc = tr.stack[tr.depth];
                    }
                }
            }
            7 => tr.pc = a[1] as usize,
            9 => {
                if tr.depth < 8 {
                    tr.stack[tr.depth] = tr.pc;
                    tr.loops[tr.depth] = a[0] as u16;
                    tr.depth += 1;
                }
            }
            10 => {
                if tr.depth == 0 {
                    return Flow::Next;
                }
                let mut c = tr.loops[tr.depth - 1];
                c = c.saturating_sub(1);
                if c == 0 {
                    tr.depth -= 1;
                } else {
                    tr.loops[tr.depth - 1] = c;
                    tr.pc = tr.stack[tr.depth - 1];
                }
            }
            11 => {
                let p = &mut tr.ports[a[0] as usize & 15];
                p.import = false;
                let v = p.value;
                self.write_reg_direct(t, a[1] as u8, v);
            }
            12 => {
                let p = &mut tr.ports[a[0] as usize & 15];
                p.value = a[1] as u16;
                p.export = true;
            }
            13 => {
                let v = tr.ports[a[0] as usize & 15].import as u16;
                self.write_reg_direct(t, 3, v);
            }
            14 => {
                let v = tr.ports[a[0] as usize & 15].export as u16;
                self.write_reg_direct(t, 3, v);
            }
            15 | 42 => {
                tr.wait = a[0] as i64;
                return if a[0] != 0 { Flow::Wait } else { Flow::Next };
            }
            16 => tr.conn = a[0] << 16 | a[1],
            17 => {
                let p = tr.parent;
                self.write_port(p, a[0] as usize & 15, a[1] as u16);
            }
            18 => {
                let c = tr.children[(a[0] >> 4) as usize & 15];
                self.write_port(c, a[0] as usize & 15, a[1] as u16);
            }
            20 => tr.last_note = (a[0] as u8).wrapping_add(tr.final_transpose),
            21 => tr.time_rel = a[0] as u8,
            22 => match a[0] {
                // Osc_Setup_Simple: vibrato on the second, tremolo on either.
                0 => tr.osc[1] = Osc { mode: 1, rate: 0.8, attack: Some(self.tabs.vib.clone()), release: Some(self.tabs.vib.clone()), width: 0.0, vertex: 1.0 },
                1 | 2 => {
                    tr.osc[a[0] as usize - 1] =
                        Osc { mode: 0, rate: 1.0, attack: Some(self.tabs.tre.clone()), release: Some(self.tabs.tre.clone()), width: 0.0, vertex: 1.0 }
                }
                _ => {}
            },
            23 => {
                // Osc_Setup_SimpleEnv: tables inside the sequence.
                let tab = self.seq_table(a[1] as usize);
                let tr = &mut self.tracks[t];
                match a[0] {
                    0 => tr.osc[0] = Osc { mode: 0, rate: 1.0, attack: tab, release: Some(self.tabs.rel.clone()), width: 1.0, vertex: 0.0 },
                    1 => tr.osc[0].release = tab,
                    _ => {}
                }
            }
            24 => {
                // Osc_Setup_ADSR: attack, hold, decay to sustain, release.
                let s = |i: usize| a[i] as u16 as i16;
                let ads = [0, s(0), 0x7fff, 0, s(1), 0x7fff, 0, s(2), s(3), 14, 0, 0];
                let rel = [0, s(4), 0, 15, 1, 0];
                tr.osc[0] = Osc {
                    mode: 0,
                    rate: 1.0,
                    attack: Some(alloc::rc::Rc::from(ads.as_slice())),
                    release: Some(alloc::rc::Rc::from(rel.as_slice())),
                    width: 1.0,
                    vertex: 0.0,
                };
            }
            25 => {
                tr.transpose = a[0] as u8 as i8;
                let p = tr.parent.map(|p| self.tracks[p].transpose);
                let tr = &mut self.tracks[t];
                tr.final_transpose = match p {
                    Some(pt) => (pt as u8).wrapping_add(tr.transpose as u8),
                    None => tr.transpose as u8,
                };
            }
            26 => {
                let i = a[0] as usize;
                if i >= 16 {
                    return Flow::Next;
                }
                if let Some(c) = tr.children[i].take() {
                    self.close_track(c);
                }
            }
            // OutSwitch, PauseStatus, FIRSet, EXTSet, IIRCutOff, Nop: no outer
            // parameters, pausing or filters here.
            27 | 30 | 45 | 46 | 49 | 60 => {}
            59 => self.skip_printf(t),
            28 => self.update_track(t, a[0]),
            29 => {
                if a[0] < 6 {
                    tr.ctl.bus[a[0] as usize] = a[1] as u16;
                }
            }
            31 => {
                let i = a[0] as usize & 7;
                tr.int_enable |= 1 << i;
                tr.int_addr[i] = a[1] as usize;
            }
            32 => tr.int_enable &= !(1u8 << (a[0] & 7)),
            33 => tr.int_active = 0,
            34 => tr.int_active = 1,
            35 => {
                tr.wait = tr.saved_wait;
                tr.int_active = 0;
                tr.pc = tr.saved_pc;
                return Flow::Retry;
            }
            36 => {
                tr.timer_count = a[0] as u8;
                tr.timer = a[1];
                tr.max_time = a[1];
            }
            37 => {
                let id = tr.conn;
                self.regist(t, id);
            }
            38 => self.unregist(t),
            39 => self.write_reg_direct(t, 3, 0xffff),
            40 => {
                // AllStop_1Shot + FlushRelease_1Shot on the track's voices.
                for s in 0..self.voices.len() {
                    if self.voices[s].as_ref().is_some_and(|v| v.mgr == Some(t) && !v.releasing) {
                        self.release_voice(s);
                    }
                }
                self.flush_release(t);
            }
            41 => self.flush_release(t),
            43 => {
                for i in 0..3 {
                    tr.reg[8 + i] = a[i] as u16;
                }
                for i in 3..5 {
                    tr.reg[8 + i] = (a[i] as f32 * 327.67) as u16;
                }
            }
            44 => {
                for i in 0..4 {
                    let m = &mut tr.mv[12 + i];
                    m.target = a[i] as u16 as i16 as f32 / 32768.0;
                    m.cur = m.target;
                    m.step = 0.0;
                    m.dur = 1.0;
                }
            }
            47 => {
                const CALC: [u8; 7] = [0, 0, 0, 1, 1, 2, 2];
                const PCALC: [u8; 7] = [0, 1, 2, 0, 2, 0, 2];
                for i in 0..3 {
                    let k = (a[i] >> 5) as usize % 7;
                    tr.pan_calc[i] = CALC[k];
                    tr.parent_pan_calc[i] = PCALC[k];
                    tr.ctl.pan_calc[i] = (a[i] & 0x1f) as u8;
                    tr.update_flags |= 8;
                }
            }
            48 => {
                let route = (a[0] & 0xf) as u8;
                let i = (a[0] >> 4 & 1) as usize;
                tr.route[i] = route;
            }
            50 => {
                // Osc_Setup_Full
                let fl = a[0];
                let i = (fl >> 4 & 1) as usize;
                if fl & 0x80 != 0 {
                    tr.osc[i] = Osc { mode: (fl & 0xf) as u8, rate: 1.0, attack: None, release: Some(self.tabs.rel.clone()), width: 1.0, vertex: 0.0 };
                    if fl & 0xf == 1 {
                        tr.osc[i].vertex = 1.0;
                    }
                }
                if fl & 0x40 != 0 {
                    let tab = self.seq_table(a[1] as usize);
                    self.tracks[t].osc[i].attack = tab;
                }
                if fl & 0x20 != 0 {
                    let tab = self.seq_table(a[2] as usize);
                    self.tracks[t].osc[i].release = tab;
                }
            }
            58 => {
                let prog = self.read_reg_direct(t, 6) as usize & 0xff;
                let ok = self.bank.vmap(prog, a[0] as u8, 127).and_then(|(_, vm)| self.archive.wave_by_id(vm.wave)).is_some();
                self.write_reg_direct(t, 3, if ok { 0 } else { 2 });
            }
            61 => {
                tr.tempo = a[0] as u16;
                if tr.parent.is_none() {
                    self.update_tempo(t);
                } else {
                    tr.tempo_sync = true;
                }
            }
            62 => {
                tr.time_base = a[0] as u16;
                if tr.parent.is_none() {
                    self.update_tempo(t);
                }
            }
            63 => {
                // Cmd_Finish: one last parameter step, then the track ends.
                let mut f = 0;
                for i in 0..18 {
                    let m = &mut self.tracks[t].mv[i];
                    if m.dur > 0.0 {
                        m.cur += m.step;
                        m.dur -= 1.0;
                        let cur = m.cur;
                        if i <= 5 || i >= 11 {
                            f |= 1 << i;
                        } else {
                            self.osc_update_param(t, i, cur);
                        }
                    }
                }
                self.seq_update(t, f);
                return Flow::Finish;
            }
            _ => {}
        }
        Flow::Next
    }

    /// `Cmd_Printf`: skip the format string and one byte per argument.
    fn skip_printf(&mut self, t: usize) {
        let mut args = 0;
        for _ in 0..0x80 {
            match self.rd8(t) {
                0 => break,
                b'\\' => {
                    if self.rd8(t) == 0 {
                        break;
                    }
                }
                b'%' => {
                    if self.rd8(t) == 0 {
                        break;
                    }
                    args += 1;
                }
                _ => {}
            }
        }
        for _ in 0..args.min(4) {
            self.rd8(t);
        }
    }

    /// An envelope table inside the sequence data.
    fn seq_table(&self, ofs: usize) -> Option<super::bank::Table> {
        let mut v = alloc::vec::Vec::new();
        for i in 0..64 {
            let o = ofs + i * 6;
            let s = |k: usize| self.rd_ofs(o + k, 2) as u16 as i16;
            let (m, tm, val) = (s(0), s(2), s(4));
            v.extend_from_slice(&[m, tm, val]);
            if (0xD..=0xF).contains(&m) {
                break;
            }
        }
        Some(v.into())
    }

    /// `GetRandom_s32`.
    pub(super) fn random(&mut self) -> i32 {
        let (v0, v1) = self.rng;
        let tmp = v1.wrapping_mul(0x98765432u32 as i32).wrapping_add(v0.wrapping_mul(0x13579BDE) >> 4).wrapping_add(1);
        self.rng = (v1, tmp);
        tmp
    }
}
