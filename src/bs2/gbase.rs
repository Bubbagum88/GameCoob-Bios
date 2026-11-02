//! BS2 base helpers (`gBase.c`, 0x81307D08..): the Hermite curve every
//! animation uses. The heap (`HeapInit`, `HeapAlloc`) is Rust's allocator.

/// `Hermite` (0x81307D08): cubic Hermite between `(t0, p0, v0)` and
/// `(t1, p1, v1)`, tangents per unit time.
pub fn hermite(t: f32, t0: f32, p0: f32, v0: f32, t1: f32, p1: f32, v1: f32) -> f32 {
    let d = t1 - t0;
    let u = t - t0;
    let inv = 1.0 / d;
    let s2 = u * u * inv;
    let s2n = s2 * inv;
    let s3 = u * s2n;
    let s3n = s3 * inv;
    p0 * (2.0 * s3n - 3.0 * s2n + 1.0) + p1 * (-2.0 * s3n + 3.0 * s2n) + v0 * (s3 - 2.0 * s2 + u) + v1 * (s3 - s2)
}
