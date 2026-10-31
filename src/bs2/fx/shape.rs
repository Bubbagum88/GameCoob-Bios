//! Mosaic keyframes (`menu_fx_shape.c`, PAL 1.0 BS2 0x81329890..0x81329B34):
//! every cube of a shape flies along a Hermite curve through its keys.

use super::{FxCube, P};
use crate::bs2::mtx::vec_hermite;
use crate::ipl::fx::Shapes;

/// Fx_EvalShape (0x813298B0): place the cubes of shape `n` at time `t`
/// (0..1) and return how many there are. A cube starts at `0.4 * (1 -
/// start/255)` and lands at `0.6 + 0.4 * (1 - end/255)`.
pub fn eval(s: &Shapes, n: usize, out: &mut [FxCube], t: f32) -> u32 {
    let count = s.count(n) as u32;
    let nk = s.n_keys(n) as u32;
    let step = 1.0 / (nk as i32 - 1) as f32;
    let mut t = t;
    if t < 0.0 {
        t = 0.0;
    }
    if t > 1.0 {
        t = 1.0;
    }
    for i in 0..(count as usize).min(out.len()) {
        let (start, end) = s.times(n, i);
        let hi = 0.6 + 0.4 * (1.0 - end as f32 / 255.0);
        let lo = 0.4 * (1.0 - start as f32 / 255.0);
        let mut u = (t - lo) / (hi - lo);
        let seg = u / step;
        // __cvt_fp2unsigned: a cube that has not started yet is on key 0.
        let mut k = seg as u32;
        if seg as u32 >= nk {
            k = nk.wrapping_sub(1);
        }
        if u < 0.0 {
            u = 0.0;
        }
        if u > 1.0 {
            u = 1.0;
        }
        let k1 = s.key(s.key_index(n, i, k as usize + 1));
        let k0 = s.key(s.key_index(n, i, k as usize));
        let v = vec_hermite(k0.pos, k0.tan_out, k1.pos, k1.tan_in, u, k as f32 * step, (k + 1) as f32 * step);
        out[i] = FxCube { scale: [P.cube_scale; 3], rot: [0; 3], pos: v };
    }
    count
}
