//! Nintendo Yay0 decompression.
//!
//! Layout: `Yay0`, u32 decompressed size, u32 link-table offset, u32 chunk
//! offset (offsets from the start of the stream), then the mask words. Each
//! mask bit (MSB first) is 1 for a literal byte from the chunk stream, or 0
//! for a back-reference from the link table: u16 `nddd` with distance
//! `ddd + 1` and length `n + 2`, or, when `n == 0`, a length of
//! `chunk byte + 0x12`.

use super::read::{be16, be32, be8};
use super::{Error, Result};
use alloc::vec::Vec;

/// Largest output we accept (the biggest IPL resource is under 100 KiB).
pub const MAX_SIZE: usize = 4 * 1024 * 1024;

/// Does `src` start with a Yay0 header?
pub fn is_yay0(src: &[u8]) -> bool {
    src.len() >= 16 && &src[..4] == b"Yay0"
}

/// Decompressed size claimed by the header.
pub fn decompressed_size(src: &[u8]) -> Result<usize> {
    if !is_yay0(src) {
        return Err(Error::BadMagic(0));
    }
    Ok(be32(src, 4)? as usize)
}

/// Decompress the Yay0 stream at the start of `src`.
pub fn decompress(src: &[u8]) -> Result<Vec<u8>> {
    let size = decompressed_size(src)?;
    if size > MAX_SIZE {
        return Err(Error::BadValue(4));
    }
    let mut lp = be32(src, 8)? as usize;
    let mut cp = be32(src, 12)? as usize;
    let mut mp = 16usize;
    let mut out: Vec<u8> = Vec::with_capacity(size);
    let mut mask = 0u32;
    let mut bits = 0u32;
    while out.len() < size {
        if bits == 0 {
            mask = be32(src, mp)?;
            mp += 4;
            bits = 32;
        }
        if mask & 0x8000_0000 != 0 {
            out.push(be8(src, cp)?);
            cp += 1;
        } else {
            let v = be16(src, lp)? as usize;
            lp += 2;
            let dist = (v & 0xFFF) + 1;
            let mut n = v >> 12;
            if n == 0 {
                n = be8(src, cp)? as usize + 0x12;
                cp += 1;
            } else {
                n += 2;
            }
            let start = out.len().checked_sub(dist).ok_or(Error::BadValue(lp - 2))?;
            let n = n.min(size - out.len());
            if dist >= n {
                out.extend_from_within(start..start + n);
            } else {
                for i in 0..n {
                    let b = out[start + i];
                    out.push(b);
                }
            }
        }
        mask <<= 1;
        bits -= 1;
    }
    Ok(out)
}

/// Compress `data` into a (valid but not size-optimal) Yay0 stream; used by
/// tests to exercise the decoder.
#[cfg(test)]
pub fn compress_simple(data: &[u8]) -> Vec<u8> {
    // Greedy: back-reference runs of identical bytes, literals otherwise.
    let mut masks: Vec<u32> = Vec::new();
    let mut links: Vec<u8> = Vec::new();
    let mut chunks: Vec<u8> = Vec::new();
    let (mut mask, mut bits) = (0u32, 0u32);
    let mut push_bit = |b: bool, masks: &mut Vec<u32>| {
        mask = (mask << 1) | b as u32;
        bits += 1;
        if bits == 32 {
            masks.push(mask);
            mask = 0;
            bits = 0;
        }
    };
    let mut i = 0;
    while i < data.len() {
        let mut run = 0;
        if i > 0 {
            while i + run < data.len() && data[i + run] == data[i - 1] && run < 0x111 {
                run += 1;
            }
        }
        if run >= 3 {
            push_bit(false, &mut masks);
            if run >= 0x12 {
                links.extend_from_slice(&0u16.to_be_bytes());
                chunks.push((run - 0x12) as u8);
            } else {
                links.extend_from_slice(&(((run - 2) as u16) << 12).to_be_bytes());
            }
            i += run;
        } else {
            push_bit(true, &mut masks);
            chunks.push(data[i]);
            i += 1;
        }
    }
    if bits != 0 {
        masks.push(mask << (32 - bits));
    }
    let link_off = 16 + masks.len() * 4;
    let chunk_off = link_off + links.len();
    let mut out = Vec::new();
    out.extend_from_slice(b"Yay0");
    out.extend_from_slice(&(data.len() as u32).to_be_bytes());
    out.extend_from_slice(&(link_off as u32).to_be_bytes());
    out.extend_from_slice(&(chunk_off as u32).to_be_bytes());
    for m in masks {
        out.extend_from_slice(&m.to_be_bytes());
    }
    out.extend_from_slice(&links);
    out.extend_from_slice(&chunks);
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round_trip() {
        let mut data = Vec::new();
        for i in 0..2000u32 {
            let b = if (i / 50) % 2 == 0 { (i % 7) as u8 } else { 0xAA };
            data.push(b);
        }
        let z = compress_simple(&data);
        assert!(z.len() < data.len());
        assert_eq!(decompress(&z).unwrap(), data);
    }

    #[test]
    fn hand_made_stream() {
        // "ab" literal, then a back-reference of distance 2, length 4 -> "ababab".
        let mut z = Vec::new();
        z.extend_from_slice(b"Yay0");
        z.extend_from_slice(&6u32.to_be_bytes());
        z.extend_from_slice(&20u32.to_be_bytes());
        z.extend_from_slice(&22u32.to_be_bytes());
        z.extend_from_slice(&0xC000_0000u32.to_be_bytes()); // 1,1,0
        z.extend_from_slice(&0x2001u16.to_be_bytes()); // n=2 -> len 4, dist 2
        z.extend_from_slice(b"ab");
        assert_eq!(decompress(&z).unwrap(), b"ababab");
    }

    #[test]
    fn bad_input_is_an_error() {
        assert!(decompress(b"Yay0").is_err());
        let mut z = Vec::new();
        z.extend_from_slice(b"Yay0");
        z.extend_from_slice(&100u32.to_be_bytes());
        z.extend_from_slice(&20u32.to_be_bytes());
        z.extend_from_slice(&22u32.to_be_bytes());
        z.extend_from_slice(&0u32.to_be_bytes()); // back-reference before any output
        z.extend_from_slice(&0x1000u16.to_be_bytes());
        assert!(decompress(&z).is_err());
    }
}
