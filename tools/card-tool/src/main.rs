//! card-tool: the memory card code the app uses (`src/card`), on the PC.
//!
//! ```text
//! card-tool info <card|save|folder>        list a card image, save or GCI folder
//! card-tool new <out.raw> <blocks> [sjis]  blank card: 59, 123, 251, 507, 1019 or 2043 blocks
//! card-tool convert <in> <out> [blocks]    any save -> .gci/.gcs/.sav (by out's extension),
//!                                          card -> folder of .gci (Dolphin GCI folder),
//!                                          folder of saves -> card image (default 251 blocks)
//! card-tool export <card> <index|all> <out-dir> [gci|gcs|sav]
//! card-tool import <card> <save>...        add saves to a card image (keeps <card>.bak)
//! card-tool erase <card> <index>           erase a save (keeps <card>.bak)
//! card-tool icons <card|save> <out-dir> [index]   banner and icon frames as PNG
//! ```
//!
//! `cargo test` here runs `src/card`'s tests.

extern crate alloc;

#[path = "../../../src/ipl/mod.rs"]
#[allow(dead_code)]
mod ipl;

#[path = "../../../src/card/mod.rs"]
#[allow(dead_code)]
mod card;

#[path = "../../menu-render/src/png.rs"]
mod png;

use card::{Card, Encoding, SaveFile, SaveFormat, BLOCKS_PER_MBIT, SIZES_MBIT, SYSTEM_BLOCKS};
use std::path::{Path, PathBuf};
use std::process::exit;

fn die(msg: impl std::fmt::Display) -> ! {
    eprintln!("card-tool: {msg}");
    exit(1)
}

fn read(p: &Path) -> Vec<u8> {
    std::fs::read(p).unwrap_or_else(|e| die(format!("{}: {e}", p.display())))
}

fn write(p: &Path, b: &[u8]) {
    std::fs::write(p, b).unwrap_or_else(|e| die(format!("{}: {e}", p.display())));
}

fn text(b: &[u8]) -> String {
    b.iter().map(|&c| if (0x20..0x7F).contains(&c) { c as char } else { '?' }).collect()
}

fn mbit_for_blocks(blocks: u16) -> u16 {
    SIZES_MBIT
        .iter()
        .copied()
        .find(|&m| m as usize * BLOCKS_PER_MBIT - SYSTEM_BLOCKS as usize == blocks as usize)
        .unwrap_or_else(|| die(format!("{blocks} blocks: not a card size (59, 123, 251, 507, 1019, 2043)")))
}

fn open_card(p: &Path) -> Card {
    Card::open(read(p)).unwrap_or_else(|e| die(format!("{}: {e}", p.display())))
}

fn open_save(p: &Path) -> (SaveFile, SaveFormat) {
    SaveFile::parse(&read(p)).unwrap_or_else(|e| die(format!("{}: {e}", p.display())))
}

fn ext(p: &Path) -> String {
    p.extension().map(|e| e.to_string_lossy().to_ascii_lowercase()).unwrap_or_default()
}

fn describe(i: Option<usize>, s: &SaveFile) {
    let d = &s.dentry;
    let (t, c) = s.comments().unwrap_or((&[], &[]));
    let perm = d.permissions();
    println!(
        "{}{}-{}  {:<32} {:>4} blk  {}{}  \"{}\" / \"{}\"",
        i.map(|i| format!("{i:3}  ")).unwrap_or_default(),
        text(&d.maker()),
        text(&d.game()),
        text(d.file_name()),
        d.block_count(),
        if perm & card::PERM_NO_COPY != 0 { "no-copy " } else { "" },
        if perm & card::PERM_NO_MOVE != 0 { "no-move" } else { "" },
        text(t),
        text(c),
    );
}

fn info(p: &Path) {
    if p.is_dir() {
        for (f, (s, fmt)) in folder_saves(p) {
            print!("{:<5} {:<40} ", fmt.ext(), f.file_name().unwrap().to_string_lossy());
            describe(None, &s);
        }
        return;
    }
    let b = read(p);
    if let Some(_) = SaveFile::detect(&b) {
        let (s, f) = open_save(p);
        println!("{} save", f.label());
        describe(None, &s);
        return;
    }
    let c = Card::open(b).unwrap_or_else(|e| die(format!("{}: {e}", p.display())));
    let (serial, ticks) = c.serial();
    println!(
        "card: {} blocks ({} Mbit), {} free, {} of 127 entries free, {:?}, serial {}, formatted at tick {ticks}",
        c.user_blocks(),
        c.size_mbit(),
        c.free_blocks(),
        c.free_entries(),
        c.encoding(),
        serial.iter().map(|b| format!("{b:02X}")).collect::<String>(),
    );
    for (i, _) in c.saves() {
        match c.read(i) {
            Ok(s) => describe(Some(i), &s),
            Err(e) => println!("{i:3}  {e}"),
        }
    }
}

/// The saves in a folder, by file name.
fn folder_saves(dir: &Path) -> Vec<(PathBuf, (SaveFile, SaveFormat))> {
    let mut v: Vec<PathBuf> = std::fs::read_dir(dir)
        .unwrap_or_else(|e| die(format!("{}: {e}", dir.display())))
        .filter_map(|e| e.ok().map(|e| e.path()))
        .filter(|p| SaveFormat::from_ext(&ext(p)).is_some())
        .collect();
    v.sort();
    v.into_iter().map(|p| { let s = open_save(&p); (p, s) }).collect()
}

fn export(c: &Card, i: usize, dir: &Path, f: SaveFormat) {
    let s = c.read(i).unwrap_or_else(|e| die(format!("save {i}: {e}")));
    let out = dir.join(format!("{}.{}", s.dolphin_name(), f.ext()));
    write(&out, &s.to_bytes(f));
    println!("{}", out.display());
}

/// Write the changed card, keeping the old file as `<card>.bak`.
fn save_card(p: &Path, c: &Card) {
    let mut bak = p.as_os_str().to_owned();
    bak.push(".bak");
    std::fs::copy(p, &bak).unwrap_or_else(|e| die(format!("backup: {e}")));
    write(p, c.as_bytes());
}

fn png(path: &Path, w: usize, h: usize, rgba: &[u8]) {
    png::write_rgba(path, w, h, rgba).unwrap_or_else(|e| die(format!("{}: {e}", path.display())));
    println!("{}", path.display());
}

fn icons(s: &SaveFile, dir: &Path) {
    let name = s.dolphin_name();
    if let Some(b) = s.banner() {
        png(&dir.join(format!("{name}-banner.png")), 96, 32, &b);
    }
    for (k, f) in s.icon_frames().iter().enumerate() {
        png(&dir.join(format!("{name}-icon{k}-{}t.png", f.ticks)), 32, 32, &f.rgba);
    }
}

fn now_ticks() -> u64 {
    // Console ticks (40.5 MHz) since 2000-01-01.
    let unix = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0);
    unix.saturating_sub(946_684_800) * 40_500_000
}

fn main() {
    let a: Vec<String> = std::env::args().collect();
    let arg = |i: usize| a.get(i).map(PathBuf::from).unwrap_or_else(|| die("missing argument; see the comment at the top of main.rs"));
    match a.get(1).map(String::as_str) {
        Some("info") => info(&arg(2)),
        Some("new") => {
            let blocks = a.get(3).and_then(|s| s.parse().ok()).unwrap_or_else(|| die("new <out> <blocks>"));
            let enc = if a.get(4).is_some_and(|s| s == "sjis") { Encoding::ShiftJis } else { Encoding::Ansi };
            let c = Card::format(mbit_for_blocks(blocks), enc, now_ticks()).unwrap();
            write(&arg(2), c.as_bytes());
        }
        Some("convert") => {
            let (src, dst) = (arg(2), arg(3));
            if src.is_dir() {
                let blocks = a.get(4).and_then(|s| s.parse().ok()).unwrap_or(251);
                let mut c = Card::format(mbit_for_blocks(blocks), Encoding::Ansi, now_ticks()).unwrap();
                for (p, (s, _)) in folder_saves(&src) {
                    if let Err(e) = c.import(&s) {
                        eprintln!("{}: {e}", p.display());
                    }
                }
                write(&dst, c.as_bytes());
            } else if let Some(f) = SaveFormat::from_ext(&ext(&dst)) {
                write(&dst, &open_save(&src).0.to_bytes(f));
            } else {
                let c = open_card(&src);
                std::fs::create_dir_all(&dst).unwrap_or_else(|e| die(e));
                let idx: Vec<usize> = c.saves().map(|(i, _)| i).collect();
                for i in idx {
                    export(&c, i, &dst, SaveFormat::Gci);
                }
            }
        }
        Some("export") => {
            let c = open_card(&arg(2));
            let dir = arg(4);
            let f = a.get(5).and_then(|e| SaveFormat::from_ext(e)).unwrap_or(SaveFormat::Gci);
            std::fs::create_dir_all(&dir).unwrap_or_else(|e| die(e));
            match a.get(3).map(String::as_str) {
                Some("all") => {
                    let idx: Vec<usize> = c.saves().map(|(i, _)| i).collect();
                    for i in idx {
                        export(&c, i, &dir, f);
                    }
                }
                Some(n) => export(&c, n.parse().unwrap_or_else(|_| die("index")), &dir, f),
                None => die("export <card> <index|all> <out-dir>"),
            }
        }
        Some("import") => {
            let p = arg(2);
            let mut c = open_card(&p);
            for s in &a[3..] {
                let (save, _) = open_save(Path::new(s));
                match c.import(&save) {
                    Ok(i) => println!("{s}: entry {i}"),
                    Err(e) => die(format!("{s}: {e}")),
                }
            }
            save_card(&p, &c);
        }
        Some("erase") => {
            let p = arg(2);
            let mut c = open_card(&p);
            let i = a.get(3).and_then(|s| s.parse().ok()).unwrap_or_else(|| die("erase <card> <index>"));
            c.delete(i).unwrap_or_else(|e| die(e));
            save_card(&p, &c);
        }
        Some("icons") => {
            let (src, dir) = (arg(2), arg(3));
            std::fs::create_dir_all(&dir).unwrap_or_else(|e| die(e));
            let b = read(&src);
            if SaveFile::detect(&b).is_some() {
                icons(&open_save(&src).0, &dir);
            } else {
                let c = open_card(&src);
                let only: Option<usize> = a.get(4).and_then(|s| s.parse().ok());
                for (i, _) in c.saves().filter(|(i, _)| only.is_none_or(|o| o == *i)) {
                    if let Ok(s) = c.read(i) {
                        icons(&s, &dir);
                    }
                }
            }
        }
        _ => die("usage: info | new | convert | export | import | erase | icons (see src/main.rs)"),
    }
}
