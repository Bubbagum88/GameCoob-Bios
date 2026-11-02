//! Voices: JAudio's logical channels (`jc_`, `jaudio/oneshot.c`,
//! `driverinterface.c`) and the DSP channel each one plays on, folded into
//! one struct. A voice plays one wave of the bank at a pitch, runs its
//! envelopes (`Bank_OscToOfs`, `bankdrv.c`) once per subframe, and works out
//! its four bus levels from its track's volume, pan and effect send
//! (`UpdateEffecterParam`).

use super::bank::{Osc, Table};
use alloc::rc::Rc;

/// The console's DSP output rate.
pub const DAC_RATE: f32 = 32028.5;
/// DAC samples per subframe; JAudio updates everything once per subframe.
pub const SUBFRAME: f32 = 80.0;
/// Envelope time units (1/600 s) to subframes.
const TIME_SCALE: f32 = (DAC_RATE / SUBFRAME) / 600.0;
/// `MAX_MIXERLEVEL` after the IPL's `Jac_SetMixerLevel(0.8, ...)`.
pub const MAX_MIXERLEVEL: f32 = 0.8 * 16384.0;

/// `Oscbuf_`: where an envelope is.
#[derive(Clone, Copy, Default)]
pub struct OscBuf {
    pub state: u8,
    curve: u8,
    index: u16,
    time: f32,
    value: f32,
    target: f32,
    delta: f32,
    pub release: u16,
}

/// The tables the driver itself owns (`jamosc.c`, `bankdrv.c`).
pub struct Tables {
    /// `FORCE_RELEASE_TABLE`, a `u32[3]` the driver reads as `s16` steps.
    pub force_release: Table,
    pub vib: Table,
    pub tre: Table,
    pub rel: Table,
}

impl Tables {
    pub fn new() -> Tables {
        Tables {
            force_release: Rc::from([0i16, 5, 0, 15, 0, 0].as_slice()),
            vib: Rc::from([0i16, 0, 0, 0, 20, 0x7fff, 0, 20, 0, 0, 0x14, -0x4000, 0, 20, 0, 13, 0, 1].as_slice()),
            tre: Rc::from([0i16, 0, 0x7fff, 0, 20, 0, 0, 20, -0x7fff, 0, 0x14, 0, 0, 0x14, 0x7fff, 13, 0, 1].as_slice()),
            rel: Rc::from([0i16, 10, 0, 15, 1, 0].as_slice()),
        }
    }
}

fn same(a: &Option<Table>, b: &Option<Table>) -> bool {
    match (a, b) {
        (Some(a), Some(b)) => Rc::ptr_eq(a, b),
        (None, None) => true,
        _ => false,
    }
}

/// `Bank_OscToOfs`: step an envelope by one subframe and return its output.
pub fn osc_to_ofs(osc: &Osc, buf: &mut OscBuf, force_release: &Table) -> f32 {
    if buf.state == 4 {
        if !same(&osc.attack, &osc.release) {
            buf.index = 0;
            buf.time = 0.0;
            buf.target = buf.value;
        }
        if osc.release.is_none() && buf.release == 0 {
            buf.release = 0x10;
        }
        if buf.release != 0 {
            buf.state = 8;
            buf.curve = (buf.release >> 14 & 3) as u8;
            buf.time = ((buf.release & 0x3fff) as f32 * TIME_SCALE).max(1.0);
            buf.target = 0.0;
            buf.delta = (buf.target - buf.value) / buf.time;
        } else {
            buf.state = 5;
        }
    }
    if buf.state == 6 {
        buf.index = 0;
        buf.time = 0.0;
        buf.target = buf.value;
        buf.state = 7;
    }
    let table = match buf.state {
        5 => osc.release.as_ref(),
        7 => Some(force_release),
        _ => osc.attack.as_ref(),
    };
    if table.is_none() && buf.state != 8 {
        buf.value = 1.0;
        return 1.0;
    }
    if buf.state == 0 {
        return 1.0;
    }
    if buf.state == 3 {
        return buf.value * osc.width + osc.vertex;
    }
    if buf.state == 1 {
        buf.state = 2;
        buf.index = 0;
        buf.time = 0.0;
        buf.target = 0.0;
        buf.release = 0;
    }
    let sub = if buf.state == 7 { 1.0 } else { osc.rate };
    buf.time -= sub;
    let mut guard = 0;
    while buf.time <= 0.0 {
        buf.value = buf.target;
        if buf.state == 8 {
            buf.state = 0;
            break;
        }
        let Some(t) = table else { break };
        let o = buf.index as usize * 3;
        let step = |k: usize| t.get(o + k).copied().unwrap_or(0xF);
        let (v0, v1, v2) = (step(0), step(1), step(2));
        guard += 1;
        if guard > 64 {
            // A table that jumps to itself without time passing.
            buf.state = 0;
            break;
        }
        match v0 {
            0xd => {
                buf.index = v2 as u16;
                continue;
            }
            0xf => {
                buf.state = 0;
                break;
            }
            0xe => {
                buf.state = 3;
                return buf.value * osc.width + osc.vertex;
            }
            _ => {}
        }
        buf.curve = v0 as u8;
        buf.target = v2 as f32 / 32768.0;
        buf.index += 1;
        if v1 != 0 {
            buf.time = v1 as u16 as f32 * TIME_SCALE;
            buf.delta = (buf.target - buf.value) / buf.time;
        }
    }
    let mut calc = -(buf.delta * buf.time - buf.target);
    buf.value = calc;
    match buf.curve {
        1 => calc = if calc > 0.0 { calc * calc } else { -calc * calc },
        2 => calc = if calc > 0.0 { libm::sqrtf(calc) } else { -libm::sqrtf(-calc) },
        _ => {}
    }
    calc * osc.width + osc.vertex
}

/// `jcs_`: a track's channel manager, which its voices read their volume,
/// pitch, pan and routing from.
#[derive(Clone, Copy)]
pub struct Ctl {
    pub volume: f32,
    pub pitch: f32,
    pub pan: f32,
    pub fxmix: f32,
    pub dolby: f32,
    pub bus: [u16; 6],
    pub pan_calc: [u8; 3],
    pub prio: u32,
    pub release_time: u16,
}

impl Ctl {
    /// `InitJcs`.
    pub const fn new() -> Ctl {
        Ctl {
            volume: 1.0,
            pitch: 1.0,
            pan: 0.5,
            fxmix: 0.0,
            dolby: 0.0,
            bus: [0x150, 0x210, 0x352, 0x412, 0, 0],
            pan_calc: [26, 1, 1],
            prio: 0x20103,
            release_time: 600,
        }
    }
}

/// One playing note.
pub struct Voice {
    /// `channelId`: tracks hold this to tell a reused slot apart.
    pub id: u16,
    /// The track whose manager this voice is in.
    pub mgr: Option<usize>,
    /// `lastManager == chanMgr`: still following that manager's values.
    /// A voice handed up to a parent when its track closes keeps the last
    /// values it had.
    pub follow: bool,
    pub velocity: u8,
    /// 0xFF once stopped.
    pub note: u8,
    /// Note length in subframes (-1: until note off), and the countdown.
    pub note_id: i32,
    pub countdown: i32,
    /// In its manager's releasing list.
    pub releasing: bool,
    pub osc: [Option<Osc>; 2],
    pub buf: [OscBuf; 2],
    pub base_pitch: f32,
    pub base_vol: f32,
    pub cur_pitch: f32,
    pub cur_vol: f32,
    pub pitch_mod: f32,
    pub vol_mod: f32,
    /// Power distribution, pan, fx send, dolby; each (initial, modulated,
    /// manager).
    pub pan: [[f32; 3]; 4],
    pub pan_calc: [u8; 3],
    pub mgr_pitch: f32,
    pub mgr_vol: f32,
    pub target_pitch: f32,
    pub sweep: u16,
    pub routing: [u16; 6],
    pub chan_prio: u32,
    /// DSP side: wave, position (32.32), pitch relative to the DAC rate.
    pub wave: usize,
    pub pos: u64,
    pub ratio: f32,
    /// Bus levels (0..1 of full scale) at the start and end of this
    /// subframe.
    pub lv0: [f32; 4],
    pub lv1: [f32; 4],
    /// DSP channel priority and age, for stealing.
    pub prio: u8,
    pub age: u32,
    /// The wave played to its end.
    pub done: bool,
    /// Stolen: the DSP halves its volume each subframe until silent.
    pub fading: bool,
}

/// `calc_sw_table`: how each of the 3 parts of a pan matrix combines,
/// written as base 3 digits (0 skip, 1 take, 2 multiply).
fn pan_calc(m1: &[f32; 3], m2: &[f32; 3], kind: u8) -> f32 {
    let mut d = [kind / 9 % 3, kind / 3 % 3, kind % 3];
    if kind >= 27 {
        d = [0; 3];
    }
    let mut c = 0.0;
    for i in 0..3 {
        match d[i] {
            1 => c += m1[i],
            2 => c += m1[i] * m2[i],
            _ => {}
        }
    }
    c
}

/// `sinf3`: a quarter sine from a 256-step table.
fn sinf3(x: f32) -> f32 {
    let i = (256.0 * x) as i32;
    libm::sinf(i as f32 * core::f32::consts::FRAC_PI_2 / 256.0)
}

impl Voice {
    /// `DoEffectOsc`.
    pub fn do_effect_osc(&mut self, mode: u8, v: f32) {
        match mode {
            0 => self.vol_mod *= v,
            1 => self.pitch_mod *= v,
            2 => self.pan[1][1] = v,
            3 => self.pan[2][1] = v,
            4 => self.pan[3][1] = v,
            _ => {}
        }
    }

    /// `UpdateEffecterParam`: the bus levels and pitch from the voice's own
    /// values and its manager's. `stereo` is the system output mode.
    pub fn update_effecter(&mut self, ctl: Option<&Ctl>, stereo: bool) {
        if let (true, Some(c)) = (self.follow, ctl) {
            self.mgr_pitch = c.pitch;
            self.mgr_vol = c.volume;
            self.pan[1][2] = c.pan;
            self.pan[2][2] = c.fxmix;
            self.pan[3][2] = c.dolby;
            self.pan_calc = c.pan_calc;
        }
        let (pan, fx, dolby);
        if stereo {
            pan = if self.pan_calc[0] == 0 { 0.5 } else { pan_calc(&self.pan[1], &self.pan[0], self.pan_calc[0]) };
            fx = pan_calc(&self.pan[2], &self.pan[0], self.pan_calc[1]);
            dolby = pan_calc(&self.pan[3], &self.pan[0], self.pan_calc[2]);
        } else {
            pan = 0.5;
            dolby = 0.0;
            fx = pan_calc(&self.pan[2], &self.pan[0], self.pan_calc[1]);
        }
        let (pan, fx, dolby) = (pan.clamp(0.0, 1.0), fx.clamp(0.0, 1.0), dolby.clamp(0.0, 1.0));
        let volume = self.cur_vol * self.vol_mod * self.mgr_vol;
        // finalPitch is a u16 in 4.12 that the DSP clamps to 0x7FFF.
        self.ratio = (self.cur_pitch * self.pitch_mod * self.mgr_pitch).clamp(0.0, 32767.0 / 4096.0);
        let angle = |sel: u16| match sel {
            1 => Some(pan),
            2 => Some(fx),
            3 => Some(dolby),
            5 => Some(1.0 - pan),
            6 => Some(1.0 - fx),
            7 => Some(1.0 - dolby),
            _ => None,
        };
        for i in 0..4 {
            let r = self.routing[i];
            if r >> 8 == 0 {
                self.lv1[i] = 0.0;
                continue;
            }
            let mut tmp = volume;
            for sel in [r >> 4 & 0xF, r & 0xF] {
                if sel != 0 {
                    // An unknown selector keeps the last angle in the C;
                    // none of the IPL's routings use one.
                    if let Some(a) = angle(sel) {
                        tmp *= sinf3(a);
                    }
                }
            }
            // Truncated to the u16 level the DSP gets.
            self.lv1[i] = (tmp.clamp(0.0, 1.0) * MAX_MIXERLEVEL) as u16 as f32;
        }
    }

    /// The pitch sweep `SetKeyTarget_1Shot` starts (`Extra_Update`).
    pub fn sweep_step(&mut self) {
        if self.sweep != 0 {
            self.cur_pitch += (self.target_pitch - self.cur_pitch) / self.sweep as f32;
            self.sweep -= 1;
        }
    }
}
