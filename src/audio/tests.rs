//! Host tests (run from `tools/audio-render`: `cargo test`). The data here is
//! made up in the test; nothing comes from a dump.

use super::data::{Wave, decode_afc};
use super::mixer::Mixer;
use alloc::vec;

#[test]
fn afc_frame_decodes() {
    // Predictor 0 (no history), scale 2^1: nibbles 1, -1, 7, -8, then zeros.
    let mut f = [0u8; 9];
    f[0] = 0x10;
    f[1] = 0x1F;
    f[2] = 0x78;
    let s = decode_afc(&f, 16);
    assert_eq!(&s[..5], &[2, -2, 14, -16, 0]);
    // Predictor 1 (h1 * 1.0): a step then holding.
    f[0] = 0x01;
    f[1] = 0x10;
    f[2] = 0;
    let s = decode_afc(&f, 4);
    assert_eq!(s, vec![1, 1, 1, 1]);
}

#[test]
fn afc_clamps() {
    let mut f = [0x77u8; 9];
    f[0] = 0xF0;
    let s = decode_afc(&f, 16);
    assert!(s.iter().all(|&x| x == 32767));
}

fn ramp(rate: f32, n: usize, looped: Option<(u32, u32)>) -> Wave {
    Wave { rate, base_key: 60, looped, pcm: (0..n).map(|i| (i % 256) as i16 * 100).collect() }
}

#[test]
fn mixer_same_rate_is_identity_and_ends() {
    let waves = [ramp(48000.0, 64, None)];
    let mut m = Mixer::new(48000);
    m.play(&waves, 0, 1.0, [1.0, 0.0]);
    let mut out = vec![0i16; 2 * 100];
    m.render(&waves, &mut out);
    for i in 0..64 {
        assert!((out[2 * i] as i32 - i as i32 * 100).abs() <= 1, "frame {i}: {}", out[2 * i]);
        assert_eq!(out[2 * i + 1], 0);
    }
    assert_eq!(m.active(), 0);
}

#[test]
fn mixer_resamples_and_loops() {
    let waves = [ramp(32000.0, 3200, Some((1600, 3200)))];
    let mut m = Mixer::new(48000);
    m.play(&waves, 0, 1.0, [0.5, 0.5]);
    let mut out = vec![0i16; 2 * 48000];
    m.render(&waves, &mut out);
    // 1 s at 48 kHz = 32000 source samples: still looping.
    assert_eq!(m.active(), 1);
    // Frame 3 is source sample 2 (3 * 2/3): 200 * 0.5.
    assert!((out[6] - 100).abs() <= 1);
}
