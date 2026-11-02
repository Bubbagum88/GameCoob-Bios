//! The lettering of the cube's side labels and face titles, for words
//! the IPL never drew. Not in BS2.
//!
//! BS2's labels ("Game Play", "Memory Card", "Options", "Calendar") are
//! pictures, one per word, in the menu's texture bank. GameCoob's new
//! sides need new words in the same lettering, so at boot this cuts the
//! letters out of those pictures (from the user's own dump; nothing
//! Nintendo-made ships here) and sets words from them. Letters no IPL word
//! has (B, F, k, v, w and the like) come from M PLUS 1p ExtraBold, a free
//! font of similar weight, drawn at the IPL letters' size and baseline by
//! `tools/label-font/make_glyphs.py` into `label_glyphs.bin` (SIL Open
//! Font License, `label_glyphs.OFL.txt`).

use crate::bs2::shader::FastTex;
use crate::ipl::gx::Texture;
use alloc::vec;
use alloc::vec::Vec;

/// Height of the IPL's title pictures (200 x 32), in texels.
pub const H: usize = 32;
const W: usize = 200;
/// The gap a space leaves, in texels (the gap in "Game Play").
const SPACE: usize = 9;

/// Each English face title (option, clock, card, game face) and the
/// columns `[x0, x1)` of its letters, spaces skipped; each end is moved
/// to the faintest column within two texels at load.
pub const SOURCES: [(&str, &[(u8, u8)]); 4] = [
    ("Options", &[(42, 66), (67, 85), (85, 95), (97, 104), (106, 123), (125, 141), (142, 158)]),
    ("Calendar", &[(35, 56), (56, 74), (75, 81), (82, 99), (101, 117), (119, 137), (138, 155), (155, 167)]),
    (
        "Memory Card",
        &[(2, 27), (28, 46), (46, 72), (72, 91), (92, 104), (104, 120), (129, 150), (150, 168), (168, 180), (180, 198)],
    ),
    ("Game Play", &[(25, 44), (45, 63), (63, 88), (89, 107), (117, 136), (136, 143), (144, 162), (162, 179)]),
];

static FALLBACK: &[u8] = include_bytes!("label_glyphs.bin");

/// One title picture's coverage and its letters' columns.
struct Source {
    word: &'static str,
    alpha: Vec<u8>,
    cols: Vec<(usize, usize)>,
}

pub struct LabelFont {
    sources: Vec<Source>,
}

impl LabelFont {
    /// `titles` are the bank textures of `SOURCES`' words, in order (None
    /// where the dump has none). A picture that is not 200 x 32 is left
    /// out; its letters then come from the fallback font.
    pub fn new(titles: [Option<&Texture>; 4]) -> LabelFont {
        let mut sources = Vec::new();
        for ((word, cols), t) in SOURCES.iter().zip(titles) {
            let Some(t) = t else { continue };
            if t.width as usize != W || t.height as usize != H {
                continue;
            }
            let alpha: Vec<u8> = t.rgba.chunks_exact(4).map(|p| p[3]).collect();
            let cov = |c: isize| -> u32 {
                if (0..W as isize).contains(&c) {
                    (0..H).map(|y| alpha[y * W + c as usize] as u32).sum()
                } else {
                    0
                }
            };
            let refine = |x: u8| -> usize {
                let x = x as isize;
                (x - 2..=x + 2).min_by_key(|&c| (cov(c), (c - x).abs())).unwrap().clamp(0, W as isize) as usize
            };
            let cols = cols.iter().map(|&(a, b)| (refine(a), refine(b))).collect();
            sources.push(Source { word, alpha, cols });
        }
        LabelFont { sources }
    }

    /// `s` set in the label lettering: white, with coverage as alpha,
    /// `H` texels high. None if a letter is in neither the IPL's words nor
    /// the fallback font.
    pub fn render(&self, s: &str) -> Option<FastTex> {
        let (w, a) = self.set(s)?;
        Some(FastTex::from_alpha(w, H, &a))
    }

    fn set(&self, s: &str) -> Option<(usize, Vec<u8>)> {
        // (width, H rows of coverage) pieces, left to right
        let mut parts: Vec<(usize, Vec<u8>)> = Vec::new();
        if let Some(p) = self.whole(s) {
            parts.push(p);
        } else {
            for ch in s.chars() {
                parts.push(if ch == ' ' { (SPACE, vec![0; SPACE * H]) } else { self.glyph(ch)? });
            }
        }
        let w: usize = parts.iter().map(|p| p.0).sum();
        let mut out = vec![0u8; w * H];
        let mut x0 = 0;
        for (pw, pa) in parts {
            for y in 0..H {
                out[y * w + x0..y * w + x0 + pw].copy_from_slice(&pa[y * pw..(y + 1) * pw]);
            }
            x0 += pw;
        }
        Some((w, out))
    }

    /// `s` cut whole from an IPL word it is part of ("Memory" from
    /// "Memory Card"), keeping the original spacing.
    fn whole(&self, s: &str) -> Option<(usize, Vec<u8>)> {
        for src in &self.sources {
            let Some(at) = src.word.find(s) else { continue };
            // letter indices (spaces skipped) of the first and last letter
            let first = src.word[..at].chars().filter(|&c| c != ' ').count();
            let n = s.chars().filter(|&c| c != ' ').count();
            if n == 0 {
                continue;
            }
            let (x0, x1) = (src.cols[first].0, src.cols[first + n - 1].1);
            return Some(src.crop(x0, x1));
        }
        None
    }

    /// One letter: from an IPL word if one has it, else the fallback font.
    fn glyph(&self, ch: char) -> Option<(usize, Vec<u8>)> {
        for src in &self.sources {
            if let Some(i) = src.word.chars().filter(|&c| c != ' ').position(|c| c == ch) {
                let (x0, x1) = src.cols[i];
                return Some(src.crop(x0, x1));
            }
        }
        fallback(ch)
    }
}

impl Source {
    fn crop(&self, x0: usize, x1: usize) -> (usize, Vec<u8>) {
        let w = x1.saturating_sub(x0);
        let mut a = Vec::with_capacity(w * H);
        for y in 0..H {
            a.extend_from_slice(&self.alpha[y * W + x0..y * W + x0 + w]);
        }
        (w, a)
    }
}

/// A letter of the fallback font (`label_glyphs.bin`: "GCLF", height,
/// count, then per letter its char, width and coverage rows).
fn fallback(ch: char) -> Option<(usize, Vec<u8>)> {
    let d = FALLBACK;
    if d.len() < 6 || &d[..4] != b"GCLF" || d[4] as usize != H {
        return None;
    }
    let mut i = 6;
    for _ in 0..d[5] {
        let (c, w) = (*d.get(i)? as char, *d.get(i + 1)? as usize);
        let a = d.get(i + 2..i + 2 + w * H)?;
        if c == ch {
            return Some((w, a.to_vec()));
        }
        i += 2 + w * H;
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn fallback_has_letters() {
        for ch in "BFkvwAD".chars() {
            let (w, a) = fallback(ch).expect("letter");
            assert!(w > 0 && a.iter().any(|&v| v > 100));
        }
        assert!(fallback('%').is_none());
    }

    #[test]
    fn words_without_dump() {
        let f = LabelFont::new([None, None, None, None]);
        let (w, a) = f.set("Back Files").expect("set");
        assert_eq!(a.len(), w * H);
        assert!(f.set("50%").is_none());
    }
}
