//! The IPL main menu, rebuilt from the decompiled BS2 menu (PAL 1.0): the
//! big glass cube with a GLH0 layout on five faces, the frame-exact turn,
//! zoom and idle wobble, the faders and the button guide. Every number is
//! a MenuWork default or a timing from the decomp's docs/menu.md.
//!
//! The four submenus:
//!
//! | face        | console                 | here                                   |
//! |-------------|-------------------------|----------------------------------------|
//! | Game Play   | disc status, START boots | the same over a [`DiscDrive`]; START on a ready disc opens the firmware boot list (BootNext) |
//! | Memory Card | browse, copy, move, erase, format | the same over [`MemCards`] (fake cards in RAM for now) |
//! | Options     | sound, screen, language | boot timeout, menu language            |
//! | Calendar    | set the clock           | set the firmware clock (SetTime)       |
//!
//! Firmware Setup, Restart and Power Off are in a system dialog (F10).

pub mod backend;
mod card;
mod game;
pub mod icons;
mod system;

use super::backdrop;
use super::fx::{self, screen, Mosaic, MosaicData};
use crate::bs2::{bs2main, camera};
use crate::bs2::mtx::{self, cos_s, sin_s, to_s16};
use crate::sdk::mtx::{self as psmtx, Mtx};
use super::render::{self, View};
use super::shader::FastTex;
use crate::sdk::gx::{Gx, GX_COLOR0A0, GX_PNMTX0};
use super::gmr::{self, Instance};
use super::ui::{self, cp1252, Rgba, Ui, UiFont, SHADOW, WHITE};
use super::Assets;
use alloc::boxed::Box;
use backend::{DiscDrive, DiscState, MemCards, SlotState};
use crate::ipl::glh::{self, tag, Layout, Strings};
use crate::ipl::{bti, Ipl};
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;

// ---------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------

/// Per-language resource ids, in SRAM language order: En, De, Fr, Es, It, Nl.
mod res {
    pub const FRONT: [u16; 6] = [57, 149, 142, 163, 156, 132];
    pub const GAME_FACE: [u16; 6] = [56, 148, 141, 162, 155, 131];
    pub const CARD_FACE: [u16; 6] = [59, 151, 144, 165, 158, 134];
    pub const CLOCK_FACE: [u16; 6] = [136, 146, 139, 160, 153, 129];
    pub const OPTION_FACE: [u16; 6] = [138, 152, 145, 166, 159, 135];
    pub const GUIDE_STR: [u16; 6] = [117, 119, 118, 121, 120, 116];
    pub const GAME_STR: [u16; 6] = [23, 25, 24, 27, 26, 22];
    pub const CARD_STR: [u16; 6] = [30, 32, 31, 34, 33, 29];
    pub const DIALOG_STR: [u16; 6] = [110, 112, 111, 114, 113, 109];
    pub const CLOCK_STR: [u16; 6] = [16, 18, 17, 20, 19, 15];
    pub const OPTION_STR: [u16; 6] = [124, 126, 125, 128, 127, 123];
    pub const WEEKDAY: [u16; 6] = [49, 71, 62, 171, 80, 40];
    pub const CLOCK: u16 = 14;
    pub const GAME: u16 = 21;
    pub const CARD: u16 = 28;
    pub const CARD_BAR: u16 = 90;
    pub const CARD_EMPTY: u16 = 87;
    pub const CARD_FILE: u16 = 88;
    pub const BACKDROP: u16 = 89;
    pub const DIGITS: u16 = 91;
    pub const DIALOG: u16 = 108;
    pub const GUIDE: u16 = 115;
    pub const OPTION: u16 = 122;
    pub const TEX_BANK: u16 = 167;
}

pub const LANGS: usize = 6;

pub struct MenuAssets {
    pub bank: Vec<FastTex>,
    pub front: Vec<Layout>,
    pub game_face: Vec<Layout>,
    pub card_face: Vec<Layout>,
    pub clock_face: Vec<Layout>,
    pub option_face: Vec<Layout>,
    pub guide: Layout,
    pub guide_str: Vec<Strings>,
    pub clock_str: Vec<Strings>,
    pub option_str: Vec<Strings>,
    pub game_str: Vec<Strings>,
    pub card_str: Vec<Strings>,
    pub dialog_str: Vec<Strings>,
    pub card: Layout,
    pub card_bar: FastTex,
    pub card_models: card::Models,
    pub digits: Vec<FastTex>,
    pub weekday: Vec<Vec<FastTex>>,
    pub clock: Layout,
    pub game: Layout,
    pub option: Layout,
    pub dialog: Layout,
    pub backdrop: FastTex,
    pub font: UiFont,
    pub fx: MosaicData,
    pub icons: icons::IconData,
}

impl MenuAssets {
    pub fn load(ipl: &Ipl) -> Option<MenuAssets> {
        let layout = |id: u16| Layout::parse(&ipl.resource(id).ok()?).ok();
        let strings = |id: u16| Strings::parse(&ipl.resource(id).ok()?).ok();
        let tex = |id: u16| Some(FastTex::new_ui(&bti::parse(&ipl.resource(id).ok()?).ok()?));
        let model = |id: u16| Some(render::Prepared::new(crate::ipl::j3d::Model::parse(&ipl.resource(id).ok()?).ok()?));
        let per_lang = |ids: &[u16; 6]| ids.iter().map(|&i| layout(i)).collect::<Option<Vec<_>>>();
        let per_lang_s = |ids: &[u16; 6]| ids.iter().map(|&i| strings(i)).collect::<Option<Vec<_>>>();
        let bank = glh::texture_bank(&ipl.resource(res::TEX_BANK).ok()?).ok()?.iter().map(FastTex::new_ui).collect();
        let mut backdrop = bti::parse(&ipl.resource(res::BACKDROP).ok()?).ok()?;
        backdrop.wrap_s = crate::ipl::gx::REPEAT;
        backdrop.wrap_t = crate::ipl::gx::REPEAT;
        // Intensity texture: a white grid with coverage alpha, tiled.
        let backdrop = FastTex::new(&backdrop).intensity_alpha();
        Some(MenuAssets {
            bank,
            front: per_lang(&res::FRONT)?,
            game_face: per_lang(&res::GAME_FACE)?,
            card_face: per_lang(&res::CARD_FACE)?,
            clock_face: per_lang(&res::CLOCK_FACE)?,
            option_face: per_lang(&res::OPTION_FACE)?,
            guide: layout(res::GUIDE)?,
            guide_str: per_lang_s(&res::GUIDE_STR)?,
            clock_str: per_lang_s(&res::CLOCK_STR)?,
            option_str: per_lang_s(&res::OPTION_STR)?,
            game_str: per_lang_s(&res::GAME_STR)?,
            card_str: per_lang_s(&res::CARD_STR)?,
            dialog_str: per_lang_s(&res::DIALOG_STR)?,
            card: layout(res::CARD)?,
            card_bar: tex(res::CARD_BAR)?,
            card_models: card::Models::new(model(res::CARD_FILE)?, model(res::CARD_EMPTY)?),
            digits: (0..10).map(|i| tex(res::DIGITS + i)).collect::<Option<Vec<_>>>()?,
            weekday: res::WEEKDAY.iter().map(|&b| (0..7).map(|i| tex(b + i)).collect::<Option<Vec<_>>>()).collect::<Option<Vec<_>>>()?,
            clock: layout(res::CLOCK)?,
            game: layout(res::GAME)?,
            option: layout(res::OPTION)?,
            dialog: layout(res::DIALOG)?,
            backdrop,
            font: UiFont::new(glh::Font::parse_rom(ipl.rom()).ok()?),
            fx: MosaicData::load(ipl)?,
            icons: icons::IconData::load(ipl)?,
        })
    }
}

// ---------------------------------------------------------------------
// Faders
// ---------------------------------------------------------------------

pub use super::fader::{Fade, Fader};

// ---------------------------------------------------------------------
// Cube motion
// ---------------------------------------------------------------------

const FACE_HALF: f32 = 144.0;
const FACE_SHOW: i16 = 0x2E3A;
const FACE_FADE: i32 = 5000;
const MAX_TIMEOUT: u16 = 30;

/// `MenuWork` (0x1B8 bytes, `gMenuWork`): the cube's motion and the
/// menu's tunables. The mosaic's text layout and colours (`fx*`,
/// `gameColor`, ...) live with the mosaic in `fx.rs`.
#[derive(Clone)]
pub struct Work {
    pub zoom_depth: f32,
    pub wobble_frame: i16,
    pub rot_x: i16,
    pub rot_y: i16,
    pub rot_step: i16,
    pub zoom: i16,
    pub wobble_speed: [i16; 3],
    pub wobble_amp: [i16; 3],
    pub wobble_speed_tx: i16,
    pub wobble_speed_ty: i16,
    pub wobble_amp_tx: i16,
    pub wobble_amp_ty: i16,
    pub wobble_phase_ry: i16,
    pub wobble_phase_tx: i16,
    /// Debug: accept input while the cube moves.
    pub input_always: bool,
    pub zoom_in_step: i16,
    pub zoom_out_step: i16,
    pub bg_min_alpha: i16,
    pub zoom_spin: f32,
    pub light_attn: f32,
    pub light_attn_k: f32,
    pub light_pos: [f32; 3],
    pub light_cutoff: f32,
    pub guide_color_a: [i16; 3],
    pub guide_color_b: [i16; 3],
    pub sub_fade_delay: [i16; 3],
}

impl Work {
    /// `Menu_Init(TRUE)`: Menu_SetDefaults, then Menu_ResetCube.
    pub fn new() -> Work {
        let mut w = Work {
            zoom_depth: 0.0,
            wobble_frame: 0,
            rot_x: 0,
            rot_y: 0,
            rot_step: 0,
            zoom: 0,
            wobble_speed: [0; 3],
            wobble_amp: [0; 3],
            wobble_speed_tx: 0,
            wobble_speed_ty: 0,
            wobble_amp_tx: 0,
            wobble_amp_ty: 0,
            wobble_phase_ry: 0,
            wobble_phase_tx: 0,
            input_always: false,
            zoom_in_step: 0,
            zoom_out_step: 0,
            bg_min_alpha: 0,
            zoom_spin: 0.0,
            light_attn: 0.0,
            light_attn_k: 0.0,
            light_pos: [0.0; 3],
            light_cutoff: 0.0,
            guide_color_a: [0; 3],
            guide_color_b: [0; 3],
            sub_fade_delay: [0; 3],
        };
        w.set_defaults();
        w.reset_cube();
        w
    }

    /// Menu_SetDefaults (0x813113EC's caller), less the mosaic's fields.
    fn set_defaults(&mut self) {
        self.rot_step = 750;
        self.zoom_depth = -70.0;
        self.zoom_in_step = 0x730;
        self.zoom_out_step = 0x578;
        self.bg_min_alpha = 0;
        self.zoom_spin = 0.0;
        self.wobble_speed = [50, 25, 25];
        self.wobble_amp = [350, 1000, 1000];
        self.wobble_speed_tx = 25;
        self.wobble_speed_ty = 50;
        self.wobble_amp_tx = 15;
        self.wobble_amp_ty = 6;
        self.wobble_phase_ry = -15000;
        self.wobble_phase_tx = -0x4000;
        self.input_always = false;
        self.light_attn = 500.0;
        self.light_attn_k = 0.375;
        self.light_pos = [296.0, 224.0, 474.0];
        self.light_cutoff = 36.0;
        self.guide_color_a = [0, 200, 140];
        self.guide_color_b = [210, 0, 70];
        self.sub_fade_delay = [0, 20, 70];
    }

    /// Menu_ResetCube (0x81311850).
    fn reset_cube(&mut self) {
        self.wobble_frame = 0;
        self.rot_x = 0;
        self.rot_y = 0;
        self.zoom = 0;
    }

    /// The idle wobble of Menu_CalcCubeMtx at wobble frame `t`.
    pub fn wobble_mtx(&self, t: i16) -> Mtx {
        let t = t as i32;
        let ty = self.wobble_amp_ty as f32 * sin_s(self.wobble_speed_ty as i32 * t);
        let tx = self.wobble_amp_tx as f32 * sin_s(self.wobble_phase_tx as i32 + self.wobble_speed_tx as i32 * t);
        let rz = to_s16(self.wobble_amp[2] as f32 * cos_s(self.wobble_speed[2] as i32 * t));
        let ry = to_s16(self.wobble_amp[1] as f32 * cos_s(self.wobble_phase_ry as i32 + self.wobble_speed[1] as i32 * t));
        let rx = to_s16(self.wobble_amp[0] as f32 * cos_s(self.wobble_speed[0] as i32 * t));
        mtx::rot_trans(rx as i32, ry as i32, rz as i32, [tx, ty, 0.0])
    }

    /// `AT_FRONT()`: the cube sits exactly on the front face.
    fn at_front(&self) -> bool {
        (self.rot_x == 0 && self.rot_y == 0) || self.input_always
    }

    /// `AT_FACE_X(a)`.
    fn at_face_x(&self, a: i16) -> bool {
        (self.rot_x == a && self.zoom == 0) || self.input_always
    }

    /// `AT_FACE_Y(a)`.
    fn at_face_y(&self, a: i16) -> bool {
        (self.rot_y == a && self.zoom == 0) || self.input_always
    }

    /// Inside a submenu: zoomed all the way in (or input forced).
    fn zoomed_in(&self) -> bool {
        self.zoom == 0x7FFF || self.input_always
    }
}

/// `ROT_TOWARD_ZERO`: move an angle one step towards 0, snapping it onto
/// the step grid first.
fn toward_zero(a: &mut i16, step: i16) {
    if *a > 0 {
        *a = step.wrapping_mul(*a / step).wrapping_sub(step);
    }
    if *a < 0 {
        *a = step.wrapping_add(step.wrapping_mul(*a / step));
    }
}

// ---------------------------------------------------------------------
// State
// ---------------------------------------------------------------------

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum St {
    Top,
    GameFace,
    Game,
    CardFace,
    Card,
    OptionFace,
    Option,
    ClockFace,
    Clock,
}

impl St {
    fn on_cube(self) -> bool {
        matches!(self, St::Top | St::GameFace | St::CardFace | St::OptionFace | St::ClockFace)
    }
}

/// New presses this frame (directions include key repeat).
#[derive(Clone, Copy, Default)]
pub struct Pad {
    pub a: bool,
    pub b: bool,
    pub start: bool,
    pub up: bool,
    pub down: bool,
    pub left: bool,
    pub right: bool,
    /// GameCoob's own keys: the system dialog, the fake drive's tray and
    /// the fake card slots.
    pub system: bool,
    pub eject: bool,
    pub slot: [bool; 2],
}

/// What the menu shows about this PC; the driver refreshes it.
#[derive(Clone, Default)]
pub struct SysInfo {
    /// year, month 1..12, day, hour, minute, second
    pub time: [i32; 6],
    /// (Boot#### number, description) in BootOrder order.
    pub entries: Vec<(u16, String)>,
    pub timeout: u16,
    pub vendor: String,
    pub fw_ui: bool,
}

/// Something only the firmware can do.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Action {
    Boot(u16),
    SetTimeout(u16),
    /// Set the firmware clock: year, month 1..12, day, hour, minute, second.
    SetTime([i32; 6]),
    FirmwareSetup,
    Restart,
    PowerOff,
}

pub struct Menu {
    /// `gMenu`.
    pub w: Work,
    /// `gMenuState`.
    pub state: St,
    /// `gMenuNextState`.
    next: St,
    /// `sCubeMtx`.
    cube: Mtx,
    guide_icon: [Fader; 6],
    guide_text: [Fader; 8],
    /// [Option, Card, Clock, Game][fx, backdrop, text]
    sub: [[Fader; 3]; 4],
    disc_banner: Fader,
    disc_none: Fader,
    disc_unknown: Fader,
    pub drive: Box<dyn DiscDrive>,
    pub cards: Box<dyn MemCards>,
    game: game::Game,
    card: card::Card,
    sys: system::System,
    slot_fill: [i32; 2],
    lang_guide: [Fader; LANGS],
    guide_alpha: i16,
    guide_blink: i16,
    guide_lang: usize,
    pub lang: usize,
    slot_glow: [i16; 2],
    // Submenus.
    boot_sel: usize,
    opt_row: usize,
    opt_edit: bool,
    opt_timeout: u16,
    opt_lang: usize,
    clock: ClockEdit,
    highlight: i16,
    highlight_up: bool,
    message: Option<(String, u32)>,
    frame: u32,
    pub info: SysInfo,
    fx: Mosaic,
    game_icon: icons::GameIcon,
    card_icon: icons::CardIcon,
    clock_icon: icons::ClockIcon,
    option_icon: icons::OptionIcon,
}

/// Calendar state (`sClockSub`, `sClockRow`, `sDateField`, `sTimeField`,
/// `gClockEdit`).
#[derive(Clone, Copy, Default)]
struct ClockEdit {
    /// 0 menu, 1 editing the date, 2 editing the time.
    sub: u8,
    /// The sub-state the mosaic showed last frame (it flashes on a change).
    shown_sub: u8,
    /// 0 date, 1 time.
    row: u8,
    /// 0 year, 1 month, 2 day.
    date_field: u8,
    /// 0 hour, 1 minute, 2 second.
    time_field: u8,
    /// Year, month 1..12, day; or hour, minute, second.
    value: [i32; 3],
}

impl ClockEdit {
    /// Clock_FixDate: the year wraps in 2000..2099 and the month in 1..12;
    /// the day wraps only while it is the field being edited, else it is
    /// clamped to the month.
    fn fix_date(&mut self) {
        let v = &mut self.value;
        if v[0] < 2000 {
            v[0] = 2099;
        }
        if v[0] > 2099 {
            v[0] = 2000;
        }
        if v[1] < 1 {
            v[1] = 12;
        }
        if v[1] > 12 {
            v[1] = 1;
        }
        let days = days_in_month(v[0], v[1]);
        if v[2] > days {
            v[2] = if self.date_field == 2 { 1 } else { days };
        }
        if v[2] < 1 {
            v[2] = days;
        }
    }

    /// Clock_FixTime: hours wrap 0..23, minutes and seconds 0..59.
    fn fix_time(&mut self) {
        let f = self.time_field as usize;
        let max = if f == 0 { 23 } else { 59 };
        if self.value[f] < 0 {
            self.value[f] = max;
        }
        if self.value[f] > max {
            self.value[f] = 0;
        }
    }

    /// The time the mosaic shows: `now`, with the field group being edited
    /// taken from the edit buffer.
    fn shown(&self, now: [i32; 6]) -> [i32; 6] {
        self.shown_with(self.sub, now)
    }

    fn shown_with(&self, sub: u8, now: [i32; 6]) -> [i32; 6] {
        let mut t = now;
        match sub {
            1 => t[..3].copy_from_slice(&self.value),
            2 => t[3..].copy_from_slice(&self.value),
            _ => {}
        }
        t
    }
}

fn days_in_month(y: i32, m: i32) -> i32 {
    let leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    match m {
        2 if leap => 29,
        2 => 28,
        4 | 6 | 9 | 11 => 30,
        _ => 31,
    }
}

const SUB_OPTION: usize = 0;
const SUB_CARD: usize = 1;
const SUB_CLOCK: usize = 2;
const SUB_GAME: usize = 3;

impl Menu {
    /// The rest of Menu_Init (Menu_InitRes and the icons), on the
    /// `MenuWork` the scene made at boot and the spin-out has been
    /// wobbling since.
    pub fn new(info: SysInfo, ma: &MenuAssets, drive: Box<dyn DiscDrive>, cards: Box<dyn MemCards>, w: Work) -> Menu {
        let card = card::Card::new(ma, &*cards);
        let delay = w.sub_fade_delay;
        let mut m = Menu {
            w,
            state: St::Top,
            next: St::Top,
            cube: psmtx::IDENTITY,
            guide_icon: [Fader::default(); 6],
            guide_text: [Fader::default(); 8],
            sub: [[0, 1, 2].map(|i| Fader::with_delay(delay[i])); 4],
            disc_banner: Fader::default(),
            disc_none: Fader::default(),
            disc_unknown: Fader::default(),
            drive,
            cards,
            game: game::Game::new(),
            card,
            sys: system::System::new(info.fw_ui),
            slot_fill: [0; 2],
            lang_guide: [Fader::default(); LANGS],
            guide_alpha: 0,
            guide_blink: 0,
            guide_lang: 0,
            lang: 0,
            slot_glow: [0; 2],
            boot_sel: 0,
            opt_row: 0,
            opt_edit: false,
            opt_timeout: 0,
            opt_lang: 0,
            clock: ClockEdit::default(),
            highlight: 50,
            highlight_up: true,
            message: None,
            frame: 0,
            info,
            fx: Mosaic::new(),
            game_icon: icons::GameIcon::new(&ma.icons),
            card_icon: icons::CardIcon::new(&ma.icons),
            clock_icon: icons::ClockIcon::new(&ma.icons),
            option_icon: icons::OptionIcon::new(&ma.icons),
        };
        m.calc_cube_mtx();
        m
    }

    /// Show a short note in the current submenu (e.g. a firmware error).
    pub fn notify(&mut self, msg: String) {
        self.message = Some((msg, self.frame));
    }

    /// Menu_HandleInput (0x81311868): turning between faces and entering
    /// submenus. Sound_Play calls are left out (no audio yet).
    fn handle_input(&mut self, p: &Pad) {
        use St::*;
        let w = &self.w;
        if p.up {
            match self.state {
                Top if w.at_front() => self.next = GameFace,
                CardFace if w.at_face_x(-0x4000) => self.next = Top,
                _ => {}
            }
        }
        if p.down {
            match self.state {
                Top if w.at_front() => self.next = CardFace,
                GameFace if w.at_face_x(0x4000) => self.next = Top,
                _ => {}
            }
        }
        if p.left {
            match self.state {
                Top if w.at_front() => self.next = OptionFace,
                ClockFace if w.at_face_y(-0x4000) => self.next = Top,
                _ => {}
            }
        }
        if p.right {
            match self.state {
                Top if w.at_front() => self.next = ClockFace,
                OptionFace if w.at_face_y(0x4000) => self.next = Top,
                _ => {}
            }
        }
        if p.a {
            match self.state {
                CardFace if w.at_face_x(-0x4000) => self.next = Card,
                GameFace if w.at_face_x(0x4000) => self.next = Game,
                ClockFace if w.at_face_y(-0x4000) => self.next = Clock,
                OptionFace if w.at_face_y(0x4000) => self.next = Option,
                _ => {}
            }
        }
        if p.b {
            match self.state {
                CardFace if w.at_face_x(-0x4000) => self.next = Top,
                GameFace if w.at_face_x(0x4000) => self.next = Top,
                ClockFace if w.at_face_y(-0x4000) => self.next = Top,
                OptionFace if w.at_face_y(0x4000) => self.next = Top,
                _ => {}
            }
        }
    }

    /// Menu_CalcCubeMtx (0x81311D50): rotation, idle wobble, then the zoom
    /// push. The big cube's model matrix (scale 1.1) is set where it is
    /// drawn.
    fn calc_cube_mtx(&mut self) {
        let w = &self.w;
        let rot = mtx::rot_trans(w.rot_x as i32, w.rot_y as i32, 0, [0.0; 3]);
        let mut m = psmtx::concat(&rot, &psmtx::IDENTITY);
        m = psmtx::concat(&w.wobble_mtx(w.wobble_frame), &m);
        self.cube = m;
        // Zooming into a submenu pushes the cube back by
        // zoomDepth * (1 - cos(zoom)).
        if w.zoom != 0 {
            let z = w.zoom_depth * (1.0 - cos_s(w.zoom as i32));
            let m = mtx::rot_trans(0, 0, to_s16(w.zoom as f32 * w.zoom_spin) as i32, [0.0, 0.0, z]);
            self.cube = psmtx::concat(&m, &self.cube);
        }
    }

    /// Menu_UpdateDiscFaders (0x81312044): banner, "no disc" or "unknown
    /// disc" on the Game Play face.
    fn update_disc_faders(&mut self) {
        if self.drive.banner().is_some() {
            self.disc_banner.step_if(true);
            self.disc_none.step_if(false);
            self.disc_unknown.step_if(false);
            return;
        }
        self.disc_banner.step_if(false);
        if self.drive.state() == DiscState::NoDisc {
            self.disc_none.step_if(true);
            self.disc_unknown.step_if(false);
            return;
        }
        self.disc_none.step_if(false);
        self.disc_unknown.step_if(true);
    }

    /// Menu_UpdateRotation (0x813120F0): turn the cube towards the face of
    /// `gMenuNextState`.
    fn update_rotation(&mut self) {
        let w = &mut self.w;
        let step = w.rot_step;
        match self.next {
            St::CardFace | St::Card => {
                w.rot_x = w.rot_x.wrapping_sub(step);
                toward_zero(&mut w.rot_y, step);
                if w.rot_x < -0x4000 {
                    w.rot_x = -0x4000;
                }
            }
            St::GameFace | St::Game => {
                w.rot_x = w.rot_x.wrapping_add(step);
                toward_zero(&mut w.rot_y, step);
                if w.rot_x > 0x4000 {
                    w.rot_x = 0x4000;
                }
            }
            St::ClockFace | St::Clock => {
                toward_zero(&mut w.rot_x, step);
                w.rot_y = w.rot_y.wrapping_sub(step);
                if w.rot_y < -0x4000 {
                    w.rot_y = -0x4000;
                }
            }
            St::OptionFace | St::Option => {
                toward_zero(&mut w.rot_x, step);
                w.rot_y = w.rot_y.wrapping_add(step);
                if w.rot_y > 0x4000 {
                    w.rot_y = 0x4000;
                }
            }
            St::Top => {
                toward_zero(&mut w.rot_x, step);
                toward_zero(&mut w.rot_y, step);
            }
        }
    }

    /// Menu_UpdateIcons (0x813123B0): face, enter or hide for each
    /// submenu's icon cube, then their matrices. It runs before
    /// Menu_AnimCube, so the icons sit in last frame's cube matrix.
    fn update_icons(&mut self, d: &icons::IconData, view: &Mtx) {
        let phase = |face: St, sub: St| match self.state {
            st if st == face => icons::Phase::Face,
            st if st == sub => icons::Phase::Enter,
            _ => icons::Phase::Hide,
        };
        let p = phase(St::CardFace, St::Card);
        self.card_icon.update(p);
        let p = phase(St::GameFace, St::Game);
        self.game_icon.update(p);
        let p = phase(St::OptionFace, St::Option);
        self.option_icon.update(p, d);
        let p = phase(St::ClockFace, St::Clock);
        self.clock_icon.update(p, &self.info.time);
        self.card_icon.calc_mtx(d, &self.cube, view);
        self.game_icon.calc_mtx(d, &self.cube, view);
        self.option_icon.calc_mtx(d, &self.cube, view);
        self.clock_icon.calc_mtx(d, &self.cube, view);
    }

    /// Menu_UpdateSlotGlow (0x8131247C): a slot's picture on the card face
    /// glows while a card is ready.
    fn update_slot_glow(&mut self) {
        for i in 0..2 {
            let d = if self.cards.state(i) == SlotState::Ready {
                let user = self.cards.total_blocks(i).max(1) as i32;
                self.slot_fill[i] = 0xFF - self.cards.free_blocks(i) as i32 * 0xFF / user;
                4
            } else {
                -4
            };
            self.slot_glow[i] += d;
            if self.slot_glow[i] > 0xFF {
                self.slot_glow[i] = 0xFF;
            }
            if self.slot_glow[i] < 60 {
                self.slot_glow[i] = 60;
            }
        }
    }

    /// Menu_UpdateGuide (0x81312580): which labels and glyph groups show.
    /// Faders a state doesn't mention keep their direction (`None`).
    fn update_guide(&mut self) {
        // Text faders: 0 Menu Selection, 1 Cancel, 2 Confirm, 3 Select,
        // 4 Change, 5 Cancel, 6 Finish, 7 Confirm.
        let (o, i) = (Some(false), Some(true));
        let text: [Option<bool>; 8] = match self.state {
            St::Top => [i, o, o, o, o, o, o, o],
            St::GameFace | St::CardFace | St::OptionFace | St::ClockFace => [o, i, i, o, o, o, o, o],
            // While the boot list is up (GameCoob's addition) it reads
            // like a list: Select, Cancel, Confirm.
            St::Game if self.game.starting() => [o, o, o, i, o, i, o, i],
            St::Game => [o, o, o, o, o, Some(!self.game.fatal()), o, o],
            St::Option | St::Clock => {
                // Option_IsTop, Clock_IsTop.
                let top = if self.state == St::Option { !self.opt_edit } else { self.clock.sub == 0 };
                [o, o, o, Some(top), Some(!top), Some(!top), Some(top), i]
            }
            St::Card => {
                let browsing = self.card.browsing();
                [o, o, o, i, None, Some(!browsing), Some(browsing), i]
            }
        };
        // Glyph groups: 0..2 inside submenus, 3 the stick on the front
        // face, 4..5 B and A on a selected face.
        let icons: [bool; 6] = match self.state {
            St::Top => [false, false, false, true, false, false],
            St::Game if self.game.starting() => [true, true, true, false, false, false],
            St::Game => [false, !self.game.fatal(), false, false, false, false],
            St::GameFace | St::CardFace | St::OptionFace | St::ClockFace => [false, false, false, false, true, true],
            St::Card | St::Option | St::Clock => [true, true, true, false, false, false],
        };
        for (f, on) in self.guide_text.iter_mut().zip(text) {
            if let Some(on) = on {
                f.step_if(on);
            }
        }
        for (f, on) in self.guide_icon.iter_mut().zip(icons) {
            f.step_if(on);
        }
        for i in 0..LANGS {
            self.lang_guide[i].step_if(i == self.guide_lang);
        }
    }

    /// Menu_UpdateSubFaders (0x81312A94).
    fn update_sub_faders(&mut self) {
        for i in 0..3 {
            self.sub[SUB_CARD][i].step_if(self.state == St::Card);
            self.sub[SUB_GAME][i].step_if(self.state == St::Game);
            self.sub[SUB_OPTION][i].step_if(self.state == St::Option);
            self.sub[SUB_CLOCK][i].step_if(self.state == St::Clock);
        }
    }

    /// Menu_UpdateCube (0x81312B9C): zoom back out of a submenu, then the
    /// face input.
    fn update_cube(&mut self, p: &Pad) {
        let step = self.w.zoom_out_step;
        if self.w.zoom > step {
            self.w.zoom -= step;
        }
        if self.w.zoom <= step {
            self.w.zoom = 0;
        }
        self.handle_input(p);
    }

    /// Menu_ZoomIn.
    fn zoom_in(&mut self) {
        let step = self.w.zoom_in_step;
        if self.w.zoom < 0x7FFF - step {
            self.w.zoom += step;
        }
        if self.w.zoom >= 0x7FFF - step {
            self.w.zoom = 0x7FFF;
        }
    }

    /// Menu_AnimCube: the wobble clock and the cube matrix (the big
    /// cube's joints are calculated where it is drawn).
    fn anim_cube(&mut self) {
        self.w.wobble_frame = self.w.wobble_frame.wrapping_add(7);
        self.calc_cube_mtx();
    }

    /// One menu frame: Menu_Update (Scene state 5), plus GameCoob's own
    /// keys and dialog first. `view` is the camera's (`Camera_GetView`).
    /// Returns an action for the firmware.
    pub fn update(&mut self, ma: &MenuAssets, view: &Mtx, pad: &Pad) -> core::option::Option<Action> {
        // GameCoob: the fake drive's tray, the fake card slots and the
        // system dialog, which takes the input while it is up. The card
        // and drive backends stand in for the card thread and the DVD
        // state machine, which run beside Menu_Update on the console.
        self.frame += 1;
        if pad.eject {
            self.drive.toggle_tray();
        }
        for k in 0..2 {
            if pad.slot[k] {
                self.cards.cycle(k);
            }
        }
        if pad.system {
            self.sys.toggle();
        }
        let none = Pad::default();
        let sys_action = if self.sys.visible() { self.sys.update(pad) } else { None };
        let p = if self.sys.visible() { &none } else { pad };
        self.drive.update();
        self.cards.update();

        // The SRAM-lost flag and the fatal disc error (SCENE_FATAL_ERROR,
        // sErrorMsg) don't happen with the dummy backends. disp->menuFlag
        // is cleared by the scene (Intro::update).
        self.update_rotation();
        self.update_icons(&ma.icons, view);
        self.update_slot_glow();
        self.update_guide();
        self.update_sub_faders();

        // One-shot actions on state changes.
        if self.state == St::CardFace && self.next == St::Card {
            // Card_Enter, CardUi_Reset, CardIcon_ResetFiles.
            self.card.enter();
        }
        if self.state == St::GameFace && self.next == St::Game {
            self.game.enter();
        }
        if self.state == St::Game && self.next == St::GameFace {
            self.disc_banner.reset();
            self.disc_none.reset();
            self.disc_unknown.reset();
        }
        // GameCoob: Options opens on its top row; a note belongs to the
        // submenu it was shown in.
        if self.state == St::OptionFace && self.next == St::Option {
            self.opt_row = 0;
            self.opt_edit = false;
        }
        if self.state != self.next {
            self.message = None;
        }

        self.state = self.next;

        let mut action = None;
        if self.state.on_cube() {
            self.update_cube(p);
            self.anim_cube();
            self.update_disc_faders();
            self.fx.clear();
        } else {
            // Inside a submenu the cube zooms away and the submenu decides
            // the next state.
            self.zoom_in();
            self.anim_cube();
            let (next, act) = self.update_sub(p);
            self.next = next;
            action = act;
        }

        // GameCoob: the highlight pulse 50..120 at 2 per frame.
        self.highlight += if self.highlight_up { 2 } else { -2 };
        if self.highlight >= 120 {
            self.highlight_up = false;
        } else if self.highlight <= 50 {
            self.highlight_up = true;
        }
        sys_action.or(action)
    }

    /// The submenus' input; returns the next menu state and any action.
    fn update_sub(&mut self, p: &Pad) -> (St, core::option::Option<Action>) {
        if self.state == St::Game {
            // Game Play keeps its drive state and faders running.
            let ready = self.w.zoomed_in();
            return self.game.update(p, &*self.drive, &self.info, ready);
        }
        if self.state == St::Card {
            let fade_done = self.sub[SUB_CARD].iter().all(|f| f.alpha() > 172);
            let next = self.card.update(p, &mut *self.cards, self.w.zoomed_in(), fade_done);
            return (next, None);
        }
        // Input waits until the zoom is complete, and in Options and
        // Calendar until the mosaic has assembled (Fx_IsReady(0)).
        if !self.w.zoomed_in() || (matches!(self.state, St::Option | St::Clock) && !self.fx.ready(0)) {
            return (self.state, None);
        }
        match self.state {
            St::Option => {
                if !self.opt_edit {
                    if p.b {
                        return (St::OptionFace, None);
                    }
                    if p.up && self.opt_row > 0 {
                        self.opt_row -= 1;
                    }
                    if p.down && self.opt_row < 1 {
                        self.opt_row += 1;
                    }
                    if p.a {
                        self.opt_edit = true;
                        self.opt_timeout = self.info.timeout;
                        self.opt_lang = self.lang;
                    }
                    return (St::Option, None);
                }
                match self.opt_row {
                    0 => {
                        if p.left {
                            self.opt_timeout = self.opt_timeout.saturating_sub(1);
                        }
                        if p.right {
                            self.opt_timeout = (self.opt_timeout + 1).min(MAX_TIMEOUT);
                        }
                        if p.b {
                            self.opt_edit = false;
                        }
                        if p.a {
                            self.opt_edit = false;
                            if self.opt_timeout != self.info.timeout {
                                self.info.timeout = self.opt_timeout;
                                self.notify(format!("Firmware boot timeout: {} s", self.opt_timeout));
                                return (St::Option, Some(Action::SetTimeout(self.opt_timeout)));
                            }
                        }
                    }
                    _ => {
                        if p.left && self.opt_lang > 0 {
                            self.opt_lang -= 1;
                        }
                        if p.right && self.opt_lang + 1 < LANGS {
                            self.opt_lang += 1;
                        }
                        self.guide_lang = self.opt_lang;
                        if p.b {
                            self.opt_edit = false;
                            self.guide_lang = self.lang;
                        }
                        if p.a {
                            self.opt_edit = false;
                            self.lang = self.opt_lang;
                        }
                    }
                }
                (St::Option, None)
            }
            St::Clock => self.update_clock(p),
            _ => (self.state, None),
        }
    }

    /// Clock_Update: Up/Down pick the date or the time row, A edits it.
    /// While editing, Left/Right move between the fields, Up/Down change
    /// the value, A writes the firmware clock and B cancels.
    fn update_clock(&mut self, p: &Pad) -> (St, core::option::Option<Action>) {
        let c = &mut self.clock;
        // Left/Right only count without Up/Down in the same frame.
        let side = !(p.up || p.down);
        match c.sub {
            0 => {
                if p.up {
                    c.row = 0;
                }
                if p.down {
                    c.row = 1;
                }
                if p.a {
                    let t = self.info.time;
                    if c.row == 0 {
                        c.value = [t[0], t[1], t[2]];
                        c.sub = 1;
                    } else {
                        c.value = [t[3], t[4], 0];
                        c.sub = 2;
                    }
                    return (St::Clock, None);
                }
                if p.b {
                    return (St::ClockFace, None);
                }
            }
            sub => {
                let date = sub == 1;
                let field = if date { &mut c.date_field } else { &mut c.time_field };
                // On PAL the date reads day / month / year, so Left moves
                // toward the year (Clock_FieldStep); the time always reads
                // hour, minute, second.
                let step: i8 = if date { 1 } else { -1 };
                if p.left && side {
                    *field = (*field as i8 + step).rem_euclid(3) as u8;
                }
                if p.right && side {
                    *field = (*field as i8 - step).rem_euclid(3) as u8;
                }
                let f = *field as usize;
                let d = p.up as i32 - p.down as i32;
                if d != 0 {
                    c.value[f] += d;
                    if date {
                        c.fix_date();
                    } else {
                        c.fix_time();
                    }
                }
                if p.b {
                    c.sub = 0;
                }
                if p.a {
                    c.sub = 0;
                    let t = c.shown_with(sub, self.info.time);
                    self.info.time = t;
                    self.notify(format!("Firmware clock set: {:02}/{:02}/{} {:02}:{:02}:{:02}", t[2], t[1], t[0], t[3], t[4], t[5]));
                    return (St::Clock, Some(Action::SetTime(t)));
                }
            }
        }
        (St::Clock, None)
    }

    // -----------------------------------------------------------------
    // Drawing
    // -----------------------------------------------------------------

    /// Scene_Draw2DSpinOut's face: the front layout on the spinning cube.
    pub fn draw_spin_out_face<'a>(gx: &mut Gx<'a>, ma: &'a MenuAssets, view: &View, cube: &Mtx, alpha: u8) {
        let mut ui = Ui { gx, view, mv: psmtx::IDENTITY };
        draw_front(&mut ui, ma, 0, view, cube, alpha);
    }

    /// Scene_Draw2DMenu (0x8130E5C0) + Menu_Draw2D.
    pub fn draw<'a>(&'a self, gx: &mut Gx<'a>, a: &'a Assets, ma: &'a MenuAssets, view: &View) {
        // The glass cube fades out to bgMinAlpha as it zooms away.
        let w = &self.w;
        let bg = (w.bg_min_alpha as i32 + (255 - w.bg_min_alpha as i32) * (0x7FFF - w.zoom as i32) / 0x7FFF) as u8;
        // Menu_CalcCubeMtx's model and view, then Menu_AnimCube's joints.
        let mut cube = Instance::new(&a.big_cube.model);
        gmr::set_model_mtx(&mut cube, &self.cube, [1.1; 3]);
        gmr::set_view_mtx(&mut cube, &view.view);
        gmr::calc_joints(&a.big_cube.model, &mut cube);
        cube.alpha = bg;
        // The glass cube's back faces, the icon cubes inside it, then its
        // front faces, each glass pass between Scene_SetAmbDark and
        // Scene_SetAmbFull.
        let glass = |gx: &mut Gx<'a>, cube: &mut Instance, packet: u16| {
            if cube.alpha > 0 {
                gx.set_chan_amb_color(GX_COLOR0A0, [0x28, 0x28, 0x28, 0xFF]);
                gmr::draw_packet(gx, &a.big_cube, cube, packet);
                gx.set_chan_amb_color(GX_COLOR0A0, [0xFF; 4]);
            }
        };
        glass(gx, &mut cube, BIG_CUBE_BACK);
        self.card_icon.draw(gx, &ma.icons);
        self.game_icon.draw(gx, &ma.icons);
        self.option_icon.draw(gx, &ma.icons);
        self.clock_icon.draw(gx, &ma.icons);
        glass(gx, &mut cube, BIG_CUBE_FRONT);
        let mut ui = Ui { gx, view, mv: psmtx::IDENTITY };
        self.draw_faces(&mut ui, ma, view, bg);
        let mut v2 = View::intro(0.0, false);
        v2.vp = view.vp;
        let mut ui = Ui { gx: ui.gx, view: &v2, mv: screen_2d() };
        self.draw_submenus(&mut ui, ma);
        self.draw_guide(&mut ui, ma, 255);
        let pulse = (self.highlight - 50) as f32 / 70.0;
        self.sys.draw(&mut ui, ma, self.lang, pulse);
    }

    fn face_mtx(&self, view: &View, rot: (i32, i32), t: [f32; 3]) -> Mtx {
        face_mtx(view, &self.cube, rot, t)
    }

    /// Menu_DrawFaces.
    fn draw_faces<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, view: &View, alpha: u8) {
        let clampf = |r: i16| (r as i32).abs().min(FACE_FADE);
        let (ax, ay) = (clampf(self.w.rot_x), clampf(self.w.rot_y));
        let front = if ax != ay { Some(ax.max(ay)) } else if ax == 0 { Some(0) } else { None };
        if let Some(m) = front {
            // Menu_DrawTopFace ignores the menu alpha.
            let fa = (255 - m * 255 / FACE_FADE) as u8;
            draw_front(ui, ma, self.lang, view, &self.cube, fa);
        }
        let face_a = |r: i16| ((alpha as i32) * (255 - (0x4000 - (r as i32).abs()) * 255 / FACE_FADE) / 255).clamp(0, 255) as f32 / 255.0;
        let l = self.lang;
        let bank = &ma.bank[..];
        // Bottom: Memory Card.
        if self.w.rot_x <= -FACE_SHOW {
            let a = face_a(self.w.rot_x);
            ui.mv = self.face_mtx(view, (0x4000, 0), [-FACE_HALF, -FACE_HALF, FACE_HALF]);
            let g = &ma.card_face[l];
            let c = with_a(WHITE, a * a);
            for f in g.frame(tag(b"fram")) {
                ui.frame(f, bank, c);
            }
            pics(ui, g, b"titl", bank, c);
            for slot in 0..2u8 {
                self.draw_slot(ui, g, bank, slot, a);
            }
        }
        // Top: Game Play.
        if self.w.rot_x >= FACE_SHOW {
            let a = face_a(self.w.rot_x);
            ui.mv = self.face_mtx(view, (-0x4000, 0), [-FACE_HALF, FACE_HALF, -FACE_HALF]);
            self.draw_game_face(ui, ma, a);
        }
        // Right: Calendar.
        if self.w.rot_y <= -FACE_SHOW {
            let a = face_a(self.w.rot_y);
            ui.mv = self.face_mtx(view, (0, 0x4000), [FACE_HALF, FACE_HALF, FACE_HALF]);
            self.draw_clock_face(ui, ma, a);
        }
        // Left: Options.
        if self.w.rot_y >= FACE_SHOW {
            let a = face_a(self.w.rot_y);
            ui.mv = self.face_mtx(view, (0, -0x4000), [-FACE_HALF, FACE_HALF, -FACE_HALF]);
            self.draw_option_face(ui, ma, a);
        }
    }

    /// Menu_DrawSlot: label, glowing frame and a bottom-anchored fill bar.
    fn draw_slot<'a>(&'a self, ui: &mut Ui<'a, '_>, g: &Layout, bank: &'a [FastTex], slot: u8, a: f32) {
        pics(ui, g, &[b't', b'e', b'x', b'a' + slot], bank, with_a(WHITE, a));
        let glow = self.slot_glow[slot as usize] as i32;
        let c = with_a(WHITE, a * glow as f32 / 255.0);
        for f in g.frame(tag(&[b'm', b'c', b'a' + slot, b'1'])) {
            ui.frame(f, bank, c);
        }
        let fill = self.slot_fill[slot as usize] * (glow - 60) / 195;
        if fill <= 0 {
            return;
        }
        for f in g.frame(tag(&[b'm', b'c', b'a' + slot, b'2'])) {
            let h = fill as f32 * f.inner_h / 255.0;
            let (x, y) = (f.x - f.inner_w / 2.0, f.y - f.inner_h / 2.0 + f.inner_h - h);
            ui.frame_rect(f, bank, c, x, y, f.inner_w, h);
        }
    }

    /// Menu_DrawGameFace + Menu_DrawBannerInfo.
    fn draw_game_face<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, a: f32) {
        let g = &ma.game_face[self.lang];
        let bank = &ma.bank[..];
        let white = with_a(WHITE, a * a);
        let shadow = with_a(SHADOW, a);
        for f in g.frame(tag(b"fram")) {
            ui.frame(f, bank, white);
        }
        pics(ui, g, b"titl", bank, white);
        for (fd, z, w) in [(&self.disc_none, b"zodi", b"nodi"), (&self.disc_unknown, b"zust", b"qust")] {
            let k = fd.alpha() as f32 / 255.0;
            if k > 0.0 {
                pics(ui, g, z, bank, with_a(shadow, k));
                pics(ui, g, w, bank, with_a(white, k));
            }
        }
        let ba = self.disc_banner.alpha() as f32 / 255.0 * a;
        let Some(b) = self.drive.banner() else { return };
        if ba <= 0.0 {
            return;
        }
        for p in g.pic(tag(b"bana")) {
            ui.pic_tex(p, &b.image, with_a(WHITE, ba));
        }
        let name = fit(&ma.font, &cp1252(&b.short_name), 192.0, 18.0);
        let maker = fit(&ma.font, &cp1252(&b.short_maker), 192.0, 18.0);
        for (t, s, c) in [(b"zame", &name, SHADOW), (b"game", &name, WHITE), (b"zakr", &maker, SHADOW), (b"makr", &maker, WHITE)] {
            if let Some(p) = g.text(tag(t)) {
                ui.text(&ma.font, p, s, with_a(c, ba), 0.0, 0.0);
            }
        }
    }

    fn draw_clock_face<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, a: f32) {
        let g = &ma.clock_face[self.lang];
        let bank = &ma.bank[..];
        let white = with_a(WHITE, a * a);
        let shadow = with_a(SHADOW, a);
        for f in g.frame(tag(b"fram")) {
            ui.frame(f, bank, white);
        }
        let [y, mo, d, h, mi, sec] = self.info.time;
        let wd = weekday(y, mo, d);
        let dg = |n: i32| &ma.digits[(n.rem_euclid(10)) as usize];
        for pass in 0..2 {
            let c = if pass == 0 { shadow } else { white };
            let z = |t: &[u8; 4]| -> [u8; 4] { if pass == 0 { [b'z', t[1], t[2], t[3]] } else { *t } };
            for t in [b"colo", b"slas", b"pare", b"titl"] {
                pics(ui, g, &z(t), bank, c);
            }
            let digits: [(&[u8; 4], i32); 14] = [
                (b"cal1", 2),
                (b"cal2", y / 100),
                (b"cal3", y / 10),
                (b"cal4", y),
                (b"cal5", mo / 10),
                (b"cal6", mo),
                (b"cal7", d / 10),
                (b"cal8", d),
                (b"tim1", h / 10),
                (b"tim2", h),
                (b"tim3", mi / 10),
                (b"tim4", mi),
                (b"tim5", sec / 10),
                (b"tim6", sec),
            ];
            for (t, n) in digits {
                for p in g.pic(tag(&z(t))) {
                    ui.pic_tex(p, dg(n), c);
                }
            }
            for p in g.pic(tag(&z(b"week"))) {
                ui.pic_tex(p, &ma.weekday[self.lang][wd], c);
            }
        }
    }

    fn draw_option_face<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, a: f32) {
        let g = &ma.option_face[self.lang];
        let bank = &ma.bank[..];
        let white = with_a(WHITE, a);
        let shadow = with_a(SHADOW, a);
        for f in g.frame(tag(b"fram")) {
            ui.frame(f, bank, white);
        }
        pics(ui, g, b"zitl", bank, shadow);
        pics(ui, g, b"titl", bank, white);
        // Rows 1 and 2 hold our settings instead of sound and screen
        // position: label left, value right, in the ROM font.
        let rows = [
            (b"txt1", b"sond", cp1252("Timeout"), cp1252(&format!("{} s", self.info.timeout))),
            (b"txt2", b"num1", cp1252("Boot entries"), cp1252(&format!("{}", self.info.entries.len()))),
        ];
        for (lt, vt, label, value) in rows.iter() {
            if let (Some(lp), Some(vp)) = (g.pic(tag(lt)).next(), g.pic(tag(vt)).next()) {
                for (c, d) in [(shadow, 2.0), (white, 0.0)] {
                    ui.label(&ma.font, lp.x - lp.w / 2.0 + d, lp.y - 9.0 + d, 18.0, label, c, 2);
                    let vx = if *vt == b"num1" { vp.x + 8.0 } else { vp.x };
                    ui.label(&ma.font, vx + d, vp.y - 11.0 + d, 22.0, value, c, 0);
                }
            }
        }
        for t in [b"zxt3", b"zang"] {
            pics(ui, g, t, bank, shadow);
        }
        for t in [b"txt3", b"lang"] {
            pics(ui, g, t, bank, white);
        }
    }

    /// Menu_DrawSubmenus: each is drawn while its first fader is above 0.
    fn draw_submenus<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets) {
        for k in [SUB_CARD, SUB_GAME, SUB_OPTION, SUB_CLOCK] {
            let f = &self.sub[k];
            let al = [f[0].alpha(), f[1].alpha(), f[2].alpha()].map(|v| v as f32 / 255.0);
            if al[0] <= 0.0 {
                continue;
            }
            let fx_alpha = f[0].alpha() as u8;
            match k {
                SUB_GAME => self.draw_game(ui, ma, al),
                SUB_CARD => self.card.draw(ui, ma, &self.w, &*self.cards, self.lang, al),
                SUB_OPTION => {
                    draw_backdrop(ui, ma, &self.w, al[1]);
                    self.draw_options(ui, ma, al);
                }
                _ => {
                    draw_backdrop(ui, ma, &self.w, al[1]);
                    self.draw_clock(ui, ma, al);
                }
            }
            let set = match k {
                SUB_GAME => Some(fx::set::GAME),
                SUB_OPTION => Some(fx::set::OPTION),
                SUB_CLOCK => Some(fx::set::CLOCK),
                _ => None,
            };
            if set.is_some() && set == self.fx.screen() {
                self.fx.draw(ui.gx, &ma.fx, ui.view, fx_alpha);
            }
            self.draw_message(ui, ma, al[2]);
        }
    }

    fn draw_message<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, a: f32) {
        if let Some((msg, at)) = &self.message {
            let age = self.frame.saturating_sub(*at) as f32;
            let k = (1.0 - (age - 180.0) / 30.0).clamp(0.0, 1.0) * a;
            ui.label(&ma.font, 292.0, 372.0, 18.0, &cp1252(msg), [1.0, 0.93, 0.6, k], 0);
        }
    }

    /// Game Play: the drive screen, with the mosaic between the banner and
    /// the status lines.
    fn draw_game<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, al: [f32; 3]) {
        self.game.draw(ui, ma, &self.w, &*self.drive, &self.info, self.lang, al);
    }

    /// Options: the firmware boot timeout and the menu language.
    fn draw_options<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, al: [f32; 3]) {
        let g = &ma.option;
        let green = rgb(0, 255, 120);
        let pairs = [(rgb(180, 255, 0), rgb(20, 100, 50)), (rgb(120, 220, 35), rgb(0, 80, 25))];
        if let Some(p) = g.text(tag(b"mesg")) {
            ui.text(&ma.font, p, &cp1252("Choose GameCoob boot settings."), with_a(WHITE, al[2]), 0.0, 0.0);
        }
        let hl = self.highlight as f32 / 120.0;
        // Row 0: timeout.
        if let Some(p) = g.text(tag(b"txt1")) {
            let k = if self.opt_row == 0 { 1.0 } else { 0.55 };
            ui.text(&ma.font, p, &cp1252("Boot Timeout"), with_a(scale(green, k), al[2]), 0.0, 0.0);
        }
        // Row 1: language.
        let lang = if self.opt_edit && self.opt_row == 1 { self.opt_lang } else { self.lang };
        if let Some(p) = g.text(tag(b"txt3")) {
            let k = if self.opt_row == 1 { 1.0 } else { 0.55 };
            let s = ma.option_str[self.guide_lang].get(3).map(|s| s.text.clone()).unwrap_or_else(|| cp1252("Language"));
            ui.text(&ma.font, p, &s, with_a(scale(green, k), al[2]), 0.0, 0.0);
        }
        let (l, d) = pairs[1];
        for i in 0..LANGS {
            let t = [b'l', b'a', b'n', b'1' + i as u8];
            let a = if i == lang { 1.0 } else { 0.45 };
            for p in g.pic(tag(&t)) {
                if let Some(tex) = ma.bank.get(p.tex as usize) {
                    ui.pic(p, tex, with_a(WHITE, a * al[2]), 0.0, 0.0);
                }
                if i == lang {
                    ui.fill(p.x - 30.0, p.y + 16.0, 60.0, 4.0, with_a(l, al[0] * if self.opt_row == 1 { 1.0 } else { 0.5 }));
                    ui.fill(p.x - 30.0, p.y + 20.0, 60.0, 2.0, with_a(d, al[0]));
                }
            }
        }
        for i in 0..5u8 {
            for p in g.pic(tag(&[b's', b'l', b'a', b'1' + i])) {
                if let Some(tex) = ma.bank.get(p.tex as usize) {
                    ui.pic(p, tex, with_a(WHITE, 0.5 * al[2]), 0.0, 0.0);
                }
            }
        }
        // Selection marker on the current row.
        let y = if self.opt_row == 0 { 102.0 } else { 316.0 };
        ui.fill(150.0, y - 2.0, 6.0, 6.0, with_a(green, hl * al[2]));
        ui.fill(432.0, y - 2.0, 6.0, 6.0, with_a(green, hl * al[2]));
    }

    /// Calendar: the firmware clock.
    fn draw_clock<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, al: [f32; 3]) {
        let g = &ma.clock;
        let s = &ma.clock_str[self.lang];
        if let Some(p) = g.text(tag(b"mesg")) {
            ui.text(&ma.font, p, &cp1252("The firmware clock."), with_a(WHITE, al[2]), 0.0, 0.0);
        }
        // Clock_Draw: the row labels use the calendar text colour.
        let yellow = rgb(255, 255, 0);
        for (i, t) in [(1usize, b"txt1"), (2, b"txt2")] {
            if let (Some(p), Some(st)) = (g.text(tag(t)), s.get(i)) {
                ui.text(&ma.font, p, &st.text, with_a(yellow, al[2]), 0.0, 0.0);
            }
        }
        for t in [b"bar1", b"bar2"] {
            for f in g.frame(tag(t)) {
                ui.frame(f, &ma.bank, [1.0, 1.0, 0.0, al[1]]);
            }
        }
    }

    /// Menu_DrawGuide: the button guide along the bottom.
    fn draw_guide<'a>(&'a self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, alpha: u8) {
        let gl = &ma.guide;
        let bank = &ma.bank[..];
        let blink = self.guide_blink as i32;
        let g: [i32; 6] = self.guide_icon.map(|f| f.alpha());
        let pulse = if blink % 256 > 127 { 100 + (blink % 128) * 155 / 127 } else { 255 - (blink % 128) * 155 / 127 };
        let base = pulse * alpha as i32 / 255;
        let col = |rgb: [u8; 3], a: i32| [rgb[0] as f32 / 255.0, rgb[1] as f32 / 255.0, rgb[2] as f32 / 255.0, a as f32 / 255.0];
        for i in 0..2u8 {
            let k = i as usize * 3;
            pics(ui, gl, &[b'b', b'a', b'c', b'1' + i * 3], bank, col([150, 150, 150], base * g[k] / 255));
            pics(ui, gl, &[b'b', b'a', b'c', b'2' + i * 3], bank, col(self.w.guide_color_b.map(|c| c as u8), base * g[k + 1] / 255));
            pics(ui, gl, &[b'b', b'a', b'c', b'3' + i * 3], bank, col(self.w.guide_color_a.map(|c| c as u8), base * g[k + 2] / 255));
        }
        // The stick cross-fades between neutral and a tilted sprite.
        let k = (blink / 128) % 8;
        let f = if k % 2 == 0 { (blink % 128) * 255 / 127 } else { 255 - (blink % 128) * 255 / 127 };
        let fa = f * alpha as i32 / 255;
        for i in 0..2u8 {
            let gi = g[i as usize * 3];
            let tilt = b'0' + i * 5 + (((k + 1) / 2 + 3) % 4 + 1) as u8;
            pics(ui, gl, &[b's', b't', b'k', tilt], bank, with_a(WHITE, ((255 - fa) * gi / 255) as f32 / 255.0));
            pics(ui, gl, &[b's', b't', b'k', b'0' + i * 5], bank, with_a(WHITE, (fa * gi / 255) as f32 / 255.0));
        }
        let fa = (alpha as i32 * self.guide_alpha as i32 / 255) as f32 / 255.0;
        for (t, d, gi) in [(b"butb", b"dot2", 1), (b"butc", b"dot5", 4), (b"buta", b"dot3", 2), (b"butd", b"dot6", 5)] {
            let c = with_a(WHITE, fa * g[gi] as f32 / 255.0);
            pics(ui, gl, t, bank, c);
            pics(ui, gl, d, bank, c);
        }
        pics(ui, gl, b"dot1", bank, with_a(WHITE, fa * g[0] as f32 / 255.0));
        pics(ui, gl, b"dot4", bank, with_a(WHITE, fa * g[3] as f32 / 255.0));
        // Labels, once per language for the cross-fade.
        const IDS: [usize; 8] = [0, 1, 2, 4, 13, 8, 11, 6];
        for lang in 0..LANGS {
            let la = self.lang_guide[lang].alpha() as f32 / 255.0 * alpha as f32 / 255.0;
            if la <= 0.0 {
                continue;
            }
            for (i, f) in self.guide_text.iter().enumerate() {
                let a = f.alpha() as f32 / 255.0 * la;
                if a <= 0.0 {
                    continue;
                }
                let Some(st) = ma.guide_str[lang].get(IDS[i]) else { continue };
                if let Some(p) = gl.texts.get(st.pane as usize) {
                    ui.text(&ma.font, p, &st.text, with_a(WHITE, a), 0.0, 0.0);
                }
            }
        }
    }

    /// The mosaic's frame (the submenus' Fx*_Update + Fx_Update). Call
    /// once per frame after [`Menu::update`].
    pub fn update_fx(&mut self, ma: &MenuAssets) {
        let d = &ma.fx;
        match self.state {
            St::Game if !self.game.starting() && self.game_icon.settled() => {
                let msg = game::Game::fx_message(&*self.drive);
                self.fx.frame(d, fx::set::GAME, self.lang, &fx::GAME_COLORS, |b| screen::game(b, msg));
            }
            St::Option if self.option_icon.settled() => {
                // Ours has two rows: the boot timeout sits in the IPL's
                // screen-position row, the language in its language row.
                let row = self.opt_row as u8 + 1;
                let t = if self.opt_edit && self.opt_row == 0 { self.opt_timeout } else { self.info.timeout };
                let o = screen::Options { sub: if self.opt_edit { row + 1 } else { 0 }, row, stereo: None, offset: Some(t as i32), lang: true };
                self.fx.frame(d, fx::set::OPTION, self.lang, &fx::OPTION_COLORS, |b| screen::options(b, &o));
            }
            St::Clock if self.clock_icon.settled() => {
                let k = &mut self.clock;
                let c = screen::Clock {
                    sub: k.sub,
                    prev_sub: k.shown_sub,
                    row: k.row,
                    date_field: k.date_field,
                    time_field: k.time_field,
                    time: k.shown(self.info.time),
                    keep_sway: true,
                };
                k.shown_sub = k.sub;
                self.fx.frame(d, fx::set::CLOCK, self.lang, &fx::CLOCK_COLORS, |b| screen::clock(b, &c));
            }
            _ => {}
        }
    }

    /// CardIcon_IsSettled: the card browser waits for this before it
    /// animates the save tiles in.
    #[allow(dead_code)]
    pub fn card_icon_settled(&self) -> bool {
        self.card_icon.settled()
    }

    /// Per-frame bookkeeping of Menu_DrawGuide (it advances its own clocks).
    pub fn tick_guide(&mut self) {
        self.guide_alpha = (self.guide_alpha + 4).min(255);
        self.guide_blink = self.guide_blink.wrapping_add(1);
    }
}

/// `sBigCube`'s packets: the faces behind the icons (`packets + 0x78`) and
/// in front of them (`packets + 0x168`).
const BIG_CUBE_BACK: u16 = 0x78 / 0x18;
const BIG_CUBE_FRONT: u16 = 0x168 / 0x18;

/// The 2D screen of Setup2DView: 584 x 448 layout units, y down, at z 0
/// in front of a camera 224 / tan(fovy / 2) away.
pub fn screen_2d() -> Mtx {
    camera::setup_2d_view(bs2main::disp_settings())
}

/// `view · cube · SRT` of a face layout (scale (1, -1, 1)).
fn face_mtx(view: &View, cube: &Mtx, rot: (i32, i32), t: [f32; 3]) -> Mtx {
    let srt = psmtx::concat(&mtx::rot_trans(rot.0, rot.1, 0, t), &psmtx::scale(1.0, -1.0, 1.0));
    psmtx::concat(&psmtx::concat(&view.view, cube), &srt)
}

/// The front face (Menu_DrawTopFace / Menu_DrawSpinOutFace); its alpha is
/// applied twice.
fn draw_front<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, lang: usize, view: &View, cube: &Mtx, alpha: u8) {
    if alpha == 0 {
        return;
    }
    let a = alpha as f32 / 255.0;
    ui.mv = face_mtx(view, cube, (0, 0), [-FACE_HALF, FACE_HALF, FACE_HALF]);
    let g = &ma.front[lang];
    for p in g.pics.iter() {
        if let Some(t) = ma.bank.get(p.tex as usize) {
            ui.pic(p, t, with_a(WHITE, a * a), 0.0, 0.0);
        }
    }
}

/// Backdrop_Draw, simplified: the 8x8 grid tile repeated over the screen,
/// lit by a soft spot from the centre instead of the GX spot light.
/// The 2D plane's matrices with its normal matrix, as the submenus load
/// them before `Backdrop_Draw`.
fn load_backdrop_mtx(ui: &mut Ui) {
    ui.load();
    let nrm = mtx::inverse33(&ui.mv).unwrap_or(psmtx::IDENTITY);
    ui.gx.load_nrm_mtx_imm(&nrm, GX_PNMTX0);
}

/// `Backdrop_Draw` on the 2D plane at fader alpha `a` (0..1).
fn draw_backdrop<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, w: &Work, a: f32) {
    load_backdrop_mtx(ui);
    backdrop::draw(ui.gx, w, &ma.backdrop, &ui.mv, (a * 255.0 + 0.5) as u8);
}

/// `Backdrop_DrawSpots` on the 2D plane at fader alpha `a` (0..1).
#[allow(clippy::too_many_arguments)]
pub(crate) fn draw_backdrop_spots<'a>(ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, w: &Work, a: f32, pos: [[f32; 3]; 2], alpha: [u8; 2], spot_alpha: u8) {
    load_backdrop_mtx(ui);
    backdrop::draw_spots(ui.gx, w, &ma.backdrop, &ui.mv, (a * 255.0 + 0.5) as u8, pos[0], pos[1], alpha[0], alpha[1], spot_alpha);
}

fn pics<'a>(ui: &mut Ui<'a, '_>, g: &Layout, t: &[u8; 4], bank: &'a [FastTex], c: Rgba) {
    if c[3] <= 0.0 {
        return;
    }
    for p in g.pic(tag(t)) {
        if let Some(tex) = bank.get(p.tex as usize) {
            ui.pic(p, tex, c, 0.0, 0.0);
        }
    }
}

fn with_a(c: Rgba, a: f32) -> Rgba {
    ui::with_alpha(c, a)
}

fn rgb(r: u8, g: u8, b: u8) -> Rgba {
    ui::rgba8([r, g, b, 255])
}

fn scale(c: Rgba, k: f32) -> Rgba {
    [c[0] * k, c[1] * k, c[2] * k, c[3]]
}

/// Cut `s` with "..." so it fits `w` at glyph size `size`.
fn fit(font: &UiFont, s: &[u8], w: f32, size: f32) -> Vec<u8> {
    if font.width(s, size, 0.0) <= w {
        return s.to_vec();
    }
    let mut v: Vec<u8> = s.to_vec();
    while !v.is_empty() && font.width(&v, size, 0.0) + font.width(b"...", size, 0.0) > w {
        v.pop();
    }
    v.extend_from_slice(b"...");
    v
}

/// Day of the week, 0 = Sunday (Sakamoto).
pub fn weekday(y: i32, m: i32, d: i32) -> usize {
    const T: [i32; 12] = [0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4];
    let y = if m < 3 { y - 1 } else { y };
    let m = (m - 1).clamp(0, 11) as usize;
    ((y + y / 4 - y / 100 + y / 400 + T[m] + d).rem_euclid(7)) as usize
}
