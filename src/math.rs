//! Tiny vector/matrix helpers. `no_std` has no float trig, so this leans on `libm`.

use core::ops::{Add, Mul, Neg, Sub};

pub const PI: f32 = core::f32::consts::PI;

pub fn sin(x: f32) -> f32 {
    libm::sinf(x)
}
pub fn cos(x: f32) -> f32 {
    libm::cosf(x)
}
pub fn sqrt(x: f32) -> f32 {
    libm::sqrtf(x)
}

pub fn clamp01(t: f32) -> f32 {
    t.clamp(0.0, 1.0)
}

pub fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + (b - a) * t
}

/// Smoothstep easing on a 0..1 parameter.
pub fn ease(t: f32) -> f32 {
    let t = clamp01(t);
    t * t * (3.0 - 2.0 * t)
}

/// Overshooting ease for "settle into place" motion.
pub fn ease_back(t: f32) -> f32 {
    let t = clamp01(t);
    let s = 1.70158;
    let u = t - 1.0;
    1.0 + u * u * ((s + 1.0) * u + s)
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Vec3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

pub const fn v3(x: f32, y: f32, z: f32) -> Vec3 {
    Vec3 { x, y, z }
}

impl Vec3 {
    pub fn dot(self, o: Vec3) -> f32 {
        self.x * o.x + self.y * o.y + self.z * o.z
    }
    pub fn cross(self, o: Vec3) -> Vec3 {
        v3(
            self.y * o.z - self.z * o.y,
            self.z * o.x - self.x * o.z,
            self.x * o.y - self.y * o.x,
        )
    }
    pub fn len(self) -> f32 {
        sqrt(self.dot(self))
    }
    pub fn norm(self) -> Vec3 {
        let l = self.len();
        if l > 0.0 { self * (1.0 / l) } else { self }
    }
    pub fn lerp(self, o: Vec3, t: f32) -> Vec3 {
        self + (o - self) * t
    }
}

impl Add for Vec3 {
    type Output = Vec3;
    fn add(self, o: Vec3) -> Vec3 {
        v3(self.x + o.x, self.y + o.y, self.z + o.z)
    }
}
impl Sub for Vec3 {
    type Output = Vec3;
    fn sub(self, o: Vec3) -> Vec3 {
        v3(self.x - o.x, self.y - o.y, self.z - o.z)
    }
}
impl Mul<f32> for Vec3 {
    type Output = Vec3;
    fn mul(self, s: f32) -> Vec3 {
        v3(self.x * s, self.y * s, self.z * s)
    }
}
impl Neg for Vec3 {
    type Output = Vec3;
    fn neg(self) -> Vec3 {
        v3(-self.x, -self.y, -self.z)
    }
}

/// Row-major 3x3 rotation matrix.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Mat3(pub [[f32; 3]; 3]);

impl Mat3 {
    pub const IDENTITY: Mat3 = Mat3([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]);

    /// Rotation of `a` radians about unit axis `k` (Rodrigues).
    pub fn axis_angle(k: Vec3, a: f32) -> Mat3 {
        let (s, c) = (sin(a), cos(a));
        let t = 1.0 - c;
        let (x, y, z) = (k.x, k.y, k.z);
        Mat3([
            [t * x * x + c, t * x * y - s * z, t * x * z + s * y],
            [t * x * y + s * z, t * y * y + c, t * y * z - s * x],
            [t * x * z - s * y, t * y * z + s * x, t * z * z + c],
        ])
    }
    pub fn rot_x(a: f32) -> Mat3 {
        Self::axis_angle(v3(1.0, 0.0, 0.0), a)
    }
    pub fn rot_y(a: f32) -> Mat3 {
        Self::axis_angle(v3(0.0, 1.0, 0.0), a)
    }

    pub fn apply(&self, v: Vec3) -> Vec3 {
        let m = &self.0;
        v3(
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z,
        )
    }

    pub fn mul(&self, o: &Mat3) -> Mat3 {
        let mut r = [[0.0; 3]; 3];
        for (i, row) in r.iter_mut().enumerate() {
            for (j, cell) in row.iter_mut().enumerate() {
                *cell = (0..3).map(|k| self.0[i][k] * o.0[k][j]).sum();
            }
        }
        Mat3(r)
    }

    /// Snap entries to -1/0/1. Keeps accumulated 90-degree rolls from drifting.
    pub fn snapped(&self) -> Mat3 {
        let mut r = self.0;
        for row in r.iter_mut() {
            for v in row.iter_mut() {
                *v = libm::roundf(*v);
            }
        }
        Mat3(r)
    }
}
