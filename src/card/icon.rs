//! A save's banner (96 x 32) and icon frames (32 x 32), decoded to RGBA8.
//!
//! They sit in the save's data at the entry's image offset: the banner
//! first (CI8 pixels then its 256-colour palette, or RGB5A3 pixels), then
//! one picture per icon frame. Frames run until the first one whose speed
//! is 0. CI8 frames with their own palette carry it right after their
//! pixels; frames on the shared palette use the one after the last frame.
//! A counted frame with no format shows the next frame that has one (or
//! nothing), which is how the console draws such icons.

use super::{BANNER_CI8, BANNER_RGB5A3, ICON_CI8_SHARED, ICON_CI8_UNIQUE, ICON_RGB5A3, SaveFile};
use crate::ipl::gx;
use alloc::vec;
use alloc::vec::Vec;

pub const BANNER_W: usize = 96;
pub const BANNER_H: usize = 32;
pub const ICON_W: usize = 32;
pub const ICON_H: usize = 32;
const PALETTE: usize = 0x200;

/// One icon frame: RGBA8 pixels and how many 1/60 s ticks it shows (4,
/// 8 or 12).
pub struct Frame {
    pub rgba: Vec<u8>,
    pub ticks: u8,
}

fn ci8(px: &[u8], pal: &[u8], w: usize, h: usize) -> Option<Vec<u8>> {
    let pal = gx::decode_palette(pal, gx::TL_RGB5A3, 256).ok()?;
    gx::decode(px, gx::C8, w, h, Some(&pal)).ok()
}

fn rgb5a3(px: &[u8], w: usize, h: usize) -> Option<Vec<u8>> {
    gx::decode(px, gx::RGB5A3, w, h, None).ok()
}

impl SaveFile {
    fn bytes_at(&self, off: usize, len: usize) -> Option<&[u8]> {
        self.data.get(off..off.checked_add(len)?)
    }

    /// Banner bytes (pixels and palette) by its format.
    fn banner_len(&self) -> usize {
        match self.dentry.banner_format() {
            BANNER_CI8 => BANNER_W * BANNER_H + PALETTE,
            BANNER_RGB5A3 => BANNER_W * BANNER_H * 2,
            _ => 0,
        }
    }

    /// The banner as RGBA8, if the save has one.
    pub fn banner(&self) -> Option<Vec<u8>> {
        let at = self.dentry.image_offset() as usize;
        let n = BANNER_W * BANNER_H;
        match self.dentry.banner_format() {
            BANNER_CI8 => {
                let b = self.bytes_at(at, n + PALETTE)?;
                ci8(&b[..n], &b[n..], BANNER_W, BANNER_H)
            }
            BANNER_RGB5A3 => rgb5a3(self.bytes_at(at, n * 2)?, BANNER_W, BANNER_H),
            _ => None,
        }
    }

    /// The icon's frames. Empty when the save has no icon or its data runs
    /// past the save.
    pub fn icon_frames(&self) -> Vec<Frame> {
        self.try_icon_frames().unwrap_or_default()
    }

    fn try_icon_frames(&self) -> Option<Vec<Frame>> {
        let fmts = self.dentry.icon_formats();
        let speeds = self.dentry.anim_speeds();
        let fmt = |i: usize| ((fmts >> (2 * i)) & 3) as u8;
        let speed = |i: usize| ((speeds >> (2 * i)) & 3) as u8;
        if fmt(0) == 0 {
            return None;
        }
        let n = (0..8).take_while(|&i| speed(i) != 0).count();
        let base = self.dentry.image_offset() as usize + self.banner_len();
        let px = ICON_W * ICON_H;
        let mut offs = [0usize; 8];
        let mut len = 0;
        let mut shared = false;
        for (i, o) in offs.iter_mut().enumerate().take(n) {
            *o = len;
            len += match fmt(i) {
                ICON_CI8_SHARED => {
                    shared = true;
                    px
                }
                ICON_RGB5A3 => px * 2,
                ICON_CI8_UNIQUE => px + PALETTE,
                _ => 0,
            };
        }
        let shared_pal = self.bytes_at(base + len, PALETTE).filter(|_| shared);
        let mut out = Vec::with_capacity(n);
        for i in 0..n {
            let mut rgba = None;
            for j in i..n {
                let at = base + offs[j];
                rgba = match fmt(j) {
                    ICON_CI8_SHARED => Some(ci8(self.bytes_at(at, px)?, shared_pal?, ICON_W, ICON_H)?),
                    ICON_RGB5A3 => Some(rgb5a3(self.bytes_at(at, px * 2)?, ICON_W, ICON_H)?),
                    ICON_CI8_UNIQUE => {
                        let b = self.bytes_at(at, px + PALETTE)?;
                        Some(ci8(&b[..px], &b[px..], ICON_W, ICON_H)?)
                    }
                    _ => continue,
                };
                break;
            }
            out.push(Frame { rgba: rgba.unwrap_or_else(|| vec![0; px * 4]), ticks: speed(i) * 4 });
        }
        Some(out)
    }
}
