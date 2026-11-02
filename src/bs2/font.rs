//! The IPL font on GX (`font.c`, 0x81307FB8..0x81309620): text boxes laid
//! out line by line and drawn one textured quad per character. Positions
//! and box sizes are 12.4 fixed point, character and line spacing and the
//! glyph size whole pixels, as in BS2.
//!
//! The ROM font itself is read and unpacked by [`crate::ipl::glh::Font`]
//! (`Font_Load`), and its sheets kept as textures in [`UiFont`]. Japanese
//! two-byte codes (`Lang_GetCode() == 2`) are not handled: the PAL IPL has
//! no Japanese.


// All of font.c is here; the menu calls the rest as it is ported.
#![allow(dead_code)]

use super::bs2main;
use super::ui::UiFont;
use crate::sdk::gx::*;
use core::sync::atomic::{AtomicU32, Ordering};

/// Lines a box can hold (`FONT_MAX_LINES`).
const MAX_LINES: usize = 64;

/// `sColorTop`, `sColorBottom`: the text gradient, 0xRRGGBBAA.
static COLOR_TOP: AtomicU32 = AtomicU32::new(0xFFFF_FFFF);
static COLOR_BOTTOM: AtomicU32 = AtomicU32::new(0xFFFF_FFFF);
/// `sShadowTop`, `sShadowBottom`: the glow colors.
static SHADOW_TOP: AtomicU32 = AtomicU32::new(0);
static SHADOW_BOTTOM: AtomicU32 = AtomicU32::new(0);

/// A text box (`FontBox`, the head of a GLH0 text pane).
#[derive(Clone, Copy, Debug)]
pub struct FontBox {
    /// Anchor and size, 12.4.
    pub x: i32,
    pub y: i32,
    pub w: i32,
    pub h: i32,
    /// 0 centre, 1 right, 2 left.
    pub halign: u8,
    /// 0 centre, 1 bottom, 2 top.
    pub valign: u8,
    pub char_space: i16,
    pub line_space: i16,
    /// Glyph size in pixels.
    pub size: u16,
}

/// `Font_Index` (0x81309600): character code to sheet index.
pub fn index(code: u32) -> usize {
    if code > 0x20 && code <= 0xFF {
        (code - 0x20) as usize
    } else {
        0
    }
}

fn char_w(f: &UiFont, b: &FontBox, code: u32) -> i32 {
    let w = f.font.widths.get(index(code)).copied().unwrap_or(0) as i32;
    b.size as i32 * (w * 16) / f.font.cell_w as i32
}

fn line_x(b: &FontBox, line_w: &[u16; MAX_LINES], line: usize) -> i32 {
    let lw = line_w[line.min(MAX_LINES - 1)] as i32;
    let x0 = match b.halign & 0x7F {
        2 => b.x - b.w / 2,
        1 => b.x + b.w / 2 - lw,
        _ => b.x - lw / 2,
    };
    x0 as u16 as i32
}

/// `Font_DrawLines` (0x81308538): draw the lines of a box measured by
/// [`measure`].
#[allow(clippy::too_many_arguments)]
fn draw_lines<'a>(gx: &mut Gx<'a>, f: &'a UiFont, b: &FontBox, s: &[u8], line_w: &[u16; MAX_LINES], height: i32, max_chars: i32, max_lines: i32, wrap: bool, stop_at_wrap: bool) {
    let (mut lines, mut chars, mut x) = (0i32, 0i32, 0i32);
    let w = b.w & 0xFFFF;
    let mut y = match b.valign {
        2 => b.y - b.h / 2,
        1 => b.y + b.h / 2 - height,
        _ => b.y - height / 2,
    };
    let mut x0 = line_x(b, line_w, 0);
    y += f.font.ascent as i32 * b.size as i32 * 16 / f.font.cell_h as i32;
    let mut i = 0;
    while let Some(&c) = s.get(i).filter(|&&c| c != 0) {
        i += 1;
        let code = c as u32;
        if code == b'\n' as u32 {
            if wrap {
                lines += 1;
                x0 = line_x(b, line_w, lines as usize);
                y += b.line_space as i32 * 16;
                x = 0;
                if max_lines != 0 && lines >= max_lines {
                    break;
                }
            }
        } else {
            let space = b.char_space as i32;
            let nx = (space * 16 + char_w(f, b, code) + x) as u16 as i32;
            let over = if space > 0 { nx - space * 16 > w } else { nx > w };
            if over {
                if wrap {
                    lines += 1;
                    x0 = line_x(b, line_w, lines as usize);
                    x = 0;
                    y += b.line_space as i32 * 16;
                    if s.get(i) == Some(&b'\n') {
                        i += 1;
                    }
                }
                if stop_at_wrap || (max_lines != 0 && lines >= max_lines) {
                    break;
                }
            }
            draw_char(gx, f, (x0 + x) as u16 as i32, y, b.size as i32, index(code));
            x = (x + space * 16 + char_w(f, b, code)) as u16 as i32;
            chars += 1;
        }
        if (max_chars != 0 && chars >= max_chars) || (max_lines != 0 && lines >= max_lines) {
            break;
        }
    }
}

/// `Font_EndLine` (0x81308978): close a line. (The SJIS punctuation that
/// hangs out past the line end never occurs in the PAL IPL.)
fn end_line(f: &UiFont, b: &FontBox, last: u32, cur_w: &mut i32, line_w: &mut [u16; MAX_LINES], line: &mut usize, height: &mut i32, next: bool) {
    let mut w = *cur_w;
    if w != 0 {
        w -= b.char_space as i32 * 16;
        if last.wrapping_sub(0x8141) <= 3 {
            let hang = bs2main::disp_settings().punct_hang as i32 * f.font.widths.get(index(last)).copied().unwrap_or(0) as i32;
            w -= b.size as i32 * hang / f.font.cell_w as i32;
        }
    }
    line_w[(*line).min(MAX_LINES - 1)] = w as u16;
    if next {
        *line += 1;
        if *line >= MAX_LINES {
            *height = 0;
        } else {
            *height += b.line_space as i32 * 16;
        }
    }
    *cur_w = 0;
}

/// `Font_Measure` (0x81308A78): line widths and the total height of a
/// box's text; returns (height, index of the last line).
#[allow(clippy::too_many_arguments)]
fn measure(f: &UiFont, b: &FontBox, s: &[u8], line_w: &mut [u16; MAX_LINES], wrap: bool, newline: bool, stop_at_wrap: bool, max_chars: i32, max_lines: i32) -> (i32, usize) {
    let size = b.size as i32;
    let ch = f.font.cell_h as i32;
    let mut height = f.font.ascent as i32 * size * 16 / ch + f.font.descent as i32 * size * 16 / ch;
    let mut line = 0usize;
    let mut cur_w = 0i32;
    let w = b.w & 0xFFFF;
    let mut last = 0u32;
    let mut chars = 0i32;
    let mut i = 0;
    while let Some(&c) = s.get(i).filter(|&&c| c != 0) {
        i += 1;
        let code = c as u32;
        if code == b'\n' as u32 {
            if newline {
                end_line(f, b, last, &mut cur_w, line_w, &mut line, &mut height, true);
                if height == 0 {
                    return (0, line);
                }
                if max_lines != 0 && line as i32 >= max_lines {
                    break;
                }
            }
        } else {
            last = code;
            let space = b.char_space as i32;
            let nw = (space * 16 + char_w(f, b, code) + cur_w) as u16 as i32;
            let over = if space > 0 { nw - space * 16 > w } else { nw > w };
            if over {
                if wrap {
                    if s.get(i) == Some(&b'\n') {
                        i += 1;
                    }
                    end_line(f, b, code, &mut cur_w, line_w, &mut line, &mut height, true);
                    if height == 0 {
                        return (0, line);
                    }
                }
                if stop_at_wrap || (max_lines != 0 && line as i32 >= max_lines) {
                    break;
                }
            }
            cur_w = (space * 16 + char_w(f, b, code) + cur_w) as u16 as i32;
            chars += 1;
        }
        last = code;
        if max_chars != 0 && chars >= max_chars {
            break;
        }
        if max_lines != 0 && line as i32 >= max_lines {
            line -= 1;
            height -= b.line_space as i32 * 16;
            break;
        }
    }
    if max_lines != 0 && line as i32 >= max_lines {
        line -= 1;
        height -= b.line_space as i32 * 16;
    } else {
        end_line(f, b, last, &mut cur_w, line_w, &mut line, &mut height, false);
    }
    (height, line)
}

fn draw_box_lines<'a>(gx: &mut Gx<'a>, f: &'a UiFont, b: &FontBox, s: &[u8], max_chars: i32, max_lines: i32) {
    let mut line_w = [0u16; MAX_LINES];
    let (height, _) = measure(f, b, s, &mut line_w, true, true, false, max_chars, max_lines);
    let height = height as u16 as i32;
    if height != 0 {
        draw_lines(gx, f, b, s, &line_w, height, max_chars, max_lines, true, false);
    }
}

/// `fn_81308DB8`: draw a box's text, wrapping at its width.
pub fn draw_box<'a>(gx: &mut Gx<'a>, f: &'a UiFont, b: &FontBox, s: &[u8]) {
    draw_box_lines(gx, f, b, s, 0, 0);
}

/// `Font_DrawBoxN` (0x81308E50): the first `max_chars` characters.
pub fn draw_box_n<'a>(gx: &mut Gx<'a>, f: &'a UiFont, b: &FontBox, s: &[u8], max_chars: i32) {
    draw_box_lines(gx, f, b, s, max_chars, 0);
}

/// `Font_DrawBoxLine1` (0x81308EF4): at most one line.
pub fn draw_box_line1<'a>(gx: &mut Gx<'a>, f: &'a UiFont, b: &FontBox, s: &[u8], max_chars: i32) {
    draw_box_lines(gx, f, b, s, max_chars, 1);
}

/// `Font_DrawBoxLine2` (0x81308F98): at most two lines.
pub fn draw_box_line2<'a>(gx: &mut Gx<'a>, f: &'a UiFont, b: &FontBox, s: &[u8], max_chars: i32) {
    draw_box_lines(gx, f, b, s, max_chars, 2);
}

/// `fn_813090A4`: the text over a glow of 5x5 faint copies in the shadow
/// colors, their alpha pulsing with `pulse` (0..`pulse_len`, stepped by
/// `Font_StepPulse` each frame).
pub fn draw_box_glow<'a>(gx: &mut Gx<'a>, f: &'a UiFont, b: &FontBox, s: &[u8], pulse: i16) {
    let disp = bs2main::disp_settings();
    let (top, bottom) = (COLOR_TOP.load(Ordering::Relaxed), COLOR_BOTTOM.load(Ordering::Relaxed));
    let alpha = (disp.glow_min as i32 + pulse as i32 * (disp.glow_max - disp.glow_min) as i32 / disp.pulse_len as i32) as u8;
    let glow = |c: u32| (c & 0xFFFF_FF00) | (((c & 0xFF) * alpha as u32 / 255) as u8 / 6) as u32;
    COLOR_TOP.store(glow(SHADOW_TOP.load(Ordering::Relaxed)), Ordering::Relaxed);
    COLOR_BOTTOM.store(glow(SHADOW_BOTTOM.load(Ordering::Relaxed)), Ordering::Relaxed);
    let mut tmp = *b;
    for i in 0..5 {
        for j in 0..5 {
            tmp.x = b.x - 0x40 + i * 0x20;
            tmp.y = b.y - 0x40 + j * 0x20;
            draw_box(gx, f, &tmp, s);
        }
    }
    COLOR_TOP.store(top, Ordering::Relaxed);
    COLOR_BOTTOM.store(bottom, Ordering::Relaxed);
    draw_box(gx, f, b, s);
}

/// `Glh_MeasureString` (0x813092D8): the widest line and the height of a
/// box's text, 12.4, without wrapping.
pub fn measure_string(f: &UiFont, b: &FontBox, s: &[u8]) -> (u16, u16) {
    let mut line_w = [0u16; MAX_LINES];
    let (h, lines) = measure(f, b, s, &mut line_w, false, true, false, 0, 0);
    let w = line_w[..=lines.min(MAX_LINES - 1)].iter().copied().max().unwrap_or(0);
    (w, h as u16)
}

/// `Font_DrawChar` (0x81309380): glyph `index` with its baseline at `y`
/// and its left edge at `x` (12.4), `size` pixels square, in the text
/// gradient.
pub fn draw_char<'a>(gx: &mut Gx<'a>, f: &'a UiFont, x: i32, y: i32, size: i32, index: usize) {
    let ft = &f.font;
    let per = (ft.cols * ft.rows) as usize;
    let (sheet, n) = (index / per, index % per);
    let tx = (n % ft.cols as usize) as i32 * ft.cell_w as i32;
    let ty = (n / ft.cols as usize) as i32 * ft.cell_h as i32;
    let (cw, ch, sw, sh) = (ft.cell_w as i32, ft.cell_h as i32, ft.sheet_w as i32, ft.sheet_h as i32);
    let top = y - ft.ascent as i32 * size * 16 / ch;
    let bottom = y + ft.descent as i32 * size * 16 / ch;
    let (s0, t0) = ((tx << 14) / sw, (ty << 14) / sh);
    let (s1, t1) = (((tx + cw) << 14) / sw, ((ty + ch) << 14) / sh);
    let (left, right) = (x, x + size * 16);
    let Some(tex) = f.sheets.get(sheet) else { return };
    let (ct, cb) = (COLOR_TOP.load(Ordering::Relaxed), COLOR_BOTTOM.load(Ordering::Relaxed));
    gx.load_tex_obj(tex, 0);
    gx.begin(GX_QUADS, 1, 4);
    for (px, py, c, s, t) in [(left, top, ct, s0, t0), (right, top, ct, s1, t0), (right, bottom, cb, s1, t1), (left, bottom, cb, s0, t1)] {
        gx.position3s16(px as i16, py as i16, 0);
        gx.color1u32(c);
        gx.tex_coord2u16(s as u16, t as u16);
    }
    gx.end();
}

/// `Font_SetColorU32` (0x81309548) / `fn_81309554`: the text gradient.
pub fn set_color(top: [u8; 4], bottom: [u8; 4]) {
    COLOR_TOP.store(u32::from_be_bytes(top), Ordering::Relaxed);
    COLOR_BOTTOM.store(u32::from_be_bytes(bottom), Ordering::Relaxed);
}

/// `fn_813095A0`: the glow colors.
pub fn set_shadow(top: [u8; 4], bottom: [u8; 4]) {
    SHADOW_TOP.store(u32::from_be_bytes(top), Ordering::Relaxed);
    SHADOW_BOTTOM.store(u32::from_be_bytes(bottom), Ordering::Relaxed);
}

/// `Font_Setup` (0x813095EC): white text.
pub fn setup() {
    set_color([0xFF; 4], [0xFF; 4]);
}
