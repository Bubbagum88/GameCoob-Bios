//! intro-render: run the rebuilt BS2 intro on the host and write chosen
//! frames to PNG, for side-by-side checks against a capture of the real IPL.
//!
//! Usage: intro-render <ipl.bin> <out-dir> [frame...]   (default: every 10th)
//! Env: HOLD_A=start-end holds the A button over that frame range.

extern crate alloc;

#[path = "../../../src/ipl/mod.rs"]
#[allow(dead_code)]
mod ipl;

#[path = "../../../src/sdk/mod.rs"]
#[allow(dead_code)]
mod sdk;

#[path = "../../../src/bs2/mod.rs"]
#[allow(dead_code)]
mod bs2;

mod png;

use std::path::PathBuf;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 3 {
        eprintln!("usage: {} <ipl.bin> <out-dir> [frame...]", args[0]);
        std::process::exit(2);
    }
    let rom = std::fs::read(&args[1]).expect("read ipl.bin");
    let out = PathBuf::from(&args[2]);
    std::fs::create_dir_all(&out).unwrap();
    let ipl = ipl::Ipl::parse(&rom).expect("parse ipl");
    let assets = bs2::Assets::load(&ipl).expect("load assets");
    let frames: Vec<u32> = if args.len() > 3 {
        args[3..].iter().map(|s| s.parse().unwrap()).collect()
    } else {
        (0..600).step_by(10).collect()
    };
    let hold: Option<(u32, u32)> = std::env::var("HOLD_A").ok().and_then(|s| {
        let (a, b) = s.split_once('-')?;
        Some((a.parse().ok()?, b.parse().ok()?))
    });
    let last = *frames.iter().max().unwrap();
    let mut intro = bs2::Intro::new(&assets);
    let mut target = bs2::raster::Target::new(bs2::render::EFB_W, bs2::render::EFB_H);
    for f in 0..=last {
        let a = hold.is_some_and(|(s, e)| f >= s && f <= e);
        if let Some(j) = intro.update(bs2::Input { a, egg: None }) {
            println!("frame {f}: jingle {j:?}");
        }
        if frames.contains(&f) {
            let t0 = std::time::Instant::now();
            let threads: usize = std::env::var("THREADS").ok().and_then(|s| s.parse().ok()).unwrap_or(1);
            intro.render(&mut target, &assets, &|job| {
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
            println!("frame {f}: phase {:?} state {:?} ({:.1} ms)", intro.phase(), intro.state, ms);
        }
    }
}
