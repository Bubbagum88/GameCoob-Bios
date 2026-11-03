//! Host tests (run from `tools/audio-render`: `cargo test`). The data here is
//! made up in the test; nothing comes from a dump.

use super::data::decode_afc;
use super::bank::{Osc, Table};
use super::voice::{OscBuf, Tables, osc_to_ofs};
use alloc::rc::Rc;

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

fn table(v: &[i16]) -> Table {
    Rc::from(v)
}

#[test]
fn envelope_attack_hold_release() {
    // The bank's envelope: straight to full, hold, then a squared fall
    // over 500/600 s.
    let o = Osc {
        mode: 0,
        rate: 1.0,
        attack: Some(table(&[0, 0, 32767, 0, 100, 32767, 14, 0, 0])),
        release: Some(table(&[1, 500, 0, 15, 0, 0])),
        width: 1.0,
        vertex: 0.0,
    };
    let force = Tables::new().force_release;
    let mut b = OscBuf::default();
    b.state = 1;
    for _ in 0..100 {
        let v = osc_to_ofs(&o, &mut b, &force);
        assert!((v - 1.0).abs() < 1e-3, "{v}");
    }
    assert_eq!(b.state, 3);
    b.state = 4;
    let mut last = 1.0;
    let mut n = 0;
    while b.state != 0 {
        let v = osc_to_ofs(&o, &mut b, &force);
        assert!(v <= last + 1e-6);
        last = v;
        n += 1;
        assert!(n < 1000);
    }
    // 500/600 s at 32028.5/80 subframes per second.
    assert!((332..=336).contains(&n), "{n}");
}

#[test]
fn zero_osc_release_never_ends() {
    // A track's unset second oscillator (all zeros) gives 1.0 until the
    // note ends, then 0.0 for good: its rate is 0, so time never passes.
    let o = Osc::default();
    let force = Tables::new().force_release;
    let mut b = OscBuf::default();
    b.state = 1;
    assert_eq!(osc_to_ofs(&o, &mut b, &force), 1.0);
    b.state = 4;
    for _ in 0..1000 {
        assert_eq!(osc_to_ofs(&o, &mut b, &force), 0.0);
    }
    assert_ne!(b.state, 0);
}
