//! The boot app's half of launching games in an emulator
//! (docs/EMULATORS.md): reads the paths file and the helper's game list
//! from the boot partition, and on a pick writes the launch request and
//! boots the game's OS.

use crate::bs2::menu::GameRow;
use crate::launch::{Boot, Game, Library, Paths, Request, LIBRARY_FILE, PATHS_FILE};
use crate::{firmware, vars};
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;
use uefi::boot;
use uefi::fs::FileSystem;
use uefi::CString16;

pub struct Launcher {
    paths: Paths,
    /// The games whose OS and emulator the paths file still names.
    games: Vec<Game>,
}

fn read_text(fs: &mut FileSystem, path: &str) -> Option<String> {
    let p = CString16::try_from(path).ok()?;
    let bytes = fs.read(p.as_ref()).ok()?;
    Some(String::from_utf8_lossy(&bytes).into_owned())
}

/// The `[boot]` part of the paths file: where the app's own files are on
/// the boot partition. The built-in paths when the file is missing or
/// broken.
pub fn boot_paths() -> Boot {
    let Ok(sfs) = boot::get_image_file_system(boot::image_handle()) else { return Boot::default() };
    let mut fs = FileSystem::new(sfs);
    read_text(&mut fs, PATHS_FILE).and_then(|t| Paths::parse(&t).ok()).map(|p| p.boot).unwrap_or_default()
}

impl Launcher {
    /// Reads both files from the partition GameCoob booted from. Missing
    /// or broken files give no games (problems go to the log).
    pub fn load() -> Launcher {
        let empty = Launcher { paths: Paths::default(), games: Vec::new() };
        let Ok(sfs) = boot::get_image_file_system(boot::image_handle()) else { return empty };
        let mut fs = FileSystem::new(sfs);
        let Some(text) = read_text(&mut fs, PATHS_FILE) else { return empty };
        let paths = match Paths::parse(&text) {
            Ok(p) => p,
            Err(errs) => {
                for e in errs {
                    log::warn!("paths.toml: {}", e);
                }
                return empty;
            }
        };
        let lib = match read_text(&mut fs, LIBRARY_FILE).map(|t| Library::parse(&t)) {
            Some(Ok(lib)) => lib,
            Some(Err(errs)) => {
                for e in errs {
                    log::warn!("library.toml: {}", e);
                }
                Library::default()
            }
            None => Library::default(),
        };
        let games: Vec<Game> = lib
            .games
            .into_iter()
            .filter(|g| paths.os_by_id(&g.os).is_some() && paths.emulator_by_id(&g.emulator).is_some_and(|e| e.os == g.os))
            .collect();
        log::info!("{} games for emulators", games.len());
        Launcher { paths, games }
    }

    /// Game Play's game list.
    pub fn rows(&self) -> Vec<GameRow> {
        self.games
            .iter()
            .map(|g| {
                let emu = self.paths.emulator_by_id(&g.emulator).map(|e| e.name.as_str()).unwrap_or("?");
                GameRow { title: g.title.clone(), detail: format!("{}, {}", emu, g.os) }
            })
            .collect()
    }

    /// Records game `i` for its OS's helper and boots that OS; returns
    /// only on failure, with the reason.
    pub fn launch(&self, i: usize) -> String {
        let Some(g) = self.games.get(i) else { return "No such game.".into() };
        let Some(os) = self.paths.os_by_id(&g.os) else { return format!("paths.toml has no os {}.", g.os) };
        let want = os.boot.to_lowercase();
        let Some(entry) = vars::boot_entries().into_iter().find(|e| e.desc.to_lowercase().contains(&want)) else {
            return format!("No boot entry is named like \"{}\".", os.boot);
        };
        // One more than the last request, so the helper runs this one once.
        let count = vars::launch_request()
            .and_then(|b| Request::parse(String::from_utf8_lossy(&b).trim_end_matches('\0')).ok())
            .map_or(1, |r| r.count.wrapping_add(1).max(1));
        let req = Request { count, os: g.os.clone(), emulator: g.emulator.clone(), path: g.path.clone(), title: g.title.clone() };
        if let Err(err) = vars::set_launch_request(&req.to_toml()) {
            return format!("Could not save the launch request: {:?}", err.status());
        }
        log::info!("launching {} on {} (Boot{:04X}, request {})", g.title, os.id, entry.num, count);
        firmware::boot_entry(entry.num)
    }
}
