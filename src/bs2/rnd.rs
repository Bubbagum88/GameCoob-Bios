//! `rnd.c` (0x81307EB8..0x81307FA8): BS2's random numbers, two LCG + LFSR
//! pairs picked by a timer bit.

use crate::sdk::os::get_tick;
use core::sync::atomic::{AtomicU32, Ordering::Relaxed};

static RAND_A: AtomicU32 = AtomicU32::new(0);
static RAND_B: AtomicU32 = AtomicU32::new(0);
static RAND_C: AtomicU32 = AtomicU32::new(0);
static RAND_D: AtomicU32 = AtomicU32::new(0);

/// `RandInit` (0x81307EB8): seed all four from the timer.
pub fn rand_init() {
    RAND_A.store(get_tick(), Relaxed);
    RAND_B.store(get_tick(), Relaxed);
    RAND_C.store(get_tick(), Relaxed);
    RAND_D.store(get_tick(), Relaxed);
    rand();
}

/// One LFSR step.
fn lfsr(s: &AtomicU32) -> u32 {
    let mut v = s.load(Relaxed);
    if v & 1 != 0 {
        v ^= 0x11020;
    }
    v >>= 1;
    s.store(v, Relaxed);
    v
}

/// `Rand` (0x81307EF8).
pub fn rand() -> u32 {
    if get_tick() & 0x100 != 0 {
        let a = RAND_A.load(Relaxed).wrapping_mul(0x41C6_4E6D).wrapping_add(12345);
        RAND_A.store(a, Relaxed);
        a ^ lfsr(&RAND_B)
    } else {
        let c = RAND_C.load(Relaxed).wrapping_mul(0x41C6_4E6D).wrapping_add(67890);
        RAND_C.store(c, Relaxed);
        c.wrapping_add(lfsr(&RAND_D))
    }
}
