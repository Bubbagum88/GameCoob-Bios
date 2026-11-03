//! The Intro Delay option's state for the system dialog: the setting and
//! what Auto picked on this PC. The hold itself, and how Auto picks, are
//! in the app's `display` module; the setting is saved as
//! `GameCoobIntroDelay`.

use core::sync::atomic::{AtomicU8, Ordering};

/// The setting (0xFF = Auto) and Auto's pick in tenths of a second, for
/// the system dialog's row.
static SETTING: AtomicU8 = AtomicU8::new(0xFF);
static AUTO_TENTHS: AtomicU8 = AtomicU8::new(0);

pub fn set_setting(s: Option<u8>) {
    SETTING.store(s.unwrap_or(0xFF), Ordering::Relaxed);
}

pub fn setting() -> Option<u8> {
    Some(SETTING.load(Ordering::Relaxed)).filter(|&s| s != 0xFF)
}

pub fn set_auto(secs: f32) {
    AUTO_TENTHS.store((secs * 10.0) as u8, Ordering::Relaxed);
}

/// Auto's pick on this PC, in seconds.
pub fn auto() -> f32 {
    AUTO_TENTHS.load(Ordering::Relaxed) as f32 / 10.0
}

/// The choices the dialog row steps through.
pub const CHOICES: [Option<u8>; 6] = [None, Some(0), Some(1), Some(2), Some(3), Some(5)];

/// The choice after `s`.
pub fn next(s: Option<u8>) -> Option<u8> {
    let i = CHOICES.iter().position(|&c| c == s).unwrap_or(0);
    CHOICES[(i + 1) % CHOICES.len()]
}
