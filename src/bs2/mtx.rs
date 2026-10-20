//! GameCube-style 3x4 matrices and 16-bit angles, as BS2 uses them.
//!
//! Matrices are row-major and act on column vectors (`v' = M v`), angles are
//! `u16` binary angles where 0x10000 is a full turn. Sine and cosine come
//! from a 4096-entry table like the IPL's, so rotations quantize the same way.

use alloc::vec::Vec;

pub type Mtx = [[f32; 4]; 3];

pub const IDENTITY: Mtx = [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0]];

pub struct SinTable {
    sin: Vec<f32>,
}

impl SinTable {
    /// 4096 entries, indexed by `angle >> 4`.
    pub fn new() -> Self {
        let n = 4096;
        let sin = (0..n + n / 4)
            .map(|i| libm::sinf(i as f32 * core::f32::consts::TAU / n as f32))
            .collect();
        Self { sin }
    }

    #[inline]
    pub fn sin(&self, a: i32) -> f32 {
        self.sin[((a as u16) >> 4) as usize]
    }

    #[inline]
    pub fn cos(&self, a: i32) -> f32 {
        self.sin[((a as u16) >> 4) as usize + 1024]
    }
}

/// `a · b`
pub fn concat(a: &Mtx, b: &Mtx) -> Mtx {
    let mut o = [[0.0; 4]; 3];
    for r in 0..3 {
        for c in 0..4 {
            o[r][c] = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c];
        }
        o[r][3] += a[r][3];
    }
    o
}

pub fn trans(x: f32, y: f32, z: f32) -> Mtx {
    [[1.0, 0.0, 0.0, x], [0.0, 1.0, 0.0, y], [0.0, 0.0, 1.0, z]]
}

pub fn scale(x: f32, y: f32, z: f32) -> Mtx {
    [[x, 0.0, 0.0, 0.0], [0.0, y, 0.0, 0.0], [0.0, 0.0, z, 0.0]]
}

/// `T(t) · Rz · Ry · Rx`
pub fn rot_trans(s: &SinTable, rx: i32, ry: i32, rz: i32, t: [f32; 3]) -> Mtx {
    let (sx, cx) = (s.sin(rx), s.cos(rx));
    let (sy, cy) = (s.sin(ry), s.cos(ry));
    let (sz, cz) = (s.sin(rz), s.cos(rz));
    [
        [cz * cy, -sz * cx + sx * cz * sy, sz * sx + cx * cz * sy, t[0]],
        [sz * cy, cz * cx + sx * sz * sy, -cz * sx + cx * sz * sy, t[1]],
        [-sy, cy * sx, cy * cx, t[2]],
    ]
}

pub fn rot_x(s: &SinTable, a: i32) -> Mtx {
    let (sn, cs) = (s.sin(a), s.cos(a));
    [[1.0, 0.0, 0.0, 0.0], [0.0, cs, -sn, 0.0], [0.0, sn, cs, 0.0]]
}

/// Scale the three basis columns (`m · diag(v)`).
pub fn scale_cols(m: &mut Mtx, v: [f32; 3]) {
    for r in m.iter_mut() {
        r[0] *= v[0];
        r[1] *= v[1];
        r[2] *= v[2];
    }
}

pub fn apply(m: &Mtx, v: [f32; 3]) -> [f32; 3] {
    [
        m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3],
        m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3],
        m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3],
    ]
}

/// Rotate a direction (no translation).
pub fn apply_dir(m: &Mtx, v: [f32; 3]) -> [f32; 3] {
    [
        m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2],
        m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2],
        m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2],
    ]
}

/// `C_MTXLookAt`: camera at `eye` looking at `at`.
pub fn look_at(eye: [f32; 3], up: [f32; 3], at: [f32; 3]) -> Mtx {
    let norm = |v: [f32; 3]| {
        let l = libm::sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]).max(1e-12);
        [v[0] / l, v[1] / l, v[2] / l]
    };
    let cross = |a: [f32; 3], b: [f32; 3]| {
        [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
    };
    let dot = |a: [f32; 3], b: [f32; 3]| a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    let look = norm([eye[0] - at[0], eye[1] - at[1], eye[2] - at[2]]);
    let right = norm(cross(up, look));
    let up = cross(look, right);
    [
        [right[0], right[1], right[2], -dot(eye, right)],
        [up[0], up[1], up[2], -dot(eye, up)],
        [look[0], look[1], look[2], -dot(eye, look)],
    ]
}

/// Cubic Hermite between `(t0, p0, m0)` and `(t1, p1, m1)`, tangents per unit time.
pub fn hermite(t: f32, t0: f32, p0: f32, m0: f32, t1: f32, p1: f32, m1: f32) -> f32 {
    let d = t - t0;
    let inv = 1.0 / (t1 - t0);
    let d2t = inv * d * d;
    let s2 = d2t * inv;
    let d3t2 = d * s2;
    let s3 = d3t2 * inv;
    m1 * (d3t2 - d2t)
        + m0 * (d + (d3t2 - 2.0 * d2t))
        + p0 * (1.0 + (2.0 * s3 - 3.0 * s2))
        + p1 * (-2.0 * s3 + 3.0 * s2)
}

/// Float to `s16` the way `fctiwz` + `sth` does it: truncate, keep the low 16 bits.
#[inline]
pub fn to_s16(x: f32) -> i16 {
    (x as i32) as i16
}
