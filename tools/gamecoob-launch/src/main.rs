//! gamecoob-launch: the OS-side half of launching games from the GameCoob
//! boot menu (docs/EMULATORS.md). It shares the file formats with the boot
//! app through `src/launch`.
//!
//! ```text
//! gamecoob-launch check [paths.toml]   check a paths file and list what it sets up
//! gamecoob-launch scan                 list this OS's games into library.toml on the ESP
//! gamecoob-launch run [--dry-run]      scan, then start the game the boot menu picked, once
//! gamecoob-launch mark                 note the current launch request as handled
//!
//! options:  --esp DIR      the ESP's folder (default: found, or mounted on Windows)
//!           --os ID        this OS's [[os]] id (default: the only one of this kind)
//!           --request FILE read the launch request from a file instead of the variable
//! ```
//!
//! `run` is what the login task calls. `cargo test` here runs
//! `src/launch`'s tests.

extern crate alloc;

#[path = "../../../src/launch/mod.rs"]
#[allow(dead_code)]
mod launch;

mod disc;
mod scan;
mod sys;

use launch::{Library, OsKind, Paths, Request, LIBRARY_FILE, PATHS_FILE};
use std::path::{Path, PathBuf};
use std::process::ExitCode;

const USAGE: &str = "usage: gamecoob-launch check [paths.toml] | scan | run [--dry-run] | mark
       options: --esp DIR, --os ID, --request FILE";

#[derive(Default)]
struct Opts {
    esp: Option<String>,
    os: Option<String>,
    request: Option<String>,
    dry_run: bool,
}

fn main() -> ExitCode {
    let mut args = std::env::args().skip(1);
    let mut words = Vec::new();
    let mut o = Opts::default();
    while let Some(a) = args.next() {
        match a.as_str() {
            "--esp" => o.esp = args.next(),
            "--os" => o.os = args.next(),
            "--request" => o.request = args.next(),
            "--dry-run" => o.dry_run = true,
            "-h" | "--help" => {
                println!("{}", USAGE);
                return ExitCode::SUCCESS;
            }
            _ => words.push(a),
        }
    }
    let result = match words.iter().map(String::as_str).collect::<Vec<_>>().as_slice() {
        ["check", file] => check(Path::new(file)),
        ["check"] => sys::esp(o.esp.as_deref()).and_then(|esp| check(&on_esp(&esp.dir, PATHS_FILE))),
        ["scan"] => setup(&o).and_then(|s| scan(&s)),
        ["run"] => setup(&o).and_then(|s| scan(&s).and_then(|_| run(&s, &o))),
        ["mark"] => setup(&o).and_then(|s| mark(&s, &o)),
        _ => {
            eprintln!("{}", USAGE);
            return ExitCode::from(2);
        }
    };
    match result {
        Ok(()) => ExitCode::SUCCESS,
        Err(e) => {
            eprintln!("gamecoob-launch: {}", e);
            ExitCode::FAILURE
        }
    }
}

/// A path on the ESP given in the boot app's `\EFI\...` form.
fn on_esp(esp: &Path, path: &str) -> PathBuf {
    path.split('\\').filter(|p| !p.is_empty()).fold(esp.to_path_buf(), |p, part| p.join(part))
}

fn load_paths(file: &Path) -> Result<Paths, String> {
    let text = std::fs::read_to_string(file).map_err(|e| format!("{}: {}", file.display(), e))?;
    Paths::parse(&text).map_err(|errs| {
        errs.iter().map(|e| format!("{}: {}", file.display(), e)).collect::<Vec<_>>().join("\n")
    })
}

fn check(file: &Path) -> Result<(), String> {
    let paths = load_paths(file)?;
    println!("boot: ipl {}  cards {}  saves {}", paths.boot.ipl, paths.boot.cards, paths.boot.saves);
    for o in &paths.os {
        println!("os {} ({:?}): boot entry containing \"{}\"", o.id, o.kind, o.boot);
    }
    for e in &paths.emulator {
        let (exe, args) = e.command("<game>");
        println!("emulator {} \"{}\" on {}: {} {}", e.id, e.name, e.os, exe, args.join(" "));
    }
    for r in &paths.roms {
        let deep = if r.recursive { " and subfolders" } else { "" };
        println!("roms on {}: {}{} (.{}) with {}", r.os, r.path, deep, r.ext.join(" ."), r.emulator);
    }
    Ok(())
}

/// The ESP, the paths file on it, and which `[[os]]` this is.
struct Setup {
    esp: sys::Esp,
    paths: Paths,
    os: String,
}

fn setup(o: &Opts) -> Result<Setup, String> {
    let esp = sys::esp(o.esp.as_deref())?;
    let paths = load_paths(&on_esp(&esp.dir, PATHS_FILE))?;
    let os = match &o.os {
        Some(id) => paths.os_by_id(id).map(|os| os.id.clone()).ok_or_else(|| format!("no [[os]] has id \"{}\"", id))?,
        None => {
            let kind = if cfg!(windows) { OsKind::Windows } else { OsKind::Linux };
            let mine: Vec<_> = paths.os.iter().filter(|os| os.kind == kind).collect();
            match mine.as_slice() {
                [one] => one.id.clone(),
                [] => return Err(format!("paths.toml has no [[os]] of kind {:?}", kind)),
                _ => return Err(format!("paths.toml has several {:?} [[os]]: pick one with --os", kind)),
            }
        }
    };
    Ok(Setup { esp, paths, os })
}

fn scan(s: &Setup) -> Result<(), String> {
    let file = on_esp(&s.esp.dir, LIBRARY_FILE);
    let mut lib = match std::fs::read_to_string(&file) {
        // A broken list is rebuilt rather than blocking the scan.
        Ok(text) => Library::parse(&text).unwrap_or_default(),
        Err(_) => Library::default(),
    };
    let games = scan::games(&s.paths, &s.os);
    println!("{} games on {}", games.len(), s.os);
    lib.replace_os(&s.os, games);
    let tmp = file.with_extension("toml.new");
    std::fs::write(&tmp, lib.to_toml()).map_err(|e| format!("{}: {}", tmp.display(), e))?;
    std::fs::rename(&tmp, &file).map_err(|e| format!("{}: {}", file.display(), e))
}

/// The pending request, if any, for this OS. A request for another OS is
/// left alone so that OS's helper still runs it.
fn pending(s: &Setup, o: &Opts) -> Result<Option<Request>, String> {
    let bytes = match &o.request {
        Some(f) => Some(std::fs::read(f).map_err(|e| format!("{}: {}", f, e))?),
        None => sys::read_request()?,
    };
    let Some(bytes) = bytes else { return Ok(None) };
    let text = String::from_utf8_lossy(&bytes);
    let text = text.trim_end_matches('\0');
    let req = Request::parse(text).map_err(|errs| {
        errs.iter().map(|e| format!("launch request: {}", e)).collect::<Vec<_>>().join("\n")
    })?;
    Ok((req.os == s.os).then_some(req))
}

fn last_count() -> u32 {
    std::fs::read_to_string(sys::state_file()).ok().and_then(|t| t.trim().parse().ok()).unwrap_or(0)
}

fn save_count(n: u32) -> Result<(), String> {
    let f = sys::state_file();
    if let Some(dir) = f.parent() {
        std::fs::create_dir_all(dir).map_err(|e| format!("{}: {}", dir.display(), e))?;
    }
    std::fs::write(&f, format!("{}\n", n)).map_err(|e| format!("{}: {}", f.display(), e))
}

fn run(s: &Setup, o: &Opts) -> Result<(), String> {
    let Some(req) = pending(s, o)? else {
        println!("no launch request for {}", s.os);
        return Ok(());
    };
    if req.count <= last_count() {
        println!("launch request {} was already handled", req.count);
        return Ok(());
    }
    let emu = s.paths.emulator_by_id(&req.emulator).ok_or_else(|| format!("no [[emulator]] has id \"{}\"", req.emulator))?;
    let (exe, args) = emu.command(&req.path);
    println!("starting {} with {}: {} {}", req.title, emu.name, exe, args.join(" "));
    if o.dry_run {
        return Ok(());
    }
    // Mark first: a game that crashes the session must not start again
    // at every login.
    save_count(req.count)?;
    std::process::Command::new(&exe).args(&args).spawn().map_err(|e| format!("{}: {}", exe, e))?;
    Ok(())
}

fn mark(s: &Setup, o: &Opts) -> Result<(), String> {
    match pending(s, o)? {
        Some(req) => {
            save_count(req.count.max(last_count()))?;
            println!("launch request {} marked as handled", req.count);
        }
        None => println!("no launch request for {}", s.os),
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn esp_paths() {
        let p = on_esp(Path::new("/efi"), "\\EFI\\GameCoob\\paths.toml");
        assert_eq!(p, Path::new("/efi").join("EFI").join("GameCoob").join("paths.toml"));
    }

    #[test]
    fn scan_writes_library_and_keeps_other_os() {
        let root = std::env::temp_dir().join(format!("gcl-scan-{}", std::process::id()));
        let esp = root.join("esp");
        let roms = root.join("roms");
        std::fs::create_dir_all(esp.join("EFI").join("GameCoob")).unwrap();
        std::fs::create_dir_all(roms.join("sub")).unwrap();
        std::fs::write(roms.join("b game.iso"), b"not a disc").unwrap();
        std::fs::write(roms.join("sub").join("a.rvz"), b"").unwrap();
        std::fs::write(roms.join("notes.txt"), b"").unwrap();
        let paths = format!(
            "[[os]]\nid = \"arch\"\nboot = \"Linux\"\n[[os]]\nid = \"windows\"\nboot = \"Windows\"\n\
             [[emulator]]\nid = \"d\"\nos = \"arch\"\nexe = \"/bin/true\"\n\
             [[roms]]\nos = \"arch\"\npath = {}\nemulator = \"d\"\nrecursive = true\n",
            launch::toml::quote(&roms.to_string_lossy())
        );
        std::fs::write(on_esp(&esp, PATHS_FILE), paths).unwrap();
        let other = Library {
            games: vec![launch::Game {
                title: "Win game".into(),
                code: None,
                os: "windows".into(),
                emulator: "w".into(),
                path: "D:\\x.iso".into(),
            }],
        };
        std::fs::write(on_esp(&esp, LIBRARY_FILE), other.to_toml()).unwrap();

        let o = Opts { esp: Some(esp.to_string_lossy().into()), os: Some("arch".into()), ..Opts::default() };
        let s = setup(&o).unwrap();
        scan(&s).unwrap();
        let lib = Library::parse(&std::fs::read_to_string(on_esp(&esp, LIBRARY_FILE)).unwrap()).unwrap();
        let titles: Vec<_> = lib.games.iter().map(|g| g.title.as_str()).collect();
        assert_eq!(titles, ["Win game", "a", "b game"]);

        // A request for this OS is pending; one for the other OS is not.
        let req = Request { count: 3, os: "arch".into(), emulator: "d".into(), path: "/x.iso".into(), title: "X".into() };
        let f = root.join("req");
        std::fs::write(&f, req.to_toml() + "\0").unwrap();
        let o2 = Opts { request: Some(f.to_string_lossy().into()), ..o };
        assert_eq!(pending(&s, &o2).unwrap(), Some(req.clone()));
        std::fs::write(&f, Request { os: "windows".into(), ..req }.to_toml()).unwrap();
        assert_eq!(pending(&s, &o2).unwrap(), None);
        drop(s);
        std::fs::remove_dir_all(&root).unwrap();
    }
}
