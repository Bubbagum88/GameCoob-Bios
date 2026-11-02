//! The alpha ramp every fading 2D element of the menu uses (`fader.c`,
//! 0x8130A86C..0x8130AB0C).

/// `FADE_IN`, `FADE_OUT`, `FADE_HALF`.
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Fade {
    In,
    Out,
    Half,
}

/// `Fader`: an alpha ramp of `length` frames after `delay` frames.
#[derive(Clone, Copy, Debug)]
pub struct Fader {
    pub frame: i16,
    pub length: i16,
    pub delay: i16,
    /// Frames since the last reset.
    pub ticks: u16,
    pub max: u8,
    /// Vertical slide while fading, in pixels.
    pub slide: i16,
}

impl Default for Fader {
    /// `Fader_Init` (0x8130AAEC).
    fn default() -> Self {
        Fader { frame: 0, length: 20, delay: 0, ticks: 0, max: 255, slide: 0 }
    }
}

impl Fader {
    pub fn with_delay(delay: i16) -> Fader {
        Fader { delay, ..Default::default() }
    }

    /// `Fader_Reset` (0x8130A86C).
    pub fn reset(&mut self) {
        self.frame = 0;
        self.ticks = 0;
    }

    /// `Fader_Step` (0x8130A87C).
    pub fn step(&mut self, mode: Fade) {
        match mode {
            Fade::In => {
                self.frame += 1;
                if self.frame > self.length + self.delay {
                    self.frame = self.length + self.delay;
                }
            }
            Fade::Out => {
                self.frame -= 1;
                if self.frame < self.delay {
                    self.frame = 0;
                }
            }
            Fade::Half => {
                if self.frame >= self.delay {
                    let half = self.length / 2;
                    if self.frame - self.delay > half {
                        self.frame -= 1;
                    } else if self.frame - self.delay < half {
                        self.frame += 1;
                    } else {
                        self.frame = half + self.delay;
                    }
                } else {
                    self.frame += 1;
                }
            }
        }
        self.ticks = self.ticks.wrapping_add(1);
    }

    /// In while `on`, out otherwise.
    pub fn step_if(&mut self, on: bool) {
        self.step(if on { Fade::In } else { Fade::Out });
    }

    /// `Fader_GetAlpha` (0x8130A964): alpha 0..max.
    pub fn alpha(&self) -> i32 {
        self.alpha_slide().0
    }

    /// `Fader_GetAlpha` with the slide offset (12.4 fixed point).
    pub fn alpha_slide(&self) -> (i32, i32) {
        let (frame, delay, length) = (self.frame as i32, self.delay as i32, self.length as i32);
        if frame >= delay {
            (self.max as i32 * (frame - delay) / length, self.slide as i32 * 16 * (delay + (length - frame)) / length)
        } else {
            (0, self.slide as i32 * 16)
        }
    }
}
