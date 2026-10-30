//! 2D drawing for the menu: GLH0 panes, 9-slice frames, ROM-font text and
//! blocky "mosaic" text, in any 3D plane (a cube face or the screen),
//! through GX after the models drawn so far.

use crate::sdk::gx::*;
use crate::sdk::mtx::Mtx;
use super::raster::Sh;
use super::render::View;
use super::font::{self, FontBox};
use super::{ginit, glh};
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

/// Draws through GX in a plane given by `mv` (layout units to view space,
/// loaded as the position matrix): a cube face, or the screen
/// (`GX_Setup2DView`). Panes, frames and text go through the ported
/// [`glh`] and [`font`] units.
pub struct Ui<'a, 'l> {
    pub gx: &'l mut Gx<'a>,
    pub view: &'l View,
    pub mv: Mtx,
}

fn rgba_u8(c: Rgba) -> [u8; 4] {
    c.map(|v| (v.clamp(0.0, 1.0) * 255.0 + 0.5) as u8)
}

impl<'a> Ui<'a, '_> {
    /// The plane's matrices, loaded before every draw (other code may
    /// have changed them in between).
    fn load(&mut self) {
        self.gx.set_projection(&self.view.projection());
        self.gx.set_viewport(0.0, 0.0, self.view.vp[0], self.view.vp[1]);
        self.gx.load_pos_mtx_imm(&self.mv, GX_PNMTX0);
        self.gx.set_current_mtx(GX_PNMTX0);
    }

    /// One quad, corners TL, TR, BR, BL in layout units: flat colored, or
    /// texture times color.
    pub fn quad(&mut self, p: [[f32; 2]; 4], uv: [[f32; 2]; 4], col: [Rgba; 4], sh: Sh<'a>) {
        self.load();
        let tex = match sh {
            Sh::Modulate { tex } => Some(tex),
            _ => None,
        };
        ginit::setup_2d(self.gx, tex.is_some(), true, true);
        if let Some(t) = tex {
            self.gx.load_tex_obj(t, 0);
        }
        self.gx.begin(GX_QUADS, 1, 4);
        for k in 0..4 {
            self.gx.position3f32(p[k][0], p[k][1], 0.0);
            self.gx.color1u32(u32::from_be_bytes(rgba_u8(col[k])));
            self.gx.tex_coord2f32(uv[k][0], uv[k][1]);
        }
        self.gx.end();
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

    /// A picture pane, moved by dx, dy (`Glh_DrawPicture`). The texture is
    /// multiplied by the color (BS2's `GX_Setup2DTex` takes only the
    /// texture's alpha; every IPL pane texture is white with alpha).
    pub fn pic(&mut self, p: &Pic, tex: &'a FastTex, c: Rgba, dx: f32, dy: f32) {
        if c[3] <= 0.0 {
            return;
        }
        self.load();
        ginit::setup_2d(self.gx, true, false, true);
        glh::draw_picture(self.gx, p, rgba_u8(c), Some(tex), glh::fx(dx), glh::fx(dy));
    }

    /// A picture pane drawn with another texture (digits, weekday names).
    pub fn pic_tex(&mut self, p: &Pic, tex: &'a FastTex, c: Rgba) {
        self.pic(&Pic { flags: 0, ..*p }, tex, c, 0.0, 0.0);
    }

    /// `Glh_DrawPaneRect`: a 9-slice frame around the inner rectangle
    /// (x, y, w, h).
    #[allow(clippy::too_many_arguments)]
    pub fn frame_rect(&mut self, f: &Frame, bank: &'a [FastTex], c: Rgba, x: f32, y: f32, w: f32, h: f32) {
        if c[3] <= 0.0 {
            return;
        }
        self.load();
        glh::draw_pane_rect(self.gx, f, bank, rgba_u8(c), glh::fx(x), glh::fx(y), glh::fx(w), glh::fx(h));
    }

    /// `Glh_DrawFrame`: the frame at its own position and size.
    pub fn frame(&mut self, f: &Frame, bank: &'a [FastTex], c: Rgba) {
        if c[3] <= 0.0 {
            return;
        }
        self.load();
        glh::draw_frame_at(self.gx, f, bank, rgba_u8(c), 0, 0);
    }

    /// Text in a text pane, as `Glh_DrawString` sets it (in color `c`).
    pub fn text(&mut self, font: &'a UiFont, p: &Text, s: &[u8], c: Rgba, dx: f32, dy: f32) {
        self.text_sized(font, p, s, c, dx, dy, p.size);
    }

    /// [`Ui::text`] at another glyph size.
    #[allow(clippy::too_many_arguments)]
    pub fn text_sized(&mut self, font: &'a UiFont, p: &Text, s: &[u8], c: Rgba, dx: f32, dy: f32, size: f32) {
        self.text_glow(font, p, s, c, None, dx, dy, size);
    }

    /// [`Ui::text`], glowing in `glow` with the given pulse when set.
    #[allow(clippy::too_many_arguments)]
    pub fn text_glow(&mut self, font: &'a UiFont, p: &Text, s: &[u8], c: Rgba, glow: Option<(Rgba, i16)>, dx: f32, dy: f32, size: f32) {
        if c[3] <= 0.0 || s.is_empty() {
            return;
        }
        self.load();
        let pane = Text { size, bg: None, ..p.clone() };
        let glow = glow.map(|(g, pulse)| (rgba_u8(g), pulse));
        glh::draw_text_in(self.gx, font, &Layout::default(), &[], &pane, s, rgba_u8(c), glow, glh::fx(dx), glh::fx(dy));
    }

    /// `Glh_DrawString` (0x8130A39C): STH0 string `st` in its text pane,
    /// over the pane's background frame (if it has one) sized to the text.
    #[allow(clippy::too_many_arguments)]
    pub fn string(&mut self, font: &'a UiFont, l: &Layout, bank: &'a [FastTex], st: &Str, c: Rgba, dx: f32, dy: f32) {
        if c[3] <= 0.0 {
            return;
        }
        let Some(p) = l.texts.get(st.pane as usize) else { return };
        self.load();
        glh::draw_text_in(self.gx, font, l, bank, p, &st.text, rgba_u8(c), None, glh::fx(dx), glh::fx(dy));
    }

    /// Text at a point with no layout pane: `align` 0 centres it on `x`,
    /// 1 ends it there, 2 starts it there; `y` is its top.
    pub fn label(&mut self, f: &'a UiFont, x: f32, y: f32, size: f32, s: &[u8], c: Rgba, align: u8) {
        if c[3] <= 0.0 || s.is_empty() {
            return;
        }
        // A box wide enough never to wrap, placed so its aligned edge is at
        // x, and with no height so its top is at y.
        const W: f32 = 1024.0;
        let bx = match align {
            1 => x - W / 2.0,
            2 => x + W / 2.0,
            _ => x,
        };
        let b = FontBox {
            x: glh::fx(bx),
            y: glh::fx(y),
            w: glh::fx(W),
            h: 0,
            halign: align,
            valign: 2,
            char_space: 0,
            line_space: (size + 4.0) as i16,
            size: size as u16,
        };
        self.load();
        glh::setup_2d_text(self.gx);
        font::set_color(rgba_u8(c), rgba_u8(c));
        font::draw_box(self.gx, f, &b, s);
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
