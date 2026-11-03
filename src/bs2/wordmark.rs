//! GameCoob's intro logo option: the intro's "GAMECUBE" wordmark respelled
//! "GAMECOOB" at load time, from the user's own dump.
//!
//! The wordmark is one 352 x 40 texture on the text model (res 0x6B). Its
//! letters are found by their empty columns: "GAM" touch and come as one
//! run, then E, C, U, B, E, and the trademark sign. "GAMEC" and B are
//! kept; each O is the C's left half and that half mirrored, so it has the
//! same rounded side on both ends. Nothing of the console's is shipped:
//! the picture is cut from the dump while the app loads.
//!
//! Which wordmark the intro draws is saved as the firmware variable
//! `GameCoobLogo` ([`crate::vars::coob_logo`]); the console's is the
//! default.

use crate::ipl::gx::Texture;
use alloc::vec::Vec;
use core::sync::atomic::{AtomicBool, Ordering};

static COOB: AtomicBool = AtomicBool::new(false);

/// Draw "GAMECOOB" (true) or the console's "GAMECUBE" (false).
pub fn set_coob(on: bool) {
    COOB.store(on, Ordering::Relaxed);
}

pub fn coob() -> bool {
    COOB.load(Ordering::Relaxed)
}

/// Column runs `[start, end)` that have any coverage.
fn runs(t: &Texture) -> Vec<(usize, usize)> {
    let (w, h) = (t.width as usize, t.height as usize);
    let lit = |x: usize| (0..h).any(|y| t.rgba[(y * w + x) * 4 + 3] != 0);
    let mut v = Vec::new();
    let mut start = None;
    for x in 0..w {
        match (lit(x), start) {
            (true, None) => start = Some(x),
            (false, Some(s)) => {
                v.push((s, x));
                start = None;
            }
            _ => {}
        }
    }
    if let Some(s) = start {
        v.push((s, w));
    }
    v
}

/// The wordmark texture respelled, or None if it isn't laid out as
/// expected (another IPL's wordmark, say).
pub fn respell(t: &Texture) -> Option<Texture> {
    let (w, h) = (t.width as usize, t.height as usize);
    if w == 0 || h == 0 || t.rgba.len() != w * h * 4 {
        return None;
    }
    let r = runs(t);
    // GAM, E, C, U, B, E, then the trademark sign (one run or more).
    if r.len() < 7 {
        return None;
    }
    let (c, u, b, e, tm) = (r[2], r[3], r[4], r[5], r[6]);
    let tail = (tm.0, r[r.len() - 1].1);
    let cw = c.1 - c.0;
    let o1 = u.0;
    let o2 = o1 + cw + (b.0 - u.1);
    let nb = o2 + cw + (e.0 - b.1);
    let ntm = nb + (b.1 - b.0) + (tm.0 - e.1);
    if ntm + (tail.1 - tail.0) > w {
        return None;
    }

    let mut out = t.clone();
    out.rgba.iter_mut().for_each(|p| *p = 0);
    let mut copy = |src: usize, dst: usize| {
        for y in 0..h {
            let (s, d) = ((y * w + src) * 4, (y * w + dst) * 4);
            out.rgba[d..d + 4].copy_from_slice(&t.rgba[s..s + 4]);
        }
    };
    // "GAMEC" where it was.
    for x in 0..c.1 {
        copy(x, x);
    }
    // The two Os.
    for o in [o1, o2] {
        for x in 0..cw {
            let half = if x < cw / 2 { x } else { cw - 1 - x };
            copy(c.0 + half, o + x);
        }
    }
    // B, then the trademark sign.
    for x in 0..b.1 - b.0 {
        copy(b.0 + x, nb + x);
    }
    for x in 0..tail.1 - tail.0 {
        copy(tail.0 + x, ntm + x);
    }
    Some(out)
}
