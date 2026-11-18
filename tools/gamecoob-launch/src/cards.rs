//! Memory card sync between the boot menu's card images on the ESP
//! (`[boot] cards`: `slot-a.raw`, `slot-b.raw`) and an emulator's cards
//! (`[[emulator]] cards`, Dolphin's `GC` folder). docs/EMULATORS.md,
//! "Memory cards".
//!
//! The sync works save by save, not card by card: one boot-menu card holds
//! the saves of every region, while Dolphin keeps one card (or GCI folder)
//! per region. A record of each save's content at the last sync tells
//! which side changed it since: a save changed (or added, or erased) on
//! one side only is copied (or erased) on the other. When both sides
//! changed the same save, `Prefer` picks the winner, except that a change
//! always beats an erase. Every file replaced is kept as `.bak` first.

use crate::card::{Card, Encoding, SaveFile, SaveFormat, BLOCK, SIZES_MBIT};
use crate::launch::{CardsFormat, Emulator};
use std::collections::{BTreeMap, BTreeSet};
use std::path::{Path, PathBuf};

/// Dolphin's region folders and file suffixes.
pub const REGIONS: [&str; 3] = ["USA", "EUR", "JAP"];
pub const SLOTS: [char; 2] = ['A', 'B'];
/// The boot app's names for a slot's card (src/boot_intro.rs), first found wins.
const ESP_NAMES: [&str; 3] = ["raw", "mcp", "gcp"];

/// Which side wins when both changed the same save.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Prefer {
    /// The boot menu's card: before a game, and at boot.
    Boot,
    /// The emulator's: after a game, and at shutdown.
    Emulator,
}

/// A save's identity: game code, maker and file name, the way
/// `Dentry::same_save` compares them.
pub fn key(s: &SaveFile) -> String {
    let d = &s.dentry;
    d.0[0..6].iter().chain(d.file_name()).map(|b| format!("{:02x}", b)).collect()
}

/// A save's content: the entry without its card-specific first block, and
/// the data. FNV-1a, 64 bit.
pub fn hash(s: &SaveFile) -> u64 {
    let mut d = s.dentry;
    d.set_first_block(0);
    let n = (d.block_count() as usize * BLOCK).min(s.data.len());
    d.0.iter().chain(&s.data[..n]).fold(0xcbf2_9ce4_8422_2325u64, |h, &b| (h ^ b as u64).wrapping_mul(0x100_0000_01b3))
}

/// Dolphin's card region for a game code's last letter.
pub fn region(game: [u8; 4]) -> usize {
    match game[3] {
        b'J' | b'K' | b'W' => 2,
        b'P' | b'D' | b'F' | b'I' | b'S' | b'H' | b'U' | b'X' | b'Y' | b'Z' | b'L' | b'M' | b'V' => 1,
        _ => 0,
    }
}

/// Keep the old file as `<name>.bak` before it is replaced or erased.
fn backup(path: &Path) -> Result<(), String> {
    let mut bak = path.as_os_str().to_owned();
    bak.push(".bak");
    std::fs::copy(path, &bak).map(|_| ()).map_err(|e| format!("{}: {}", path.display(), e))
}

/// Write through a temporary file, so a crash never leaves half a card.
fn write_file(path: &Path, bytes: &[u8]) -> Result<(), String> {
    let mut tmp = path.as_os_str().to_owned();
    tmp.push(".new");
    std::fs::write(&tmp, bytes).map_err(|e| format!("{}: {}", path.display(), e))?;
    std::fs::rename(&tmp, path).map_err(|e| format!("{}: {}", path.display(), e))
}

/// Files written into the user's emulator folder by the root service on
/// Linux get that folder's owner, so the emulator can still change them.
#[cfg(unix)]
fn own_like(path: &Path, like: &Path) {
    use std::os::unix::fs::MetadataExt;
    if let Ok(m) = std::fs::metadata(like) {
        let _ = std::os::unix::fs::chown(path, Some(m.uid()), Some(m.gid()));
    }
}

#[cfg(not(unix))]
fn own_like(_: &Path, _: &Path) {}

/// The boot menu's card for one slot.
struct EspCard {
    path: PathBuf,
    card: Card,
    changed: bool,
}

impl EspCard {
    fn open(cards_dir: &Path, slot: char) -> Result<Option<EspCard>, String> {
        let lower = slot.to_ascii_lowercase();
        let Some(path) = ESP_NAMES.iter().map(|e| cards_dir.join(format!("slot-{}.{}", lower, e))).find(|p| p.is_file()) else {
            return Ok(None);
        };
        let img = std::fs::read(&path).map_err(|e| format!("{}: {}", path.display(), e))?;
        let card = Card::open(img).map_err(|e| format!("{}: {}", path.display(), e))?;
        Ok(Some(EspCard { path, card, changed: false }))
    }

    fn saves(&self) -> Vec<SaveFile> {
        self.card.saves().filter_map(|(i, _)| self.card.read(i).ok()).collect()
    }

    fn save(&mut self) -> Result<(), String> {
        if self.changed {
            backup(&self.path)?;
            write_file(&self.path, self.card.as_bytes())?;
        }
        Ok(())
    }
}

/// Put `save` on `card` in place of its older copy, leaving the card as it
/// was if it doesn't fit.
fn replace(card: &mut Card, save: &SaveFile) -> Result<(), String> {
    let mut next = card.clone();
    if let Some(i) = next.find(&save.dentry) {
        next.delete(i).map_err(|e| e.to_string())?;
    }
    next.import(save).map_err(|e| e.to_string())?;
    *card = next;
    Ok(())
}

fn erase(card: &mut Card, save: &SaveFile) -> Result<(), String> {
    if let Some(i) = card.find(&save.dentry) {
        card.delete(i).map_err(|e| e.to_string())?;
    }
    Ok(())
}

/// One region's raw card in the emulator's folder.
struct RawCard {
    path: PathBuf,
    card: Option<Card>,
    changed: bool,
}

/// The emulator's cards for one slot, every region.
struct EmuCards {
    dir: PathBuf,
    format: CardsFormat,
    slot: char,
    /// GCI files found, by save key.
    gci: BTreeMap<String, PathBuf>,
    /// Raw cards by region.
    raw: Vec<RawCard>,
}

impl EmuCards {
    fn gci_dir(&self, region: usize) -> PathBuf {
        self.dir.join(REGIONS[region]).join(format!("Card {}", self.slot))
    }

    /// Reads every region's saves.
    fn open(dir: &Path, format: CardsFormat, slot: char) -> Result<(EmuCards, Vec<SaveFile>), String> {
        let mut c = EmuCards { dir: dir.to_path_buf(), format, slot, gci: BTreeMap::new(), raw: Vec::new() };
        let mut saves = Vec::new();
        match format {
            CardsFormat::Gci => {
                for r in 0..REGIONS.len() {
                    let Ok(list) = std::fs::read_dir(c.gci_dir(r)) else { continue };
                    let mut files: Vec<_> = list.filter_map(|e| e.ok()).map(|e| e.path()).collect();
                    files.sort();
                    for f in files {
                        if !f.extension().is_some_and(|e| e.eq_ignore_ascii_case("gci")) {
                            continue;
                        }
                        let Ok(bytes) = std::fs::read(&f) else { continue };
                        match SaveFile::parse(&bytes) {
                            Ok((s, _)) => {
                                // Two files with one save: the first stays, as in Dolphin.
                                if !c.gci.contains_key(&key(&s)) {
                                    c.gci.insert(key(&s), f);
                                    saves.push(s);
                                }
                            }
                            Err(e) => eprintln!("gamecoob-launch: skipping {}: {}", f.display(), e),
                        }
                    }
                }
            }
            CardsFormat::Raw => {
                for r in REGIONS {
                    let path = dir.join(format!("MemoryCard{}.{}.raw", slot, r));
                    let card = match std::fs::read(&path) {
                        Ok(img) => Some(Card::open(img).map_err(|e| format!("{}: {}", path.display(), e))?),
                        Err(_) => None,
                    };
                    if let Some(card) = &card {
                        saves.extend(card.saves().filter_map(|(i, _)| card.read(i).ok()));
                    }
                    c.raw.push(RawCard { path, card, changed: false });
                }
            }
        }
        Ok((c, saves))
    }

    fn put(&mut self, save: &SaveFile) -> Result<(), String> {
        let r = region(save.dentry.game());
        match self.format {
            CardsFormat::Gci => {
                let path = match self.gci.get(&key(save)) {
                    Some(p) => {
                        backup(p)?;
                        p.clone()
                    }
                    None => {
                        let d = self.gci_dir(r);
                        std::fs::create_dir_all(&d).map_err(|e| format!("{}: {}", d.display(), e))?;
                        for p in [&self.dir.join(REGIONS[r]), &d] {
                            own_like(p, &self.dir);
                        }
                        d.join(format!("{}.gci", save.dolphin_name()))
                    }
                };
                write_file(&path, &save.to_bytes(SaveFormat::Gci))?;
                own_like(&path, &self.dir);
                self.gci.insert(key(save), path);
                Ok(())
            }
            CardsFormat::Raw => {
                let rc = &mut self.raw[r];
                // Dolphin makes a 2043-block card the first time a region
                // is played; make the same one if it hasn't yet.
                let card = match &mut rc.card {
                    Some(c) => c,
                    None => {
                        let enc = if r == 2 { Encoding::ShiftJis } else { Encoding::Ansi };
                        let ticks = std::time::SystemTime::now()
                            .duration_since(std::time::UNIX_EPOCH)
                            .map(|d| d.as_nanos() as u64)
                            .unwrap_or(0);
                        let mbit = SIZES_MBIT[SIZES_MBIT.len() - 1];
                        rc.card.insert(Card::format(mbit, enc, ticks).map_err(|e| e.to_string())?)
                    }
                };
                replace(card, save)?;
                rc.changed = true;
                Ok(())
            }
        }
    }

    fn remove(&mut self, save: &SaveFile) -> Result<(), String> {
        match self.format {
            CardsFormat::Gci => {
                if let Some(p) = self.gci.remove(&key(save)) {
                    // Renamed, not deleted: Dolphin only loads `.gci` files.
                    let mut bak = p.as_os_str().to_owned();
                    bak.push(".bak");
                    std::fs::rename(&p, &bak).map_err(|e| format!("{}: {}", p.display(), e))?;
                }
                Ok(())
            }
            CardsFormat::Raw => {
                let rc = &mut self.raw[region(save.dentry.game())];
                if let Some(card) = &mut rc.card {
                    erase(card, save)?;
                    rc.changed = true;
                }
                Ok(())
            }
        }
    }

    fn save(&mut self) -> Result<(), String> {
        for rc in &mut self.raw {
            let Some(card) = &rc.card else { continue };
            if !rc.changed {
                continue;
            }
            if rc.path.is_file() {
                backup(&rc.path)?;
            }
            write_file(&rc.path, card.as_bytes())?;
            own_like(&rc.path, &self.dir);
        }
        Ok(())
    }
}

/// Save keys to content hashes at the last sync, per slot.
pub type Record = BTreeMap<(char, String), u64>;

pub fn read_record(path: &Path) -> Record {
    let text = std::fs::read_to_string(path).unwrap_or_default();
    text.lines()
        .filter_map(|l| {
            let mut f = l.split(' ');
            let slot = f.next()?.chars().next()?;
            let key = f.next()?.to_string();
            let hash = u64::from_str_radix(f.next()?, 16).ok()?;
            Some(((slot, key), hash))
        })
        .collect()
}

pub fn write_record(path: &Path, rec: &Record) -> Result<(), String> {
    let text: String = rec.iter().map(|((slot, key), h)| format!("{} {} {:016x}\n", slot, key, h)).collect();
    write_file(path, text.as_bytes())
}

/// What one sync did, for the log line.
#[derive(Default, Debug, PartialEq, Eq)]
pub struct Counts {
    pub to_emulator: usize,
    pub to_boot: usize,
    pub failed: usize,
}

/// Syncs both slots of one emulator. `esp_cards` is the `[boot] cards`
/// folder on the ESP; `record` is this emulator's record file.
pub fn sync(esp_cards: &Path, emu: &Emulator, record: &Path, prefer: Prefer) -> Result<Counts, String> {
    let Some(dir) = &emu.cards else { return Ok(Counts::default()) };
    let dir = Path::new(dir);
    if !dir.is_dir() {
        return Err(format!("{}: no such folder (the emulator's cards)", dir.display()));
    }
    let old = read_record(record);
    let mut new = Record::new();
    let mut counts = Counts::default();
    for slot in SLOTS {
        let Some(mut esp) = EspCard::open(esp_cards, slot)? else {
            // No card in this slot: keep what was known, sync nothing.
            new.extend(old.iter().filter(|((s, _), _)| *s == slot).map(|(k, v)| (k.clone(), *v)));
            continue;
        };
        let (mut emu_cards, emu_saves) = EmuCards::open(dir, emu.cards_format, slot)?;
        let boot: BTreeMap<String, SaveFile> = esp.saves().into_iter().map(|s| (key(&s), s)).collect();
        let other: BTreeMap<String, SaveFile> = emu_saves.into_iter().map(|s| (key(&s), s)).collect();
        let keys: BTreeSet<&String> =
            boot.keys().chain(other.keys()).chain(old.keys().filter(|(s, _)| *s == slot).map(|(_, k)| k)).collect();
        for k in keys {
            let (b, e) = (boot.get(k), other.get(k));
            let (bh, eh) = (b.map(hash), e.map(hash));
            let was = old.get(&(slot, k.clone())).copied();
            let rec_key = (slot, k.clone());
            if bh == eh {
                if let Some(h) = bh {
                    new.insert(rec_key, h);
                }
                continue;
            }
            let to_boot = match (bh != was, eh != was) {
                (false, true) => true,
                (true, false) => false,
                _ if bh.is_none() => true,
                _ if eh.is_none() => false,
                _ => prefer == Prefer::Emulator,
            };
            let (result, h) = if to_boot {
                esp.changed = true;
                let r = match e {
                    Some(s) => replace(&mut esp.card, s),
                    None => erase(&mut esp.card, b.unwrap()),
                };
                (r, eh)
            } else {
                let r = match b {
                    Some(s) => emu_cards.put(s),
                    None => emu_cards.remove(e.unwrap()),
                };
                (r, bh)
            };
            match result {
                Ok(()) => {
                    if to_boot { counts.to_boot += 1 } else { counts.to_emulator += 1 }
                    if let Some(h) = h {
                        new.insert(rec_key, h);
                    }
                }
                Err(err) => {
                    let s = e.or(b).unwrap();
                    let side = if to_boot { "the boot menu's" } else { "the emulator's" };
                    eprintln!("gamecoob-launch: slot {}: {:?} not copied to {} card: {}", slot, s.dentry, side, err);
                    counts.failed += 1;
                    // Unchanged record: tried again next time.
                    if let Some(w) = was {
                        new.insert(rec_key, w);
                    }
                }
            }
        }
        emu_cards.save()?;
        esp.save()?;
    }
    write_record(record, &new)?;
    Ok(counts)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::card::Dentry;

    fn save(game: &[u8; 4], name: &str, fill: u8) -> SaveFile {
        let mut d = [0u8; 0x40];
        d[0..4].copy_from_slice(game);
        d[4..6].copy_from_slice(b"01");
        d[8..8 + name.len()].copy_from_slice(name.as_bytes());
        let mut dentry = Dentry(d);
        dentry.set_block_count(1);
        SaveFile { dentry, data: vec![fill; BLOCK] }
    }

    fn card_with(saves: &[&SaveFile]) -> Vec<u8> {
        let mut c = Card::format(4, Encoding::Ansi, 1).unwrap();
        for s in saves {
            c.import(s).unwrap();
        }
        c.into_bytes()
    }

    fn names(img: &[u8]) -> Vec<(String, u8)> {
        let c = Card::open(img.to_vec()).unwrap();
        let mut v: Vec<_> = c
            .saves()
            .map(|(i, d)| (String::from_utf8_lossy(d.file_name()).into_owned(), c.read(i).unwrap().data[0]))
            .collect();
        v.sort();
        v
    }

    fn emulator(dir: &Path, format: CardsFormat) -> Emulator {
        Emulator {
            id: "d".into(),
            name: "Dolphin".into(),
            os: "arch".into(),
            exe: "/bin/true".into(),
            args: vec![],
            cards: Some(dir.to_string_lossy().into()),
            cards_format: format,
        }
    }

    fn gci_files(dir: &Path) -> Vec<String> {
        let mut v: Vec<_> = std::fs::read_dir(dir)
            .map(|l| l.filter_map(|e| e.ok()).map(|e| e.file_name().to_string_lossy().into_owned()).collect())
            .unwrap_or_default();
        v.sort();
        v
    }

    #[test]
    fn regions() {
        assert_eq!(region(*b"GALE"), 0);
        assert_eq!(region(*b"GALP"), 1);
        assert_eq!(region(*b"GALJ"), 2);
    }

    #[test]
    fn gci_two_way() {
        let root = std::env::temp_dir().join(format!("gcl-cards-gci-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&root);
        let (esp, gc) = (root.join("esp"), root.join("GC"));
        std::fs::create_dir_all(&esp).unwrap();
        std::fs::create_dir_all(gc.join("USA").join("Card A")).unwrap();
        let rec = root.join("rec.txt");
        let emu = emulator(&gc, CardsFormat::Gci);
        let (us, pal, dol) = (save(b"GALE", "us", 1), save(b"GALP", "pal", 2), save(b"GZLE", "dolphin", 3));
        std::fs::write(esp.join("slot-a.raw"), card_with(&[&us, &pal])).unwrap();
        std::fs::write(gc.join("USA").join("Card A").join("x.gci"), dol.to_bytes(SaveFormat::Gci)).unwrap();

        // First sync merges both sides, by region on Dolphin's side.
        let c = sync(&esp, &emu, &rec, Prefer::Boot).unwrap();
        assert_eq!(c, Counts { to_emulator: 2, to_boot: 1, failed: 0 });
        assert_eq!(names(&std::fs::read(esp.join("slot-a.raw")).unwrap()), [("dolphin".into(), 3), ("pal".into(), 2), ("us".into(), 1)]);
        assert_eq!(gci_files(&gc.join("USA").join("Card A")), ["01-GALE-us.gci", "x.gci"]);
        assert_eq!(gci_files(&gc.join("EUR").join("Card A")), ["01-GALP-pal.gci"]);
        assert!(esp.join("slot-a.raw.bak").is_file());

        // Nothing changed: nothing copied.
        assert_eq!(sync(&esp, &emu, &rec, Prefer::Boot).unwrap(), Counts::default());

        // The game saves in Dolphin and erases another save: both reach the ESP.
        std::fs::write(gc.join("USA").join("Card A").join("x.gci"), save(b"GZLE", "dolphin", 9).to_bytes(SaveFormat::Gci)).unwrap();
        std::fs::remove_file(gc.join("EUR").join("Card A").join("01-GALP-pal.gci")).unwrap();
        let c = sync(&esp, &emu, &rec, Prefer::Emulator).unwrap();
        assert_eq!(c, Counts { to_emulator: 0, to_boot: 2, failed: 0 });
        assert_eq!(names(&std::fs::read(esp.join("slot-a.raw")).unwrap()), [("dolphin".into(), 9), ("us".into(), 1)]);

        // The boot menu changes a save; Dolphin changed it too: the
        // preferred side wins, the other is kept as .bak.
        std::fs::write(esp.join("slot-a.raw"), card_with(&[&save(b"GZLE", "dolphin", 5), &us])).unwrap();
        std::fs::write(gc.join("USA").join("Card A").join("x.gci"), save(b"GZLE", "dolphin", 6).to_bytes(SaveFormat::Gci)).unwrap();
        sync(&esp, &emu, &rec, Prefer::Boot).unwrap();
        let (s, _) = SaveFile::parse(&std::fs::read(gc.join("USA").join("Card A").join("x.gci")).unwrap()).unwrap();
        assert_eq!(s.data[0], 5);
        assert!(gc.join("USA").join("Card A").join("x.gci.bak").is_file());
        std::fs::remove_dir_all(&root).unwrap();
    }

    #[test]
    fn raw_cards_and_missing_slot() {
        let root = std::env::temp_dir().join(format!("gcl-cards-raw-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&root);
        let (esp, gc) = (root.join("esp"), root.join("GC"));
        std::fs::create_dir_all(&esp).unwrap();
        std::fs::create_dir_all(&gc).unwrap();
        let rec = root.join("rec.txt");
        let emu = emulator(&gc, CardsFormat::Raw);
        let (us, jp) = (save(b"GALE", "us", 1), save(b"GALJ", "jp", 2));
        std::fs::write(gc.join("MemoryCardB.USA.raw"), card_with(&[&us])).unwrap();
        std::fs::write(esp.join("slot-b.raw"), card_with(&[&jp])).unwrap();

        let c = sync(&esp, &emu, &rec, Prefer::Boot).unwrap();
        assert_eq!(c, Counts { to_emulator: 1, to_boot: 1, failed: 0 });
        // A region Dolphin hadn't made a card for yet gets a fresh one.
        let jap = std::fs::read(gc.join("MemoryCardB.JAP.raw")).unwrap();
        assert_eq!(Card::open(jap.clone()).unwrap().encoding(), Encoding::ShiftJis);
        assert_eq!(names(&jap), [("jp".into(), 2)]);
        assert_eq!(names(&std::fs::read(esp.join("slot-b.raw")).unwrap()), [("jp".into(), 2), ("us".into(), 1)]);
        // No slot-a card on the ESP: slot A is left alone.
        assert!(!gc.join("MemoryCardA.USA.raw").exists());

        // Erased in the boot menu: erased in Dolphin.
        std::fs::write(esp.join("slot-b.raw"), card_with(&[&jp])).unwrap();
        sync(&esp, &emu, &rec, Prefer::Emulator).unwrap();
        assert!(names(&std::fs::read(gc.join("MemoryCardB.USA.raw")).unwrap()).is_empty());
        std::fs::remove_dir_all(&root).unwrap();
    }
}
