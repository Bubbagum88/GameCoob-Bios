//! Card Files: GameCoob's own file explorer for memory card files on the
//! PC's drives. It browses the volumes the firmware can read and shows
//! card images (`.raw`, `.mcp`, `.gcp`) and single saves (`.gci`, `.gcs`,
//! `.sav`), and:
//!
//! * puts a card image in slot A or B (the Memory Card screen then works
//!   on it, and changes are written back to the file),
//! * unpacks a card image into a folder of `.gci` saves (a Dolphin GCI
//!   folder) and packs such a folder back into a card image,
//! * makes a new blank card image of any size,
//! * converts a save between `.gci`, `.gcs` and `.sav`, and copies it onto
//!   the card in a slot,
//! * exports a save from the card in a slot as any of the three formats,
//!   into the folder last browsed.
//!
//! It is drawn as a dialog over the menu, in the same layout as the
//! system dialog.

use super::backend::MemCards;
use super::card_image::{card_text, ImageCards};
use super::volumes::{self, Entry, Volume, Volumes};
use super::{Fader, MenuAssets, Pad, WHITE, with_a};
use super::super::ui::{cp1252, Ui};
use crate::card::{self, Card, Encoding, SaveFile, SaveFormat};
use crate::ipl::glh::tag;
use alloc::boxed::Box;
use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec::Vec;

/// Rows of the list shown at once.
const ROWS: usize = 9;
/// Frames a result message stays.
const MSG_FRAMES: u32 = 240;
/// Card sizes offered for a new image, in blocks.
const NEW_SIZES: [u16; 6] = [59, 123, 251, 507, 1019, 2043];
const CARD_EXTS: [&str; 3] = ["raw", "mcp", "gcp"];
/// Where exports go before any folder was browsed (boot volume).
const DEFAULT_EXPORT: &str = "\\EFI\\GameCoob\\saves";

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum Kind {
    Up,
    NewCard,
    Dir,
    CardImage,
    Save,
}

/// What a list row is.
#[derive(Clone, Debug)]
struct Row {
    kind: Kind,
    name: String,
    size: u64,
}

#[derive(Clone, PartialEq, Eq, Debug)]
enum Where {
    Volumes,
    Folder { vol: String, path: String },
    /// The saves on the card in a slot.
    Slot(usize),
}

/// What a popup entry does.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum Act {
    Open,
    LoadSlot(usize),
    Unpack,
    Pack,
    ImportTo(usize),
    Convert(SaveFormat),
    NewCard(u16),
    Export(SaveFormat),
    Erase,
    Eject(usize),
    Cancel,
}

struct Popup {
    items: Vec<(String, Act)>,
    sel: usize,
}

/// What the info line says about the selected row (read once per file).
#[derive(Clone)]
struct Info {
    key: String,
    text: String,
}

pub struct Explorer {
    pub open: bool,
    fader: Fader,
    at: Where,
    rows: Vec<Row>,
    /// Volume list (when `at` is `Volumes`), by row.
    vols: Vec<Volume>,
    sel: usize,
    top: usize,
    popup: Option<Popup>,
    message: Option<(String, u32)>,
    info: Option<Info>,
    /// The folder exports go to: the last one browsed.
    export_to: Option<(String, String)>,
    /// Rows to restore when going back up: (where, sel).
    back: Vec<(Where, usize)>,
}

impl Default for Explorer {
    fn default() -> Self {
        Explorer::new()
    }
}

fn kind_of(e: &Entry) -> Option<Kind> {
    if e.dir {
        return Some(Kind::Dir);
    }
    let x = volumes::ext(&e.name);
    if CARD_EXTS.contains(&x.as_str()) {
        Some(Kind::CardImage)
    } else if SaveFormat::from_ext(&x).is_some() {
        Some(Kind::Save)
    } else {
        None
    }
}

fn stem(name: &str) -> &str {
    name.rsplit_once('.').map_or(name, |(s, _)| s)
}

/// The images slot handles live in [`ImageCards`]; with the made-up cards
/// still in, the first image loaded swaps them out for real ones.
fn images(cards: &mut Box<dyn MemCards>) -> &mut ImageCards {
    if cards.images().is_none() {
        *cards = Box::new(ImageCards::new());
    }
    cards.images().unwrap()
}

fn slot_name(s: usize) -> &'static str {
    ["A", "B"][s & 1]
}

impl Explorer {
    pub fn new() -> Explorer {
        Explorer {
            open: false,
            fader: Fader::default(),
            at: Where::Volumes,
            rows: Vec::new(),
            vols: Vec::new(),
            sel: 0,
            top: 0,
            popup: None,
            message: None,
            info: None,
            export_to: None,
            back: Vec::new(),
        }
    }

    /// Show the explorer at its volume list.
    pub fn show(&mut self, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        self.open = true;
        self.popup = None;
        self.back.clear();
        self.go(Where::Volumes, vols, cards);
    }

    /// True while it is on screen (it takes all input).
    pub fn visible(&self) -> bool {
        self.open || self.fader.alpha() > 0
    }

    /// Show a line (a write-back failure) the next time it is on screen.
    pub fn note(&mut self, s: String) {
        self.say(s);
    }

    fn say(&mut self, s: impl Into<String>) {
        self.message = Some((s.into(), MSG_FRAMES));
    }

    /// Move to `w` and read its rows.
    fn go(&mut self, w: Where, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        self.at = w;
        self.sel = 0;
        self.top = 0;
        self.info = None;
        self.refresh(vols, cards);
    }

    /// Re-read the rows of where we are, keeping the selection if it fits.
    fn refresh(&mut self, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        self.rows.clear();
        self.info = None;
        match self.at.clone() {
            Where::Volumes => {
                self.vols = vols.volumes();
                for v in &self.vols {
                    let mut name = if v.label.is_empty() { String::from("Drive") } else { v.label.clone() };
                    if v.boot {
                        name.push_str(" (GameCoob)");
                    }
                    self.rows.push(Row { kind: Kind::Dir, name, size: v.size });
                }
                for s in 0..2 {
                    if let Some(h) = cards.images().and_then(|c| c.name(s).map(String::from)) {
                        let file = volumes::file_name(volumes::split_handle(&h).1).to_string();
                        self.rows.push(Row { kind: Kind::CardImage, name: format!("Slot {}: {file}", slot_name(s)), size: s as u64 });
                    }
                }
            }
            Where::Folder { vol, path } => {
                self.export_to = Some((vol.clone(), path.clone()));
                self.rows.push(Row { kind: Kind::Up, name: String::from(".."), size: 0 });
                self.rows.push(Row { kind: Kind::NewCard, name: String::from("New card image..."), size: 0 });
                match vols.list(&vol, &path) {
                    Ok(mut es) => {
                        es.sort_by(|a, b| b.dir.cmp(&a.dir).then_with(|| a.name.to_ascii_lowercase().cmp(&b.name.to_ascii_lowercase())));
                        for e in es {
                            if let Some(k) = kind_of(&e) {
                                self.rows.push(Row { kind: k, name: e.name, size: e.size });
                            }
                        }
                    }
                    Err(e) => self.say(format!("Can't read this folder ({e})")),
                }
            }
            Where::Slot(s) => {
                self.rows.push(Row { kind: Kind::Up, name: String::from(".."), size: 0 });
                if let Some(c) = cards.images().and_then(|c| c.card(s)) {
                    let enc = c.encoding();
                    for (i, d) in c.saves() {
                        let title = c.read(i).ok().and_then(|f| f.comments().map(|(t, _)| card_text(t, enc))).unwrap_or_default();
                        let name = if title.is_empty() { card_text(d.file_name(), enc) } else { title };
                        self.rows.push(Row { kind: Kind::Save, name, size: i as u64 });
                    }
                }
            }
        }
        if self.sel >= self.rows.len() {
            self.sel = self.rows.len().saturating_sub(1);
        }
    }

    fn path_of(&self, row: &Row) -> Option<(String, String)> {
        match &self.at {
            Where::Folder { vol, path } => Some((vol.clone(), volumes::join(path, &row.name))),
            _ => None,
        }
    }

    /// The popup for the selected row.
    fn popup_for(&self, cards: &mut Box<dyn MemCards>) -> Option<Popup> {
        let row = self.rows.get(self.sel)?;
        let mut items: Vec<(String, Act)> = Vec::new();
        let has_image = |s: usize, c: &mut Box<dyn MemCards>| c.images().and_then(|c| c.card(s)).is_some();
        match (&self.at, row.kind) {
            (Where::Volumes, Kind::CardImage) => {
                let s = row.size as usize;
                items.push((String::from("Show saves"), Act::Open));
                items.push((format!("Take out of slot {}", slot_name(s)), Act::Eject(s)));
            }
            (_, Kind::Up) | (Where::Volumes, _) => return None,
            (Where::Folder { .. }, Kind::Dir) => {
                items.push((String::from("Open"), Act::Open));
                items.push((String::from("Pack saves into a card image"), Act::Pack));
            }
            (Where::Folder { .. }, Kind::NewCard) => {
                for b in NEW_SIZES {
                    items.push((format!("{b} blocks"), Act::NewCard(b)));
                }
            }
            (Where::Folder { .. }, Kind::CardImage) => {
                items.push((String::from("Put in slot A"), Act::LoadSlot(0)));
                items.push((String::from("Put in slot B"), Act::LoadSlot(1)));
                items.push((String::from("Unpack to a GCI folder"), Act::Unpack));
            }
            (Where::Folder { .. }, Kind::Save) => {
                for s in 0..2 {
                    if has_image(s, cards) {
                        items.push((format!("Copy to card in slot {}", slot_name(s)), Act::ImportTo(s)));
                    }
                }
                let own = SaveFormat::from_ext(&volumes::ext(&row.name));
                for f in SaveFormat::ALL {
                    if Some(f) != own {
                        items.push((format!("Save as {}", f.label()), Act::Convert(f)));
                    }
                }
            }
            (Where::Slot(_), Kind::Save) => {
                for f in SaveFormat::ALL {
                    items.push((format!("Export as {}", f.label()), Act::Export(f)));
                }
                items.push((String::from("Erase"), Act::Erase));
            }
            _ => return None,
        }
        items.push((String::from("Cancel"), Act::Cancel));
        Some(Popup { items, sel: 0 })
    }

    pub fn update(&mut self, p: &Pad, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        self.fader.step_if(self.open);
        if let Some((_, t)) = &mut self.message {
            *t = t.saturating_sub(1);
            if *t == 0 {
                self.message = None;
            }
        }
        if !self.open {
            return;
        }
        if let Some(pop) = &mut self.popup {
            let n = pop.items.len();
            if p.up {
                pop.sel = (pop.sel + n - 1) % n;
            }
            if p.down {
                pop.sel = (pop.sel + 1) % n;
            }
            if p.b {
                self.popup = None;
            } else if p.a {
                let act = pop.items[pop.sel].1;
                self.popup = None;
                self.run(act, vols, cards);
            }
            return;
        }
        let n = self.rows.len();
        if n > 0 {
            if p.up {
                self.sel = (self.sel + n - 1) % n;
            }
            if p.down {
                self.sel = (self.sel + 1) % n;
            }
            if p.left {
                self.sel = self.sel.saturating_sub(ROWS);
            }
            if p.right {
                self.sel = (self.sel + ROWS).min(n - 1);
            }
        }
        if self.sel < self.top {
            self.top = self.sel;
        }
        if self.sel >= self.top + ROWS {
            self.top = self.sel + 1 - ROWS;
        }
        if p.b {
            self.up(vols, cards);
        } else if p.a {
            self.enter(vols, cards);
        }
        self.update_info(vols, cards);
    }

    /// Back one level; from the volume list, close.
    fn up(&mut self, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        match self.back.pop() {
            Some((w, sel)) => {
                self.go(w, vols, cards);
                self.sel = sel.min(self.rows.len().saturating_sub(1));
                self.top = self.sel.saturating_sub(ROWS / 2);
            }
            None => self.open = false,
        }
    }

    fn descend(&mut self, w: Where, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        self.back.push((self.at.clone(), self.sel));
        self.go(w, vols, cards);
    }

    /// A on a row: open folders and volumes, a popup for the rest.
    fn enter(&mut self, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        let Some(row) = self.rows.get(self.sel).cloned() else { return };
        match (&self.at, row.kind) {
            (_, Kind::Up) => self.up(vols, cards),
            (Where::Volumes, Kind::Dir) => {
                if let Some(v) = self.vols.get(self.sel) {
                    let w = Where::Folder { vol: v.id.clone(), path: String::from("\\") };
                    self.descend(w, vols, cards);
                }
            }
            (Where::Folder { .. }, Kind::Dir) => {
                if let Some((vol, path)) = self.path_of(&row) {
                    self.descend(Where::Folder { vol, path }, vols, cards);
                }
            }
            _ => self.popup = self.popup_for(cards),
        }
    }

    fn run(&mut self, act: Act, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        let Some(row) = self.rows.get(self.sel).cloned() else { return };
        let r = match act {
            Act::Cancel => Ok(None),
            Act::Open => {
                match (&self.at, row.kind) {
                    (Where::Volumes, Kind::CardImage) => self.descend(Where::Slot(row.size as usize), vols, cards),
                    _ => self.enter(vols, cards),
                }
                Ok(None)
            }
            Act::Eject(s) => {
                let r = flush(vols, images(cards).eject(s));
                self.refresh(vols, cards);
                r.map(|_| Some(format!("Took the card out of slot {}.", slot_name(s))))
            }
            Act::LoadSlot(s) => self.load_slot(&row, s, vols, cards),
            Act::Unpack => self.unpack(&row, vols),
            Act::Pack => self.pack(&row, vols),
            Act::NewCard(blocks) => self.new_card(blocks, vols),
            Act::ImportTo(s) => self.import(&row, s, vols, cards),
            Act::Convert(f) => self.convert(&row, f, vols),
            Act::Export(f) => self.export(&row, f, vols, cards),
            Act::Erase => {
                let Where::Slot(s) = self.at else { return };
                match images(cards).edit(s, |c| c.delete(row.size as usize)) {
                    Some(Ok(())) => Ok(Some(String::from("Erased."))),
                    Some(Err(e)) => Err(e.to_string()),
                    None => Err(String::from("The card is busy")),
                }
            }
        };
        match r {
            Ok(Some(m)) => self.say(m),
            Ok(None) => {}
            Err(e) => self.say(e),
        }
        if !matches!(act, Act::Open | Act::Cancel | Act::Eject(_)) {
            let sel = self.sel;
            self.refresh(vols, cards);
            self.sel = sel.min(self.rows.len().saturating_sub(1));
        }
    }

    fn load_slot(&mut self, row: &Row, s: usize, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) -> Result<Option<String>, String> {
        let (vol, path) = self.path_of(row).ok_or("")?;
        let bytes = vols.read(&vol, &path)?;
        let ic = images(cards);
        // Write back what the slot held before it goes.
        let old = ic.eject(s);
        let bad = Card::open(bytes.clone()).err();
        ic.insert(s, volumes::handle(&vol, &path), bytes);
        flush(vols, old)?;
        Ok(Some(match bad {
            None => format!("{} is in slot {}.", row.name, slot_name(s)),
            Some(e) => format!("In slot {}, but {e}.", slot_name(s)),
        }))
    }

    fn read_card(&self, row: &Row, vols: &mut dyn Volumes) -> Result<Card, String> {
        let (vol, path) = self.path_of(row).ok_or("")?;
        Card::open(vols.read(&vol, &path)?).map_err(|e| e.to_string())
    }

    fn unpack(&mut self, row: &Row, vols: &mut dyn Volumes) -> Result<Option<String>, String> {
        let c = self.read_card(row, vols)?;
        let Where::Folder { vol, path } = self.at.clone() else { return Ok(None) };
        let taken = vols.list(&vol, &path)?;
        let dir_name = free_dir(&taken, stem(&row.name));
        let dir = volumes::join(&path, &dir_name);
        vols.mkdir(&vol, &dir)?;
        let mut n = 0;
        let mut written: Vec<Entry> = Vec::new();
        for (i, _) in c.saves() {
            let s = c.read(i).map_err(|e| e.to_string())?;
            let name = volumes::free_name(&written, &s.dolphin_name(), "gci");
            vols.write(&vol, &volumes::join(&dir, &name), &s.to_bytes(SaveFormat::Gci))?;
            written.push(Entry { name, dir: false, size: 0 });
            n += 1;
        }
        Ok(Some(format!("{n} saves unpacked into {dir_name}.")))
    }

    fn pack(&mut self, row: &Row, vols: &mut dyn Volumes) -> Result<Option<String>, String> {
        let (vol, dir) = self.path_of(row).ok_or("")?;
        let mut saves = Vec::new();
        for e in vols.list(&vol, &dir)? {
            if kind_of(&e) == Some(Kind::Save) {
                let b = vols.read(&vol, &volumes::join(&dir, &e.name))?;
                if let Ok((s, _)) = SaveFile::parse(&b) {
                    saves.push(s);
                }
            }
        }
        if saves.is_empty() {
            return Err(String::from("No saves in that folder"));
        }
        let need: u32 = saves.iter().map(|s| s.dentry.block_count() as u32).sum();
        let blocks = NEW_SIZES.iter().copied().find(|&b| b >= 251 && b as u32 >= need).unwrap_or(2043);
        let mut c = blank_card(blocks);
        let mut skipped = 0;
        for s in &saves {
            if c.import(s).is_err() {
                skipped += 1;
            }
        }
        let Where::Folder { path, .. } = &self.at else { return Ok(None) };
        let taken = vols.list(&vol, path)?;
        let name = volumes::free_name(&taken, &row.name, "raw");
        vols.write(&vol, &volumes::join(path, &name), c.as_bytes())?;
        Ok(Some(match skipped {
            0 => format!("Packed {} saves into {name}.", saves.len()),
            k => format!("{name}: {k} saves didn't fit or clash."),
        }))
    }

    fn new_card(&mut self, blocks: u16, vols: &mut dyn Volumes) -> Result<Option<String>, String> {
        let Where::Folder { vol, path } = self.at.clone() else { return Ok(None) };
        let taken = vols.list(&vol, &path)?;
        let name = volumes::free_name(&taken, &format!("card-{blocks}"), "raw");
        vols.write(&vol, &volumes::join(&path, &name), blank_card(blocks).as_bytes())?;
        Ok(Some(format!("Made {name}.")))
    }

    fn read_save(&self, row: &Row, vols: &mut dyn Volumes) -> Result<SaveFile, String> {
        let (vol, path) = self.path_of(row).ok_or("")?;
        SaveFile::parse(&vols.read(&vol, &path)?).map(|r| r.0).map_err(|e| e.to_string())
    }

    fn import(&mut self, row: &Row, s: usize, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) -> Result<Option<String>, String> {
        let save = self.read_save(row, vols)?;
        match images(cards).edit(s, |c| c.import(&save)) {
            Some(Ok(_)) => Ok(Some(format!("Copied to the card in slot {}.", slot_name(s)))),
            Some(Err(e)) => Err(e.to_string()),
            None => Err(String::from("The card is busy")),
        }
    }

    fn convert(&mut self, row: &Row, f: SaveFormat, vols: &mut dyn Volumes) -> Result<Option<String>, String> {
        let save = self.read_save(row, vols)?;
        let Where::Folder { vol, path } = self.at.clone() else { return Ok(None) };
        let taken = vols.list(&vol, &path)?;
        let name = volumes::free_name(&taken, stem(&row.name), f.ext());
        vols.write(&vol, &volumes::join(&path, &name), &save.to_bytes(f))?;
        Ok(Some(format!("Saved {name}.")))
    }

    fn export(&mut self, row: &Row, f: SaveFormat, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) -> Result<Option<String>, String> {
        let Where::Slot(s) = self.at else { return Ok(None) };
        let save = images(cards).card(s).ok_or("No card")?.read(row.size as usize).map_err(|e| e.to_string())?;
        let (vol, dir) = match &self.export_to {
            Some(t) => t.clone(),
            None => {
                let v = vols.volumes().into_iter().find(|v| v.boot).ok_or("No drive to write to")?;
                let _ = vols.mkdir(&v.id, "\\EFI\\GameCoob");
                vols.mkdir(&v.id, DEFAULT_EXPORT)?;
                (v.id, String::from(DEFAULT_EXPORT))
            }
        };
        let taken = vols.list(&vol, &dir)?;
        let name = volumes::free_name(&taken, &save.dolphin_name(), f.ext());
        vols.write(&vol, &volumes::join(&dir, &name), &save.to_bytes(f))?;
        Ok(Some(format!("Saved {name} in {dir}.")))
    }

    /// Fill the info line for the selected row (once per row).
    fn update_info(&mut self, vols: &mut dyn Volumes, cards: &mut Box<dyn MemCards>) {
        let Some(row) = self.rows.get(self.sel).cloned() else {
            self.info = None;
            return;
        };
        let key = format!("{:?}{}{}", self.at, self.sel, row.name);
        if self.info.as_ref().is_some_and(|i| i.key == key) {
            return;
        }
        let text = match (&self.at, row.kind) {
            (Where::Volumes, Kind::Dir) => self
                .vols
                .get(self.sel)
                .map(|v| format!("{} free of {}", volumes::size_text(v.free), volumes::size_text(v.size)))
                .unwrap_or_default(),
            (Where::Volumes, Kind::CardImage) => card_info(cards.images().and_then(|c| c.card(row.size as usize))),
            (Where::Folder { .. }, Kind::CardImage) => match self.read_card(&row, vols) {
                Ok(c) => card_info(Some(&c)),
                Err(e) => format!("Not a usable card image: {e}"),
            },
            (Where::Folder { .. }, Kind::Save) => match self.read_save(&row, vols) {
                Ok(s) => save_info(&s, Encoding::Ansi),
                Err(e) => format!("Not a usable save: {e}"),
            },
            (Where::Slot(s), Kind::Save) => {
                let c = cards.images().and_then(|c| c.card(*s));
                c.and_then(|c| c.read(row.size as usize).ok().map(|f| save_info(&f, c.encoding()))).unwrap_or_default()
            }
            (_, Kind::NewCard) => String::from("Make a blank, formatted card image here"),
            _ => String::new(),
        };
        self.info = Some(Info { key, text });
    }

    fn title(&self) -> String {
        match &self.at {
            Where::Volumes => String::from("Card Files"),
            Where::Folder { vol, path } => {
                let v = self.back.first().and_then(|(_, sel)| self.vol_label(vol, *sel));
                format!("{}{}", v.unwrap_or_default(), path)
            }
            Where::Slot(s) => format!("Card in slot {}", slot_name(*s)),
        }
    }

    fn vol_label(&self, id: &str, _sel: usize) -> Option<String> {
        self.vols.iter().find(|v| v.id == id).map(|v| if v.label.is_empty() { String::from("Drive") } else { v.label.clone() })
    }

    pub fn draw<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, pulse: f32) {
        let a = self.fader.alpha() as f32 / 255.0;
        if a <= 0.0 {
            return;
        }
        ui.fill(-40.0, -40.0, 720.0, 528.0, [0.0, 0.0, 0.0, 0.7 * a]);
        let d = &ma.dialog;
        let font = &ma.font;
        // The 2D layout is 584 units wide (centre 292), like the dialogs.
        let (x0, x1) = (40.0, 544.0);
        if let Some(f) = d.frame(tag(b"mes0")).next() {
            ui.frame_rect(f, &ma.bank, with_a(WHITE, a), x0 - 16.0, 36.0, x1 - x0 + 32.0, 376.0);
        }
        let grey = [0.7, 0.72, 0.8, 1.0];
        ui.label(font, x0, 48.0, 20.0, &cp1252(&self.title()), with_a(WHITE, a), 2);
        // The rows.
        let (top_y, pitch, size) = (84.0, 28.0, 18.0);
        if let Some(f) = d.frame(tag(b"mes1")).next() {
            if self.sel >= self.top && self.sel < self.top + ROWS && !self.rows.is_empty() {
                let y = top_y + (self.sel - self.top) as f32 * pitch;
                ui.frame_rect(f, &ma.bank, with_a(WHITE, a * (0.75 + 0.25 * pulse)), x0 - 6.0, y - 4.0, x1 - x0 + 12.0, size + 8.0);
            }
        }
        for (i, row) in self.rows.iter().enumerate().skip(self.top).take(ROWS) {
            let y = top_y + (i - self.top) as f32 * pitch;
            let k = if i == self.sel { 1.0 } else { 0.8 };
            let name = match row.kind {
                Kind::Dir if !matches!(self.at, Where::Volumes) => format!("{}\\", row.name),
                _ => row.name.clone(),
            };
            ui.label(font, x0, y, size, &cp1252(&clip(&name, 34)), with_a(WHITE, a * k), 2);
            let right = match (&self.at, row.kind) {
                (Where::Volumes, Kind::Dir) => volumes::size_text(row.size),
                (Where::Folder { .. }, Kind::CardImage) => String::from("Card"),
                (Where::Folder { .. }, Kind::Save) => volumes::ext(&row.name).to_ascii_uppercase(),
                (Where::Slot(_), Kind::Save) => String::from("Save"),
                _ => String::new(),
            };
            ui.label(font, x1, y, size, &cp1252(&right), with_a(grey, a * k), 1);
        }
        if self.rows.len() > ROWS {
            let t = format!("{}/{}", self.sel + 1, self.rows.len());
            ui.label(font, x1, 48.0, 16.0, &cp1252(&t), with_a(grey, a), 1);
        }
        // Info or result line.
        let (line, col) = match (&self.message, &self.info) {
            (Some((m, _)), _) => (m.clone(), [1.0, 0.9, 0.5, 1.0]),
            (None, Some(i)) => (i.text.clone(), grey),
            _ => (String::new(), grey),
        };
        ui.label(font, x0, 346.0, 16.0, &cp1252(&clip(&line, 56)), with_a(col, a), 2);
        ui.label(font, 292.0, 380.0, 16.0, b"A  Open / Choose       B  Back", with_a(grey, a), 0);
        // The popup.
        if let Some(pop) = &self.popup {
            let n = pop.items.len();
            let w = pop.items.iter().map(|(t, _)| font.width(&cp1252(t), size, 0.0)).fold(0.0f32, f32::max) + 32.0;
            let h = n as f32 * pitch + 16.0;
            let (px, py) = (x1 - w + 8.0, (220.0 - h / 2.0).max(60.0));
            ui.fill(px, py, w, h, [0.05, 0.05, 0.25, 0.92 * a]);
            if let Some(f) = d.frame(tag(b"mes1")).next() {
                ui.frame_rect(f, &ma.bank, with_a(WHITE, a), px, py, w, h);
                let y = py + 8.0 + pop.sel as f32 * pitch;
                ui.frame_rect(f, &ma.bank, with_a(WHITE, a * (0.75 + 0.25 * pulse)), px + 6.0, y - 3.0, w - 12.0, size + 6.0);
            }
            for (i, (t, _)) in pop.items.iter().enumerate() {
                let k = if i == pop.sel { 1.0 } else { 0.75 };
                ui.label(font, px + 16.0, py + 8.0 + i as f32 * pitch, size, &cp1252(t), with_a(WHITE, a * k), 2);
            }
        }
    }
}

/// Write a card's changed bytes back to the file it came from.
pub fn flush(vols: &mut dyn Volumes, changed: Option<(String, Vec<u8>)>) -> Result<(), String> {
    match changed {
        Some((h, b)) => {
            let (id, path) = volumes::split_handle(&h);
            vols.write(id, path, &b)
        }
        None => Ok(()),
    }
}

fn blank_card(blocks: u16) -> Card {
    let mbit = card::SIZES_MBIT.iter().copied().find(|&m| m as usize * card::BLOCKS_PER_MBIT - card::SYSTEM_BLOCKS as usize == blocks as usize).unwrap_or(16);
    // A fixed format time: the serial only has to differ between cards a
    // console sees together, and the slot keeps the image it was given.
    Card::format(mbit, Encoding::Ansi, 0x1D3A_7C00 ^ blocks as u64).unwrap()
}

fn free_dir(taken: &[Entry], stem: &str) -> String {
    let has = |n: &str| taken.iter().any(|e| e.name.eq_ignore_ascii_case(n));
    if !has(stem) {
        return String::from(stem);
    }
    (2..).map(|i| format!("{stem}-{i}")).find(|n| !has(n)).unwrap()
}

fn card_info(c: Option<&Card>) -> String {
    match c {
        Some(c) => format!("{} blocks, {} free, {} saves", c.user_blocks(), c.free_blocks(), c.saves().count()),
        None => String::from("Not a usable card"),
    }
}

fn save_info(s: &SaveFile, enc: Encoding) -> String {
    let d = &s.dentry;
    let (t, c) = s.comments().unwrap_or((&[], &[]));
    let mut out = format!("{} blocks  {}", d.block_count(), card_text(t, enc));
    if !c.is_empty() {
        out.push_str(" - ");
        out.push_str(&card_text(c, enc));
    }
    out
}

fn clip(s: &str, n: usize) -> String {
    if s.chars().count() <= n {
        return s.to_string();
    }
    let mut o: String = s.chars().take(n - 3).collect();
    o.push_str("...");
    o
}

#[cfg(test)]
mod tests {
    use super::*;
    use alloc::collections::BTreeMap;
    use alloc::vec;

    /// Volumes in RAM: one volume, files by path.
    struct RamVols {
        files: BTreeMap<String, Vec<u8>>,
        dirs: Vec<String>,
    }

    impl Volumes for RamVols {
        fn volumes(&mut self) -> Vec<Volume> {
            vec![Volume { id: "v".into(), label: "STICK".into(), size: 1 << 30, free: 1 << 29, boot: true }]
        }
        fn list(&mut self, _: &str, path: &str) -> Result<Vec<Entry>, String> {
            let pre = if path.ends_with('\\') { path.to_string() } else { format!("{path}\\") };
            let mut out = Vec::new();
            for (p, b) in &self.files {
                if let Some(n) = p.strip_prefix(&pre).filter(|n| !n.contains('\\')) {
                    out.push(Entry { name: n.into(), dir: false, size: b.len() as u64 });
                }
            }
            for d in &self.dirs {
                if let Some(n) = d.strip_prefix(&pre).filter(|n| !n.contains('\\') && !n.is_empty()) {
                    out.push(Entry { name: n.into(), dir: true, size: 0 });
                }
            }
            Ok(out)
        }
        fn read(&mut self, _: &str, path: &str) -> Result<Vec<u8>, String> {
            self.files.get(path).cloned().ok_or_else(|| "no file".into())
        }
        fn write(&mut self, _: &str, path: &str, b: &[u8]) -> Result<(), String> {
            self.files.insert(path.into(), b.to_vec());
            Ok(())
        }
        fn mkdir(&mut self, _: &str, path: &str) -> Result<(), String> {
            self.dirs.push(path.into());
            Ok(())
        }
    }

    fn save(name: &str) -> SaveFile {
        let mut d = [0u8; card::DENTRY];
        d[0..6].copy_from_slice(b"GCOBGC");
        d[8..8 + name.len()].copy_from_slice(name.as_bytes());
        d[0x38..0x3A].copy_from_slice(&1u16.to_be_bytes());
        SaveFile { dentry: card::Dentry(d), data: vec![0; card::BLOCK] }
    }

    fn press(e: &mut Explorer, v: &mut RamVols, c: &mut Box<dyn MemCards>, p: Pad) {
        e.update(&p, v, c);
        for _ in 0..30 {
            c.update();
        }
    }

    fn pick(e: &mut Explorer, v: &mut RamVols, c: &mut Box<dyn MemCards>, name: &str) {
        e.sel = e.rows.iter().position(|r| r.name == name).unwrap_or_else(|| panic!("{name} in {:?}", e.rows));
        press(e, v, c, Pad { a: true, ..Pad::default() });
    }

    fn choose(e: &mut Explorer, v: &mut RamVols, c: &mut Box<dyn MemCards>, item: &str) {
        let p = e.popup.as_mut().expect("popup");
        p.sel = p.items.iter().position(|(t, _)| t == item).unwrap_or_else(|| panic!("{item} in {:?}", p.items.iter().map(|i| &i.0).collect::<Vec<_>>()));
        press(e, v, c, Pad { a: true, ..Pad::default() });
    }

    #[test]
    fn walk_load_convert_pack() {
        let mut v = RamVols { files: BTreeMap::new(), dirs: vec!["\\saves".into()] };
        v.files.insert("\\saves\\alpha.gci".into(), save("alpha").to_bytes(SaveFormat::Gci));
        v.files.insert("\\readme.txt".into(), b"hi".to_vec());
        let mut c: Box<dyn MemCards> = Box::new(super::super::backend::FakeCards::new());
        let mut e = Explorer::new();
        e.show(&mut v, &mut c);
        pick(&mut e, &mut v, &mut c, "STICK (GameCoob)");
        // The text file is hidden.
        assert!(e.rows.iter().all(|r| r.name != "readme.txt"));
        // A new card image at the root, put in slot A.
        pick(&mut e, &mut v, &mut c, "New card image...");
        choose(&mut e, &mut v, &mut c, "59 blocks");
        assert!(v.files.contains_key("\\card-59.raw"));
        pick(&mut e, &mut v, &mut c, "card-59.raw");
        choose(&mut e, &mut v, &mut c, "Put in slot A");
        assert_eq!(c.images().unwrap().card(0).unwrap().user_blocks(), 59);
        // The save: convert it, then copy it onto the card.
        pick(&mut e, &mut v, &mut c, "saves");
        pick(&mut e, &mut v, &mut c, "alpha.gci");
        choose(&mut e, &mut v, &mut c, "Save as SAV (Action Replay)");
        assert_eq!(SaveFile::parse(&v.files["\\saves\\alpha.sav"]).unwrap().0, save("alpha"));
        pick(&mut e, &mut v, &mut c, "alpha.gci");
        choose(&mut e, &mut v, &mut c, "Copy to card in slot A");
        assert_eq!(c.images().unwrap().card(0).unwrap().saves().count(), 1);
        // The change is written back through the menu's write-back.
        let changed = c.take_changed(0);
        flush(&mut v, changed).unwrap();
        assert_eq!(Card::open(v.files["\\card-59.raw"].clone()).unwrap().saves().count(), 1);
        // Back up, pack the saves folder into a card.
        press(&mut e, &mut v, &mut c, Pad { b: true, ..Pad::default() });
        pick(&mut e, &mut v, &mut c, "saves");
        e.popup = None;
        // (A on a folder opens it; the popup comes from the action list.)
        press(&mut e, &mut v, &mut c, Pad { b: true, ..Pad::default() });
        let i = e.rows.iter().position(|r| r.name == "saves").unwrap();
        e.sel = i;
        e.popup = e.popup_for(&mut c);
        choose(&mut e, &mut v, &mut c, "Pack saves into a card image");
        // alpha.gci and alpha.sav are the same save: one goes in.
        let packed = Card::open(v.files["\\saves.raw"].clone()).unwrap();
        assert_eq!((packed.user_blocks(), packed.saves().count()), (251, 1));
        // Unpack it again.
        pick(&mut e, &mut v, &mut c, "saves.raw");
        choose(&mut e, &mut v, &mut c, "Unpack to a GCI folder");
        assert!(v.files.contains_key("\\saves-2\\GC-GCOB-alpha.gci"));
        // Export from the slot card.
        press(&mut e, &mut v, &mut c, Pad { b: true, ..Pad::default() });
        pick(&mut e, &mut v, &mut c, "Slot A: card-59.raw");
        choose(&mut e, &mut v, &mut c, "Show saves");
        pick(&mut e, &mut v, &mut c, "alpha");
        choose(&mut e, &mut v, &mut c, "Export as GCS (GameShark)");
        assert!(v.files.contains_key("\\GC-GCOB-alpha.gcs"), "{:?}", v.files.keys().collect::<Vec<_>>());
    }
}
