//! IPL sound, rebuilt from the JAudio decomp (`decomp/src/jaudio`, map in
//! `docs/AUDIO.md`). Everything here is plain computation, so the host tool
//! `tools/audio-render` runs the same code; the sound card driver is
//! `crate::hda`.
//!
//! BS2 makes every sound through one call, `Sound_Play(id)` (and the spin and
//! ambient helpers), which writes ports of the always-running `iplrom.com`
//! sequence. That sequence interpreter is the next step of the port. Until
//! then [`Engine::sound_play`] starts the boot jingles straight from the
//! bank's instruments and the other ids are silent.

pub mod data;
pub mod mixer;
#[cfg(test)]
mod tests;

use data::SoundArchive;
use mixer::{Mixer, VoiceId};

/// The two waves (left, right) of each boot jingle, as `ipl.bnk` lays them
/// out: instrument 1 (waves 0/1), 2 (waves 2/3) and 7/6 (waves 9/10), each
/// split by key into a low and a high half. Which jingle id picks which
/// pair comes from `iplrom.com`; this order is a stand-in until the
/// sequence is ported.
const JINGLE_WAVES: [(usize, usize); 3] = [(0, 1), (2, 3), (9, 10)];

/// Plays the IPL's sounds into a stereo stream.
pub struct Engine {
    pub archive: SoundArchive,
    pub mixer: Mixer,
    jingle: [Option<VoiceId>; 2],
}

impl Engine {
    pub fn new(archive: SoundArchive, rate: u32) -> Engine {
        let mut mixer = Mixer::new(rate);
        // Two hard-panned jingle halves at full scale peak near 0 dBFS.
        mixer.master = 0.8;
        Engine { archive, mixer, jingle: [None; 2] }
    }

    /// `Sound_Play(id)` (`jaudio/ipl_8135A8E0.c`). 0..2 are the boot jingle
    /// and its two Z easter eggs.
    pub fn sound_play(&mut self, id: u32) {
        if let Some(&(l, r)) = JINGLE_WAVES.get(id as usize) {
            for v in self.jingle.iter_mut().filter_map(|v| v.take()) {
                self.mixer.stop(v);
            }
            let w = &self.archive.waves;
            self.jingle = [self.mixer.play(w, l, 1.0, [1.0, 0.0]), self.mixer.play(w, r, 1.0, [0.0, 1.0])];
        }
    }

    /// Stop every sound (gInit's reset, `fn_8135B180`).
    pub fn reset(&mut self) {
        self.mixer.stop_all();
        self.jingle = [None; 2];
    }

    /// Render the next `out.len() / 2` stereo frames (interleaved i16).
    pub fn render(&mut self, out: &mut [i16]) {
        self.mixer.render(&self.archive.waves, out);
    }
}
