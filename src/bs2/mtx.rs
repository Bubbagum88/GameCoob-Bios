//! BS2 math (`mtx.c`, 0x81307208..0x81307D08): rotations from 16-bit
//! binary angles (0x10000 is a full turn) and the sine table they read.
//!
//! Like the IPL, sine and cosine come from one global table built by
//! [`sin_table_init`] (BS2Main builds it with 12 bits, 4096 entries), so
//! rotations quantize the same way. Angles are taken as `i32` and only the
//! low 16 bits count, which is what passing them as `s16` does in C.

use crate::sdk::mtx::{Mtx, Vec3};
use alloc::vec::Vec;
use core::sync::atomic::{AtomicPtr, AtomicU32, Ordering};

/// `sSinTable`: `count * 5 / 4` entries, so the cosine is the same table a
/// quarter turn on (`sCosTable`).
static SIN_TABLE: AtomicPtr<f32> = AtomicPtr::new(core::ptr::null_mut());
/// `sSinCount` (entries per turn) and `sSinShift` (16 - bits).
static SIN_COUNT: AtomicU32 = AtomicU32::new(0);
static SIN_SHIFT: AtomicU32 = AtomicU32::new(0);

/// `SinTableInit` (0x81307B0C): a table of `2^bits` sines per turn, plus a
/// quarter turn for the cosines. Built once; later calls keep the first
/// table.
pub fn sin_table_init(bits: u8) {
    if !SIN_TABLE.load(Ordering::Acquire).is_null() {
        return;
    }
    let count = 1u32 << bits;
    let table: Vec<f32> = (0..count + count / 4).map(|j| libm::sinf(6.283_185_5 / count as f32 * j as f32)).collect();
    SIN_SHIFT.store(16 - bits as u32, Ordering::Relaxed);
    SIN_COUNT.store(count, Ordering::Relaxed);
    SIN_TABLE.store(table.leak().as_mut_ptr(), Ordering::Release);
}

#[inline]
fn table_at(i: u32) -> f32 {
    let p = SIN_TABLE.load(Ordering::Acquire);
    assert!(!p.is_null(), "sin_table_init not called");
    // SAFETY: the table is leaked (never freed or written after init) and
    // holds count * 5/4 entries; i < count + count / 4.
    unsafe { *p.add(i as usize) }
}

/// `SinS` (0x81307CD0)
#[inline]
pub fn sin_s(angle: i32) -> f32 {
    table_at((angle as u16 as u32) >> SIN_SHIFT.load(Ordering::Relaxed))
}

/// `CosS` (0x81307CEC)
#[inline]
pub fn cos_s(angle: i32) -> f32 {
    table_at(((angle as u16 as u32) >> SIN_SHIFT.load(Ordering::Relaxed)) + SIN_COUNT.load(Ordering::Relaxed) / 4)
}

/// `MtxRotAxis` (0x81307208): rotation about one axis (`'x'`, `'y'` or
/// `'z'`, either case). Any other axis leaves the matrix all zero, where the
/// C version leaves it untouched.
pub fn rot_axis(axis: char, angle: i32) -> Mtx {
    let (s, c) = (sin_s(angle), cos_s(angle));
    match axis {
        'x' | 'X' => [[1.0, 0.0, 0.0, 0.0], [0.0, c, -s, 0.0], [0.0, s, c, 0.0]],
        'y' | 'Y' => [[c, 0.0, s, 0.0], [0.0, 1.0, 0.0, 0.0], [-s, 0.0, c, 0.0]],
        'z' | 'Z' => [[c, -s, 0.0, 0.0], [s, c, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0]],
        _ => [[0.0; 4]; 3],
    }
}

/// `MtxRotTrans` (0x813073B4): `T(t) · Rz · Ry · Rx`.
pub fn rot_trans(rx: i32, ry: i32, rz: i32, t: Vec3) -> Mtx {
    let (sx, cx) = (sin_s(rx), cos_s(rx));
    let (sy, cy) = (sin_s(ry), cos_s(ry));
    let (sz, cz) = (sin_s(rz), cos_s(rz));
    let czsy = cz * sy;
    let szsy = sz * sy;
    [
        [cz * cy, -sz * cx + sx * czsy, sz * sx + cx * czsy, t[0]],
        [sz * cy, cz * cx + sx * szsy, -cz * sx + cx * szsy, t[1]],
        [-sy, cy * sx, cy * cx, t[2]],
    ]
}

/// `MtxRot` (0x81307510): `Rz · Ry · Rx`.
#[allow(dead_code)] // no caller ported yet
pub fn rot(rx: i32, ry: i32, rz: i32) -> Mtx {
    rot_trans(rx, ry, rz, [0.0; 3])
}

/// Scale / rotate / translate record (`SRT`).
#[derive(Clone, Copy)]
pub struct Srt {
    pub s: Vec3,
    pub r: [i16; 3],
    pub t: Vec3,
}

/// `MtxFromSRT` (0x8130764C): [`rot_trans`] with each basis column scaled
/// where its scale is not 1.
pub fn from_srt(srt: &Srt) -> Mtx {
    let mut m = rot_trans(srt.r[0] as i32, srt.r[1] as i32, srt.r[2] as i32, srt.t);
    for (c, &s) in srt.s.iter().enumerate() {
        if s != 1.0 {
            for row in m.iter_mut() {
                row[c] *= s;
            }
        }
    }
    m
}

/// `MtxScaleCols` (0x81307754): scale the three basis columns (`m · diag(s)`).
pub fn scale_cols(m: &mut Mtx, s: Vec3) {
    for row in m.iter_mut() {
        row[0] *= s[0];
        row[1] *= s[1];
        row[2] *= s[2];
    }
}

/// `MtxInverse33` (0x81307820): the transposed inverse of the 3x3 part
/// (the normal matrix) with the translation cleared, or `None` when it is
/// singular.
pub fn inverse33(a: &Mtx) -> Option<Mtx> {
    let det = a[0][0] * a[1][1] * a[2][2] + a[0][1] * a[1][2] * a[2][0] + a[0][2] * a[1][0] * a[2][1]
        - a[2][0] * a[1][1] * a[0][2]
        - a[1][0] * a[0][1] * a[2][2]
        - a[0][0] * a[2][1] * a[1][2];
    if det == 0.0 {
        return None;
    }
    let inv = 1.0 / det;
    Some([
        [
            inv * (a[1][1] * a[2][2] - a[2][1] * a[1][2]),
            inv * -(a[1][0] * a[2][2] - a[2][0] * a[1][2]),
            inv * (a[1][0] * a[2][1] - a[2][0] * a[1][1]),
            0.0,
        ],
        [
            inv * -(a[0][1] * a[2][2] - a[2][1] * a[0][2]),
            inv * (a[0][0] * a[2][2] - a[2][0] * a[0][2]),
            inv * -(a[0][0] * a[2][1] - a[2][0] * a[0][1]),
            0.0,
        ],
        [
            inv * (a[0][1] * a[1][2] - a[1][1] * a[0][2]),
            inv * -(a[0][0] * a[1][2] - a[1][0] * a[0][2]),
            inv * (a[0][0] * a[1][1] - a[1][0] * a[0][1]),
            0.0,
        ],
    ])
}

/// `VecHermite` (0x813079FC): [`hermite`](super::gbase::hermite) per
/// component between key times `t0` and `t1`, holding the end points outside.
#[allow(clippy::too_many_arguments)]
#[allow(dead_code)] // no caller ported yet
pub fn vec_hermite(p0: Vec3, v0: Vec3, p1: Vec3, v1: Vec3, t: f32, t0: f32, t1: f32) -> Vec3 {
    if t < t0 {
        p0
    } else if t > t1 {
        p1
    } else {
        core::array::from_fn(|i| super::gbase::hermite(t, t0, p0[i], v0[i], t1, p1[i], v1[i]))
    }
}

/// Float to `s16` the way `fctiwz` + `sth` does it: truncate, keep the low
/// 16 bits. (Not a BS2 function: what every `(s16)` cast of a float compiles to.)
#[inline]
pub fn to_s16(x: f32) -> i16 {
    (x as i32) as i16
}
