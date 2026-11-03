//! Launching games in a PC emulator from the boot menu (docs/EMULATORS.md).
//!
//! The boot app cannot run an emulator, and it can only read the FAT EFI
//! system partition (ESP). So the work is split: a helper on each OS
//! (`tools/gamecoob-launch`) scans the ROM folders and writes the game
//! list to the ESP; the menu lists it, and a pick is stored in the
//! `GameCoobLaunch` UEFI variable before the PC boots that OS, where the
//! helper starts the emulator.
//!
//! This module holds the three files both sides share, all in the small
//! TOML of [`toml`]:
//! - [`Paths`]: `\EFI\GameCoob\paths.toml`, written by the user: where the
//!   boot app's files live, which OSes there are, the emulators and the
//!   ROM folders.
//! - [`Library`]: `\EFI\GameCoob\library.toml`, written by the helper.
//! - [`Request`]: the `GameCoobLaunch` variable, written by the boot app.
//!
//! Nothing here touches a disk or a variable: text in, values out.

pub mod toml;

#[cfg(test)]
mod tests;

use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec::Vec;
use toml::{quote, Entry, Error, Table, Value};

/// Where the paths file lives on the ESP.
pub const PATHS_FILE: &str = "\\EFI\\GameCoob\\paths.toml";
/// Where the helper writes the game list on the ESP.
pub const LIBRARY_FILE: &str = "\\EFI\\GameCoob\\library.toml";
/// The UEFI variable (under the app's vendor GUID) holding a [`Request`].
pub const REQUEST_VAR: &str = "GameCoobLaunch";

/// File extensions a `[[roms]]` folder scans when it lists none.
pub const DEFAULT_EXTS: [&str; 7] = ["iso", "gcm", "rvz", "ciso", "gcz", "wbfs", "dol"];

/// The kind of OS, which decides how the helper is installed and how
/// paths are spelled.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum OsKind {
    Windows,
    Linux,
}

/// `[[os]]`: an OS the PC can boot into.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Os {
    /// Short name the other tables refer to, e.g. `windows` or `arch`.
    pub id: String,
    pub kind: OsKind,
    /// Text in the description of the firmware boot entry (`Boot####`)
    /// that starts this OS, e.g. `Windows Boot Manager`; the boot app sets
    /// BootNext to the first entry containing it.
    pub boot: String,
    /// Where this OS mounts the ESP, for the helper (`/efi`, `/boot`,
    /// `/boot/efi`). Unset: the helper looks in the usual places (Windows
    /// mounts it itself).
    pub esp: Option<String>,
}

/// `[[emulator]]`: a program on one OS that runs games.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Emulator {
    pub id: String,
    /// Shown in the menu.
    pub name: String,
    /// The [`Os::id`] it is installed on.
    pub os: String,
    /// Full path of the program.
    pub exe: String,
    /// Arguments; `{rom}` becomes the game's path (see [`Emulator::command`]).
    pub args: Vec<String>,
    /// The emulator's memory card folder, for card syncing (later step).
    pub cards: Option<String>,
}

impl Emulator {
    /// The program and its arguments for a game.
    pub fn command(&self, rom: &str) -> (String, Vec<String>) {
        let args = self.args.iter().map(|a| a.replace("{rom}", rom)).collect();
        (self.exe.clone(), args)
    }
}

/// `[[roms]]`: a folder of games on one OS, played with one emulator.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Roms {
    pub os: String,
    pub path: String,
    pub emulator: String,
    /// Extensions to list, lower case, without the dot.
    pub ext: Vec<String>,
    /// Also scan folders inside `path`.
    pub recursive: bool,
}

/// `[boot]`: the boot app's own files on the ESP.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Boot {
    /// The user's IPL dump.
    pub ipl: String,
    /// Memory card images (`slot-a.raw` ...).
    pub cards: String,
    /// Where the card explorer exports saves.
    pub saves: String,
}

impl Default for Boot {
    fn default() -> Self {
        Boot {
            ipl: "\\EFI\\GameCoob\\ipl.bin".to_string(),
            cards: "\\EFI\\GameCoob\\cards".to_string(),
            saves: "\\EFI\\GameCoob\\saves".to_string(),
        }
    }
}

/// The whole paths file. Every part is optional: an empty file gives the
/// built-in [`Boot`] paths and no emulators.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Paths {
    pub boot: Boot,
    pub os: Vec<Os>,
    pub emulator: Vec<Emulator>,
    pub roms: Vec<Roms>,
}

impl Paths {
    /// Parses and checks a paths file. All problems found are returned, so
    /// `gamecoob-launch check` can list them at once.
    pub fn parse(text: &str) -> Result<Paths, Vec<Error>> {
        let tables = toml::parse(text).map_err(|e| Vec::from([e]))?;
        let mut errs = Vec::new();
        let mut paths = Paths::default();
        for t in &tables {
            match (t.name.as_str(), t.array) {
                ("", _) => no_keys(t, &mut errs),
                ("boot", false) => {
                    let mut r = Reader::new(t, &mut errs);
                    if let Some(v) = r.opt_str("ipl") {
                        paths.boot.ipl = v;
                    }
                    if let Some(v) = r.opt_str("cards") {
                        paths.boot.cards = v;
                    }
                    if let Some(v) = r.opt_str("saves") {
                        paths.boot.saves = v;
                    }
                    r.done();
                }
                ("os", true) => {
                    let mut r = Reader::new(t, &mut errs);
                    let id = r.str("id");
                    let kind = match r.opt_str("kind").as_deref() {
                        Some("windows") => Some(OsKind::Windows),
                        Some("linux") => Some(OsKind::Linux),
                        // Guess from the id so the common cases need no `kind`.
                        None if id.to_ascii_lowercase().contains("windows") => Some(OsKind::Windows),
                        None => Some(OsKind::Linux),
                        Some(other) => {
                            r.fail("kind", format!("kind must be \"windows\" or \"linux\", not \"{}\"", other));
                            None
                        }
                    };
                    let boot = r.str("boot");
                    let esp = r.opt_str("esp");
                    r.done();
                    if let Some(kind) = kind {
                        paths.os.push(Os { id, kind, boot, esp });
                    }
                }
                ("emulator", true) => {
                    let mut r = Reader::new(t, &mut errs);
                    let id = r.str("id");
                    let name = r.opt_str("name").unwrap_or_else(|| id.clone());
                    let os = r.str("os");
                    let exe = r.str("exe");
                    let args = r.opt_strs("args").unwrap_or_else(|| Vec::from(["{rom}".to_string()]));
                    let cards = r.opt_str("cards");
                    r.done();
                    paths.emulator.push(Emulator { id, name, os, exe, args, cards });
                }
                ("roms", true) => {
                    let mut r = Reader::new(t, &mut errs);
                    let os = r.str("os");
                    let path = r.str("path");
                    let emulator = r.str("emulator");
                    let ext = r
                        .opt_strs("ext")
                        .map(|v| v.into_iter().map(|e| e.trim_start_matches('.').to_ascii_lowercase()).collect())
                        .unwrap_or_else(|| DEFAULT_EXTS.iter().map(|e| e.to_string()).collect());
                    let recursive = r.opt_bool("recursive").unwrap_or(false);
                    r.done();
                    paths.roms.push(Roms { os, path, emulator, ext, recursive });
                }
                (name, array) => {
                    let (open, close) = if array { ("[[", "]]") } else { ("[", "]") };
                    errs.push(Error::new(
                        t.line,
                        format!("unknown table {}{}{} (expected [boot], [[os]], [[emulator]] or [[roms]])", open, name, close),
                    ));
                }
            }
        }
        paths.check_refs(&tables, &mut errs);
        if errs.is_empty() { Ok(paths) } else { Err(errs) }
    }

    /// Ids are unique and every `os`/`emulator` named exists, on the
    /// right OS.
    fn check_refs(&self, tables: &[Table], errs: &mut Vec<Error>) {
        let line_of = |name: &str, n: usize, key: &str| {
            tables
                .iter()
                .filter(|t| t.name == name)
                .nth(n)
                .and_then(|t| t.get(key).map(|e| e.line).or(Some(t.line)))
                .unwrap_or(0)
        };
        for (n, o) in self.os.iter().enumerate() {
            if self.os[..n].iter().any(|p| p.id == o.id) {
                errs.push(Error::new(line_of("os", n, "id"), format!("os id \"{}\" is used twice", o.id)));
            }
        }
        for (n, e) in self.emulator.iter().enumerate() {
            if self.emulator[..n].iter().any(|p| p.id == e.id) {
                errs.push(Error::new(line_of("emulator", n, "id"), format!("emulator id \"{}\" is used twice", e.id)));
            }
            if self.os_by_id(&e.os).is_none() {
                errs.push(Error::new(line_of("emulator", n, "os"), format!("no [[os]] has id \"{}\"", e.os)));
            }
        }
        for (n, r) in self.roms.iter().enumerate() {
            if self.os_by_id(&r.os).is_none() {
                errs.push(Error::new(line_of("roms", n, "os"), format!("no [[os]] has id \"{}\"", r.os)));
            }
            match self.emulator_by_id(&r.emulator) {
                None => errs.push(Error::new(
                    line_of("roms", n, "emulator"),
                    format!("no [[emulator]] has id \"{}\"", r.emulator),
                )),
                Some(e) if e.os != r.os => errs.push(Error::new(
                    line_of("roms", n, "emulator"),
                    format!("emulator \"{}\" is on os \"{}\", but these roms are on \"{}\"", e.id, e.os, r.os),
                )),
                Some(_) => {}
            }
        }
    }

    pub fn os_by_id(&self, id: &str) -> Option<&Os> {
        self.os.iter().find(|o| o.id == id)
    }

    pub fn emulator_by_id(&self, id: &str) -> Option<&Emulator> {
        self.emulator.iter().find(|e| e.id == id)
    }
}

/// `[[game]]` in the library: one game the helper found.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Game {
    /// From the disc header when the helper can read it, else the file name.
    pub title: String,
    /// The 6-character game code (e.g. `GALE01`), when known.
    pub code: Option<String>,
    pub os: String,
    pub emulator: String,
    /// Full path on that OS.
    pub path: String,
}

/// The game list, as the helper last wrote it.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Library {
    pub games: Vec<Game>,
}

impl Library {
    pub fn parse(text: &str) -> Result<Library, Vec<Error>> {
        let tables = toml::parse(text).map_err(|e| Vec::from([e]))?;
        let mut errs = Vec::new();
        let mut lib = Library::default();
        for t in &tables {
            match (t.name.as_str(), t.array) {
                ("", _) => no_keys(t, &mut errs),
                ("game", true) => {
                    let mut r = Reader::new(t, &mut errs);
                    let game = Game {
                        title: r.str("title"),
                        code: r.opt_str("code"),
                        os: r.str("os"),
                        emulator: r.str("emulator"),
                        path: r.str("path"),
                    };
                    r.done();
                    lib.games.push(game);
                }
                _ => errs.push(Error::new(t.line, format!("unknown table {} (expected [[game]])", t.name))),
            }
        }
        if errs.is_empty() { Ok(lib) } else { Err(errs) }
    }

    pub fn to_toml(&self) -> String {
        let mut out = String::from("# Written by gamecoob-launch; edits are lost on its next scan.\n");
        for g in &self.games {
            out.push_str("\n[[game]]\n");
            out.push_str(&format!("title = {}\n", quote(&g.title)));
            if let Some(code) = &g.code {
                out.push_str(&format!("code = {}\n", quote(code)));
            }
            out.push_str(&format!("os = {}\n", quote(&g.os)));
            out.push_str(&format!("emulator = {}\n", quote(&g.emulator)));
            out.push_str(&format!("path = {}\n", quote(&g.path)));
        }
        out
    }
}

/// What the boot app asks the helper to start, stored in [`REQUEST_VAR`].
/// The helper remembers the last `count` it ran, so each request runs once
/// without having to delete the variable (which needs admin on Linux).
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Request {
    /// Bumped by the boot app on every launch.
    pub count: u32,
    pub os: String,
    pub emulator: String,
    pub path: String,
    pub title: String,
}

impl Request {
    pub const VERSION: i64 = 1;

    pub fn parse(text: &str) -> Result<Request, Vec<Error>> {
        let tables = toml::parse(text).map_err(|e| Vec::from([e]))?;
        let mut errs = Vec::new();
        let Some(root) = tables.first() else { return Err(errs) };
        if let Some(t) = tables.get(1) {
            errs.push(Error::new(t.line, "a launch request has no tables"));
        }
        let mut r = Reader::new(root, &mut errs);
        let version = r.int("version");
        let count = r.int("count");
        let req = Request {
            count: count.clamp(0, u32::MAX as i64) as u32,
            os: r.str("os"),
            emulator: r.str("emulator"),
            path: r.str("path"),
            title: r.opt_str("title").unwrap_or_default(),
        };
        r.done();
        if version != Self::VERSION && errs.is_empty() {
            errs.push(Error::new(0, format!("launch request version {} is not supported", version)));
        }
        if errs.is_empty() { Ok(req) } else { Err(errs) }
    }

    pub fn to_toml(&self) -> String {
        format!(
            "version = {}\ncount = {}\nos = {}\nemulator = {}\npath = {}\ntitle = {}\n",
            Self::VERSION,
            self.count,
            quote(&self.os),
            quote(&self.emulator),
            quote(&self.path),
            quote(&self.title)
        )
    }
}

fn no_keys(t: &Table, errs: &mut Vec<Error>) {
    if let Some(e) = t.entries.first() {
        errs.push(Error::new(e.line, format!("{} must be inside a table such as [boot]", e.key)));
    }
}

/// Reads typed keys from one table, collecting errors, then refuses any
/// key it was not asked for (a typo'd key is an error, not ignored).
struct Reader<'a> {
    t: &'a Table,
    errs: &'a mut Vec<Error>,
    used: Vec<&'a str>,
}

impl<'a> Reader<'a> {
    fn new(t: &'a Table, errs: &'a mut Vec<Error>) -> Self {
        Reader { t, errs, used: Vec::new() }
    }

    fn entry(&mut self, key: &'a str) -> Option<&'a Entry> {
        self.used.push(key);
        self.t.get(key)
    }

    fn fail(&mut self, key: &str, msg: String) {
        let line = self.t.get(key).map(|e| e.line).unwrap_or(self.t.line);
        self.errs.push(Error::new(line, msg));
    }

    fn missing(&mut self, key: &str) {
        let what = if self.t.name.is_empty() { String::new() } else { format!(" in [[{}]]", self.t.name) };
        self.errs.push(Error::new(self.t.line, format!("{} is missing{}", key, what)));
    }

    fn opt_str(&mut self, key: &'a str) -> Option<String> {
        match &self.entry(key)?.value {
            Value::Str(s) => Some(s.clone()),
            _ => {
                self.fail(key, format!("{} must be text in quotes", key));
                None
            }
        }
    }

    fn str(&mut self, key: &'a str) -> String {
        if self.t.get(key).is_none() {
            self.used.push(key);
            self.missing(key);
            return String::new();
        }
        self.opt_str(key).unwrap_or_default()
    }

    fn opt_strs(&mut self, key: &'a str) -> Option<Vec<String>> {
        let v = &self.entry(key)?.value;
        let strs = match v {
            Value::Array(items) => items.iter().map(|i| i.as_str().map(String::from)).collect::<Option<Vec<_>>>(),
            _ => None,
        };
        if strs.is_none() {
            self.fail(key, format!("{} must be a list of text, like [\"a\", \"b\"]", key));
        }
        strs
    }

    fn opt_bool(&mut self, key: &'a str) -> Option<bool> {
        match &self.entry(key)?.value {
            Value::Bool(b) => Some(*b),
            _ => {
                self.fail(key, format!("{} must be true or false", key));
                None
            }
        }
    }

    fn int(&mut self, key: &'a str) -> i64 {
        match self.entry(key).map(|e| &e.value) {
            Some(Value::Int(n)) => *n,
            Some(_) => {
                self.fail(key, format!("{} must be a whole number", key));
                0
            }
            None => {
                self.missing(key);
                0
            }
        }
    }

    fn done(self) {
        for e in &self.t.entries {
            if !self.used.contains(&e.key.as_str()) {
                let table = if self.t.name.is_empty() { "here".to_string() } else { format!("in {}", self.t.name) };
                self.errs.push(Error::new(e.line, format!("unknown key {} {}", e.key, table)));
            }
        }
    }
}
