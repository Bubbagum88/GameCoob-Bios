//! GameCoob's sub-cubes: picking Memory Card or Options on the main cube
//! dives into a second cube with its own four sides, turned and entered
//! the same way. Not in BS2; the main cube and every screen it already
//! had stay as they were.
//!
//! The sides reuse the main cube's face positions (and so its turning,
//! zoom and icon code): up is `GameFace`, down `CardFace`, left
//! `OptionFace`, right `ClockFace`. Each sub-cube keeps the screen it grew
//! out of on that screen's old side, so the Memory Card screen is still
//! down and the Options screen still left.

use super::{with_a, MenuAssets, St, SHADOW, WHITE};
use super::super::ui::{cp1252, Rgba, Ui};
use crate::bs2::mtx;
use crate::ipl::glh::tag;
use crate::sdk::mtx::{self as psmtx, Mtx};
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;

/// Which cube is up.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Level {
    /// BS2's cube: Game Play, Memory Card, Options, Calendar.
    Main,
    /// Memory Card, Files, Drives; up goes back.
    Memory,
    /// Console (the Options screen), Boot, Firmware, Controls.
    Settings,
}

/// A screen that only exists on a sub-cube.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Page {
    Files,
    Drives,
    Boot,
    Firmware,
    Controls,
}

/// What A (or a turn, for "back") on a side leads to.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Target {
    /// One of BS2's own screens (`St::Game`, `Card`, `Option`, `Clock`).
    Screen(St),
    Page(Page),
    /// Dive into another cube.
    Cube(Level),
}

/// The four side labels of a sub-cube's front face: up, left, right, down.
fn labels(l: Level) -> [&'static str; 4] {
    match l {
        Level::Main => ["", "", "", ""],
        Level::Memory => ["Back", "Files", "Drives", "Memory Card"],
        Level::Settings => ["Boot", "Console", "Firmware", "Controls"],
    }
}

impl Level {
    /// A on a side (a `*Face` state).
    pub fn target(self, face: St) -> Option<Target> {
        Some(match (self, face) {
            // Minimal mode: the console's screens, no sub-cubes.
            (Level::Main, St::CardFace) if super::minimal() => Target::Screen(St::Card),
            (Level::Main, St::OptionFace) if super::minimal() => Target::Screen(St::Option),
            (Level::Main, St::CardFace) => Target::Cube(Level::Memory),
            (Level::Main, St::OptionFace) => Target::Cube(Level::Settings),
            (Level::Main, St::GameFace) => Target::Screen(St::Game),
            (Level::Main, St::ClockFace) => Target::Screen(St::Clock),
            (Level::Memory, St::CardFace) => Target::Screen(St::Card),
            (Level::Memory, St::OptionFace) => Target::Page(Page::Files),
            (Level::Memory, St::ClockFace) => Target::Page(Page::Drives),
            (Level::Memory, St::GameFace) => Target::Cube(Level::Main),
            (Level::Settings, St::OptionFace) => Target::Screen(St::Option),
            (Level::Settings, St::GameFace) => Target::Page(Page::Boot),
            (Level::Settings, St::ClockFace) => Target::Page(Page::Firmware),
            (Level::Settings, St::CardFace) => Target::Page(Page::Controls),
            _ => return None,
        })
    }

    /// The main cube's side this cube was entered from.
    pub fn home_face(self) -> St {
        match self {
            Level::Main => St::Top,
            Level::Memory => St::CardFace,
            Level::Settings => St::OptionFace,
        }
    }

    /// The side whose screen (and icon cube) BS2 already had on this cube.
    pub fn shows_icon(self, face: St) -> bool {
        match self {
            Level::Main => true,
            Level::Memory => face == St::CardFace,
            Level::Settings => face == St::OptionFace,
        }
    }

    /// The face of a side whose content is new (title and a few lines on
    /// a plain glass face), or None for the sides drawn as on the main
    /// cube.
    pub fn plain_face(self, face: St) -> Option<(&'static str, &'static [&'static str])> {
        if self == Level::Main || self.shows_icon(face) {
            return None;
        }
        // Title only: the side's icon cubes show through the middle.
        Some(match self.target(face)? {
            Target::Cube(_) => ("Main Menu", &[]),
            Target::Page(p) => (p.title(), &[]),
            Target::Screen(_) => return None,
        })
    }
}

impl Page {
    pub fn title(self) -> &'static str {
        match self {
            Page::Files => "Files",
            Page::Drives => "Drives",
            Page::Boot => "Boot",
            Page::Firmware => "Firmware",
            Page::Controls => "Controls",
        }
    }

    /// The side it sits on, where B returns to.
    pub fn face(self) -> St {
        match self {
            Page::Files => St::OptionFace,
            Page::Drives => St::ClockFace,
            Page::Boot => St::GameFace,
            Page::Firmware => St::ClockFace,
            Page::Controls => St::CardFace,
        }
    }

    /// The page's body lines.
    pub fn lines(self, entries: &[(u16, String)], fw_ui: bool) -> Vec<String> {
        let s = |t: &str| String::from(t);
        match self {
            Page::Boot => {
                let mut v = Vec::new();
                v.push(s("Boot order:"));
                for (i, (n, d)) in entries.iter().take(8).enumerate() {
                    v.push(format!("{}.  {}  (Boot{:04X})", i + 1, d, n));
                }
                if entries.is_empty() {
                    v.push(s("No boot entries."));
                }
                v.push(s(""));
                v.push(s("Editing the order comes later."));
                v
            }
            Page::Firmware => {
                let mut v = Vec::new();
                v.push(s("A opens the system menu:"));
                v.push(s(if fw_ui { "firmware setup, restart, power off." } else { "restart or power off." }));
                v.push(s(""));
                v.push(s("Minimal Mode is there too: the menu as"));
                v.push(s("close to the console's as it gets."));
                v.push(s(""));
                v.push(s("F10 opens it from anywhere."));
                v
            }
            Page::Files => [
                "Browse and convert memory card",
                "files and card images on your drives.",
                "",
                "Coming soon.",
            ]
            .map(s)
            .to_vec(),
            Page::Drives => [
                "Your hard drives and SSDs, their",
                "partitions, sizes and health.",
                "",
                "Coming soon.",
            ]
            .map(s)
            .to_vec(),
            Page::Controls => [
                "Map keys and controllers",
                "to the GameCube buttons.",
                "",
                "Coming soon.",
            ]
            .map(s)
            .to_vec(),
        }
    }
}

/// Size of the side labels on a sub-cube's front face, in layout units.
const LABEL_SIZE: f32 = 26.0;

/// A sub-cube's front face: one label along each side, reading like the
/// main cube's (left side bottom to top, right side top to bottom). `mv`
/// is the front face's layout matrix (288 x 288 units, y down).
pub fn draw_front<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, l: Level, mv: &Mtx, alpha: u8) {
    if alpha == 0 {
        return;
    }
    let a = alpha as f32 / 255.0;
    let c = with_a(WHITE, a * a);
    let sh = with_a(SHADOW, a * a);
    let [up, left, right, down] = labels(l);
    // (text, centre, rotation about the face normal)
    let at: [(&str, [f32; 2], i32); 4] = [
        (up, [144.0, 49.0], 0),
        (left, [48.0, 144.0], -0x4000),
        (right, [240.0, 144.0], 0x4000),
        (down, [144.0, 239.0], 0),
    ];
    for (s, p, r) in at {
        if s.is_empty() {
            continue;
        }
        let t = psmtx::concat(mv, &mtx::rot_trans(0, 0, r, [p[0], p[1], 0.0]));
        ui.mv = t;
        let s = cp1252(s);
        bold_label(ui, ma, 0.0, 0.0, LABEL_SIZE, &s, c, sh);
    }
}

/// A side with new content: the Options face's glass frame, a title and
/// a couple of lines. `ui.mv` is already the face's matrix.
pub fn draw_plain_face<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, lang: usize, title: &str, lines: &[&str], a: f32) {
    let g = &ma.option_face[lang];
    let bank = &ma.bank[..];
    for f in g.frame(tag(b"fram")) {
        ui.frame(f, bank, with_a(WHITE, a));
    }
    let (white, shadow) = (with_a(WHITE, a), with_a(SHADOW, a));
    let t = cp1252(title);
    ui.label(&ma.font, 146.0, 42.0, 34.0, &t, shadow, 0);
    ui.label(&ma.font, 144.0, 40.0, 34.0, &t, white, 0);
    for (i, s) in lines.iter().enumerate() {
        let s = cp1252(s);
        let y = 130.0 + i as f32 * 32.0;
        ui.label(&ma.font, 146.0, y + 2.0, 22.0, &s, shadow, 0);
        ui.label(&ma.font, 144.0, y, 22.0, &s, white, 0);
    }
}

/// A page on the 2D screen (584 x 448 units): the title and its lines,
/// over the backdrop the caller drew.
pub fn draw_page<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, title: &str, lines: &[String], a: f32) {
    let (white, shadow) = (with_a(WHITE, a), with_a(SHADOW, a));
    let t = cp1252(title);
    ui.label(&ma.font, 294.0, 62.0, 36.0, &t, shadow, 0);
    ui.label(&ma.font, 292.0, 60.0, 36.0, &t, white, 0);
    for (i, s) in lines.iter().enumerate() {
        let s = cp1252(s);
        let y = 140.0 + i as f32 * 28.0;
        ui.label(&ma.font, 90.0, y + 2.0, 20.0, &s, shadow, 2);
        ui.label(&ma.font, 88.0, y, 20.0, &s, white, 2);
    }
}

/// A side label like the IPL's pre-drawn ones: centred on (x, y), heavier
/// than the ROM font (drawn three times a little apart) over a soft
/// shadow.
pub fn bold_label<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, x: f32, y: f32, size: f32, s: &[u8], c: Rgba, sh: Rgba) {
    let top = y - size / 2.0;
    ui.label(&ma.font, x + 2.0, top + 2.0, size, s, sh, 0);
    for dx in [-0.8, 0.8, 0.0] {
        ui.label(&ma.font, x + dx, top, size, s, c, 0);
    }
}

/// The icon cubes of the new sides: cube, side, tint (the icon colour of
/// the main-cube side the cube grew out of) and picture.
pub fn icons(d: &super::icons::IconData) -> Vec<(Level, St, super::icons::PixelIcon)> {
    use super::icons::{PixelIcon, Side, TINT_CARD, TINT_OPTION};
    const FILES: &[&str] = &[
        "#####...",
        "#...##..",
        "#...#.#.",
        "#...####",
        "#......#",
        "#.####.#",
        "#......#",
        "#.####.#",
        "#......#",
        "########",
    ];
    // A 3.5" hard drive seen from the top: the case, the platter with its
    // spindle, and the head arm swinging in from the bottom right.
    const DRIVES: &[&str] = &[
        "############",
        "#..........#",
        "#...####...#",
        "#..#....#..#",
        "#.#......#.#",
        "#.#..##..#.#",
        "#.#..##..#.#",
        "#.#....#.#.#",
        "#..#....##.#",
        "#...####.#.#",
        "#.......##.#",
        "#.......##.#",
        "#..........#",
        "#.##....##.#",
        "############",
    ];
    const BOOT: &[&str] = &[
        "....#....",
        ".#..#..#.",
        "#...#...#",
        "#...#...#",
        "#.......#",
        "#.......#",
        ".#.....#.",
        "..#####..",
    ];
    const FIRMWARE: &[&str] = &[
        "..#.#.#..",
        ".#######.",
        "##.....##",
        ".#.###.#.",
        "##.###.##",
        ".#.###.#.",
        "##.....##",
        ".#######.",
        "..#.#.#..",
    ];
    const CONTROLS: &[&str] = &[
        "...###...",
        "...#.#...",
        "####.####",
        "#.......#",
        "####.####",
        "...#.#...",
        "...###...",
    ];
    alloc::vec![
        (Level::Memory, St::OptionFace, PixelIcon::new(d, Side::Left, TINT_CARD, FILES)),
        (Level::Memory, St::ClockFace, PixelIcon::new(d, Side::Right, TINT_CARD, DRIVES)),
        (Level::Settings, St::GameFace, PixelIcon::new(d, Side::Up, TINT_OPTION, BOOT)),
        (Level::Settings, St::ClockFace, PixelIcon::new(d, Side::Right, TINT_OPTION, FIRMWARE)),
        (Level::Settings, St::CardFace, PixelIcon::new(d, Side::Down, TINT_OPTION, CONTROLS)),
    ]
}
