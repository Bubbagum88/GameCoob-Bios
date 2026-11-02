//! The sound archive BS2 links into bs2.dol (RAM 0x813F7620 in PAL 1.0,
//! `jaudio/ipl_8135B360.c`), found in the user's own dump at boot:
//!
//! | file         | what                                                  |
//! | ------------ | ----------------------------------------------------- |
//! | `ipl.bnk`    | `IBNK` instrument bank (sequence port, next step)     |
//! | `ipl.ws`     | `WSYS` wave table: format, rate, loop, offset per wave |
//! | `ipl_0.aw`   | the waves, AFC 4-bit ADPCM, right after the table      |
//! | `iplrom.com` | the Jam sequence (next step)                          |
//!
//! Every wave is decoded to 16-bit PCM once at load (about 2.3 MB), so the
//! mixer never decodes ADPCM while playing.

use alloc::vec::Vec;

/// One decoded wave of `ipl_0.aw`.
pub struct Wave {
    /// Sample rate the wave was recorded at (32000 or 44100 in PAL 1.0).
    pub rate: f32,
    /// MIDI key the wave plays at its own rate.
    pub base_key: u8,
    /// Loop start and end (end exclusive), in samples.
    pub looped: Option<(u32, u32)>,
    pub pcm: Vec<i16>,
}

/// The sound archive: bank, waves, and the sequence once it is ported.
pub struct SoundArchive {
    /// ROM offset of `ipl.bnk` (the archive start).
    pub offset: usize,
    pub bank: Vec<u8>,
    pub waves: Vec<Wave>,
    /// Bytes from the end of `ipl_0.aw` on: `iplrom.com` starts here.
    pub seq: Vec<u8>,
}

/// AFC predictor coefficients (4.11 fixed point), shared by every AFC wave.
const AFC_COEF: [(i32, i32); 16] = [
    (0, 0),
    (0x800, 0),
    (0, 0x800),
    (0x400, 0x400),
    (0x1000, -0x800),
    (0xE00, -0x600),
    (0xC00, -0x400),
    (0x1200, -0xA00),
    (0x1068, -0x8C8),
    (0x12C0, -0x8FC),
    (0x1400, -0xC00),
    (0x800, -0x800),
    (0x400, -0x400),
    (-0x400, 0x400),
    (-0x400, 0),
    (-0x800, 0),
];

/// Decode AFC 4-bit ADPCM: 9-byte frames (scale/predictor byte, then 16
/// signed nibbles), `n` samples.
pub fn decode_afc(data: &[u8], n: usize) -> Vec<i16> {
    let mut out = Vec::with_capacity(n);
    let (mut h1, mut h2) = (0i32, 0i32);
    for f in data.chunks_exact(9) {
        if out.len() >= n {
            break;
        }
        let scale = 1i32 << (f[0] >> 4);
        let (c1, c2) = AFC_COEF[(f[0] & 15) as usize];
        for &b in &f[1..] {
            for nib in [b >> 4, b & 15] {
                let d = ((nib as i32) << 28) >> 28;
                let s = (((d * scale) << 11) + c1 * h1 + c2 * h2) >> 11;
                let s = s.clamp(-32768, 32767);
                h2 = h1;
                h1 = s;
                out.push(s as i16);
            }
        }
    }
    out.truncate(n);
    out
}

/// AFC 2-bit ADPCM: 5-byte frames of 16 two-bit nibbles. Not used by any
/// known IPL, decoded for completeness.
fn decode_afc2(data: &[u8], n: usize) -> Vec<i16> {
    let mut out = Vec::with_capacity(n);
    let (mut h1, mut h2) = (0i32, 0i32);
    for f in data.chunks_exact(5) {
        if out.len() >= n {
            break;
        }
        let scale = 1i32 << (f[0] >> 4);
        let (c1, c2) = AFC_COEF[(f[0] & 15) as usize];
        for &b in &f[1..] {
            for sh in [6, 4, 2, 0] {
                let d = (((b >> sh) as i32 & 3) << 30) >> 30;
                let s = (((d * scale) << 13) + c1 * h1 + c2 * h2) >> 11;
                let s = s.clamp(-32768, 32767);
                h2 = h1;
                h1 = s;
                out.push(s as i16);
            }
        }
    }
    out.truncate(n);
    out
}

fn be32(b: &[u8], o: usize) -> Option<u32> {
    Some(u32::from_be_bytes(b.get(o..o + 4)?.try_into().ok()?))
}

/// Parse `WSYS` at `ws` and decode its waves from the archive at `aw`.
/// Returns the waves and the end of the wave data (relative to `aw`).
fn parse_wsys(rom: &[u8], ws: usize, aw: usize) -> Option<(Vec<Wave>, usize)> {
    let winf = ws + be32(rom, ws + 0x10)? as usize;
    if rom.get(winf..winf + 4)? != b"WINF" {
        return None;
    }
    let mut waves = Vec::new();
    let mut end = 0usize;
    for g in 0..be32(rom, winf + 4)? as usize {
        let goff = ws + be32(rom, winf + 8 + g * 4)? as usize;
        for i in 0..be32(rom, goff + 0x70)? as usize {
            let w = ws + be32(rom, goff + 0x74 + i * 4)? as usize;
            let e = rom.get(w..w + 0x24)?;
            let fmt = e[1];
            let base_key = e[2];
            let rate = f32::from_bits(be32(e, 4)?);
            let (off, len) = (be32(e, 8)? as usize, be32(e, 12)? as usize);
            let looped = be32(e, 16)? == 0xFFFF_FFFF;
            let (ls, le, ns) = (be32(e, 20)?, be32(e, 24)?, be32(e, 28)? as usize);
            let raw = rom.get(aw + off..aw + off + len)?;
            let pcm = match fmt {
                0 => decode_afc(raw, ns),
                1 => decode_afc2(raw, ns),
                2 => raw.iter().take(ns).map(|&b| (b as i8 as i16) << 8).collect(),
                3 => raw.chunks_exact(2).take(ns).map(|c| i16::from_be_bytes([c[0], c[1]])).collect(),
                _ => return None,
            };
            end = end.max(off + len);
            let looped = (looped && ls < le && le as usize <= pcm.len()).then_some((ls, le));
            waves.push(Wave { rate: if rate > 0.0 { rate } else { 32000.0 }, base_key, looped, pcm });
        }
    }
    Some((waves, end))
}

impl SoundArchive {
    /// Find the archive in a descrambled ROM: the `IBNK` bank, the `WSYS`
    /// table after it, the waves after that (each 32-byte aligned).
    pub fn find(rom: &[u8]) -> Option<SoundArchive> {
        let bnk = (0..rom.len().saturating_sub(8)).step_by(0x20).find(|&o| &rom[o..o + 4] == b"IBNK")?;
        let bnk_len = be32(rom, bnk + 4)? as usize;
        let ws = (bnk + bnk_len + 0x1F) & !0x1F;
        if rom.get(ws..ws + 4)? != b"WSYS" {
            return None;
        }
        let aw = (ws + be32(rom, ws + 4)? as usize + 0x1F) & !0x1F;
        let (waves, end) = parse_wsys(rom, ws, aw)?;
        let seq = aw + ((end + 0x1F) & !0x1F);
        let seq = rom.get(seq..(seq + 0x1000).min(rom.len()))?.to_vec();
        Some(SoundArchive { offset: bnk, bank: rom[bnk..bnk + bnk_len].to_vec(), waves, seq })
    }
}
