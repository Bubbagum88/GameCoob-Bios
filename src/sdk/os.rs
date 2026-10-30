//! `OSGetTick`, the one OS call ported so far.

#[cfg(not(target_os = "uefi"))]
use core::sync::atomic::{AtomicU32, Ordering};

/// The host tools' stand-in time base: it moves on 0x40 a read, so bit
/// 0x100 flips every four reads and renders come out the same every run.
#[cfg(not(target_os = "uefi"))]
static TICK: AtomicU32 = AtomicU32::new(0);

/// `OSGetTick`: the low 32 bits of the time base. On UEFI the CPU's time
/// stamp counter, slowed to roughly the console's 40.5 MHz.
pub fn get_tick() -> u32 {
    #[cfg(target_os = "uefi")]
    {
        // SAFETY: rdtsc has no preconditions on x86_64.
        (unsafe { core::arch::x86_64::_rdtsc() } >> 6) as u32
    }
    #[cfg(not(target_os = "uefi"))]
    {
        TICK.fetch_add(0x40, Ordering::Relaxed)
    }
}
