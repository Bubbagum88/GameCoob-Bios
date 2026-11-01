//! menu-render: run the rebuilt BS2 intro into the rebuilt cube menu on the
//! host, with scripted pad input, and write chosen frames to PNG for
//! side-by-side checks against a capture of the real IPL.
//!
//! Usage: menu-render <ipl.bin> <out-dir> <script> [frame...]
//!
//! The script has one `frame button` pair per line (frames count from boot,
//! like a Dolphin frame dump): button is one of up, down, left, right, a, b,
//! start, x, y, z, system (F10), eject (D), slot1, slot2, reset (R). Pad
//! buttons are held on controller 1 for that one frame and go through BS2's
//! pad code, so the same button on back-to-back frames is one long press.
//! Lines starting with `#` are ignored. Without frames, every 10th
//! frame up to the last scripted frame + 200 is written.
//! Env: THREADS=n rasterizes on n threads; SKIP=1 skips the intro;
//! PADTRACE=1 prints the pad code's new presses;
//! RES=WxH renders at that size instead of 640x448;
//! CARDS=a,b sets each slot to saves, none or a blank card of N blocks, or
//! to a card image file (.raw/.mcp/.gcp) loaded as a real card (both slots then);
//! DUMP=layout:N, strings:N, model:N or raw:N prints that resource and exits
//! (raw writes the decompressed bytes to stdout).

extern crate alloc;

#[path = "../../../src/ipl/mod.rs"]
#[allow(dead_code)]
mod ipl;

#[path = "../../../src/sdk/mod.rs"]
#[allow(dead_code)]
mod sdk;

#[path = "../../../src/card/mod.rs"]
#[allow(dead_code)]
mod card;

#[path = "../../../src/bs2/mod.rs"]
#[allow(dead_code)]
mod bs2;

mod png;

use bs2::menu::backend::{FakeCards, FakeDrive};
use bs2::bs2main::{Bs2Main, FrameInput};
use bs2::menu::{MenuAssets, SysInfo};
use bs2::pad;
use std::collections::HashMap;
use sdk::mtx as psmtx;
use std::path::PathBuf;

fn info(frame: u32) -> SysInfo {
    // 2024-05-17 13:37:00, ticking at 50 frames per second.
    let s = 13 * 3600 + 37 * 60 + frame / 50;
    SysInfo {
        time: [2024, 5, 17, (s / 3600) as i32 % 24, (s / 60 % 60) as i32, (s % 60) as i32],
        entries: vec![
            (0x0001, "Windows Boot Manager".into()),
            (0x0003, "UEFI: Fedora Linux".into()),
            (0x0000, "UEFI Shell".into()),
            (0x0004, "EFI Network".into()),
        ],
        timeout: 5,
        vendor: "EDK II".into(),
        fw_ui: true,
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 4 {
        eprintln!("usage: {} <ipl.bin> <out-dir> <script> [frame...]", args[0]);
        std::process::exit(2);
    }
    let rom = std::fs::read(&args[1]).expect("read ipl.bin");
    let out = PathBuf::from(&args[2]);
    std::fs::create_dir_all(&out).unwrap();
    let mut script: HashMap<u32, Vec<String>> = HashMap::new();
    for line in std::fs::read_to_string(&args[3]).expect("read script").lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let mut it = line.split_whitespace();
        let (Some(f), Some(k)) = (it.next(), it.next()) else { continue };
        script.entry(f.parse().expect("frame number")).or_default().push(k.to_lowercase());
    }
    let ipl = ipl::Ipl::parse(&rom).expect("parse ipl");
    // DUMP=layout:N or DUMP=strings:N prints a resource and exits.
    if let Ok(d) = std::env::var("DUMP") {
        let (kind, id) = d.split_once(':').expect("DUMP=kind:id");
        let r = ipl.resource(id.parse().expect("id")).expect("resource");
        match kind {
            "model" => {
                let m = ipl::j3d::Model::parse(&r).expect("model");
                for (i, t) in m.textures.iter().enumerate() {
                    println!("tex {i}: {:?} fmt {} {}x{} wrap {} {}", t.name, t.format, t.width, t.height, t.wrap_s, t.wrap_t);
                    png::write_rgba(&out.join(format!("tex{i}.png")), t.width as usize, t.height as usize, &t.rgba).unwrap();
                }
                for (i, d) in m.draws.iter().enumerate() {
                    let sh = &m.shapes[d.shape as usize];
                    let (mut lo, mut hi) = ([f32::MAX; 3], [f32::MIN; 3]);
                    for v in &sh.vertices {
                        let p = ipl::j3d::Model::joint_world(&m, v.joint);
                        let q = psmtx::mult_vec(&p, v.pos);
                        for k in 0..3 {
                            lo[k] = lo[k].min(q[k]);
                            hi[k] = hi[k].max(q[k]);
                        }
                    }
                    let mt = &m.materials[d.material as usize];
                    if std::env::var("VERTS").is_ok() { for v in sh.vertices.iter() { println!("  v {:?} n {:?} j {}", v.pos, v.normal, v.joint); } }
                    if std::env::var("MATS").is_ok() { let mut c = mt.clone(); c.raw.clear(); println!("{c:#?}"); }
                    println!("draw {i}: shape {} mat {} {:?} flag {} colors {:?} tex {:?} bbox {lo:?}..{hi:?}", d.shape, d.material, mt.name, mt.flag, mt.mat_colors, mt.tex_gens.len());
                    if std::env::var("VERBOSE").is_ok() {
                        let mut m2 = mt.clone();
                        m2.raw.clear();
                        println!("{m2:#?}");
                        println!("verts {:?}", sh.vertices.iter().map(|v| (v.pos, v.uv, v.color, v.normal)).collect::<Vec<_>>());
                    }
                    println!("draw {i}: shape {} mat {} {:?} flag {} colors {:?} tev {:?} maps {:?} stages {} tex {:?} bbox {lo:?}..{hi:?}", d.shape, d.material, mt.name, mt.flag, mt.mat_colors, mt.tev_colors, mt.textures, mt.tev_stages.len(), mt.tex_gens.len());
                }
            }
            "inf1" => {
                let b = &r[..];
                let be32 = |o: usize| u32::from_be_bytes([b[o], b[o + 1], b[o + 2], b[o + 3]]) as usize;
                let mut s = 0x20;
                while &b[s..s + 4] != b"INF1" { s += be32(s + 4); }
                let mut p = s + be32(s + 0x14);
                let mut n = 0;
                let mut depth = 0;
                loop {
                    let ty = u16::from_be_bytes([b[p], b[p + 1]]);
                    let i = u16::from_be_bytes([b[p + 2], b[p + 3]]);
                    p += 4;
                    match ty {
                        0 => break,
                        1 => depth += 1,
                        2 => depth -= 1,
                        _ => { println!("node {n:2} (+{:4}) {}{ty:#x} {i}", n * 24, "  ".repeat(depth)); n += 1; }
                    }
                }
            }
            "layout" => println!("{:#?}", ipl::glh::Layout::parse(&r).expect("layout")),
            "raw" => {
                use std::io::Write;
                std::io::stdout().write_all(&r).unwrap();
            }
            _ => {
                for (i, s) in ipl::glh::Strings::parse(&r).expect("strings").0.iter().enumerate() {
                    println!("{i:3} pane {:2} {:?}", s.pane, String::from_utf8_lossy(&s.text));
                }
            }
        }
        return;
    }
    let assets = bs2::Assets::load(&ipl).expect("load intro assets");
    let ma = MenuAssets::load(&ipl).expect("load menu assets");
    let last_input = script.keys().copied().max().unwrap_or(0);
    let frames: Vec<u32> =
        if args.len() > 4 { args[4..].iter().map(|s| s.parse().unwrap()).collect() } else { (0..last_input + 200).step_by(10).collect() };
    let last = *frames.iter().max().unwrap();
    let threads: usize = std::env::var("THREADS").ok().and_then(|s| s.parse().ok()).unwrap_or(1);

    // CARDS=a,b sets each slot to `saves`, `none` or a blank card of N blocks,
    // or (for both slots then) a card image file loaded as a real card.
    let spec: Vec<String> = std::env::var("CARDS").map(|v| v.split(',').map(String::from).collect()).unwrap_or_default();
    let cards: Box<dyn bs2::menu::backend::MemCards> = if spec.iter().any(|c| std::path::Path::new(c).is_file()) {
        let mut cards = bs2::menu::card_image::ImageCards::new();
        for (i, c) in spec.iter().enumerate().take(2) {
            if c != "none" {
                cards.insert(i, c.clone(), std::fs::read(c).expect("read card image"));
            }
        }
        Box::new(cards)
    } else {
        let mut cards = FakeCards::new();
        for (i, c) in spec.iter().enumerate().take(2) {
            match c.as_str() {
                "saves" => {}
                "none" => cards.set_blank(i, None),
                n => cards.set_blank(i, Some(n.parse().expect("CARDS=saves|none|blocks|image file"))),
            }
        }
        Box::new(cards)
    };
    let mut bs = Bs2Main::new(&assets, Box::new(FakeDrive::new()), cards);
    if std::env::var("SKIP").is_ok() {
        bs.skip_intro();
    }
    // RES=WxH renders at another size, as the app does on a bigger screen.
    let (rw, rh) = std::env::var("RES")
        .ok()
        .and_then(|r| r.split_once('x').map(|(w, h)| (w.parse().unwrap(), h.parse().unwrap())))
        .unwrap_or((bs2::render::EFB_W, bs2::render::EFB_H));
    let mut target = bs2::raster::Target::new(rw, rh);
    for f in 1..=last {
        let keys = script.get(&f).cloned().unwrap_or_default();
        let has = |k: &str| keys.iter().any(|x| x == k);
        if let Some(m) = &mut bs.menu {
            m.info.time = info(f).time;
        }
        // Script keys hold their button on controller 1 for one frame.
        let mut input = FrameInput { system: has("system"), eject: has("eject"), slot: [has("slot1"), has("slot2")], reset: has("reset"), ..FrameInput::default() };
        for (k, b) in [
            ("a", pad::PAD_BUTTON_A),
            ("b", pad::PAD_BUTTON_B),
            ("start", pad::PAD_BUTTON_START),
            ("up", pad::PAD_BUTTON_UP),
            ("down", pad::PAD_BUTTON_DOWN),
            ("left", pad::PAD_BUTTON_LEFT),
            ("right", pad::PAD_BUTTON_RIGHT),
            ("x", pad::PAD_BUTTON_X),
            ("y", pad::PAD_BUTTON_Y),
            ("z", pad::PAD_TRIGGER_Z),
        ] {
            if has(k) {
                input.pads[0].button |= b;
            }
        }
        let in_menu = bs.menu.is_some();
        if std::env::var("PADTRACE").is_ok() && (bs.pads.menu.dirs_new | bs.pads.menu.button_down) != 0 {
            println!("pad f{} dirs_new {:#06x} down {:#06x}", f - 1, bs.pads.menu.dirs_new, bs.pads.menu.button_down);
        }
        if let Some(act) = bs.update(&assets, &ma, &input, || info(f)) {
            println!("frame {f}: action {act:?}");
        }
        if !in_menu {
            if std::env::var("TRACE").is_ok() {
                let (intro, sc) = (&bs.intro, &bs.intro.scene);
                println!(
                    "trace f{f} state={:?} spin={} speed={:.3} accel={:.3} wph={} wamp={} t={} mwob={} land={} drop={}",
                    intro.state, sc.spin, sc.spin_speed, sc.spin_accel, sc.wobble_phase, sc.wobble_amp, sc.out_frame, intro.menu.wobble_frame, sc.land_impulse, sc.drop_impulse
                );
            }
            if bs.menu.is_some() {
                println!("frame {f}: menu starts");
            }
        } else if bs.menu.is_none() {
            println!("frame {f}: reset, intro again");
        }
        if frames.contains(&f) {
            let t0 = std::time::Instant::now();
            bs.render(&mut target, &assets, &ma, &|job| {
                std::thread::scope(|sc| {
                    for _ in 1..threads {
                        sc.spawn(|| job.work());
                    }
                    job.work();
                });
            });
            let ms = t0.elapsed().as_secs_f64() * 1e3;
            let rgba: Vec<u8> = target
                .color
                .iter()
                .flat_map(|c| [c[0], c[1], c[2]].map(|x| (x.clamp(0.0, 1.0) * 255.0 + 0.5) as u8).into_iter().chain([255]))
                .collect();
            png::write_rgba(&out.join(format!("f{f:04}.png")), target.w, target.h, &rgba).unwrap();
            let st = bs.menu.as_ref().map(|m| format!("{:?}", m.state)).unwrap_or_else(|| format!("intro {:?}", bs.intro.state));
            println!("frame {f}: {st} ({ms:.1} ms)");
        }
    }
}
