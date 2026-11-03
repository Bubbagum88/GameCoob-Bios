//! Lists the games in this OS's `[[roms]]` folders.

use crate::disc;
use crate::launch::{Game, Paths};
use std::path::Path;

/// Every game in the `[[roms]]` folders of `os`, sorted by title. Missing
/// folders are reported on stderr and skipped, so one unplugged drive
/// doesn't empty the list.
pub fn games(paths: &Paths, os: &str) -> Vec<Game> {
    let mut out = Vec::new();
    for roms in paths.roms.iter().filter(|r| r.os == os) {
        let dir = Path::new(&roms.path);
        if !dir.is_dir() {
            eprintln!("gamecoob-launch: rom folder {} not found, skipped", roms.path);
            continue;
        }
        let mut files = Vec::new();
        walk(dir, roms.recursive, &mut files);
        for file in files {
            let ext = file.extension().and_then(|e| e.to_str()).unwrap_or("").to_ascii_lowercase();
            if !roms.ext.contains(&ext) {
                continue;
            }
            let header = disc::read(&file);
            let stem = file.file_stem().and_then(|s| s.to_str()).unwrap_or("?").to_string();
            let title = header.as_ref().map(|h| h.title.clone()).filter(|t| !t.is_empty()).unwrap_or(stem);
            out.push(Game {
                title,
                code: header.map(|h| h.code),
                os: os.to_string(),
                emulator: roms.emulator.clone(),
                path: file.to_string_lossy().into_owned(),
            });
        }
    }
    out.sort_by(|a, b| a.title.to_lowercase().cmp(&b.title.to_lowercase()).then(a.path.cmp(&b.path)));
    out
}

fn walk(dir: &Path, recursive: bool, out: &mut Vec<std::path::PathBuf>) {
    let Ok(entries) = std::fs::read_dir(dir) else { return };
    for e in entries.flatten() {
        let p = e.path();
        match e.file_type() {
            Ok(t) if t.is_dir() && recursive => walk(&p, true, out),
            Ok(t) if t.is_file() => out.push(p),
            // Follow links to files (and to folders when recursive).
            Ok(t) if t.is_symlink() => {
                if p.is_file() {
                    out.push(p);
                } else if recursive && p.is_dir() {
                    walk(&p, true, out);
                }
            }
            _ => {}
        }
    }
}
