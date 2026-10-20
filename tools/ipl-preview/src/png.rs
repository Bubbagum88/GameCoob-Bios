//! Minimal PNG writer (stored zlib blocks, no compression).

use std::io::Write;
use std::path::Path;

fn crc32(data: &[u8]) -> u32 {
    let mut c = 0xFFFF_FFFFu32;
    for &b in data {
        c ^= b as u32;
        for _ in 0..8 {
            c = if c & 1 != 0 { 0xEDB8_8320 ^ (c >> 1) } else { c >> 1 };
        }
    }
    !c
}

fn chunk(out: &mut Vec<u8>, ty: &[u8; 4], data: &[u8]) {
    out.extend_from_slice(&(data.len() as u32).to_be_bytes());
    let mut body = ty.to_vec();
    body.extend_from_slice(data);
    out.extend_from_slice(&body);
    out.extend_from_slice(&crc32(&body).to_be_bytes());
}

pub fn write_rgba(path: &Path, w: usize, h: usize, rgba: &[u8]) -> std::io::Result<()> {
    let mut raw = Vec::with_capacity((w * 4 + 1) * h);
    for y in 0..h {
        raw.push(0);
        raw.extend_from_slice(&rgba[y * w * 4..(y + 1) * w * 4]);
    }
    let mut z = vec![0x78, 0x01];
    let blocks: Vec<&[u8]> = raw.chunks(65535).collect();
    for (i, blk) in blocks.iter().enumerate() {
        z.push((i + 1 == blocks.len()) as u8);
        z.extend_from_slice(&(blk.len() as u16).to_le_bytes());
        z.extend_from_slice(&(!(blk.len() as u16)).to_le_bytes());
        z.extend_from_slice(blk);
    }
    let (mut a, mut b) = (1u32, 0u32);
    for &x in &raw {
        a = (a + x as u32) % 65521;
        b = (b + a) % 65521;
    }
    z.extend_from_slice(&((b << 16) | a).to_be_bytes());
    let mut png = b"\x89PNG\r\n\x1a\n".to_vec();
    let mut ihdr = Vec::new();
    ihdr.extend_from_slice(&(w as u32).to_be_bytes());
    ihdr.extend_from_slice(&(h as u32).to_be_bytes());
    ihdr.extend_from_slice(&[8, 6, 0, 0, 0]);
    chunk(&mut png, b"IHDR", &ihdr);
    chunk(&mut png, b"IDAT", &z);
    chunk(&mut png, b"IEND", &[]);
    std::fs::File::create(path)?.write_all(&png)
}
