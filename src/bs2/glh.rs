//! GLH0 layout drawing on GX (`glh.c`, 0x81309620..0x8130A86C): picture
//! panes, 9-slice frames and STH0 strings in the text panes, from the
//! current TXH0 texture bank.
//!
//! The layouts come parsed ([`crate::ipl::glh`]) with positions in pixels;
//! they are turned back into BS2's 12.4 fixed point here. Textures come
//! decoded ([`FastTex`]) in place of the TXH0 headers.


// All of glh.c is here; the menu calls the rest as it is ported.
#![allow(dead_code)]

use super::bs2main;
use super::font::{self, FontBox};
use super::ginit;
use super::shader::FastTex;
use super::ui::UiFont;
use crate::ipl::glh::{Frame, Layout, Pic, Strings, Text};
use crate::sdk::gx::*;

/// Pixels to 12.4.
pub fn fx(v: f32) -> i32 {
    (v * 16.0) as i32
}

/// A text pane as the font draws it.
pub fn font_box(t: &Text) -> FontBox {
    FontBox {
        x: fx(t.x),
        y: fx(t.y),
        w: fx(t.w),
        h: fx(t.h),
        halign: t.h_align,
        valign: t.v_align,
        char_space: t.spacing as i16,
        line_space: t.line as i16,
        size: t.size as u16,
    }
}

/// `GX_Setup2DTex` (0x8130963C)
pub fn setup_2d_tex(gx: &mut Gx) {
    ginit::setup_2d(gx, true, false, false);
}

/// `GX_Setup2DText` (0x81309668)
pub fn setup_2d_text(gx: &mut Gx) {
    font::setup();
    ginit::setup_2d(gx, true, true, false);
}

/// Texture coordinates of a corner or edge: `flip` bit 0 swaps t, bit 1 s.
fn flip_st(flip: u8) -> (u16, u16, u16, u16) {
    let (t0, t1) = if flip & 1 != 0 { (0x4000, 0) } else { (0, 0x4000) };
    let (s0, s1) = if flip & 2 != 0 { (0x4000, 0) } else { (0, 0x4000) };
    (s0, s1, t0, t1)
}

fn quad(gx: &mut Gx, x: i32, y: i32, w: i32, h: i32, st: [(u16, u16); 4]) {
    gx.begin(GX_QUADS, 1, 4);
    for (k, (px, py)) in [(x, y), (x + w, y), (x + w, y + h), (x, y + h)].into_iter().enumerate() {
        gx.position3s16(px as i16, py as i16, 0);
        gx.tex_coord2s16(st[k].0 as i16, st[k].1 as i16);
    }
    gx.end();
}

/// `Glh_DrawCorner` (0x81309698): one frame corner.
fn draw_corner<'a>(gx: &mut Gx<'a>, x: i32, y: i32, w: i32, h: i32, tex: Option<&'a FastTex>, flip: u8) {
    let (s0, s1, t0, t1) = flip_st(flip);
    if let Some(t) = tex {
        gx.load_tex_obj(t, 0);
    }
    quad(gx, x, y, w, h, [(s0, t0), (s1, t0), (s1, t1), (s0, t1)]);
}

/// `Glh_DrawEdge` (0x813097A4): one frame edge, the corner texture's first
/// texel column (`horiz`) or row stretched.
fn draw_edge<'a>(gx: &mut Gx<'a>, x: i32, y: i32, w: i32, h: i32, tex: Option<&'a FastTex>, flip: u8, horiz: bool) {
    let (s0, s1, t0, t1) = flip_st(flip);
    if let Some(t) = tex {
        gx.load_tex_obj(t, 0);
    }
    if horiz {
        quad(gx, x, y, w, h, [(s0, t0), (s0, t0), (s0, t1), (s0, t1)]);
    } else {
        quad(gx, x, y, w, h, [(s0, t0), (s1, t0), (s1, t0), (s0, t0)]);
    }
}

/// `Glh_DrawSlices`: the 9-slice border around x, y, w, h (12.4).
fn draw_slices<'a>(gx: &mut Gx<'a>, f: &Frame, bank: &'a [FastTex], x: i32, y: i32, w: i32, h: i32) {
    let tex = |k: usize| bank.get(f.tex[k] as usize);
    let Some(t0) = tex(0) else { return };
    let (tw, th) = t0.size();
    let (cw, ch) = ((tw as i32 * 16) as i16 as i32, (th as i32 * 16) as i16 as i32);
    let (left, top, right, bottom) = (x - cw, y - ch, x + w, y + h);
    draw_corner(gx, left, top, cw, ch, tex(0), f.flips >> 6);
    draw_corner(gx, right, top, cw, ch, tex(1), f.flips >> 4);
    draw_corner(gx, left, bottom, cw, ch, tex(2), f.flips >> 2);
    draw_corner(gx, right, bottom, cw, ch, tex(3), f.flips);
    draw_edge(gx, x, top, w, ch, tex(1), f.flips >> 4, true);
    draw_edge(gx, x, bottom, w, ch, tex(3), f.flips, true);
    draw_edge(gx, left, y, cw, h, tex(2), f.flips >> 2, false);
    draw_edge(gx, right, y, cw, h, tex(3), f.flips, false);
}

/// `Glh_DrawPaneRect`: frame `f` around x, y, w, h (12.4): the inside in
/// the frame's corner colors with their alpha scaled by `c`'s, the border
/// in `c`.
#[allow(clippy::too_many_arguments)]
pub fn draw_pane_rect<'a>(gx: &mut Gx<'a>, f: &Frame, bank: &'a [FastTex], c: [u8; 4], x: i32, y: i32, w: i32, h: i32) {
    let corner = |k: usize| {
        let col = f.colors[k];
        let a = (c[3] as u32 * col[3] as u32 / 255).min(0xFF);
        u32::from_be_bytes([col[0], col[1], col[2], a as u8])
    };
    ginit::setup_2d(gx, false, true, false);
    gx.begin(GX_QUADS, 1, 4);
    for (px, py, k) in [(x, y, 0), (x + w, y, 1), (x + w, y + h, 3), (x, y + h, 2)] {
        gx.position3s16(px as i16, py as i16, 0);
        gx.color1u32(corner(k));
    }
    gx.end();
    ginit::setup_2d(gx, true, false, false);
    gx.set_chan_mat_color(GX_COLOR0A0, c);
    draw_slices(gx, f, bank, x, y, w, h);
}

/// `Glh_DrawTextBg`: frame `f` sized w x h, centred on its own position
/// moved by dx, dy.
#[allow(clippy::too_many_arguments)]
fn draw_text_bg<'a>(gx: &mut Gx<'a>, f: &Frame, bank: &'a [FastTex], c: [u8; 4], dx: i32, dy: i32, w: i32, h: i32) {
    draw_pane_rect(gx, f, bank, c, dx + (fx(f.x) - w / 2), dy + (fx(f.y) - h / 2), w, h);
}

/// `Glh_DrawFrame` (0x81309C3C): every frame with this tag at its own size.
pub fn draw_frame<'a>(gx: &mut Gx<'a>, tag: u32, l: &Layout, bank: &'a [FastTex], c: [u8; 4]) {
    for f in l.frames.iter().filter(|f| f.tag == tag) {
        draw_frame_at(gx, f, bank, c, 0, 0);
    }
}

/// One frame at its own size, moved by dx, dy (12.4).
pub fn draw_frame_at<'a>(gx: &mut Gx<'a>, f: &Frame, bank: &'a [FastTex], c: [u8; 4], dx: i32, dy: i32) {
    draw_text_bg(gx, f, bank, c, dx, dy, fx(f.inner_w), fx(f.inner_h));
}

/// `Glh_Fit` (0x81309CEC): place an image `size` long along one axis of a
/// pane spanning lo..hi; returns (outLo, outHi, texLo, texHi).
#[allow(clippy::too_many_arguments)]
fn fit(fit: u8, flags: u8, lo: i32, mid: i32, hi: i32, size: i32, mask_lo: u8, mask_hi: u8) -> (i32, i32, i32, i32) {
    let stretch = flags & 0x18 != 0;
    if fit & mask_lo != 0 && fit & mask_hi != 0 {
        (lo, hi, 0, 0x4000)
    } else if fit & mask_lo != 0 {
        if !stretch && hi - lo > size {
            (lo, size, 0, 0x4000)
        } else {
            (lo, hi, 0, ((hi - lo) << 14) / size)
        }
    } else if fit & mask_hi != 0 {
        if !stretch && hi - lo > size {
            (hi - size, hi, 0, 0x4000)
        } else {
            (lo, hi, ((lo + (size - hi)) << 14) / size, 0x4000)
        }
    } else if !stretch && hi - lo > size {
        let out_lo = mid - size / 2;
        (out_lo, out_lo + size, 0, 0x4000)
    } else {
        let d = ((hi - lo) << 13) / size;
        (lo, hi, 0x2000 - d, d + 0x2000)
    }
}

/// `Glh_DrawPicture` (0x81309E54): picture pane `p` with `tex` (or the
/// last loaded texture, sized 96 x 32, when `None`), moved by dx, dy.
pub fn draw_picture<'a>(gx: &mut Gx<'a>, p: &Pic, c: [u8; 4], tex: Option<&'a FastTex>, dx: i32, dy: i32) {
    gx.set_chan_mat_color(GX_COLOR0A0, c);
    let (px, py, pw, ph) = (fx(p.x), fx(p.y), fx(p.w), fx(p.h));
    let (x0, y0) = (dx + px - pw / 2, dy + py - ph / 2);
    let (x1, y1) = (x0 + pw, y0 + ph);
    let (tw, th) = match tex {
        Some(t) => {
            let (w, h) = t.size();
            if p.flags & 4 != 0 { (h as i32, w as i32) } else { (w as i32, h as i32) }
        }
        None => (0x60, 0x20),
    };
    // The texture's size in texels against the pane's 12.4: BS2 compares
    // them as they are.
    let (py0, py1, mut t0, mut t1) = fit(p.fit, p.flags, y0, dy + py, y1, th, 2, 1);
    let (px0, px1, mut s0, mut s1) = fit(p.fit, p.flags, x0, dx + px, x1, tw, 8, 4);
    if p.flags & 1 != 0 {
        core::mem::swap(&mut t0, &mut t1);
    }
    if p.flags & 2 != 0 {
        core::mem::swap(&mut s0, &mut s1);
    }
    if let Some(t) = tex {
        gx.load_tex_obj(t, 0);
    }
    gx.set_chan_mat_color(GX_COLOR0A0, c);
    let st = if p.flags & 4 != 0 { [(s0, t1), (s0, t0), (s1, t0), (s1, t1)] } else { [(s0, t0), (s1, t0), (s1, t1), (s0, t1)] };
    gx.begin(GX_QUADS, 1, 4);
    for (k, (x, y)) in [(px0, py0), (px1, py0), (px1, py1), (px0, py1)].into_iter().enumerate() {
        gx.position3s16(x as i16, y as i16, 0);
        gx.tex_coord2u16(st[k].0 as u16, st[k].1 as u16);
    }
    gx.end();
}

/// `Glh_DrawAll` (0x8130A158): every picture pane.
pub fn draw_all<'a>(gx: &mut Gx<'a>, l: &Layout, bank: &'a [FastTex], c: [u8; 4]) {
    for p in &l.pics {
        draw_picture(gx, p, c, bank.get(p.tex as usize), 0, 0);
    }
}

/// `Glh_DrawPane` (0x8130A1F8) / `Glh_DrawPaneOfs` (0x8130A284): every
/// picture pane with this tag, moved by dx, dy (12.4).
pub fn draw_pane<'a>(gx: &mut Gx<'a>, tag: u32, l: &Layout, bank: &'a [FastTex], c: [u8; 4], dx: i32, dy: i32) {
    for p in l.pics.iter().filter(|p| p.tag == tag) {
        draw_picture(gx, p, c, bank.get(p.tex as usize), dx, dy);
    }
}

/// `Glh_DrawPaneTex` (0x8130A318): every picture pane with this tag, with
/// another texture.
pub fn draw_pane_tex<'a>(gx: &mut Gx<'a>, tag: u32, l: &Layout, c: [u8; 4], tex: &'a FastTex) {
    for p in l.pics.iter().filter(|p| p.tag == tag) {
        draw_picture(gx, p, c, Some(tex), 0, 0);
    }
}

/// `Glh_DrawString` (0x8130A39C): text `s` in text pane `pane`, moved by
/// dx, dy (12.4), over the pane's background frame sized to it; `glow`
/// draws it glowing with the given pulse.
#[allow(clippy::too_many_arguments)]
pub fn draw_text_in<'a>(gx: &mut Gx<'a>, f: &'a UiFont, l: &Layout, bank: &'a [FastTex], pane: &Text, s: &[u8], c: [u8; 4], glow: Option<([u8; 4], i16)>, dx: i32, dy: i32) {
    let disp = bs2main::disp_settings();
    let mut b = font_box(pane);
    let (w, h) = font::measure_string(f, &b, s);
    let (w, h) = (w as i32, h as i32);
    if let Some(bg) = pane.bg.and_then(|i| l.frames.get(i as usize)) {
        let pad = disp.text_pad as i32 * 32;
        draw_text_bg(gx, bg, bank, c, dx, dy, w + pad, h + pad);
    }
    b.x += dx;
    b.y += dy;
    if b.halign & 0x7F == 0 {
        b.w = w + 0x100;
        b.h = h + 0x100;
    }
    setup_2d_text(gx);
    font::set_color(c, c);
    match glow {
        Some((g, pulse)) => {
            font::set_shadow(g, g);
            font::draw_box_glow(gx, f, &b, s, pulse);
        }
        None => font::draw_box(gx, f, &b, s),
    }
}

/// `Glh_DrawString` for STH0 string `idx`.
#[allow(clippy::too_many_arguments)]
pub fn draw_string<'a>(gx: &mut Gx<'a>, f: &'a UiFont, idx: usize, sth: &Strings, l: &Layout, bank: &'a [FastTex], c: [u8; 4], glow: Option<([u8; 4], i16)>, dx: i32, dy: i32) {
    let Some(st) = sth.get(idx) else { return };
    let Some(pane) = l.texts.get(st.pane as usize) else { return };
    draw_text_in(gx, f, l, bank, pane, &st.text, c, glow, dx, dy);
}

/// `Glh_DrawText` (0x8130A5D0)
pub fn draw_text<'a>(gx: &mut Gx<'a>, f: &'a UiFont, idx: usize, sth: &Strings, l: &Layout, bank: &'a [FastTex], c: [u8; 4]) {
    draw_string(gx, f, idx, sth, l, bank, c, None, 0, 0);
}

/// `Glh_DrawBannerText` (0x8130A5FC): `s` typed into text pane `idx`, up
/// to `max_chars` characters, on `lines` lines (0 = as many as fit).
#[allow(clippy::too_many_arguments)]
pub fn draw_banner_text<'a>(gx: &mut Gx<'a>, f: &'a UiFont, idx: usize, l: &Layout, bank: &'a [FastTex], c: [u8; 4], s: &[u8], max_chars: i32, lines: i32) {
    let Some(pane) = l.texts.get(idx) else { return };
    let b = font_box(pane);
    if let Some(bg) = pane.bg.and_then(|i| l.frames.get(i as usize)) {
        // BS2 measures the pane's default string here, not `s`.
        let (w, h) = font::measure_string(f, &b, &pane.default);
        let pad = bs2main::disp_settings().text_pad as i32 * 32;
        draw_text_bg(gx, bg, bank, c, 0, 0, w as i32 + pad, h as i32 + pad);
    }
    setup_2d_text(gx);
    font::set_color(c, c);
    match lines {
        0 => font::draw_box_n(gx, f, &b, s, max_chars),
        1 => font::draw_box_line1(gx, f, &b, s, max_chars),
        2 => font::draw_box_line2(gx, f, &b, s, max_chars),
        _ => {}
    }
}

/// `Glh_DrawBannerLine` (0x8130A75C): one line in every text pane with
/// this tag.
pub fn draw_banner_line<'a>(gx: &mut Gx<'a>, f: &'a UiFont, tag: u32, l: &Layout, bank: &'a [FastTex], c: [u8; 4], s: &[u8], max_chars: i32) {
    for i in (0..l.texts.len()).filter(|&i| l.texts[i].tag == tag) {
        draw_banner_text(gx, f, i, l, bank, c, s, max_chars, 1);
    }
}

/// `Glh_DrawBannerBox` (0x8130A7E4): two lines in every text pane with
/// this tag.
pub fn draw_banner_box<'a>(gx: &mut Gx<'a>, f: &'a UiFont, tag: u32, l: &Layout, bank: &'a [FastTex], c: [u8; 4], s: &[u8], max_chars: i32) {
    for i in (0..l.texts.len()).filter(|&i| l.texts[i].tag == tag) {
        draw_banner_text(gx, f, i, l, bank, c, s, max_chars, 2);
    }
}
