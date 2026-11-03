//! GameCoob's sub-menus: turning the main menu's cube to Game Play, Memory
//! or Options keeps the same cube going. Once it has turned all the way,
//! that side is a new front with its own sides around it, turned to and
//! entered with A as BS2's (`Menu::roll_in`); B, or turning back the way
//! it came, rolls back to the main front (`Menu::roll_back`). No A, no
//! zoom and no second cube in between. Not in BS2; the screens it already
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
    /// Memory Card, Files, Drives; no up side (B goes back).
    Memory,
    /// Game Play (the console's screen), Boot, Apps; no down side (B goes back).
    Game,
    /// Console (the Options screen), Boot, Firmware, Controls.
    Settings,
}

/// A screen that only exists on a sub-cube.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Page {
    /// Pick an OS to start now (BootNext).
    BootNow,
    Apps,
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
        Level::Memory => ["", "Files", "Drives", "Memory Card"],
        Level::Game => ["Game Play", "Boot", "Apps", ""],
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
            (Level::Main, St::GameFace) if !super::minimal() => Target::Cube(Level::Game),
            (Level::Main, St::CardFace) => Target::Cube(Level::Memory),
            (Level::Main, St::OptionFace) => Target::Cube(Level::Settings),
            (Level::Main, St::GameFace) => Target::Screen(St::Game),
            (Level::Main, St::ClockFace) => Target::Screen(St::Clock),
            (Level::Memory, St::CardFace) => Target::Screen(St::Card),
            (Level::Memory, St::OptionFace) => Target::Page(Page::Files),
            (Level::Memory, St::ClockFace) => Target::Page(Page::Drives),
            (Level::Game, St::GameFace) => Target::Screen(St::Game),
            (Level::Game, St::OptionFace) => Target::Page(Page::BootNow),
            (Level::Game, St::ClockFace) => Target::Page(Page::Apps),
            (Level::Settings, St::OptionFace) => Target::Screen(St::Option),
            (Level::Settings, St::GameFace) => Target::Page(Page::Boot),
            (Level::Settings, St::ClockFace) => Target::Page(Page::Firmware),
            (Level::Settings, St::CardFace) => Target::Page(Page::Controls),
            _ => return None,
        })
    }

    /// The main menu's side this sub-menu rolled in from.
    pub fn home_face(self) -> St {
        match self {
            Level::Main => St::Top,
            Level::Memory => St::CardFace,
            Level::Game => St::GameFace,
            Level::Settings => St::OptionFace,
        }
    }

    /// The side whose screen (and icon cube) BS2 already had on this cube.
    pub fn shows_icon(self, face: St) -> bool {
        match self {
            // its sub-menu sides show no icon: they roll on to a new front
            Level::Main => super::minimal() || face == St::ClockFace,
            Level::Memory => face == St::CardFace,
            Level::Game => face == St::GameFace,
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
            Target::Page(p) => (p.title(), &[]),
            Target::Cube(_) | Target::Screen(_) => return None,
        })
    }
}

impl Page {
    pub fn title(self) -> &'static str {
        match self {
            Page::BootNow => "Boot",
            Page::Apps => "Apps",
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
            Page::BootNow => St::OptionFace,
            Page::Apps => St::ClockFace,
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
            Page::BootNow => {
                let mut v = Vec::new();
                v.push(s("Start which system?"));
                for (d, _) in entries.iter().take(BOOT_ROWS).map(|(n, d)| (d, n)) {
                    v.push(d.clone());
                }
                if entries.is_empty() {
                    v.push(s("No boot entries."));
                }
                v
            }
            Page::Apps => [
                "Start .efi apps from your drives:",
                "the UEFI Shell, memory testers,",
                "firmware flashers.",
                "",
                "Coming soon.",
            ]
            .map(s)
            .to_vec(),
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

/// Boot entries the Boot page lists.
pub const BOOT_ROWS: usize = 8;

/// Layout units per texel of the front face's labels (the IPL draws its
/// 200 x 32 word pictures there at 168 x 27).
pub const FRONT_SCALE: f32 = 168.0 / 200.0;

/// Every word [`word_label`] draws, lettered once at load.
pub fn words() -> Vec<&'static str> {
    let mut v: Vec<&'static str> = Vec::new();
    let pages = [Page::BootNow, Page::Apps, Page::Files, Page::Drives, Page::Boot, Page::Firmware, Page::Controls];
    let all = [Level::Memory, Level::Game, Level::Settings].into_iter().flat_map(labels);
    for w in all.chain(pages.into_iter().map(Page::title)).chain([super::MEMORY_LABEL]) {
        if !w.is_empty() && !v.contains(&w) {
            v.push(w);
        }
    }
    v
}

/// A sub-cube's front face: one label along each side, reading like the
/// main cube's (left side bottom to top, right side top to bottom). `mv`
/// is the front face's layout matrix (288 x 288 units, y down).
pub fn draw_front<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, l: Level, mv: &Mtx, alpha: u8) {
    if alpha == 0 {
        return;
    }
    let a = alpha as f32 / 255.0;
    let c = with_a(WHITE, a * a);
    let [up, left, right, down] = labels(l);
    // (text, centre, rotation about the face normal), where the main
    // cube has its labels
    let at: [(&str, [f32; 2], i32); 4] = [
        (up, [144.0, 48.0], 0),
        (left, [48.0, 144.0], -0x4000),
        (right, [239.0, 144.0], 0x4000),
        (down, [144.0, 239.0], 0),
    ];
    for (s, p, r) in at {
        if s.is_empty() {
            continue;
        }
        ui.mv = psmtx::concat(mv, &mtx::rot_trans(0, 0, r, [p[0], p[1], 0.0]));
        word_label(ui, ma, 0.0, 0.0, FRONT_SCALE, s, c);
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
    // where the Options face has its title picture
    let (tx, ty) = g.pic(tag(b"titl")).next().map_or((144.0, 40.0), |p| (p.x, p.y));
    word_label(ui, ma, tx, ty, 1.0, title, white);
    for (i, s) in lines.iter().enumerate() {
        let s = cp1252(s);
        let y = 130.0 + i as f32 * 32.0;
        ui.label(&ma.font, 146.0, y + 2.0, 22.0, &s, shadow, 0);
        ui.label(&ma.font, 144.0, y, 22.0, &s, white, 0);
    }
}

/// A page on the 2D screen (584 x 448 units): the title and its lines,
/// over the backdrop the caller drew.
/// `sel` highlights that line (a list to pick from).
pub fn draw_page<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, title: &str, lines: &[String], sel: Option<usize>, a: f32) {
    let (white, shadow) = (with_a(WHITE, a), with_a(SHADOW, a));
    let t = cp1252(title);
    ui.label(&ma.font, 294.0, 62.0, 36.0, &t, shadow, 0);
    ui.label(&ma.font, 292.0, 60.0, 36.0, &t, white, 0);
    for (i, s) in lines.iter().enumerate() {
        let s = cp1252(s);
        let y = 140.0 + i as f32 * 28.0;
        ui.label(&ma.font, 90.0, y + 2.0, 20.0, &s, shadow, 2);
        let c = if sel == Some(i) { with_a([1.0, 0.93, 0.6, 1.0], a) } else { white };
        ui.label(&ma.font, 88.0, y, 20.0, &s, c, 2);
        if sel == Some(i) {
            ui.label(&ma.font, 70.0, y, 20.0, b">", c, 2);
        }
    }
}

/// A label in the IPL's lettering, centred on (x, y) at `scale` layout
/// units per texel, like BS2's word pictures (white, no shadow). Words
/// the label font could not set fall back to the ROM font, made heavier.
pub fn word_label<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, x: f32, y: f32, scale: f32, s: &str, c: Rgba) {
    if let Some(t) = ma.label(s) {
        let (w, h) = t.size();
        let (w, h) = (w * scale, h * scale);
        ui.rect(x - w / 2.0, y - h / 2.0, w, h, [[0.0, 0.0], [1.0, 0.0], [1.0, 1.0], [0.0, 1.0]], c, Some(t));
        return;
    }
    let size = 31.0 * scale;
    let s = cp1252(s);
    let top = y - size / 2.0;
    ui.label(&ma.font, x + 2.0, top + 2.0, size, &s, with_a(SHADOW, c[3] * 0.5), 0);
    for dx in [-0.8, 0.8, 0.0] {
        ui.label(&ma.font, x + dx, top, size, &s, c, 0);
    }
}

/// The icon cubes of the new sides: cube, side, tint (the icon colour of
/// the main-cube side the cube grew out of) and picture.
pub fn icons(d: &super::icons::IconData) -> Vec<(Level, St, super::icons::PixelIcon)> {
    use super::icons::{Motion, PixelIcon, Side, TINT_CARD, TINT_GAME, TINT_OPTION};
    // Bitmaps: `.` is empty, `#` a still cube, any other character a cube
    // of that motion group.
    // Nine app tiles that flip over in turn, clockwise round the edge,
    // then the middle.
    const APPS: &[&str] = &[
        "11.22.33",
        "11.22.33",
        "........",
        "44.55.66",
        "44.55.66",
        "........",
        "77.88.99",
        "77.88.99",
    ];
    const APPS_MOTION: &[Motion] = &[Motion::Flip { groups: b"123698745", period: 16 }];
    // A folder; its two sheets lift a little now and then.
    const FILES: &[&str] = &[
        "#####...",
        "#...##..",
        "#...#.#.",
        "#...####",
        "#......#",
        "#.pppp.#",
        "#......#",
        "#.pppp.#",
        "#......#",
        "########",
    ];
    const FILES_MOTION: &[Motion] = &[Motion::Lift { group: b'p', period: 150 }];
    // A 3.5" hard drive seen from the top: the case, the platter (which
    // spins) and the head arm swinging in from the bottom right to the
    // spindle.
    const DRIVES: &[&str] = &[
        "############",
        "#..........#",
        "#...oooo...#",
        "#..o....o..#",
        "#.o......o.#",
        "#.o..#...o.#",
        "#.o...#..o.#",
        "#.o....#.o.#",
        "#..o....o#.#",
        "#...oooo.#.#",
        "#.......##.#",
        "#.......##.#",
        "#..........#",
        "#.##....##.#",
        "############",
    ];
    const DRIVES_MOTION: &[Motion] = &[Motion::Orbit { group: b'o', speed: -400 }];
    // The power symbol: a pulse climbs the bar, then the ring ripples.
    const BOOT: &[&str] = &[
        "....b....",
        ".o..b..o.",
        "o...b...o",
        "o...b...o",
        "o.......o",
        "o.......o",
        ".o.....o.",
        "..ooooo..",
    ];
    const BOOT_MOTION: &[Motion] = &[
        Motion::Climb { group: b'b', period: 100 },
        Motion::Ripple { group: b'o', period: 100, delay: 30 },
    ];
    // A chip: a light runs round its pins, and the other way round its
    // core.
    const FIRMWARE: &[&str] = &[
        "..p.p.p..",
        ".#######.",
        "p#.....#p",
        ".#.ccc.#.",
        "p#.c#c.#p",
        ".#.ccc.#.",
        "p#.....#p",
        ".#######.",
        "..p.p.p..",
    ];
    const FIRMWARE_MOTION: &[Motion] = &[Motion::Chase { group: b'p', period: 90 }, Motion::Chase { group: b'c', period: -90 }];
    // A d-pad pressed up, right, down, left.
    const CONTROLS: &[&str] = &[
        "...uuu...",
        "...u.u...",
        "...u.u...",
        "lll#.#rrr",
        "l.......r",
        "lll#.#rrr",
        "...d.d...",
        "...d.d...",
        "...ddd...",
    ];
    const CONTROLS_MOTION: &[Motion] = &[Motion::Press { groups: b"urdl", period: 30 }];
    alloc::vec![
        (Level::Memory, St::OptionFace, PixelIcon::new(d, Side::Left, TINT_CARD, FILES, FILES_MOTION)),
        (Level::Memory, St::ClockFace, PixelIcon::new(d, Side::Right, TINT_CARD, DRIVES, DRIVES_MOTION)),
        (Level::Game, St::OptionFace, PixelIcon::new(d, Side::Left, TINT_GAME, BOOT, BOOT_MOTION)),
        (Level::Game, St::ClockFace, PixelIcon::new(d, Side::Right, TINT_GAME, APPS, APPS_MOTION)),
        (Level::Settings, St::GameFace, PixelIcon::new(d, Side::Up, TINT_OPTION, BOOT, BOOT_MOTION)),
        (Level::Settings, St::ClockFace, PixelIcon::new(d, Side::Right, TINT_OPTION, FIRMWARE, FIRMWARE_MOTION)),
        (Level::Settings, St::CardFace, PixelIcon::new(d, Side::Down, TINT_OPTION, CONTROLS, CONTROLS_MOTION)),
    ]
}
