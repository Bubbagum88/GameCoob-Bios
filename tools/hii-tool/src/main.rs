//! hii-tool: print a firmware's setup options from the files GameCoob's
//! Firmware page saves (Export forms): `hii.bin` (the HII database's
//! package lists) and `config.txt` (the config routing export).
//!
//! Usage: hii-tool <hii.bin> [config.txt]
//!
//! Every form set, form and question is listed with its current value,
//! what hides or locks it, and whether GameCoob can change it.

extern crate alloc;

#[path = "../../../src/hii/mod.rs"]
#[allow(dead_code)]
mod hii;

use hii::ifr::Kind;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        eprintln!("usage: {} <hii.bin> [config.txt]", args[0]);
        std::process::exit(2);
    }
    let bin = std::fs::read(&args[1]).expect("read hii.bin");
    let cfg = args.get(2).map(|p| std::fs::read_to_string(p).expect("read config.txt")).unwrap_or_default();
    // The app saves the config export as UTF-8 text.
    let db = hii::Db::load(&bin, cfg.trim(), &mut |_, _| None);
    println!("{} form sets, {} stores", db.sets.len(), db.stores.len());
    for s in &db.stores {
        let have = s.have.iter().filter(|&&h| h).count();
        println!("store {} {} ({} bytes, {} read, {})", hii::cfg::guid_hex(&s.guid), s.name, s.orig.len(), have,
            if s.hdr.is_some() { "routable" } else { "no route" });
    }
    for (si, set) in db.sets.iter().enumerate() {
        println!();
        println!("== {} [{}]", db.str(si, set.fs.title), hii::cfg::guid_hex(&set.fs.guid));
        for f in &set.fs.forms {
            println!("  -- form {:#x}: {}", f.id, db.str(si, f.title));
            for it in &f.items {
                let prompt = db.str(si, it.prompt);
                let kind = match &it.kind {
                    Kind::Subtitle => "subtitle",
                    Kind::Text(_) => "text",
                    Kind::Ref { .. } => "ref",
                    Kind::OneOf(_) => "oneof",
                    Kind::Checkbox { .. } => "checkbox",
                    Kind::Numeric { .. } => "numeric",
                    Kind::String => "string",
                    Kind::Password => "password",
                    Kind::Date => "date",
                    Kind::Time => "time",
                    Kind::Action => "action",
                    Kind::OrderedList => "orderedlist",
                    Kind::ResetButton => "reset",
                };
                let mut flags = String::new();
                if db.hidden(si, &it.conds) {
                    flags.push_str(" hidden");
                }
                if db.grayed(si, &it.conds) {
                    flags.push_str(" grey");
                }
                let edit = match db.editable(si, it) {
                    Ok(()) => "editable".to_string(),
                    Err(l) => format!("{l:?}"),
                };
                let opts: Vec<String> = db.options(si, it).into_iter().map(|(t, v)| format!("{t}={v}")).collect();
                println!("    {kind:<9} {prompt:?} = {:?} [{edit}{flags}]{}", db.value_text(si, it, true),
                    if opts.is_empty() { String::new() } else { format!(" {{{}}}", opts.join(", ")) });
            }
        }
    }
}
