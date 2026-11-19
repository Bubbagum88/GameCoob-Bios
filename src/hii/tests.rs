//! Tests on hand-built packages (`cargo test` in tools/hii-tool).

use super::*;
use alloc::vec;
use alloc::vec::Vec;

const FS_GUID: [u8; 16] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
const VS_GUID: [u8; 16] = [0xA0; 16];

fn op(code: u8, scope: bool, body: &[u8]) -> Vec<u8> {
    let mut v = vec![code, (body.len() + 2) as u8 | if scope { 0x80 } else { 0 }];
    v.extend_from_slice(body);
    v
}

fn end() -> Vec<u8> {
    op(ifr::OP_END, false, &[])
}

fn le(v: u16) -> [u8; 2] {
    v.to_le_bytes()
}

/// A question header: prompt, help, question id, varstore, offset, flags.
fn qhdr(prompt: u16, qid: u16, vs: u16, off: u16, flags: u8) -> Vec<u8> {
    [&le(prompt)[..], &le(0), &le(qid), &le(vs), &le(off), &[flags]].concat()
}

/// Strings 1.. in en-US.
fn strings(list: &[&str]) -> Vec<u8> {
    let mut blocks = Vec::new();
    for s in list {
        blocks.push(0x14);
        for u in s.encode_utf16() {
            blocks.extend_from_slice(&u.to_le_bytes());
        }
        blocks.extend_from_slice(&[0, 0]);
    }
    blocks.push(0);
    let lang = b"en-US\0";
    let hdr = 46 + lang.len();
    let mut p = Vec::new();
    let len = (hdr + blocks.len()) as u32;
    p.extend_from_slice(&(len | 0x04 << 24).to_le_bytes());
    p.extend_from_slice(&(hdr as u32).to_le_bytes());
    p.extend_from_slice(&(hdr as u32).to_le_bytes());
    p.extend_from_slice(&[0; 32]);
    p.extend_from_slice(&le(0));
    p.extend_from_slice(lang);
    p.extend_from_slice(&blocks);
    p
}

fn package_list(ifr: &[u8], strs: &[u8]) -> Vec<u8> {
    let mut forms = Vec::new();
    forms.extend_from_slice(&((ifr.len() as u32 + 4) | 0x02 << 24).to_le_bytes());
    forms.extend_from_slice(ifr);
    let endp = (4u32 | 0xDF << 24).to_le_bytes();
    let body = [&forms[..], strs, &endp].concat();
    let mut v = vec![0x55; 16];
    v.extend_from_slice(&((body.len() + 20) as u32).to_le_bytes());
    v.extend_from_slice(&body);
    v
}

// Strings: 1 "Setup", 2 "Main", 3 "Fan Mode", 4 "Quiet", 5 "Loud",
// 6 "Turbo", 7 "Wake on LAN", 8 "Fan Speed", 9 "Advanced", 10 "Hidden",
// 11 "Locked"
const STRS: &[&str] = &["Setup", "Main", "Fan Mode", "Quiet", "Loud", "Turbo", "Wake on LAN", "Fan Speed", "Advanced", "Hidden", "Locked"];

/// One form set, varstore 1 "Setup" (8 bytes): one-of at 0 (qid 1, three
/// choices, Turbo shown only while Wake on LAN is on), checkbox at 1
/// (qid 2), numeric u16 at 2 (qid 3, 10..100 step 10), a question
/// suppressed while the fan mode is Quiet, a locked one, and a ref.
fn sample() -> Vec<u8> {
    let mut b = Vec::new();
    b.extend(op(ifr::OP_FORM_SET, true, &[&FS_GUID[..], &le(1), &le(0), &[0]].concat()));
    b.extend(op(ifr::OP_VARSTORE, false, &[&VS_GUID[..], &le(1), &le(8), b"Setup\0"].concat()));
    b.extend(op(ifr::OP_FORM, true, &[&le(1)[..], &le(2)].concat()));
    // one-of
    b.extend(op(ifr::OP_ONE_OF, true, &[&qhdr(3, 1, 1, 0, 0)[..], &[0], &[0, 2, 1]].concat()));
    b.extend(op(ifr::OP_ONE_OF_OPTION, false, &[&le(4)[..], &[0x10, 0], &[0]].concat()));
    b.extend(op(ifr::OP_ONE_OF_OPTION, false, &[&le(5)[..], &[0, 0], &[1]].concat()));
    // suppressif NOT (ideqval q2 == 1) on the Turbo option
    b.extend(op(ifr::OP_SUPPRESS_IF, true, &[]));
    b.extend(op(0x12, false, &[&le(2)[..], &le(1)].concat()));
    b.extend(op(0x17, false, &[]));
    b.extend(op(ifr::OP_ONE_OF_OPTION, false, &[&le(6)[..], &[0, 0], &[2]].concat()));
    b.extend(end());
    b.extend(end());
    // checkbox
    b.extend(op(ifr::OP_CHECKBOX, false, &[&qhdr(7, 2, 1, 1, 0)[..], &[0]].concat()));
    // numeric u16
    b.extend(op(ifr::OP_NUMERIC, false, &[&qhdr(8, 3, 1, 2, 0)[..], &[1], &le(10), &le(100), &le(10)].concat()));
    // suppressif ideqval q1 == 0: "Hidden"
    b.extend(op(ifr::OP_SUPPRESS_IF, true, &[]));
    b.extend(op(0x12, false, &[&le(1)[..], &le(0)].concat()));
    b.extend(op(ifr::OP_CHECKBOX, false, &[&qhdr(10, 4, 1, 4, 0)[..], &[0]].concat()));
    b.extend(end());
    // grayoutif TRUE: "Locked"
    b.extend(op(ifr::OP_GRAY_OUT_IF, true, &[]));
    b.extend(op(0x46, false, &[]));
    b.extend(op(ifr::OP_CHECKBOX, false, &[&qhdr(11, 5, 1, 5, 0)[..], &[0]].concat()));
    b.extend(end());
    // goto form 2
    b.extend(op(ifr::OP_REF, false, &[&qhdr(9, 6, 0, 0, 0)[..], &le(2)].concat()));
    b.extend(end()); // form 1
    b.extend(op(ifr::OP_FORM, true, &[&le(2)[..], &le(9)].concat()));
    b.extend(op(ifr::OP_SUBTITLE, false, &[&le(9)[..], &le(0), &[0]].concat()));
    b.extend(end());
    b.extend(end()); // form set
    package_list(&b, &strings(STRS))
}

/// The export: VS_GUID/"Setup", one block of 8 bytes. Fan mode 1 (Loud),
/// Wake on LAN off, speed 50.
fn sample_config() -> String {
    let mut s = format!("GUID={}&NAME=", cfg::guid_hex(&VS_GUID));
    for u in "Setup".encode_utf16() {
        s.push_str(&format!("{u:04x}"));
    }
    s.push_str("&PATH=0104140000");
    s.push_str(&cfg::block(0, &[1, 0, 50, 0, 0, 0, 0, 0]));
    // a defaults record must not count
    s.push_str(&format!("&GUID={}&NAME=", cfg::guid_hex(&VS_GUID)));
    for u in "Setup".encode_utf16() {
        s.push_str(&format!("{u:04x}"));
    }
    s.push_str("&PATH=0104140000&ALTCFG=0000");
    s.push_str(&cfg::block(0, &[9, 9, 9, 9, 9, 9, 9, 9]));
    s
}

fn load() -> Db {
    Db::load(&sample(), &sample_config(), &mut |_, _| None)
}

#[test]
fn strings_and_forms() {
    let db = load();
    assert_eq!(db.sets.len(), 1);
    assert_eq!(db.menu_sets(), vec![0]);
    let s = &db.sets[0];
    assert_eq!(db.str(0, s.fs.title), "Setup");
    assert_eq!(s.fs.forms.len(), 2);
    let items = &s.fs.forms[0].items;
    let names: Vec<&str> = items.iter().map(|i| db.str(0, i.prompt)).collect();
    assert_eq!(names, ["Fan Mode", "Wake on LAN", "Fan Speed", "Hidden", "Locked", "Advanced"]);
    assert!(matches!(items[5].kind, Kind::Ref { form: 2, formset: None }));
    assert_eq!(db.find_form(0, 2), Some(1));
}

#[test]
fn values_and_conditions() {
    let mut db = load();
    let items = db.sets[0].fs.forms[0].items.clone();
    assert_eq!(db.value_text(0, &items[0], true), "Loud");
    assert_eq!(db.value_text(0, &items[1], true), "Off");
    assert_eq!(db.value_text(0, &items[2], true), "50");
    // Turbo is hidden while Wake on LAN is off
    assert_eq!(db.options(0, &items[0]).len(), 2);
    // "Hidden" shows while the fan mode isn't Quiet
    assert!(!db.hidden(0, &items[3].conds));
    assert!(db.step(0, 0, 0, 1)); // Loud -> Quiet (wraps)
    assert_eq!(db.value_text(0, &items[0], true), "Quiet");
    assert!(db.hidden(0, &items[3].conds));
    assert!(db.step(0, 0, 1, 1)); // Wake on LAN on
    assert_eq!(db.options(0, &items[0]).len(), 3);
    assert_eq!(db.editable(0, &items[4]), Err(Locked::ReadOnly));
    assert!(!db.step(0, 0, 4, 1));
    assert_eq!(db.editable(0, &items[5]), Err(Locked::Kind));
}

#[test]
fn numeric_steps_clamp() {
    let mut db = load();
    for _ in 0..10 {
        db.step(0, 0, 2, 1);
    }
    let it = db.sets[0].fs.forms[0].items[2].clone();
    assert_eq!(db.value_text(0, &it, true), "100");
    for _ in 0..20 {
        db.step(0, 0, 2, -1);
    }
    assert_eq!(db.value_text(0, &it, true), "10");
}

#[test]
fn changes_and_route_strings() {
    let mut db = load();
    assert!(!db.pending());
    db.step(0, 0, 1, 1); // Wake on LAN on: byte 1
    db.step(0, 0, 2, 1); // speed 60: byte 2
    let ch = db.changes();
    assert_eq!(ch.len(), 2);
    assert_eq!((ch[0].prompt.as_str(), ch[0].old.as_str(), ch[0].new.as_str()), ("Wake on LAN", "Off", "On"));
    assert_eq!((ch[1].old.as_str(), ch[1].new.as_str()), ("50", "60"));
    let (new, old) = db.route_strings();
    let hdr = db.stores[0].hdr.clone().unwrap();
    assert_eq!(new, format!("{hdr}&OFFSET=0001&WIDTH=0002&VALUE=3c01"));
    assert_eq!(old, format!("{hdr}&OFFSET=0001&WIDTH=0002&VALUE=3200"));
    // Both parse back to the same store and bytes.
    let r = cfg::parse(&new);
    assert_eq!(r.len(), 1);
    assert_eq!(r[0].name, "Setup");
    assert_eq!(r[0].blocks, vec![(1, vec![1, 60])]);
    db.discard();
    assert!(!db.pending());
}

#[test]
fn store_without_export_is_read_only() {
    let mut db = Db::load(&sample(), "", &mut |name, guid| {
        assert_eq!((name, guid), ("Setup", &VS_GUID));
        Some(vec![0, 1, 20, 0, 0, 0, 0, 0])
    });
    let it = db.sets[0].fs.forms[0].items[1].clone();
    assert_eq!(db.value_text(0, &it, true), "On");
    assert_eq!(db.editable(0, &it), Err(Locked::NoValue));
    assert!(!db.step(0, 0, 1, 1));
}

#[test]
fn damaged_input_does_not_panic() {
    let full = sample();
    for n in 0..full.len() {
        let db = Db::load(&full[..n], &sample_config()[..n.min(40)], &mut |_, _| None);
        for (si, s) in db.sets.iter().enumerate() {
            for f in &s.fs.forms {
                for it in &f.items {
                    let _ = db.value_text(si, it, true);
                    let _ = db.hidden(si, &it.conds);
                }
            }
        }
    }
    let mut junk = full.clone();
    for (i, b) in junk.iter_mut().enumerate().skip(20) {
        *b = b.wrapping_mul(31).wrapping_add(i as u8);
    }
    let _ = Db::load(&junk, "GUID=zz&NAME=&OFFSET=&VALUE=&&&", &mut |_, _| None);
}
