//! The JAudio glue BS2 calls (`jaudio/ipl_8135A8E0.c`): `Sound_Play`,
//! `Sound_UpdateSpin` and friends. Here they only queue the call; the
//! driver drains the queue into the sound engine once per frame, so the
//! menu code can call them from anywhere, as the C does.

use core::sync::atomic::{AtomicU64, AtomicUsize, Ordering};

/// One glue call.
#[derive(Clone, Copy, PartialEq, Debug)]
pub enum SoundCmd {
    /// `Sound_Play(id)`.
    Play(u32),
    /// `Sound_UpdateSpin(mode, level)`: 0 while the cube spins in the
    /// intro, 1 for a slow spin-out turn.
    Spin(u8, f32),
    /// `Sound_StartAmbient(id)`.
    StartAmbient(u32),
    /// `Sound_StopAmbient()`.
    StopAmbient,
    /// `Sound_SetOutputMode(mode)`: 0 mono, 1 stereo.
    OutputMode(u32),
    /// gInit's reset (`fn_8135B180`): stop everything, restart the sequence.
    Reset,
}

impl SoundCmd {
    fn encode(self) -> u64 {
        let (tag, a, b) = match self {
            SoundCmd::Play(id) => (1, 0, id),
            SoundCmd::Spin(m, l) => (2, m as u32, l.to_bits()),
            SoundCmd::StartAmbient(id) => (3, 0, id),
            SoundCmd::StopAmbient => (4, 0, 0),
            SoundCmd::OutputMode(m) => (5, 0, m),
            SoundCmd::Reset => (6, 0, 0),
        };
        (tag as u64) << 56 | (a as u64 & 0xFF) << 32 | b as u64
    }

    fn decode(v: u64) -> Option<SoundCmd> {
        let (a, b) = ((v >> 32) as u8, v as u32);
        Some(match v >> 56 {
            1 => SoundCmd::Play(b),
            2 => SoundCmd::Spin(a, f32::from_bits(b)),
            3 => SoundCmd::StartAmbient(b),
            4 => SoundCmd::StopAmbient,
            5 => SoundCmd::OutputMode(b),
            6 => SoundCmd::Reset,
            _ => return None,
        })
    }
}

const SIZE: usize = 64;
static RING: [AtomicU64; SIZE] = [const { AtomicU64::new(0) }; SIZE];
static HEAD: AtomicUsize = AtomicUsize::new(0);
static TAIL: AtomicUsize = AtomicUsize::new(0);

/// Queue a call; dropped if a frame somehow queued more than fit.
pub fn push(c: SoundCmd) {
    let h = HEAD.load(Ordering::Relaxed);
    if h.wrapping_sub(TAIL.load(Ordering::Acquire)) >= SIZE {
        return;
    }
    RING[h % SIZE].store(c.encode(), Ordering::Relaxed);
    HEAD.store(h.wrapping_add(1), Ordering::Release);
}

/// Hand every queued call to `f`, oldest first.
pub fn drain(mut f: impl FnMut(SoundCmd)) {
    let h = HEAD.load(Ordering::Acquire);
    let mut t = TAIL.load(Ordering::Relaxed);
    while t != h {
        if let Some(c) = SoundCmd::decode(RING[t % SIZE].load(Ordering::Relaxed)) {
            f(c);
        }
        t = t.wrapping_add(1);
    }
    TAIL.store(t, Ordering::Release);
}

/// `Sound_Play` (0x8135AA80).
pub fn play(id: u32) {
    push(SoundCmd::Play(id));
}

/// `Sound_UpdateSpin` (0x8135A8E0).
pub fn update_spin(mode: u8, level: f32) {
    push(SoundCmd::Spin(mode, level));
}

/// `Sound_StartAmbient`.
pub fn start_ambient(id: u32) {
    push(SoundCmd::StartAmbient(id));
}

/// `Sound_StopAmbient`.
pub fn stop_ambient() {
    push(SoundCmd::StopAmbient);
}

/// `Sound_SetOutputMode`.
pub fn set_output_mode(mode: u32) {
    push(SoundCmd::OutputMode(mode));
}

/// Sound effect ids `Sound_Play` takes (`bs2/menu.h`; 0..2 are the
/// jingles, 14..21 the memory card screen's).
pub mod se {
    pub const FACE_SELECT: u32 = 3;
    pub const FACE_BACK: u32 = 4;
    pub const ENTER: u32 = 5;
    pub const EXIT: u32 = 6;
    pub const CANCEL: u32 = 7;
    pub const DECIDE: u32 = 8;
    pub const CHANGE: u32 = 9;
    pub const CURSOR_H: u32 = 10;
    pub const CURSOR: u32 = 11;
    pub const CONFIRM: u32 = 12;
    pub const BUZZER: u32 = 13;
    /// The memory card screen's (`SE_CARD_14..21`), named by where
    /// `memcard.c` plays them.
    pub const CARD_ERASED: u32 = 15;
    pub const CARD_DONE: u32 = 16;
    pub const CARD_SEND_A: u32 = 17;
    pub const CARD_SEND_B: u32 = 18;
    pub const CARD_COPIED: u32 = 19;
    pub const CARD_MOVED_FROM_A: u32 = 20;
    pub const CARD_MOVED_FROM_B: u32 = 21;
    pub const GAME_START: u32 = 22;
}
