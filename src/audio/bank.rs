//! `ipl.bnk`, the `IBNK` instrument bank (`jaudio/bankdrv.c`, layout in
//! `decomp/include/jaudio/bx.h`). Read once at load into plain structs; the
//! pointers in the file are offsets from the `IBNK` header.
//!
//! An instrument picks a key region by key, then a velocity region by
//! velocity; that region names a wave by its sound id and scales its volume
//! and pitch. Envelopes and LFOs are `Osc`s: a mode (what they modulate),
//! a rate, and attack/release tables of (curve, time, value) steps.

use alloc::collections::BTreeMap;
use alloc::rc::Rc;
use alloc::vec::Vec;

/// An envelope table: (curve, time, value) triples, ending in a 0xD (jump),
/// 0xE (hold) or 0xF (stop) step. Shared, because `Bank_OscToOfs` tells
/// "same table" apart by address.
pub type Table = Rc<[i16]>;

/// `Osc_`: an envelope or LFO. The default is all zeros, like a track's
/// second oscillator that the sequence never sets up.
#[derive(Clone, Default)]
pub struct Osc {
    /// 0 volume, 1 pitch, 2 pan, 3 fx send, 4 dolby.
    pub mode: u8,
    pub rate: f32,
    pub attack: Option<Table>,
    pub release: Option<Table>,
    pub width: f32,
    pub vertex: f32,
}

/// `Rand_`: a random offset applied once at note on.
pub struct Rand {
    pub id: u8,
    pub value: f32,
    pub range: f32,
}

/// `Sense_`: velocity or key scaling applied once at note on.
pub struct Sense {
    pub id: u8,
    pub kind: u8,
    pub threshold: u8,
    pub min: f32,
    pub max: f32,
}

/// `Vmap_`: one velocity region.
pub struct Vmap {
    pub base_vel: u8,
    pub wave: u16,
    pub vol: f32,
    pub pitch: f32,
}

/// `InstKeymap_`: one key region.
pub struct Keymap {
    pub base_key: u8,
    pub vmaps: Vec<Vmap>,
}

/// `Inst_`.
pub struct Inst {
    pub flag: u32,
    /// `mFreqMultiplier` in the decomp, but `Play_1shot` scales the volume
    /// by it.
    pub vol: f32,
    /// `mGainMultiplier`, which `Play_1shot` scales the pitch by.
    pub pitch: f32,
    pub osc: [Option<Osc>; 2],
    pub rand: [Option<Rand>; 2],
    pub sense: [Option<Sense>; 2],
    pub keys: Vec<Keymap>,
}

pub struct Bank {
    pub inst: Vec<Option<Inst>>,
}

fn be32(b: &[u8], o: usize) -> Option<u32> {
    Some(u32::from_be_bytes(b.get(o..o + 4)?.try_into().ok()?))
}

fn f32at(b: &[u8], o: usize) -> Option<f32> {
    Some(f32::from_bits(be32(b, o)?))
}

/// A pointer field: `None` for null.
fn ptr(b: &[u8], o: usize) -> Option<Option<usize>> {
    let p = be32(b, o)? as usize;
    Some((p != 0).then_some(p))
}

struct Reader<'a> {
    b: &'a [u8],
    tables: BTreeMap<usize, Table>,
}

impl Reader<'_> {
    fn table(&mut self, o: usize) -> Option<Table> {
        if let Some(t) = self.tables.get(&o) {
            return Some(t.clone());
        }
        let mut v = Vec::new();
        for i in 0..64 {
            let at = o + i * 6;
            let s = |k: usize| self.b.get(at + k..at + k + 2).map(|x| i16::from_be_bytes([x[0], x[1]]));
            let (m, t, val) = (s(0)?, s(2)?, s(4)?);
            v.extend_from_slice(&[m, t, val]);
            if (0xD..=0xF).contains(&m) {
                break;
            }
        }
        let t: Table = v.into();
        self.tables.insert(o, t.clone());
        Some(t)
    }

    fn osc(&mut self, o: usize) -> Option<Osc> {
        let b = self.b;
        let attack = match ptr(b, o + 8)? {
            Some(p) => Some(self.table(p)?),
            None => None,
        };
        let release = match ptr(b, o + 12)? {
            Some(p) => Some(self.table(p)?),
            None => None,
        };
        Some(Osc { mode: *b.get(o)?, rate: f32at(b, o + 4)?, attack, release, width: f32at(b, o + 16)?, vertex: f32at(b, o + 20)? })
    }

    fn inst(&mut self, o: usize) -> Option<Inst> {
        let b = self.b;
        if b.get(o..o + 4)? != b"INST" {
            return None;
        }
        let mut osc = [None, None];
        let mut rand = [None, None];
        let mut sense = [None, None];
        for i in 0..2 {
            if let Some(p) = ptr(b, o + 0x10 + i * 4)? {
                osc[i] = Some(self.osc(p)?);
            }
            if let Some(p) = ptr(b, o + 0x18 + i * 4)? {
                rand[i] = Some(Rand { id: *b.get(p)?, value: f32at(b, p + 4)?, range: f32at(b, p + 8)? });
            }
            if let Some(p) = ptr(b, o + 0x20 + i * 4)? {
                sense[i] = Some(Sense {
                    id: *b.get(p)?,
                    kind: *b.get(p + 1)?,
                    threshold: *b.get(p + 2)?,
                    min: f32at(b, p + 4)?,
                    max: f32at(b, p + 8)?,
                });
            }
        }
        let mut keys = Vec::new();
        for k in 0..(be32(b, o + 0x28)? as usize).min(128) {
            let Some(kp) = ptr(b, o + 0x2C + k * 4)? else { continue };
            let mut vmaps = Vec::new();
            for v in 0..(be32(b, kp + 4)? as usize).min(128) {
                let Some(vp) = ptr(b, kp + 8 + v * 4)? else { continue };
                vmaps.push(Vmap {
                    base_vel: *b.get(vp)?,
                    wave: u16::from_be_bytes([*b.get(vp + 6)?, *b.get(vp + 7)?]),
                    vol: f32at(b, vp + 8)?,
                    pitch: f32at(b, vp + 12)?,
                });
            }
            keys.push(Keymap { base_key: *b.get(kp)?, vmaps });
        }
        Some(Inst { flag: be32(b, o + 4)?, vol: f32at(b, o + 8)?, pitch: f32at(b, o + 12)?, osc, rand, sense, keys })
    }
}

impl Bank {
    /// Parse `ipl.bnk` (starting at its `IBNK` header).
    pub fn parse(b: &[u8]) -> Option<Bank> {
        if b.get(..4)? != b"IBNK" || b.get(0x20..0x24)? != b"BANK" {
            return None;
        }
        let mut r = Reader { b, tables: BTreeMap::new() };
        let mut inst = Vec::with_capacity(128);
        for i in 0..128 {
            inst.push(match ptr(b, 0x24 + i * 4)? {
                Some(p) => r.inst(p),
                None => None,
            });
        }
        Some(Bank { inst })
    }

    /// `Bank_GetInstVmap`: the velocity region for `key`/`vel`.
    pub fn vmap(&self, prog: usize, key: u8, vel: u8) -> Option<(&Inst, &Vmap)> {
        let inst = self.inst.get(prog)?.as_ref()?;
        let km = inst.keys.iter().find(|k| key <= k.base_key)?;
        let vm = km.vmaps.iter().find(|v| vel <= v.base_vel)?;
        Some((inst, vm))
    }
}
