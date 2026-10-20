//! GameCube GX texture decoding to RGBA8, plus a small sampler.

use super::read::be16;
use super::{Error, Result};
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;

pub const I4: u8 = 0;
pub const I8: u8 = 1;
pub const IA4: u8 = 2;
pub const IA8: u8 = 3;
pub const RGB565: u8 = 4;
pub const RGB5A3: u8 = 5;
pub const RGBA8: u8 = 6;
pub const C4: u8 = 8;
pub const C8: u8 = 9;
pub const C14X2: u8 = 10;
pub const CMPR: u8 = 14;

/// Palette entry formats.
pub const TL_IA8: u8 = 0;
pub const TL_RGB565: u8 = 1;
pub const TL_RGB5A3: u8 = 2;

/// Wrap modes (`GX_CLAMP`, `GX_REPEAT`, `GX_MIRROR`).
pub const CLAMP: u8 = 0;
pub const REPEAT: u8 = 1;
pub const MIRROR: u8 = 2;

/// Human-readable name of a texture format id.
pub fn format_name(fmt: u8) -> &'static str {
    match fmt {
        I4 => "I4",
        I8 => "I8",
        IA4 => "IA4",
        IA8 => "IA8",
        RGB565 => "RGB565",
        RGB5A3 => "RGB5A3",
        RGBA8 => "RGBA8",
        C4 => "C4",
        C8 => "C8",
        C14X2 => "C14X2",
        CMPR => "CMPR",
        _ => "?",
    }
}

/// (block width, block height, bytes per block) of a format.
pub fn block(fmt: u8) -> Option<(usize, usize, usize)> {
    Some(match fmt {
        I4 | C4 | CMPR => (8, 8, 32),
        I8 | IA4 | C8 => (8, 4, 32),
        IA8 | RGB565 | RGB5A3 | C14X2 => (4, 4, 32),
        RGBA8 => (4, 4, 64),
        _ => return None,
    })
}

/// Bytes of mip level 0 of a `w`x`h` texture.
pub fn data_size(fmt: u8, w: usize, h: usize) -> Option<usize> {
    let (bw, bh, bb) = block(fmt)?;
    Some(w.div_ceil(bw) * h.div_ceil(bh) * bb)
}

pub fn is_paletted(fmt: u8) -> bool {
    matches!(fmt, C4 | C8 | C14X2)
}

#[inline]
pub fn rgb565(v: u16) -> [u8; 4] {
    let (r, g, b) = ((v >> 11) & 31, (v >> 5) & 63, v & 31);
    [(r << 3 | r >> 2) as u8, (g << 2 | g >> 4) as u8, (b << 3 | b >> 2) as u8, 255]
}

#[inline]
pub fn rgb5a3(v: u16) -> [u8; 4] {
    if v & 0x8000 != 0 {
        let (r, g, b) = ((v >> 10) & 31, (v >> 5) & 31, v & 31);
        [(r << 3 | r >> 2) as u8, (g << 3 | g >> 2) as u8, (b << 3 | b >> 2) as u8, 255]
    } else {
        let (a, r, g, b) = ((v >> 12) & 7, (v >> 8) & 15, (v >> 4) & 15, v & 15);
        [(r * 17) as u8, (g * 17) as u8, (b * 17) as u8, ((a << 5) | (a << 2) | (a >> 1)) as u8]
    }
}

#[inline]
pub fn ia8(v: u16) -> [u8; 4] {
    let (a, i) = ((v >> 8) as u8, v as u8);
    [i, i, i, a]
}

/// Decode `count` palette entries of format `tl_fmt`.
pub fn decode_palette(data: &[u8], tl_fmt: u8, count: usize) -> Result<Vec<[u8; 4]>> {
    let conv: fn(u16) -> [u8; 4] = match tl_fmt {
        TL_IA8 => ia8,
        TL_RGB565 => rgb565,
        TL_RGB5A3 => rgb5a3,
        f => return Err(Error::Unsupported(f as u32)),
    };
    (0..count).map(|i| be16(data, i * 2).map(conv)).collect()
}

fn cmpr_sub(blk: &[u8], out: &mut [[u8; 4]; 16]) {
    let c0 = u16::from_be_bytes([blk[0], blk[1]]);
    let c1 = u16::from_be_bytes([blk[2], blk[3]]);
    let (a, b) = (rgb565(c0), rgb565(c1));
    let mix = |wa: u16, wb: u16, d: u16| -> [u8; 4] {
        let f = |i: usize| ((wa * a[i] as u16 + wb * b[i] as u16) / d) as u8;
        [f(0), f(1), f(2), 255]
    };
    let cols = if c0 > c1 {
        [a, b, mix(2, 1, 3), mix(1, 2, 3)]
    } else {
        [a, b, mix(1, 1, 2), [0, 0, 0, 0]]
    };
    let idx = u32::from_be_bytes([blk[4], blk[5], blk[6], blk[7]]);
    for (k, o) in out.iter_mut().enumerate() {
        *o = cols[((idx >> (30 - 2 * k)) & 3) as usize];
    }
}

/// Decode mip level 0 of a GX texture into `w * h * 4` RGBA bytes.
/// Paletted formats need `palette`.
pub fn decode(data: &[u8], fmt: u8, w: usize, h: usize, palette: Option<&[[u8; 4]]>) -> Result<Vec<u8>> {
    let (bw, bh, bb) = block(fmt).ok_or(Error::Unsupported(fmt as u32))?;
    let need = data_size(fmt, w, h).ok_or(Error::Unsupported(fmt as u32))?;
    if data.len() < need {
        return Err(Error::Truncated(need));
    }
    let pal = if is_paletted(fmt) {
        match palette {
            Some(p) if !p.is_empty() => p,
            _ => return Err(Error::BadValue(0)),
        }
    } else {
        &[][..]
    };
    let pick = |i: usize| pal[i % pal.len()];
    let mut out = vec![0u8; w * h * 4];
    let mut put = |x: usize, y: usize, c: [u8; 4]| {
        if x < w && y < h {
            let o = (y * w + x) * 4;
            out[o..o + 4].copy_from_slice(&c);
        }
    };
    let mut p = 0;
    for by in (0..h).step_by(bh) {
        for bx in (0..w).step_by(bw) {
            let blk = &data[p..p + bb];
            match fmt {
                I4 | C4 => {
                    for k in 0..64 {
                        let v = (blk[k >> 1] >> if k & 1 == 0 { 4 } else { 0 }) & 15;
                        let c = if fmt == I4 {
                            let v = v * 17;
                            [v, v, v, v]
                        } else {
                            pick(v as usize)
                        };
                        put(bx + k % 8, by + k / 8, c);
                    }
                }
                I8 | C8 => {
                    for k in 0..32 {
                        let v = blk[k];
                        let c = if fmt == I8 { [v, v, v, v] } else { pick(v as usize) };
                        put(bx + k % 8, by + k / 8, c);
                    }
                }
                IA4 => {
                    for k in 0..32 {
                        let (a, i) = ((blk[k] >> 4) * 17, (blk[k] & 15) * 17);
                        put(bx + k % 8, by + k / 8, [i, i, i, a]);
                    }
                }
                IA8 | RGB565 | RGB5A3 | C14X2 => {
                    for k in 0..16 {
                        let v = u16::from_be_bytes([blk[2 * k], blk[2 * k + 1]]);
                        let c = match fmt {
                            IA8 => ia8(v),
                            RGB565 => rgb565(v),
                            RGB5A3 => rgb5a3(v),
                            _ => pick((v & 0x3FFF) as usize),
                        };
                        put(bx + k % 4, by + k / 4, c);
                    }
                }
                RGBA8 => {
                    for k in 0..16 {
                        let c = [blk[2 * k + 1], blk[32 + 2 * k], blk[33 + 2 * k], blk[2 * k]];
                        put(bx + k % 4, by + k / 4, c);
                    }
                }
                _ => {
                    // CMPR: 2x2 DXT1-like sub-blocks.
                    let mut sub = [[0u8; 4]; 16];
                    for s in 0..4 {
                        cmpr_sub(&blk[s * 8..s * 8 + 8], &mut sub);
                        let (sx, sy) = ((s & 1) * 4, (s >> 1) * 4);
                        for (k, &c) in sub.iter().enumerate() {
                            put(bx + sx + k % 4, by + sy + k / 4, c);
                        }
                    }
                }
            }
            p += bb;
        }
    }
    Ok(out)
}

/// A decoded texture (mip level 0) with its sampler state.
#[derive(Clone, Debug)]
pub struct Texture {
    pub name: String,
    /// GX format id it was stored in (see [`format_name`]).
    pub format: u8,
    pub width: u16,
    pub height: u16,
    /// [`CLAMP`], [`REPEAT`] or [`MIRROR`].
    pub wrap_s: u8,
    pub wrap_t: u8,
    /// GX filter ids (0 = near, 1 = linear, 2.. = mipmap variants).
    pub min_filter: u8,
    pub mag_filter: u8,
    pub mip_count: u8,
    /// `width * height * 4` bytes, row-major, RGBA.
    pub rgba: Vec<u8>,
}

#[inline]
fn floorf(x: f32) -> f32 {
    libm::floorf(x)
}

fn wrap(i: i32, n: i32, mode: u8) -> usize {
    (match mode {
        REPEAT => i.rem_euclid(n),
        MIRROR => {
            let m = i.rem_euclid(2 * n);
            if m >= n { 2 * n - 1 - m } else { m }
        }
        _ => i.clamp(0, n - 1),
    }) as usize
}

impl Texture {
    /// Texel at integer coordinates after applying the wrap modes.
    pub fn texel(&self, x: i32, y: i32) -> [u8; 4] {
        let (w, h) = (self.width as i32, self.height as i32);
        if w == 0 || h == 0 {
            return [255; 4];
        }
        let o = (wrap(y, h, self.wrap_t) * w as usize + wrap(x, w, self.wrap_s)) * 4;
        match self.rgba.get(o..o + 4) {
            Some(p) => [p[0], p[1], p[2], p[3]],
            None => [255; 4],
        }
    }

    /// Nearest-neighbour sample at normalized (s, t).
    pub fn sample_nearest(&self, s: f32, t: f32) -> [u8; 4] {
        let x = floorf(s * self.width as f32) as i32;
        let y = floorf(t * self.height as f32) as i32;
        self.texel(x, y)
    }

    /// Bilinear sample at normalized (s, t), returned as 0..1 floats.
    pub fn sample_bilinear(&self, s: f32, t: f32) -> [f32; 4] {
        let fx = s * self.width as f32 - 0.5;
        let fy = t * self.height as f32 - 0.5;
        let (x0, y0) = (floorf(fx), floorf(fy));
        let (ax, ay) = (fx - x0, fy - y0);
        let (x0, y0) = (x0 as i32, y0 as i32);
        let (a, b) = (self.texel(x0, y0), self.texel(x0 + 1, y0));
        let (c, d) = (self.texel(x0, y0 + 1), self.texel(x0 + 1, y0 + 1));
        let mut out = [0.0; 4];
        for i in 0..4 {
            let top = a[i] as f32 + (b[i] as f32 - a[i] as f32) * ax;
            let bot = c[i] as f32 + (d[i] as f32 - c[i] as f32) * ax;
            out[i] = (top + (bot - top) * ay) * (1.0 / 255.0);
        }
        out
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn i8_tiling() {
        // 8x4 block: byte k is pixel (k % 8, k / 8). Two blocks side by side.
        let data: Vec<u8> = (0..64).collect();
        let px = decode(&data, I8, 16, 4, None).unwrap();
        let at = |x: usize, y: usize| px[(y * 16 + x) * 4];
        assert_eq!(at(0, 0), 0);
        assert_eq!(at(7, 3), 31);
        assert_eq!(at(8, 0), 32);
        assert_eq!(at(15, 3), 63);
    }

    #[test]
    fn i4_and_ia4() {
        let mut data = vec![0u8; 32];
        data[0] = 0xF1; // pixel 0 = 15, pixel 1 = 1
        let px = decode(&data, I4, 8, 8, None).unwrap();
        assert_eq!(&px[0..4], &[255; 4]);
        assert_eq!(&px[4..8], &[17; 4]);
        let px = decode(&data, IA4, 8, 4, None).unwrap();
        assert_eq!(&px[0..4], &[17, 17, 17, 255]);
    }

    #[test]
    fn rgba8_planes() {
        // AR plane then GB plane.
        let mut data = vec![0u8; 64];
        data[0] = 0x80; // A
        data[1] = 0x11; // R
        data[32] = 0x22; // G
        data[33] = 0x33; // B
        let px = decode(&data, RGBA8, 4, 4, None).unwrap();
        assert_eq!(&px[0..4], &[0x11, 0x22, 0x33, 0x80]);
    }

    #[test]
    fn colour_conversions() {
        assert_eq!(rgb565(0xFFFF), [255, 255, 255, 255]);
        assert_eq!(rgb565(0xF800), [255, 0, 0, 255]);
        assert_eq!(rgb5a3(0xFC00), [255, 0, 0, 255]);
        assert_eq!(rgb5a3(0x7F00), [255, 0, 0, 255]);
        assert_eq!(rgb5a3(0x0F00)[3], 0);
        assert_eq!(ia8(0x80FF), [255, 255, 255, 0x80]);
    }

    #[test]
    fn cmpr_two_colour() {
        // One 8x8 block = four 4x4 sub-blocks. Sub-block 0: c0 white > c1 black,
        // indices all 0 -> white; index 3 -> 1/3 white.
        let mut data = vec![0u8; 32];
        for s in 0..4 {
            data[s * 8..s * 8 + 4].copy_from_slice(&[0xFF, 0xFF, 0x00, 0x00]);
        }
        data[4] = 0xC0; // texel 0 uses index 3
        let px = decode(&data, CMPR, 8, 8, None).unwrap();
        assert_eq!(&px[0..4], &[85, 85, 85, 255]);
        assert_eq!(&px[4..8], &[255, 255, 255, 255]);
        // Sub-block 1 starts at x = 4.
        assert_eq!(&px[16..20], &[255, 255, 255, 255]);
    }

    #[test]
    fn paletted() {
        let pal = decode_palette(&[0xFF, 0xFF, 0xF8, 0x00], TL_RGB565, 2).unwrap();
        let mut data = vec![0u8; 32];
        data[0] = 0x01;
        let px = decode(&data, C4, 8, 8, Some(&pal)).unwrap();
        assert_eq!(&px[0..4], &[255, 255, 255, 255]);
        assert_eq!(&px[4..8], &[255, 0, 0, 255]);
        assert!(decode(&data, C4, 8, 8, None).is_err());
    }

    #[test]
    fn truncated_is_error() {
        assert!(decode(&[0u8; 31], I8, 8, 4, None).is_err());
        assert!(decode(&[0u8; 32], 7, 8, 4, None).is_err());
    }

    #[test]
    fn wrapping() {
        assert_eq!(wrap(-1, 4, REPEAT), 3);
        assert_eq!(wrap(4, 4, MIRROR), 3);
        assert_eq!(wrap(-1, 4, MIRROR), 0);
        assert_eq!(wrap(9, 4, CLAMP), 3);
    }
}
