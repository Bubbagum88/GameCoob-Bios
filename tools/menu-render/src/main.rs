//! menu-render: run the rebuilt BS2 intro into the rebuilt cube menu on the
//! host, with scripted pad input, and write chosen frames to PNG for
//! side-by-side checks against a capture of the real IPL.
//!
//! Usage: menu-render <ipl.bin> <out-dir> <script> [frame...]
//!
//! The script has one `frame button` pair per line (frames count from boot,
//! like a Dolphin frame dump): button is one of up, down, left, right, a, b,
//! start, system (F10), eject (D), slot1, slot2. Lines starting with `#` are ignored. Without frames, every 10th
//! frame up to the last scripted frame + 200 is written.
//! Env: THREADS=n rasterizes on n threads; SKIP=1 skips the intro;
//! DUMP=layout:N, strings:N or model:N prints that resource and exits.

extern crate alloc;

#[path = "../../../src/ipl/mod.rs"]
#[allow(dead_code)]
mod ipl;

#[path = "../../../src/bs2/mod.rs"]
#[allow(dead_code)]
mod bs2;

mod png;

use bs2::menu::backend::{FakeCards, FakeDrive};
use bs2::menu::{Menu, MenuAssets, Pad, SysInfo};
use std::collections::HashMap;
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
                        let q = bs2::mtx::apply(&p, v.pos);
                        for k in 0..3 {
                            lo[k] = lo[k].min(q[k]);
                            hi[k] = hi[k].max(q[k]);
                        }
                    }
                    let mt = &m.materials[d.material as usize];
                    if std::env::var("VERTS").is_ok() { for v in sh.vertices.iter() { println!("  v {:?} n {:?} j {}", v.pos, v.normal, v.joint); } }
                    if std::env::var("MATS").is_ok() { let mut c = mt.clone(); c.raw.clear(); println!("{c:#?}"); }
                    println!("draw {i}: shape {} mat {} {:?} flag {} colors {:?} tex {:?} bbox {lo:?}..{hi:?}", d.shape, d.material, mt.name, mt.flag, mt.mat_colors, mt.tex_gens.len());
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

    let mut intro = bs2::Intro::new(&assets);
    if std::env::var("SKIP").is_ok() {
        while !intro.finished() {
            intro.update(bs2::Input { a: false, egg: None });
        }
    }
    let mut menu: Option<Menu> = None;
    let mut target = bs2::raster::Target::new(bs2::render::EFB_W, bs2::render::EFB_H);
    for f in 1..=last {
        let keys = script.get(&f).cloned().unwrap_or_default();
        let has = |k: &str| keys.iter().any(|x| x == k);
        match &mut menu {
            None => {
                intro.update(bs2::Input { a: has("a"), egg: None });
                if std::env::var("TRACE").is_ok() {
                    let sc = &intro.scene;
                    println!(
                        "trace f{f} state={:?} spin={} speed={:.3} accel={:.3} wph={} wamp={} t={} mwob={} land={} drop={}",
                        intro.state, sc.spin, sc.spin_speed, sc.spin_accel, sc.wobble_phase, sc.wobble_amp, intro.spin_frame, intro.wobble_frame, sc.land_impulse, sc.drop_impulse
                    );
                }
                if intro.finished() {
                    println!("frame {f}: menu starts");
                    menu = Some(Menu::new(info(f), Box::new(FakeDrive::new()), Box::new(FakeCards::new()), intro.wobble_frame, &intro.sin));
                }
            }
            Some(m) => {
                m.info.time = info(f).time;
                let pad = Pad {
                    a: has("a"),
                    b: has("b"),
                    start: has("start"),
                    up: has("up"),
                    down: has("down"),
                    left: has("left"),
                    right: has("right"),
                    system: has("system"),
                    eject: has("eject"),
                    slot: [has("slot1"), has("slot2")],
                };
                if let Some(act) = m.update(&pad, &intro.sin) {
                    println!("frame {f}: action {act:?}");
                }
                m.tick_guide();
            }
        }
        if frames.contains(&f) {
            let t0 = std::time::Instant::now();
            intro.render_with_menu(&mut target, &assets, &ma, menu.as_ref(), &|job| {
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
            let st = menu.as_ref().map(|m| format!("{:?}", m.state)).unwrap_or_else(|| format!("intro {:?}", intro.state));
            println!("frame {f}: {st} ({ms:.1} ms)");
        }
    }
}
