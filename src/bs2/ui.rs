//! 2D drawing for the menu: GLH0 panes, 9-slice frames, ROM-font text and
//! blocky "mosaic" text, as textured quads in any 3D plane (a cube face or
//! the screen), queued into the same display list as the models.

use super::mtx::{self, Mtx};
use super::raster::{BlendMode, Prim, SVert, Sh, State};
use super::render::View;
use super::shader::FastTex;
use crate::ipl::glh::{Font, Frame, Layout, Pic, Str, Text};
use alloc::vec::Vec;

pub type Rgba = [f32; 4];

pub const WHITE: Rgba = [1.0; 4];
/// Drop shadow colour of the "z" panes (40, 40, 40).
pub const SHADOW: Rgba = [40.0 / 255.0, 40.0 / 255.0, 40.0 / 255.0, 1.0];

pub fn rgba8(c: [u8; 4]) -> Rgba {
    c.map(|v| v as f32 / 255.0)
}

pub fn with_alpha(c: Rgba, a: f32) -> Rgba {
    [c[0], c[1], c[2], c[3] * a]
}

/// GX_Setup2D also sets GX_CULL_BACK, so faces turned away are not drawn.
const ST: State = State { z_compare: false, z_update: false, blend: BlendMode::Alpha, cull: 2 };

/// The ROM font with one texture per glyph sheet.
pub struct UiFont {
    pub font: Font,
    pub sheets: Vec<FastTex>,
}

impl UiFont {
    pub fn new(font: Font) -> UiFont {
        let (w, h) = (font.sheet_w as usize, font.sheet_h as usize);
        let sheets = font.sheets.iter().map(|s| FastTex::from_alpha(w, h, s)).collect();
        UiFont { font, sheets }
    }

    /// Advance of `c` at glyph size `size`.
    pub fn adv(&self, c: u8, size: f32, spacing: f32) -> f32 {
        self.font.advance(c) as f32 * size / self.font.cell_w as f32 + spacing
    }

    pub fn width(&self, s: &[u8], size: f32, spacing: f32) -> f32 {
        s.iter().map(|&c| self.adv(c, size, spacing)).sum()
    }

    /// Coverage 0..1 at glyph-cell pixel (x, y).
    /// Width and height of (possibly multi-line) text set in pane `p`.
    pub fn measure(&self, p: &Text, s: &[u8], size: f32) -> (f32, f32) {
        let line_h = if p.line > 0.0 { p.line.max(size) } else { size };
        let lines = s.split(|&c| c == b'\n');
        let (mut w, mut n) = (0.0f32, 0);
        for l in lines {
            w = w.max(self.width(l, size, p.spacing));
            n += 1;
        }
        (w, (n as f32 - 1.0) * line_h + size)
    }

    pub fn coverage(&self, c: u8, x: i32, y: i32) -> f32 {
        let f = &self.font;
        let Some(g) = f.glyph(c) else { return 0.0 };
        if x < 0 || y < 0 || x >= f.cell_w as i32 || y >= f.cell_h as i32 {
            return 0.0;
        }
        let (sh, cx, cy) = f.cell(g);
        let Some(sheet) = f.sheets.get(sh) else { return 0.0 };
        let i = (cy as usize + y as usize) * f.sheet_w as usize + cx as usize + x as usize;
        sheet.get(i).copied().unwrap_or(0) as f32 / 255.0
    }
}

/// Text from our own strings: plain ASCII maps 1:1 onto Windows-1252.
pub fn cp1252(s: &str) -> Vec<u8> {
    s.chars().map(|c| if (c as u32) < 0x100 { c as u8 } else { b'?' }).collect()
}

/// Draws into `list` in a plane given by `mv` (layout units to view space).
pub struct Ui<'a, 'l> {
    pub list: &'l mut Vec<Prim<'a>>,
    pub view: &'l View,
    pub mv: Mtx,
}

impl<'a> Ui<'a, '_> {
    /// One quad, corners TL, TR, BR, BL in layout units.
    pub fn quad(&mut self, p: [[f32; 2]; 4], uv: [[f32; 2]; 4], col: [Rgba; 4], sh: Sh<'a>) {
        let mut v = [SVert::default(); 4];
        for k in 0..4 {
            let q = mtx::apply(&self.mv, [p[k][0], p[k][1], 0.0]);
            let Some(s) = self.view.project(q) else { return };
            v[k] = SVert { x: s[0], y: s[1], z: s[2].clamp(0.0, 1.0), rw: s[3], color: col[k], uv: [uv[k], [0.0; 2]] };
        }
        self.list.push(Prim { v: [v[0], v[1], v[2]], st: ST, sh });
        self.list.push(Prim { v: [v[0], v[2], v[3]], st: ST, sh });
    }

    pub fn rect(&mut self, x: f32, y: f32, w: f32, h: f32, uv: [[f32; 2]; 4], c: Rgba, tex: Option<&'a FastTex>) {
        if c[3] <= 0.0 {
            return;
        }
        let sh = match tex {
            Some(t) => Sh::Modulate { tex: t },
            None => Sh::Flat,
        };
        self.quad([[x, y], [x + w, y], [x + w, y + h], [x, y + h]], uv, [c; 4], sh);
    }

    pub fn fill(&mut self, x: f32, y: f32, w: f32, h: f32, c: Rgba) {
        self.rect(x, y, w, h, [[0.0; 2]; 4], c, None);
    }

    /// A picture pane (flags: bit 0 mirrors t, bit 1 mirrors s, bit 2 turns
    /// 90 degrees; `Glh_DrawPaneQuad` 0x81309E54).
    pub fn pic(&mut self, p: &Pic, tex: &'a FastTex, c: Rgba, dx: f32, dy: f32) {
        let (mut s0, mut s1, mut t0, mut t1) = (0.0, 1.0, 0.0, 1.0);
        if p.flags & 1 != 0 {
            core::mem::swap(&mut t0, &mut t1);
        }
        if p.flags & 2 != 0 {
            core::mem::swap(&mut s0, &mut s1);
        }
        let uv = if p.flags & 4 != 0 {
            [[s0, t1], [s0, t0], [s1, t0], [s1, t1]]
        } else {
            [[s0, t0], [s1, t0], [s1, t1], [s0, t1]]
        };
        self.rect(p.x - p.w / 2.0 + dx, p.y - p.h / 2.0 + dy, p.w, p.h, uv, c, Some(tex));
    }

    /// A picture pane drawn with another texture (digits, weekday names).
    pub fn pic_tex(&mut self, p: &Pic, tex: &'a FastTex, c: Rgba) {
        self.pic(&Pic { flags: 0, ..*p }, tex, c, 0.0, 0.0);
    }

    /// `Glh_DrawPaneRect`: a 9-slice frame around the inner rectangle
    /// (x, y, w, h). The centre takes the frame's own corner colours (alpha
    /// scaled by `c`), the textured border takes `c`.
    pub fn frame_rect(&mut self, f: &Frame, bank: &'a [FastTex], c: Rgba, x: f32, y: f32, w: f32, h: f32) {
        if c[3] <= 0.0 {
            return;
        }
        let corner = |k: usize| rgba8(f.colors[k]);
        let col = [corner(0), corner(1), corner(3), corner(2)].map(|k| [k[0], k[1], k[2], k[3] * c[3]]);
        self.quad([[x, y], [x + w, y], [x + w, y + h], [x, y + h]], [[0.0; 2]; 4], col, Sh::Flat);
        let Some(t0) = bank.get(f.tex[0] as usize) else { return };
        let (cw, ch) = t0.size();
        let flip = [f.flips >> 6, f.flips >> 4, f.flips >> 2, f.flips].map(|v| v & 3);
        let tex = |k: usize| bank.get(f.tex[k] as usize);
        // Corners: TL, TR, BL, BR.
        let pos = [(x - cw, y - ch), (x + w, y - ch), (x - cw, y + h), (x + w, y + h)];
        for k in 0..4 {
            let Some(t) = tex(k) else { continue };
            let (s0, s1) = if flip[k] & 2 != 0 { (1.0, 0.0) } else { (0.0, 1.0) };
            let (u0, u1) = if flip[k] & 1 != 0 { (1.0, 0.0) } else { (0.0, 1.0) };
            self.rect(pos[k].0, pos[k].1, cw, ch, [[s0, u0], [s1, u0], [s1, u1], [s0, u1]], c, Some(t));
        }
        // Edges stretch one row or column of a corner texture.
        let edge = |k: usize, along_x: bool| -> [[f32; 2]; 4] {
            let a = if flip[k] & 2 != 0 { 1.0 } else { 0.0 };
            let b = if flip[k] & 1 != 0 { 1.0 } else { 0.0 };
            if along_x {
                // s fixed, t runs top to bottom.
                [[a, b], [a, b], [a, 1.0 - b], [a, 1.0 - b]]
            } else {
                // t fixed, s runs left to right.
                [[a, b], [1.0 - a, b], [1.0 - a, b], [a, b]]
            }
        };
        if let Some(t) = tex(1) {
            self.rect(x, y - ch, w, ch, edge(1, true), c, Some(t));
        }
        if let Some(t) = tex(3) {
            self.rect(x, y + h, w, ch, edge(3, true), c, Some(t));
            self.rect(x + w, y, cw, h, edge(3, false), c, Some(t));
        }
        if let Some(t) = tex(2) {
            self.rect(x - cw, y, cw, h, edge(2, false), c, Some(t));
        }
    }

    /// `Glh_DrawFrame`: the frame at its own position and size.
    pub fn frame(&mut self, f: &Frame, bank: &'a [FastTex], c: Rgba) {
        self.frame_rect(f, bank, c, f.x - f.inner_w / 2.0, f.y - f.inner_h / 2.0, f.inner_w, f.inner_h);
    }

    /// Text in a text pane, aligned like BS2's string drawing; the pane's
    /// top/bottom colours are multiplied by `c`.
    pub fn text(&mut self, font: &'a UiFont, p: &Text, s: &[u8], c: Rgba, dx: f32, dy: f32) {
        self.text_sized(font, p, s, c, dx, dy, p.size);
    }

    #[allow(clippy::too_many_arguments)]
    pub fn text_sized(&mut self, font: &'a UiFont, p: &Text, s: &[u8], c: Rgba, dx: f32, dy: f32, size: f32) {
        if c[3] <= 0.0 || s.is_empty() {
            return;
        }
        let line_h = if p.line > 0.0 { p.line.max(size) } else { size };
        let lines: Vec<&[u8]> = s.split(|&ch| ch == b'\n').collect();
        let total = (lines.len() as f32 - 1.0) * line_h + size;
        let (cx, cy) = (p.x + dx, p.y + dy);
        let top = match p.v_align {
            1 => cy + p.h / 2.0 - total,
            2 => cy - p.h / 2.0,
            _ => cy - total / 2.0,
        };
        let ct = rgba8(p.top);
        let cb = rgba8(p.bottom);
        let mul = |a: Rgba| [a[0] * c[0], a[1] * c[1], a[2] * c[2], a[3] * c[3]];
        let (ct, cb) = (mul(ct), mul(cb));
        let f = &font.font;
        let (cw, chh) = (f.cell_w as f32, f.cell_h as f32);
        for (li, line) in lines.iter().enumerate() {
            let lw = font.width(line, size, p.spacing);
            let mut x = match p.h_align {
                1 => cx + p.w / 2.0 - lw,
                2 => cx - p.w / 2.0,
                _ => cx - lw / 2.0,
            };
            let y = top + li as f32 * line_h;
            for &ch in line.iter() {
                if let Some(g) = f.glyph(ch) {
                    let (sh, gx, gy) = f.cell(g);
                    if let Some(tex) = font.sheets.get(sh) {
                        let (sw, shh) = tex.size();
                        let (u0, v0) = (gx as f32 / sw, gy as f32 / shh);
                        let (u1, v1) = ((gx as f32 + cw) / sw, (gy as f32 + chh) / shh);
                        let gw = size * cw / chh;
                        self.quad(
                            [[x, y], [x + gw, y], [x + gw, y + size], [x, y + size]],
                            [[u0, v0], [u1, v0], [u1, v1], [u0, v1]],
                            [ct, ct, cb, cb],
                            Sh::Modulate { tex },
                        );
                    }
                }
                x += font.adv(ch, size, p.spacing);
            }
        }
    }

    /// `Glh_DrawString` (0x8130A39C): STH0 string `st` in its text pane,
    /// over the pane's background frame (if it has one) sized to the text
    /// plus 8 units of padding on every side.
    #[allow(clippy::too_many_arguments)]
    pub fn string(&mut self, font: &'a UiFont, l: &Layout, bank: &'a [FastTex], st: &Str, c: Rgba, dx: f32, dy: f32) {
        if c[3] <= 0.0 {
            return;
        }
        let Some(p) = l.texts.get(st.pane as usize) else { return };
        if let Some(f) = p.bg.and_then(|i| l.frames.get(i as usize)) {
            let (w, h) = font.measure(p, &st.text, p.size);
            let (w, h) = (w + 16.0, h + 16.0);
            self.frame_rect(f, bank, c, f.x - w / 2.0 + dx, f.y - h / 2.0 + dy, w, h);
        }
        self.text(font, p, &st.text, c, dx, dy);
    }

    /// Text at a point with no layout pane (left, top).
    pub fn label(&mut self, font: &'a UiFont, x: f32, y: f32, size: f32, s: &[u8], c: Rgba, align: u8) {
        let p = Text {
            tag: 0,
            x,
            y,
            w: 0.0,
            h: 0.0,
            top: [255; 4],
            bottom: [255; 4],
            h_align: align,
            v_align: 2,
            spacing: 0.0,
            line: size + 4.0,
            size,
            bg: None,
            default: Vec::new(),
        };
        self.text(font, &p, s, c, 0.0, 0.0);
    }

    /// Text built from small blocks, like the IPL's cube mosaic: the ROM
    /// font sampled on a coarse grid, each lit cell a block shaded from
    /// `light` (top) to `dark` (bottom). `cell` is the block pitch; `x`
    /// is the centre of the line and `y` its top.
    #[allow(clippy::too_many_arguments)]
    pub fn mosaic(&mut self, font: &UiFont, x: f32, y: f32, cell: f32, s: &[u8], light: Rgba, dark: Rgba, a: f32) {
        if a <= 0.0 || s.is_empty() {
            return;
        }
        // Each block covers a 3x3 patch of the 24-pixel glyph cell.
        const K: i32 = 3;
        let f = &font.font;
        let rows = f.cell_h as i32 / K;
        let cols_of = |c: u8| (f.advance(c) as i32 + K - 1) / K;
        let total: i32 = s.iter().map(|&c| cols_of(c) + 1).sum::<i32>() - 1;
        let mut bx = x - total as f32 * cell / 2.0;
        let blk = cell * 0.82;
        for &c in s {
            let n = cols_of(c);
            for gy in 0..rows {
                for gx in 0..n {
                    let mut cov = 0.0;
                    for sy in 0..K {
                        for sx in 0..K {
                            cov += font.coverage(c, gx * K + sx, gy * K + sy);
                        }
                    }
                    if cov / (K * K) as f32 > 0.45 {
                        let t = gy as f32 / rows as f32;
                        let top = lerp4(light, dark, t);
                        let bot = lerp4(light, dark, t + 0.6 / rows as f32);
                        let (px, py) = (bx + gx as f32 * cell, y + gy as f32 * cell);
                        let col = [with_alpha(top, a), with_alpha(top, a), with_alpha(bot, a), with_alpha(bot, a)];
                        self.quad([[px, py], [px + blk, py], [px + blk, py + blk], [px, py + blk]], [[0.0; 2]; 4], col, Sh::Flat);
                    }
                }
            }
            bx += (n + 1) as f32 * cell;
        }
    }
}

fn lerp4(a: Rgba, b: Rgba, t: f32) -> Rgba {
    let t = t.clamp(0.0, 1.0);
    [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t, a[3] + (b[3] - a[3]) * t]
}
