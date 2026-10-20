//! J3D keyframe animations used by the intro: joint animation (`bck1`,
//! ANK1) and the IPL's material color animation (`IPK1`).

use super::mtx::hermite;
use alloc::vec::Vec;

fn be16(b: &[u8], o: usize) -> Option<u16> {
    Some(u16::from_be_bytes(b.get(o..o + 2)?.try_into().ok()?))
}
fn be32(b: &[u8], o: usize) -> Option<u32> {
    Some(u32::from_be_bytes(b.get(o..o + 4)?.try_into().ok()?))
}
fn bef(b: &[u8], o: usize) -> Option<f32> {
    be32(b, o).map(f32::from_bits)
}

/// One animated value: a constant or Hermite keys `(time, value, in, out)`.
#[derive(Clone, Debug)]
pub struct Track {
    keys: Vec<[f32; 4]>,
}

impl Track {
    fn read(count: u16, index: u16, tangents: u16, table: &dyn Fn(usize) -> Option<f32>, time_scale: f32) -> Option<Track> {
        let (count, index) = (count as usize, index as usize);
        if count <= 1 {
            return Some(Track { keys: alloc::vec![[0.0, table(index)?, 0.0, 0.0]] });
        }
        let stride = if tangents == 0 { 3 } else { 4 };
        let mut keys = Vec::with_capacity(count);
        for k in 0..count {
            let b = index + k * stride;
            let t = table(b)? * time_scale;
            let v = table(b + 1)?;
            // Tangents are per unit of stored time.
            let tin = table(b + 2)? / time_scale;
            let tout = if stride == 4 { table(b + 3)? / time_scale } else { tin };
            keys.push([t, v, tin, tout]);
        }
        Some(Track { keys })
    }

    pub fn sample(&self, t: f32) -> f32 {
        let k = &self.keys;
        if k.len() == 1 || t <= k[0][0] {
            return k[0][1];
        }
        let last = k[k.len() - 1];
        if t >= last[0] {
            return last[1];
        }
        let i = k.iter().position(|e| e[0] > t).unwrap_or(k.len() - 1);
        let (a, b) = (k[i - 1], k[i]);
        hermite(t, a[0], a[1], a[3], b[0], b[1], b[2])
    }
}

/// Scale, rotation (0x8000 = 180 degrees) and translation per axis.
pub struct JointAnim {
    pub s: [Track; 3],
    pub r: [Track; 3],
    pub t: [Track; 3],
}

pub struct Bck {
    pub joints: Vec<JointAnim>,
}

impl Bck {
    pub fn parse(b: &[u8]) -> Option<Bck> {
        if b.get(4..8)? != b"bck1" {
            return None;
        }
        let a = 0x20;
        if b.get(a..a + 4)? != b"ANK1" {
            return None;
        }
        let amul = *b.get(a + 9)?;
        let nj = be16(b, a + 0xC)? as usize;
        let oj = a + be32(b, a + 0x14)? as usize;
        let os = a + be32(b, a + 0x18)? as usize;
        let or = a + be32(b, a + 0x1C)? as usize;
        let ot = a + be32(b, a + 0x20)? as usize;
        let rscale = (1u32 << amul.min(15)) as f32;
        let sc = |i: usize| bef(b, os + i * 4);
        let rt = |i: usize| be16(b, or + i * 2).map(|v| v as i16 as f32 * rscale);
        let tt = |i: usize| bef(b, ot + i * 4);
        let mut joints = Vec::with_capacity(nj);
        for j in 0..nj.min(256) {
            let e = oj + j * 54;
            let trk = |ax: usize, kind: usize, tab: &dyn Fn(usize) -> Option<f32>| {
                let o = e + ax * 18 + kind * 6;
                Track::read(be16(b, o)?, be16(b, o + 2)?, be16(b, o + 4)?, tab, 1.0)
            };
            let axis3 = |kind: usize, tab: &dyn Fn(usize) -> Option<f32>| -> Option<[Track; 3]> {
                Some([trk(0, kind, tab)?, trk(1, kind, tab)?, trk(2, kind, tab)?])
            };
            joints.push(JointAnim { s: axis3(0, &sc)?, r: axis3(1, &rt)?, t: axis3(2, &tt)? });
        }
        Some(Bck { joints })
    }
}

/// Material color animation: per material, RGBA tracks driving matColor 0.
pub struct Ipk {
    pub entries: Vec<(u16, [Track; 4])>,
}

impl Ipk {
    pub fn parse(b: &[u8]) -> Option<Ipk> {
        if b.get(0..4)? != b"IPK1" {
            return None;
        }
        let remap = be16(b, 0xE)? as usize;
        let ent = be32(b, 0x10)? as usize;
        let tabs = [be32(b, 0x14)? as usize, be32(b, 0x18)? as usize, be32(b, 0x1C)? as usize, be32(b, 0x20)? as usize];
        let n = ent.checked_sub(remap)? / 2;
        // Key times are stored in seconds at 30 frames per second.
        let mut entries = Vec::with_capacity(n);
        for e in 0..n.min(256) {
            let mat = be16(b, remap + e * 2)?;
            let o = ent + e * 24;
            let ch = |c: usize| {
                let tab = |i: usize| bef(b, tabs[c] + i * 4);
                Track::read(be16(b, o + c * 6)?, be16(b, o + c * 6 + 2)?, be16(b, o + c * 6 + 4)?, &tab, 30.0)
            };
            entries.push((mat, [ch(0)?, ch(1)?, ch(2)?, ch(3)?]));
        }
        Some(Ipk { entries })
    }
}
