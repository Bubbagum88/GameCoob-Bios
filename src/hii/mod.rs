//! The firmware's own setup options, read the way its setup screen reads
//! them: the HII database's forms (IFR) and strings say what each option
//! is, its choices and where its value lives; the config routing
//! protocol's export says the current values; changes go back through the
//! same protocol to the driver that owns them. Never raw offsets into a
//! guessed variable.
//!
//! This module is plain data and logic (no firmware calls), so the host
//! tools and tests run it on a saved export. The UEFI side
//! (`src/setup.rs`) fetches the exports and routes the changes.

pub mod cfg;
pub mod ifr;
pub mod pkg;
#[cfg(test)]
mod tests;

use alloc::collections::BTreeMap;
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;
use ifr::{CondKind, FormSet, Item, Kind, Question};
use pkg::Strings;

/// What the firmware side provides (`src/setup.rs` on UEFI).
pub trait Firmware {
    /// `ExportPackageLists` of every package list.
    fn hii_export(&mut self) -> Result<Vec<u8>, String>;
    /// `ExportConfig`: every store's current values.
    fn config_export(&mut self) -> Result<String, String>;
    /// A UEFI variable (name, vendor GUID).
    fn variable(&mut self, name: &str, guid: &[u8; 16]) -> Option<Vec<u8>>;
    /// `RouteConfig`: hand a `<MultiConfigResp>` to its drivers.
    fn route_config(&mut self, cfg: &str) -> Result<(), String>;
}

/// No firmware (host tools without an export to show).
pub struct NoFirmware;

impl Firmware for NoFirmware {
    fn hii_export(&mut self) -> Result<Vec<u8>, String> {
        Err(String::from("No firmware forms here."))
    }
    fn config_export(&mut self) -> Result<String, String> {
        Err(String::from("No firmware here."))
    }
    fn variable(&mut self, _name: &str, _guid: &[u8; 16]) -> Option<Vec<u8>> {
        None
    }
    fn route_config(&mut self, _cfg: &str) -> Result<(), String> {
        Err(String::from("No firmware here."))
    }
}

/// A variable store, shared by every form set that names it (vendor setups
/// spread one `Setup` variable over many form sets).
#[derive(Clone, Debug)]
pub struct Store {
    pub guid: [u8; 16],
    pub name: String,
    pub efi: bool,
    /// The routing header from the export; None when the export did not
    /// list this store (then it is shown, never written).
    pub hdr: Option<String>,
    /// The values as read, and as edited.
    pub orig: Vec<u8>,
    pub cur: Vec<u8>,
    /// Which bytes were read at all.
    pub have: Vec<bool>,
}

/// One form set and what it needs to show its questions.
pub struct Set {
    pub fs: FormSet,
    /// Index into [`Db::strings`].
    pub strings: usize,
    /// Its varstore ids and the [`Db::stores`] each is.
    pub stores: Vec<(u16, usize)>,
    pub questions: BTreeMap<u16, Question>,
}

/// Everything the firmware's setup knows.
pub struct Db {
    pub strings: Vec<Strings>,
    pub sets: Vec<Set>,
    pub stores: Vec<Store>,
}

/// A value change waiting to be saved.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Change {
    pub set: usize,
    pub form: usize,
    pub item: usize,
    pub prompt: String,
    pub old: String,
    pub new: String,
}

/// Why a question can't be changed here.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Locked {
    /// Not a kind GameCoob edits (strings, dates, passwords, actions...).
    Kind,
    /// Read-only, or greyed out by its form right now.
    ReadOnly,
    /// The driver wants to see the change as it happens (a callback):
    /// only its own setup screen can do that.
    Callback,
    /// The value couldn't be read, or the store can't be written back.
    NoValue,
}

impl Locked {
    pub fn why(self) -> &'static str {
        match self {
            Locked::Kind => "Shown only: change it in the firmware's setup.",
            Locked::ReadOnly => "Locked by the firmware right now.",
            Locked::Callback => "Needs the firmware's own setup screen.",
            Locked::NoValue => "The firmware didn't share this value.",
        }
    }
}

const EMPTY: &str = "";

impl Db {
    /// Build from `ExportPackageLists` bytes and the `ExportConfig`
    /// string. `var` reads a UEFI variable (name, vendor GUID) for stores
    /// the export left out, so they at least show their values.
    pub fn load(hii: &[u8], config: &str, var: &mut dyn FnMut(&str, &[u8; 16]) -> Option<Vec<u8>>) -> Db {
        let mut db = Db { strings: Vec::new(), sets: Vec::new(), stores: Vec::new() };
        for pl in pkg::parse(hii) {
            let si = db.strings.len();
            for f in &pl.forms {
                for fs in ifr::parse(f) {
                    let mut questions = BTreeMap::new();
                    for fm in &fs.forms {
                        for it in &fm.items {
                            if let Some(q) = it.q {
                                questions.entry(q.id).or_insert(q);
                            }
                        }
                    }
                    let mut stores = Vec::new();
                    for vs in fs.varstores.iter().filter(|v| !v.name_value) {
                        let k = match db.stores.iter().position(|s| s.guid == vs.guid && s.name == vs.name) {
                            Some(k) => k,
                            None => {
                                db.stores.push(Store {
                                    guid: vs.guid,
                                    name: vs.name.clone(),
                                    efi: vs.efi,
                                    hdr: None,
                                    orig: alloc::vec![0; usize::from(vs.size)],
                                    cur: Vec::new(),
                                    have: alloc::vec![false; usize::from(vs.size)],
                                });
                                db.stores.len() - 1
                            }
                        };
                        stores.push((vs.id, k));
                    }
                    db.sets.push(Set { fs, strings: si, stores, questions });
                }
            }
            db.strings.push(pl.strings);
        }
        for r in cfg::parse(config).into_iter().filter(|r| !r.alt) {
            let Some(s) = db.stores.iter_mut().find(|s| s.guid == r.guid && s.name == r.name) else { continue };
            for (o, b) in &r.blocks {
                let end = o + b.len();
                if s.orig.len() < end {
                    s.orig.resize(end, 0);
                    s.have.resize(end, false);
                }
                s.orig[*o..end].copy_from_slice(b);
                s.have[*o..end].iter_mut().for_each(|h| *h = true);
            }
            s.hdr = Some(r.hdr.clone());
        }
        for s in db.stores.iter_mut().filter(|s| s.hdr.is_none() && !s.name.is_empty()) {
            if let Some(v) = var(&s.name, &s.guid) {
                let n = v.len().max(s.orig.len());
                s.orig.resize(n, 0);
                s.have.resize(n, false);
                s.orig[..v.len()].copy_from_slice(&v);
                s.have[..v.len()].iter_mut().for_each(|h| *h = true);
            }
        }
        for s in db.stores.iter_mut() {
            s.cur = s.orig.clone();
        }
        db
    }

    pub fn str(&self, set: usize, id: u16) -> &str {
        match self.sets.get(set) {
            Some(s) if id != 0 => self.strings[s.strings].get(id),
            _ => EMPTY,
        }
    }

    /// Form sets worth listing: titled, with something to show on their
    /// first form (pages the firmware fills in only while its own setup
    /// runs are empty here), first copy of each GUID and title.
    pub fn menu_sets(&self) -> Vec<usize> {
        let mut seen: Vec<([u8; 16], &str)> = Vec::new();
        let mut v = Vec::new();
        for (i, s) in self.sets.iter().enumerate() {
            let title = self.str(i, s.fs.title).trim();
            let Some(first) = s.fs.forms.first() else { continue };
            let shows = first.items.iter().any(|it| !self.str(i, it.prompt).trim().is_empty() && !self.hidden(i, &it.conds));
            if title.is_empty() || !shows || seen.iter().any(|(g, t)| *g == s.fs.guid || *t == title) {
                continue;
            }
            seen.push((s.fs.guid, title));
            v.push(i);
        }
        v
    }

    pub fn find_set(&self, guid: &[u8; 16]) -> Option<usize> {
        self.sets.iter().position(|s| &s.fs.guid == guid)
    }

    pub fn find_form(&self, set: usize, id: u16) -> Option<usize> {
        self.sets.get(set)?.fs.forms.iter().position(|f| f.id == id)
    }

    fn store(&self, set: usize, q: &Question) -> Option<usize> {
        if q.vsid == 0 || q.bits || q.width == 0 {
            return None;
        }
        self.sets.get(set)?.stores.iter().find(|(id, _)| *id == q.vsid).map(|&(_, k)| k)
    }

    fn read(buf: &[u8], have: &[bool], q: &Question) -> Option<u64> {
        let (o, w) = (usize::from(q.offset), usize::from(q.width));
        if !have.get(o..o + w)?.iter().all(|&h| h) {
            return None;
        }
        let mut v = [0u8; 8];
        v[..w].copy_from_slice(buf.get(o..o + w)?);
        Some(u64::from_le_bytes(v))
    }

    /// A question's value as edited.
    pub fn value(&self, set: usize, q: &Question) -> Option<u64> {
        let s = &self.stores[self.store(set, q)?];
        Self::read(&s.cur, &s.have, q)
    }

    /// A question's value as the firmware has it.
    pub fn orig_value(&self, set: usize, q: &Question) -> Option<u64> {
        let s = &self.stores[self.store(set, q)?];
        Self::read(&s.orig, &s.have, q)
    }

    fn set_value(&mut self, set: usize, q: &Question, v: u64) -> bool {
        let Some(k) = self.store(set, q) else { return false };
        let (o, w) = (usize::from(q.offset), usize::from(q.width));
        let s = &mut self.stores[k];
        match s.cur.get_mut(o..o + w) {
            Some(dst) => {
                dst.copy_from_slice(&v.to_le_bytes()[..w]);
                true
            }
            None => false,
        }
    }

    /// Run a condition's expression: Some(result), or None if it uses
    /// something not handled here.
    pub fn eval(&self, set: usize, cond: u16) -> Option<bool> {
        let s = self.sets.get(set)?;
        let e = &s.fs.conds.get(usize::from(cond))?.expr;
        let qv = |id: u16| s.questions.get(&id).and_then(|q| self.value(set, q));
        let mut st: Vec<u64> = Vec::new();
        let mut at = 0;
        while at + 2 <= e.len() {
            let (op, len) = (e[at], usize::from(e[at + 1] & 0x7F));
            if len < 2 || at + len > e.len() {
                return None;
            }
            let o = &e[at..at + len];
            let u16at = |i: usize| pkg::u16le(o, i);
            macro_rules! pop {
                () => {
                    st.pop()?
                };
            }
            match op {
                0x12 => st.push(u64::from(qv(u16at(2))? == u64::from(u16at(4)))),
                0x13 => st.push(u64::from(qv(u16at(2))? == qv(u16at(4))?)),
                0x14 => {
                    let v = qv(u16at(2))?;
                    let n = usize::from(u16at(4));
                    st.push(u64::from((0..n).any(|i| u64::from(u16at(6 + 2 * i)) == v)));
                }
                0x15 => {
                    let (b, a) = (pop!(), pop!());
                    st.push(u64::from(a != 0 && b != 0));
                }
                0x16 => {
                    let (b, a) = (pop!(), pop!());
                    st.push(u64::from(a != 0 || b != 0));
                }
                0x17 => {
                    let a = pop!();
                    st.push(u64::from(a == 0));
                }
                0x37 => {
                    let a = pop!();
                    st.push(!a);
                }
                0x2F..=0x3E => {
                    let (b, a) = (pop!(), pop!());
                    st.push(match op {
                        0x2F => u64::from(a == b),
                        0x30 => u64::from(a != b),
                        0x31 => u64::from(a > b),
                        0x32 => u64::from(a >= b),
                        0x33 => u64::from(a < b),
                        0x34 => u64::from(a <= b),
                        0x35 => a & b,
                        0x36 => a | b,
                        0x38 => a.checked_shl(b as u32).unwrap_or(0),
                        0x39 => a.checked_shr(b as u32).unwrap_or(0),
                        0x3A => a.wrapping_add(b),
                        0x3B => a.wrapping_sub(b),
                        0x3C => a.wrapping_mul(b),
                        0x3D => a.checked_div(b)?,
                        _ => a.checked_rem(b)?,
                    });
                }
                0x40 => st.push(qv(u16at(2))?),
                0x42 => st.push(u64::from(o.get(2).copied()?)),
                0x43 => st.push(u64::from(u16at(2))),
                0x44 => st.push(u64::from(pkg::u32le(o, 2))),
                0x45 => st.push(u64::from(pkg::u32le(o, 2)) | (u64::from(pkg::u32le(o, 6)) << 32)),
                0x46 | 0x53 => st.push(1),
                0x47 | 0x52 => st.push(0),
                0x54 => st.push(u64::MAX),
                0x48 => {}
                0x4A => {
                    let a = pop!();
                    st.push(u64::from(a != 0));
                }
                0x57 => {
                    let a = *st.last()?;
                    st.push(a);
                }
                _ => return None,
            }
            at += len;
        }
        if st.len() == 1 { st.pop().map(|v| v != 0) } else { None }
    }

    /// Hidden by a suppress-if or disable-if that is true.
    pub fn hidden(&self, set: usize, conds: &[u16]) -> bool {
        conds.iter().any(|&c| {
            let k = self.sets[set].fs.conds[usize::from(c)].kind;
            k != CondKind::GrayOut && self.eval(set, c) == Some(true)
        })
    }

    /// Greyed out by a gray-out-if that is true or can't be worked out.
    pub fn grayed(&self, set: usize, conds: &[u16]) -> bool {
        conds.iter().any(|&c| {
            let k = self.sets[set].fs.conds[usize::from(c)].kind;
            k == CondKind::GrayOut && self.eval(set, c) != Some(false)
        })
    }

    /// Ok if GameCoob may change this item now.
    pub fn editable(&self, set: usize, it: &Item) -> Result<(), Locked> {
        let Some(q) = &it.q else { return Err(Locked::Kind) };
        if !matches!(it.kind, Kind::OneOf(_) | Kind::Checkbox { .. } | Kind::Numeric { .. }) {
            return Err(Locked::Kind);
        }
        if q.flags & ifr::QF_READ_ONLY != 0 || self.grayed(set, &it.conds) {
            return Err(Locked::ReadOnly);
        }
        if q.flags & ifr::QF_CALLBACK != 0 {
            return Err(Locked::Callback);
        }
        let k = self.store(set, q).ok_or(Locked::NoValue)?;
        if self.stores[k].hdr.is_none() || self.value(set, q).is_none() {
            return Err(Locked::NoValue);
        }
        Ok(())
    }

    /// The one-of choices on show now, as (text, value).
    pub fn options(&self, set: usize, it: &Item) -> Vec<(String, u64)> {
        match &it.kind {
            Kind::OneOf(opts) => opts
                .iter()
                .filter(|o| !self.hidden(set, &o.conds))
                .map(|o| (String::from(self.str(set, o.text)), o.value))
                .collect(),
            _ => Vec::new(),
        }
    }

    /// An item's value as text, from the edited (`cur`) or firmware values.
    pub fn value_text(&self, set: usize, it: &Item, cur: bool) -> String {
        let Some(q) = &it.q else {
            return match it.kind {
                Kind::Text(t) => String::from(self.str(set, t)),
                _ => String::new(),
            };
        };
        let v = if cur { self.value(set, q) } else { self.orig_value(set, q) };
        match (&it.kind, v) {
            (Kind::OneOf(opts), Some(v)) => opts
                .iter()
                .find(|o| o.value == v)
                .map_or_else(|| format!("0x{v:X}"), |o| String::from(self.str(set, o.text))),
            (Kind::Checkbox { .. }, Some(v)) => String::from(if v != 0 { "On" } else { "Off" }),
            (Kind::Numeric { hex: true, .. }, Some(v)) => format!("0x{v:X}"),
            (Kind::Numeric { .. }, Some(v)) => format!("{v}"),
            (Kind::OneOf(_) | Kind::Checkbox { .. } | Kind::Numeric { .. }, None) => String::from("?"),
            _ => String::new(),
        }
    }

    /// Step an editable item: `dir` -1 or +1 (one-of: next shown choice;
    /// checkbox: flip; numeric: one step, within its range). True if the
    /// value changed.
    pub fn step(&mut self, set: usize, form: usize, item: usize, dir: i32) -> bool {
        let it = self.sets[set].fs.forms[form].items[item].clone();
        if self.editable(set, &it).is_err() {
            return false;
        }
        let q = it.q.unwrap();
        let Some(v) = self.value(set, &q) else { return false };
        let nv = match &it.kind {
            Kind::OneOf(_) => {
                let opts = self.options(set, &it);
                if opts.is_empty() {
                    return false;
                }
                let i = opts.iter().position(|o| o.1 == v).map_or(0, |i| i as i32 + dir);
                opts[i.rem_euclid(opts.len() as i32) as usize].1
            }
            Kind::Checkbox { .. } => u64::from(v == 0),
            Kind::Numeric { min, max, step, .. } => {
                let step = (*step).max(1);
                if dir < 0 { v.saturating_sub(step).max(*min) } else { v.saturating_add(step).min(*max) }
            }
            _ => return false,
        };
        nv != v && self.set_value(set, &q, nv)
    }

    /// Every edited question, once per stored value.
    pub fn changes(&self) -> Vec<Change> {
        let mut seen: Vec<(usize, u16)> = Vec::new();
        let mut v = Vec::new();
        for (si, s) in self.sets.iter().enumerate() {
            for (fi, f) in s.fs.forms.iter().enumerate() {
                for (ii, it) in f.items.iter().enumerate() {
                    let Some(q) = &it.q else { continue };
                    let Some(k) = self.store(si, q) else { continue };
                    if seen.contains(&(k, q.offset)) || self.value(si, q) == self.orig_value(si, q) {
                        continue;
                    }
                    seen.push((k, q.offset));
                    v.push(Change {
                        set: si,
                        form: fi,
                        item: ii,
                        prompt: String::from(self.str(si, it.prompt)),
                        old: self.value_text(si, it, false),
                        new: self.value_text(si, it, true),
                    });
                }
            }
        }
        v
    }

    pub fn pending(&self) -> bool {
        self.stores.iter().any(|s| s.cur != s.orig)
    }

    /// Does any edited question need a restart to take effect?
    pub fn needs_reset(&self) -> bool {
        self.changes().iter().any(|c| {
            let it = &self.sets[c.set].fs.forms[c.form].items[c.item];
            it.q.is_some_and(|q| q.flags & ifr::QF_RESET_REQUIRED != 0)
        })
    }

    /// The `<MultiConfigResp>` that writes the edits (only the changed
    /// bytes), and the one that puts the old bytes back (the backup).
    pub fn route_strings(&self) -> (String, String) {
        let (mut new, mut old) = (String::new(), String::new());
        for s in self.stores.iter().filter(|s| s.cur != s.orig) {
            let Some(hdr) = &s.hdr else { continue };
            let runs = cfg::diff_runs(&s.orig, &s.cur);
            for (out, buf) in [(&mut new, &s.cur), (&mut old, &s.orig)] {
                if !out.is_empty() {
                    out.push('&');
                }
                out.push_str(hdr);
                for &(o, n) in &runs {
                    out.push_str(&cfg::block(o, &buf[o..o + n]));
                }
            }
        }
        (new, old)
    }

    /// Forget the edits.
    pub fn discard(&mut self) {
        for s in self.stores.iter_mut() {
            s.cur = s.orig.clone();
        }
    }
}
