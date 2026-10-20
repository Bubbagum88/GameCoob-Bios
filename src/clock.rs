//! Monotonic seconds from the CPU timestamp counter, calibrated against the
//! firmware's `Stall()` at startup. UEFI's wall clock is only 1 s resolution.

use core::arch::x86_64::_rdtsc;
use core::time::Duration;
use uefi::boot;

pub struct Clock {
    start: u64,
    ticks_per_sec: f64,
}

impl Clock {
    pub fn calibrate() -> Self {
        let t0 = unsafe { _rdtsc() };
        boot::stall(Duration::from_millis(50));
        let t1 = unsafe { _rdtsc() };
        let ticks_per_sec = ((t1 - t0) as f64 / 0.05).max(1.0);
        Self { start: unsafe { _rdtsc() }, ticks_per_sec }
    }

    pub fn now(&self) -> f32 {
        let t = unsafe { _rdtsc() };
        (t.wrapping_sub(self.start) as f64 / self.ticks_per_sec) as f32
    }
}
