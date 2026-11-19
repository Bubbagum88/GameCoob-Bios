//! GameCoob's Firmware page (Settings ▸ Firmware): the PC's own setup
//! options, read from the firmware's HII forms (`crate::hii`) and shown as
//! lists: form sets, their forms, and each question with its value.
//!
//! Safety first, as the roadmap asks:
//! - Changes are only staged. Nothing reaches the firmware until Review
//!   lists every change (old and new value) and Save is picked there.
//! - Before saving, the old values of exactly the bytes being changed are
//!   written to `\EFI\GameCoob\Setup\backup-<time>.cfg` on the boot drive.
//!   If that fails, nothing is saved. Backups lists those files and puts
//!   one back.
//! - Only one-of, checkbox and numeric questions change, and only when
//!   the firmware lets them (not read-only, not greyed out, no driver
//!   callback, value routable). Everything else is shown, not touched.
//! - Writes go through the config routing protocol to the owning driver,
//!   the path the firmware's own setup screen uses.

use super::subcube::word_label;
use super::volumes::{Volumes, join};
use super::{MenuAssets, Pad, SHADOW, WHITE, with_a};
use super::super::sound::{self, se};
use super::super::ui::{cp1252, Ui};
use crate::hii::ifr::Kind;
use crate::hii::{Db, Firmware};
use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec::Vec;

/// Where backups and exports go on the boot drive.
pub const DIR: &str = "\\EFI\\GameCoob\\Setup";
/// Rows on screen at once.
const ROWS: usize = 8;

#[derive(Clone, Debug, PartialEq)]
enum View {
    Root,
    Form { set: usize, form: usize },
    Review,
    Backups(Vec<String>),
    Restore(String),
}

struct Level {
    view: View,
    sel: usize,
    top: usize,
}

#[derive(Clone, Debug, PartialEq)]
enum Act {
    /// Not selectable.
    None,
    System,
    Open(View),
    Edit { set: usize, form: usize, item: usize },
    Locked(&'static str),
    Review,
    Discard,
    Save,
    Back,
    Backups,
    Export,
    Restore(String),
    DoRestore(String),
}

struct Row {
    label: String,
    value: String,
    act: Act,
    /// A heading (subtitle).
    head: bool,
    help: String,
}

impl Row {
    fn new(label: impl Into<String>, act: Act) -> Row {
        Row { label: label.into(), value: String::new(), act, head: false, help: String::new() }
    }
    fn selectable(&self) -> bool {
        self.act != Act::None
    }
}

/// What the page asks of the menu after a frame.
#[derive(Default)]
pub struct Out {
    /// B on the top list: back to the cube.
    pub leave: bool,
    /// Open the system dialog (firmware setup, restart, power off).
    pub system: bool,
    pub note: Option<String>,
}

pub struct FwSetup {
    db: Option<Db>,
    error: Option<String>,
    stack: Vec<Level>,
}

impl FwSetup {
    pub fn new() -> FwSetup {
        FwSetup { db: None, error: None, stack: alloc::vec![Level { view: View::Root, sel: 0, top: 0 }] }
    }

    /// Back to the top list (the page was entered again).
    pub fn enter(&mut self) {
        self.stack.truncate(1);
        if self.db.is_none() {
            self.error = None;
        }
    }

    fn load(&mut self, fw: &mut dyn Firmware) {
        let hii = match fw.hii_export() {
            Ok(b) => b,
            Err(e) => {
                self.error = Some(e);
                return;
            }
        };
        // Without the values the forms still show, read-only.
        let cfg = fw.config_export().unwrap_or_default();
        let db = Db::load(&hii, &cfg, &mut |n, g| fw.variable(n, g));
        if db.menu_sets().is_empty() {
            self.error = Some(String::from("This firmware lists no setup forms."));
        }
        self.db = Some(db);
    }

    fn title(&self) -> String {
        let Some(db) = &self.db else { return String::from("Firmware") };
        match &self.stack.last().unwrap().view {
            View::Root => String::from("Firmware"),
            View::Form { set, form } => {
                let f = &db.sets[*set].fs.forms[*form];
                let t = db.str(*set, f.title);
                String::from(if t.trim().is_empty() { db.str(*set, db.sets[*set].fs.title) } else { t })
            }
            View::Review => String::from("Review"),
            View::Backups(_) | View::Restore(_) => String::from("Backups"),
        }
    }

    fn rows(&self) -> Vec<Row> {
        let mut v = Vec::new();
        let view = &self.stack.last().unwrap().view;
        match view {
            View::Root => {
                let mut r = Row::new("System menu", Act::System);
                r.help = String::from("Firmware setup, restart, power off and GameCoob's own switches.");
                v.push(r);
                if let Some(e) = &self.error {
                    v.push(Row::new(e.clone(), Act::None));
                }
                if let Some(db) = &self.db {
                    for s in db.menu_sets() {
                        let mut r = Row::new(db.str(s, db.sets[s].fs.title), Act::Open(View::Form { set: s, form: 0 }));
                        r.value = String::from(">");
                        r.help = String::from(db.str(s, db.sets[s].fs.help));
                        v.push(r);
                    }
                    let n = db.changes().len();
                    if n > 0 {
                        let mut r = Row::new(format!("Review and save ({n} change{})", if n == 1 { "" } else { "s" }), Act::Review);
                        r.help = String::from("Nothing is saved until you pick Save there.");
                        v.push(r);
                        v.push(Row::new("Discard changes", Act::Discard));
                    }
                    let mut r = Row::new("Backups", Act::Backups);
                    r.help = format!("Settings from before each save, in {DIR}.");
                    v.push(r);
                    let mut r = Row::new("Save forms to the boot drive", Act::Export);
                    r.help = format!("Copies the forms and values to {DIR} for a bug report.");
                    v.push(r);
                }
            }
            View::Form { set, form } => {
                let Some(db) = &self.db else { return v };
                let (set, form) = (*set, *form);
                for (i, it) in db.sets[set].fs.forms[form].items.iter().enumerate() {
                    if db.hidden(set, &it.conds) {
                        continue;
                    }
                    let prompt = db.str(set, it.prompt).trim().to_string();
                    let help = db.str(set, it.help).to_string();
                    let mut r = match &it.kind {
                        Kind::Subtitle => {
                            if prompt.is_empty() {
                                continue;
                            }
                            let mut r = Row::new(prompt, Act::None);
                            r.head = true;
                            r
                        }
                        Kind::Text(_) => {
                            if prompt.is_empty() {
                                continue;
                            }
                            let mut r = Row::new(prompt, Act::None);
                            r.value = db.value_text(set, it, true);
                            r
                        }
                        Kind::Ref { form: to, formset } => {
                            let target = match formset {
                                Some(g) => db.find_set(g).and_then(|s| db.find_form(s, *to).map(|f| (s, f))),
                                None => db.find_form(set, *to).map(|f| (set, f)),
                            };
                            let act = match target {
                                Some((s, f)) if !db.grayed(set, &it.conds) => Act::Open(View::Form { set: s, form: f }),
                                _ => Act::Locked("That page isn't available here."),
                            };
                            let mut r = Row::new(prompt, act);
                            r.value = String::from(">");
                            r
                        }
                        _ => {
                            let act = match db.editable(set, it) {
                                Ok(()) => Act::Edit { set, form, item: i },
                                Err(l) => Act::Locked(l.why()),
                            };
                            let mut r = Row::new(prompt, act);
                            r.value = db.value_text(set, it, true);
                            if db.value_text(set, it, false) != r.value {
                                r.value.insert(0, '*');
                            }
                            r
                        }
                    };
                    r.help = help;
                    if r.label.is_empty() {
                        continue;
                    }
                    v.push(r);
                }
                if v.is_empty() {
                    v.push(Row::new("Nothing to show on this page.", Act::None));
                }
            }
            View::Review => {
                let Some(db) = &self.db else { return v };
                for c in db.changes() {
                    let mut r = Row::new(c.prompt, Act::None);
                    r.value = format!("{} > {}", c.old, c.new);
                    v.push(r);
                }
                if db.needs_reset() {
                    v.push(Row::new("Some take effect after a restart.", Act::None));
                }
                let mut r = Row::new("Save to the firmware", Act::Save);
                r.help = format!("The old values are backed up to {DIR} first.");
                v.push(r);
                v.push(Row::new("Keep editing", Act::Back));
            }
            View::Backups(files) => {
                if files.is_empty() {
                    v.push(Row::new("No backups yet.", Act::None));
                }
                for f in files {
                    let mut r = Row::new(f.clone(), Act::Restore(f.clone()));
                    r.help = String::from("A puts these settings back.");
                    v.push(r);
                }
            }
            View::Restore(f) => {
                v.push(Row::new(format!("Put back the settings in {f}?"), Act::None));
                v.push(Row::new("Restore", Act::DoRestore(f.clone())));
                v.push(Row::new("Cancel", Act::Back));
            }
        }
        v
    }

    fn first_selectable(rows: &[Row], from: usize) -> usize {
        (from..rows.len()).find(|&i| rows[i].selectable()).unwrap_or(0)
    }

    fn push(&mut self, view: View) {
        self.stack.push(Level { view, sel: 0, top: 0 });
        let rows = self.rows();
        let l = self.stack.last_mut().unwrap();
        l.sel = Self::first_selectable(&rows, 0);
    }

    /// One frame of input.
    pub fn update(&mut self, p: &Pad, fw: &mut dyn Firmware, vols: &mut dyn Volumes, now: [i32; 6]) -> Out {
        let mut out = Out::default();
        if self.db.is_none() && self.error.is_none() {
            self.load(fw);
        }
        let rows = self.rows();
        let n = rows.len();
        {
            let l = self.stack.last_mut().unwrap();
            if !rows.get(l.sel).is_some_and(Row::selectable) {
                l.sel = Self::first_selectable(&rows, l.sel.min(n.saturating_sub(1)));
            }
            if p.up {
                if let Some(i) = (0..l.sel).rev().find(|&i| rows[i].selectable()) {
                    l.sel = i;
                    sound::play(se::CURSOR);
                } else {
                    // show the headings above the first row
                    l.top = 0;
                }
            }
            if p.down {
                if let Some(i) = (l.sel + 1..n).find(|&i| rows[i].selectable()) {
                    l.sel = i;
                    sound::play(se::CURSOR);
                }
            }
            if l.sel < l.top {
                l.top = l.sel;
            }
            if l.sel >= l.top + ROWS {
                l.top = l.sel + 1 - ROWS;
            }
        }
        let sel = self.stack.last().unwrap().sel;
        let act = rows.get(sel).map_or(Act::None, |r| r.act.clone());
        if p.b {
            sound::play(se::CANCEL);
            if self.stack.len() > 1 {
                self.stack.pop();
            } else {
                out.leave = true;
            }
            return out;
        }
        if let Act::Edit { set, form, item } = act {
            let dir = if p.left { -1 } else if p.right || p.a { 1 } else { 0 };
            if dir != 0 {
                let db = self.db.as_mut().unwrap();
                if db.step(set, form, item, dir) {
                    sound::play(se::CURSOR_H);
                } else {
                    sound::play(se::BUZZER);
                }
            }
            return out;
        }
        if !p.a {
            return out;
        }
        match act {
            Act::None | Act::Edit { .. } => {}
            Act::Locked(why) => {
                sound::play(se::BUZZER);
                out.note = Some(String::from(why));
            }
            Act::System => {
                sound::play(se::DECIDE);
                out.system = true;
            }
            Act::Open(v) => {
                sound::play(se::DECIDE);
                self.push(v);
            }
            Act::Review => {
                sound::play(se::DECIDE);
                self.push(View::Review);
            }
            Act::Discard => {
                sound::play(se::CANCEL);
                if let Some(db) = self.db.as_mut() {
                    db.discard();
                }
                out.note = Some(String::from("Changes discarded."));
            }
            Act::Back => {
                sound::play(se::CANCEL);
                self.stack.pop();
            }
            Act::Backups => {
                sound::play(se::DECIDE);
                let files = boot_volume(vols)
                    .and_then(|id| vols.list(&id, DIR).ok())
                    .map(|v| {
                        let mut f: Vec<String> =
                            v.into_iter().filter(|e| !e.dir && e.name.to_ascii_lowercase().ends_with(".cfg")).map(|e| e.name).collect();
                        // newest first: the names carry the time
                        f.sort_unstable_by(|a, b| b.cmp(a));
                        f
                    })
                    .unwrap_or_default();
                self.push(View::Backups(files));
            }
            Act::Restore(f) => {
                sound::play(se::DECIDE);
                self.push(View::Restore(f));
            }
            Act::Export => {
                out.note = Some(match self.export(fw, vols) {
                    Ok(()) => {
                        sound::play(se::CONFIRM);
                        format!("Saved hii.bin and config.txt to {DIR}.")
                    }
                    Err(e) => {
                        sound::play(se::BUZZER);
                        e
                    }
                });
            }
            Act::Save => {
                out.note = Some(match self.save(fw, vols, now) {
                    Ok(reset) => {
                        sound::play(se::CONFIRM);
                        self.stack.truncate(1);
                        String::from(if reset { "Saved. Restart to use the new settings." } else { "Saved." })
                    }
                    Err(e) => {
                        sound::play(se::BUZZER);
                        e
                    }
                });
            }
            Act::DoRestore(f) => {
                out.note = Some(match self.restore(fw, vols, &f) {
                    Ok(()) => {
                        sound::play(se::CONFIRM);
                        self.stack.truncate(1);
                        String::from("Restored. Restart to use those settings.")
                    }
                    Err(e) => {
                        sound::play(se::BUZZER);
                        e
                    }
                });
            }
        }
        out
    }

    /// Back up the old values, then route the new ones. Ok(needs restart).
    fn save(&mut self, fw: &mut dyn Firmware, vols: &mut dyn Volumes, now: [i32; 6]) -> Result<bool, String> {
        let db = self.db.as_ref().ok_or_else(|| String::from("Nothing loaded."))?;
        let (new, old) = db.route_strings();
        if new.is_empty() {
            return Err(String::from("Nothing to save."));
        }
        let reset = db.needs_reset();
        let id = boot_volume(vols).ok_or_else(|| String::from("No boot drive for the backup: not saved."))?;
        let [y, mo, d, h, mi, s] = now;
        let name = format!("backup-{y:04}{mo:02}{d:02}-{h:02}{mi:02}{s:02}.cfg");
        let _ = vols.mkdir(&id, "\\EFI\\GameCoob");
        let _ = vols.mkdir(&id, DIR);
        vols.write(&id, &join(DIR, &name), old.as_bytes()).map_err(|e| format!("Backup failed ({e}): not saved."))?;
        fw.route_config(&new)?;
        // Read back what the firmware now has.
        self.db = None;
        self.load(fw);
        Ok(reset)
    }

    fn restore(&mut self, fw: &mut dyn Firmware, vols: &mut dyn Volumes, file: &str) -> Result<(), String> {
        let id = boot_volume(vols).ok_or_else(|| String::from("No boot drive."))?;
        let b = vols.read(&id, &join(DIR, file))?;
        let s = core::str::from_utf8(&b).map_err(|_| String::from("That backup is damaged."))?;
        fw.route_config(s.trim())?;
        self.db = None;
        self.load(fw);
        Ok(())
    }

    fn export(&mut self, fw: &mut dyn Firmware, vols: &mut dyn Volumes) -> Result<(), String> {
        let id = boot_volume(vols).ok_or_else(|| String::from("No boot drive."))?;
        let hii = fw.hii_export()?;
        let cfg = fw.config_export().unwrap_or_default();
        let _ = vols.mkdir(&id, "\\EFI\\GameCoob");
        let _ = vols.mkdir(&id, DIR);
        vols.write(&id, &join(DIR, "hii.bin"), &hii)?;
        vols.write(&id, &join(DIR, "config.txt"), cfg.as_bytes())?;
        Ok(())
    }

    /// The page over the backdrop (584 x 448 layout units).
    pub fn draw<'a>(&self, ui: &mut Ui<'a, '_>, ma: &'a MenuAssets, a: f32) {
        let (white, shadow) = (with_a(WHITE, a), with_a(SHADOW, a));
        let gold = with_a([1.0, 0.93, 0.6, 1.0], a);
        let dim = with_a([0.62, 0.66, 0.74, 1.0], a);
        let title = self.title();
        if ma.label(&title).is_some() {
            word_label(ui, ma, 292.0, 62.0, 1.0, &title, white);
        } else {
            let t = cp1252(&clip(&title, 30));
            ui.label(&ma.font, 294.0, 50.0, 30.0, &t, shadow, 0);
            ui.label(&ma.font, 292.0, 48.0, 30.0, &t, white, 0);
        }
        let rows = self.rows();
        let l = self.stack.last().unwrap();
        let size = 18.0;
        let (x0, x1) = (78.0, 516.0);
        for (k, r) in rows.iter().enumerate().skip(l.top).take(ROWS) {
            let y = 104.0 + (k - l.top) as f32 * 26.0;
            let on = k == l.sel && r.selectable();
            let locked = matches!(r.act, Act::Locked(_));
            let c = if on {
                gold
            } else if r.head {
                with_a([0.75, 0.85, 1.0, 1.0], a)
            } else if r.act == Act::None || locked {
                dim
            } else {
                white
            };
            let v = cp1252(&clip(&r.value, 22));
            let room = x1 - x0 - if r.value.is_empty() { 0.0 } else { ma.font.width(&v, size, 0.0) + 20.0 };
            let label = fit(ma, &r.label, size, room);
            ui.label(&ma.font, x0 + 2.0, y + 2.0, size, &label, shadow, 2);
            ui.label(&ma.font, x0, y, size, &label, c, 2);
            if !r.value.is_empty() {
                let vc = if on { gold } else if locked { dim } else { white };
                ui.label(&ma.font, x1 + 2.0, y + 2.0, size, &v, shadow, 1);
                ui.label(&ma.font, x1, y, size, &v, vc, 1);
            }
            if on {
                ui.label(&ma.font, x0 - 16.0, y, size, b">", gold, 2);
            }
        }
        // more above / below
        if l.top > 0 {
            ui.label(&ma.font, 292.0, 86.0, 14.0, b"...", dim, 0);
        }
        if l.top + ROWS < rows.len() {
            ui.label(&ma.font, 292.0, 104.0 + ROWS as f32 * 26.0 - 6.0, 14.0, b"...", dim, 0);
        }
        // The selected row's help, or why it is locked.
        if let Some(r) = rows.get(l.sel) {
            let help = match &r.act {
                Act::Locked(why) => String::from(*why),
                _ if matches!(r.act, Act::Edit { .. }) && r.help.trim().is_empty() => String::from("Left / Right change it."),
                _ => r.help.clone(),
            };
            let help = help.split(['\n', '\r']).map(str::trim).filter(|s| !s.is_empty()).collect::<Vec<_>>().join(" ");
            if !help.is_empty() {
                ui.label(&ma.font, 292.0, 346.0, 15.0, &cp1252(&clip(&help, 72)), dim, 0);
            }
        }
    }
}

/// `s` cut to `n` characters, with "..." when cut.
fn clip(s: &str, n: usize) -> String {
    if s.chars().count() <= n {
        return String::from(s);
    }
    let mut t: String = s.chars().take(n.saturating_sub(3)).collect();
    t.push_str("...");
    t
}

/// `s` in the ROM font, cut with "..." to fit `w` layout units.
fn fit(ma: &MenuAssets, s: &str, size: f32, w: f32) -> Vec<u8> {
    let full = cp1252(s);
    if ma.font.width(&full, size, 0.0) <= w {
        return full;
    }
    let mut n = s.chars().count();
    while n > 4 {
        n -= 1;
        let t = cp1252(&clip(s, n));
        if ma.font.width(&t, size, 0.0) <= w {
            return t;
        }
    }
    cp1252(&clip(s, 4))
}

fn boot_volume(vols: &mut dyn Volumes) -> Option<String> {
    vols.volumes().into_iter().find(|v| v.boot).map(|v| v.id)
}
