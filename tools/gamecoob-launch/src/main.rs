//! gamecoob-launch: the OS-side half of launching games from the GameCoob
//! boot menu (docs/EMULATORS.md). It shares the file formats with the boot
//! app through `src/launch`.
//!
//! ```text
//! gamecoob-launch check <paths.toml>   check a paths file and list what it sets up
//! ```
//!
//! Scanning ROM folders, writing the game list and starting the emulator a
//! launch request names come in the next steps. `cargo test` here runs
//! `src/launch`'s tests.

extern crate alloc;

#[path = "../../../src/launch/mod.rs"]
#[allow(dead_code)]
mod launch;

use launch::Paths;
use std::process::ExitCode;

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().skip(1).collect();
    match args.iter().map(String::as_str).collect::<Vec<_>>().as_slice() {
        ["check", file] => check(file),
        _ => {
            eprintln!("usage: gamecoob-launch check <paths.toml>");
            ExitCode::from(2)
        }
    }
}

fn check(file: &str) -> ExitCode {
    let text = match std::fs::read_to_string(file) {
        Ok(t) => t,
        Err(e) => {
            eprintln!("{}: {}", file, e);
            return ExitCode::FAILURE;
        }
    };
    let paths = match Paths::parse(&text) {
        Ok(p) => p,
        Err(errs) => {
            for e in errs {
                eprintln!("{}: {}", file, e);
            }
            return ExitCode::FAILURE;
        }
    };
    println!("boot: ipl {}  cards {}  saves {}", paths.boot.ipl, paths.boot.cards, paths.boot.saves);
    for o in &paths.os {
        let esp = o.esp.as_deref().map(|e| format!(", ESP at {}", e)).unwrap_or_default();
        println!("os {} ({:?}): boot entry containing \"{}\"{}", o.id, o.kind, o.boot, esp);
    }
    for e in &paths.emulator {
        let (exe, args) = e.command("<game>");
        println!("emulator {} \"{}\" on {}: {} {}", e.id, e.name, e.os, exe, args.join(" "));
    }
    for r in &paths.roms {
        let deep = if r.recursive { " and subfolders" } else { "" };
        println!("roms on {}: {}{} (.{}) with {}", r.os, r.path, deep, r.ext.join(" ."), r.emulator);
    }
    ExitCode::SUCCESS
}
