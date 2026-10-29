//! The SDK matrix library (`mtx.h`): 3x4 matrices that are row-major and act
//! on column vectors (`v' = M v`), like `PSMTX*` and `C_MTX*`.

pub type Mtx = [[f32; 4]; 3];
pub type Vec3 = [f32; 3];

pub const IDENTITY: Mtx = [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0]];

/// `PSMTXConcat`: `a · b`.
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

/// `PSMTXTrans`
pub fn trans(x: f32, y: f32, z: f32) -> Mtx {
    [[1.0, 0.0, 0.0, x], [0.0, 1.0, 0.0, y], [0.0, 0.0, 1.0, z]]
}

/// `PSMTXScale`
pub fn scale(x: f32, y: f32, z: f32) -> Mtx {
    [[x, 0.0, 0.0, 0.0], [0.0, y, 0.0, 0.0], [0.0, 0.0, z, 0.0]]
}

/// `PSMTXMultVec`: transform a point.
pub fn mult_vec(m: &Mtx, v: Vec3) -> Vec3 {
    [
        m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3],
        m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3],
        m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3],
    ]
}

/// `PSMTXMultVecSR`: rotate and scale a direction (no translation).
pub fn mult_vec_sr(m: &Mtx, v: Vec3) -> Vec3 {
    [
        m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2],
        m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2],
        m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2],
    ]
}

/// `C_MTXLookAt`: camera at `eye` looking at `at`.
pub fn look_at(eye: Vec3, up: Vec3, at: Vec3) -> Mtx {
    let norm = |v: Vec3| {
        let l = libm::sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]).max(1e-12);
        [v[0] / l, v[1] / l, v[2] / l]
    };
    let cross = |a: Vec3, b: Vec3| [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
    let dot = |a: Vec3, b: Vec3| a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    let look = norm([eye[0] - at[0], eye[1] - at[1], eye[2] - at[2]]);
    let right = norm(cross(up, look));
    let up = cross(look, right);
    [
        [right[0], right[1], right[2], -dot(eye, right)],
        [up[0], up[1], up[2], -dot(eye, up)],
        [look[0], look[1], look[2], -dot(eye, look)],
    ]
}

/// A 4x4 projection matrix (`Mtx44`).
pub type Mtx44 = [[f32; 4]; 4];

/// `C_MTXPerspective`: `fovy` in degrees; clip-space z runs from -w at the
/// near plane to 0 at the far plane, as GX expects.
pub fn perspective(fovy: f32, aspect: f32, n: f32, f: f32) -> Mtx44 {
    let angle = fovy * 0.5 * 0.017_453_292;
    let cot = 1.0 / libm::tanf(angle);
    let tmp = 1.0 / (f - n);
    [
        [cot / aspect, 0.0, 0.0, 0.0],
        [0.0, cot, 0.0, 0.0],
        [0.0, 0.0, -n * tmp, -(f * n) * tmp],
        [0.0, 0.0, -1.0, 0.0],
    ]
}

/// `C_MTXOrtho`
pub fn ortho(t: f32, b: f32, l: f32, r: f32, n: f32, f: f32) -> Mtx44 {
    let (x, y, z) = (1.0 / (r - l), 1.0 / (t - b), 1.0 / (f - n));
    [
        [2.0 * x, 0.0, 0.0, -(r + l) * x],
        [0.0, 2.0 * y, 0.0, -(t + b) * y],
        [0.0, 0.0, -z, -f * z],
        [0.0, 0.0, 0.0, 1.0],
    ]
}
