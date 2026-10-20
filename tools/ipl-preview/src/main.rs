//! ipl-preview: render the IPL's intro models and textures to PNG with the
//! same `src/ipl` code the UEFI app uses, and print what was parsed.
//!
//! Usage: ipl-preview <ipl.bin> [out-dir]
//!
//! Build: cargo run --release --target x86_64-unknown-linux-gnu -- <ipl.bin> /tmp/previews

extern crate alloc;

#[path = "../../../src/ipl/mod.rs"]
#[allow(dead_code)]
mod ipl;

mod png;
mod raster;

use ipl::gx::{self, Texture};
use ipl::j3d::Model;
use std::path::{Path, PathBuf};
use std::time::Instant;

const MODELS: [u16; 10] = [0x23, 0x24, 0x66, 0x67, 0x6B, 0xA8, 0x57, 0x58, 0x68, 0xB2];
const BTI_IDS: [u16; 1] = [0x65];

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        eprintln!("usage: {} <ipl.bin> [out-dir]", args[0]);
        std::process::exit(2);
    }
    let out = PathBuf::from(args.get(2).map(String::as_str).unwrap_or("previews"));
    std::fs::create_dir_all(&out).expect("create out dir");
    let rom = std::fs::read(&args[1]).expect("read ipl.bin");

    let t0 = Instant::now();
    let ipl = match ipl::Ipl::parse(&rom) {
        Ok(i) => i,
        Err(e) => {
            eprintln!("parse failed: {e:?}");
            std::process::exit(1);
        }
    };
    println!(
        "IPL: descrambled + archive found in {:.1} ms; archive at ROM {:#x} with {} entries",
        t0.elapsed().as_secs_f64() * 1e3,
        ipl.archive_offset(),
        ipl.resource_count()
    );

    let t1 = Instant::now();
    let scanned = ipl::find_archive(ipl.rom());
    println!(
        "archive scan (fallback for other IPL versions) found {:x?} in {:.1} ms",
        scanned,
        t1.elapsed().as_secs_f64() * 1e3
    );
    let mut line = String::new();
    for id in 0..ipl.resource_count() as u16 {
        let magic = match ipl.resource(id) {
            Ok(d) => String::from_utf8_lossy(&d[..d.len().min(4)]).replace(|c: char| !c.is_ascii_graphic(), "."),
            Err(e) => format!("{e:?}"),
        };
        line += &format!(" {id:02x}:{magic:<4}");
        if id % 12 == 11 {
            println!("  {line}");
            line.clear();
        }
    }
    if !line.is_empty() {
        println!("  {line}");
    }

    let mut sheet: Vec<(String, Texture)> = Vec::new();
    for id in MODELS {
        let (rom_off, _) = ipl.resource_span(id).unwrap_or((0, 0));
        println!("\n== resource {id:#04x} (ROM {rom_off:#08x}) ==");
        let data = match ipl.resource(id) {
            Ok(d) => d,
            Err(e) => {
                println!("  resource error: {e:?}");
                continue;
            }
        };
        let t = Instant::now();
        let model = match Model::parse(&data) {
            Ok(m) => m,
            Err(e) => {
                println!("  model parse error: {e:?}");
                continue;
            }
        };
        println!("  parsed {} bytes in {:.2} ms", data.len(), t.elapsed().as_secs_f64() * 1e3);
        print_model(&model);
        let img = raster::render_model(&model, 640, 480);
        let path = out.join(format!("model_{id:02x}.png"));
        png::write_rgba(&path, 640, 480, &img).expect("write png");
        println!("  -> {}", path.display());
        for (i, tex) in model.textures.iter().enumerate() {
            let name = format!("{id:02x}_tex{i:02}_{}_{}x{}", gx::format_name(tex.format), tex.width, tex.height);
            write_texture(&out, &name, tex);
            sheet.push((name, tex.clone()));
        }
    }
    for id in BTI_IDS {
        println!("\n== resource {id:#04x} (BTI) ==");
        match ipl.resource(id).and_then(|d| ipl::bti::parse(&d)) {
            Ok(tex) => {
                println!("  {} {}x{} wrap {}/{}", gx::format_name(tex.format), tex.width, tex.height, tex.wrap_s, tex.wrap_t);
                let name = format!("{id:02x}_bti_{}_{}x{}", gx::format_name(tex.format), tex.width, tex.height);
                write_texture(&out, &name, &tex);
                sheet.push((name, tex));
            }
            Err(e) => println!("  error: {e:?}"),
        }
    }
    let (w, h, img) = raster::contact_sheet(&sheet);
    let path = out.join("textures_sheet.png");
    png::write_rgba(&path, w, h, &img).expect("write sheet");
    println!("\ncontact sheet ({} textures, row-major, 4 per row) -> {}", sheet.len(), path.display());
    for (i, (n, _)) in sheet.iter().enumerate() {
        println!("  [{i:2}] {n}");
    }
}

fn write_texture(out: &Path, name: &str, tex: &Texture) {
    let dir = out.join("textures");
    std::fs::create_dir_all(&dir).expect("create textures dir");
    png::write_rgba(&dir.join(format!("{name}.png")), tex.width as usize, tex.height as usize, &tex.rgba).expect("write texture");
}

fn blend_name(f: u8) -> &'static str {
    ["ZERO", "ONE", "SRCCLR", "INVSRCCLR", "SRCALPHA", "INVSRCALPHA", "DSTALPHA", "INVDSTALPHA"].get(f as usize).copied().unwrap_or("?")
}

fn print_model(m: &Model) {
    let (v, t) = m.counts();
    let (lo, hi) = m.bounds();
    println!(
        "  {} joints={} shapes={} materials={} textures={} draws={} verts={} tris={} bounds=({:.1},{:.1},{:.1})..({:.1},{:.1},{:.1})",
        String::from_utf8_lossy(&m.kind),
        m.joints.len(),
        m.shapes.len(),
        m.materials.len(),
        m.textures.len(),
        m.draws.len(),
        v,
        t,
        lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]
    );
    for (i, j) in m.joints.iter().enumerate() {
        println!(
            "  joint {i:2} '{}' parent={:?} S=({:.2},{:.2},{:.2}) R=({:#06x},{:#06x},{:#06x}) T=({:.1},{:.1},{:.1})",
            j.name, j.parent, j.scale[0], j.scale[1], j.scale[2], j.rotation[0] as u16, j.rotation[1] as u16, j.rotation[2] as u16,
            j.translation[0], j.translation[1], j.translation[2]
        );
    }
    for d in &m.draws {
        let s = &m.shapes[d.shape as usize];
        let out = raster::outward_ratio(m, s);
        println!(
            "  draw shape {:2} mat {:2} joint {:2}: {} verts {} tris nrm={} col={} uv={} vtx-joint={} outward-CCW={:.2}",
            d.shape, d.material, d.joint, s.vertices.len(), s.indices.len() / 3, s.has_normal as u8, s.has_color as u8, s.has_uv as u8,
            s.joint, out
        );
    }
    for (i, mt) in m.materials.iter().enumerate() {
        let c = mt.chans[0].unwrap_or_default();
        let a = mt.chans[1].unwrap_or_default();
        println!(
            "  mat {i:2} '{}' flag={} cull={} matColor={:02x?} chan0(lit={} src={}) alpha0(lit={} src={}) tex={:?} blend=({} {} {}) z=({} f{} upd {}) acmp=({} {} op{} {} {})",
            mt.name, mt.flag, ["NONE", "FRONT", "BACK", "ALL"].get(mt.cull as usize).unwrap_or(&"?"), mt.mat_colors[0], c.lighting as u8, c.mat_src,
            a.lighting as u8, a.mat_src, mt.texture(), mt.blend.mode, blend_name(mt.blend.src), blend_name(mt.blend.dst),
            mt.z_mode.enable as u8, mt.z_mode.func, mt.z_mode.update as u8, mt.alpha_compare.comp0, mt.alpha_compare.ref0,
            mt.alpha_compare.op, mt.alpha_compare.comp1, mt.alpha_compare.ref1
        );
        for (k, g) in mt.tex_gens.iter().enumerate() {
            let tm = (g.mtx >= 30 && g.mtx < 60).then(|| mt.tex_mtx[((g.mtx - 30) / 3) as usize]).flatten();
            println!("      texgen{k}: kind {} src {} mtx {} -> {:?}", g.kind, g.src, g.mtx, tm.map(|t| (t.mode, t.m)));
        }
        println!("      tevColors(REG0,REG1,REG2,PREV)={:?}", mt.tev_colors);
        for (k, st) in mt.tev_stages.iter().enumerate() {
            let o = mt.tev_orders[k];
            println!(
                "      stage{k}: order(tc {} map {} ch {}) C=({:?} op{} b{} s{} cl{} ->{}) A=({:?} op{} b{} s{} cl{} ->{})",
                o.texcoord, o.texmap, o.channel, st.color_in, st.color_op, st.color_bias, st.color_scale, st.color_clamp as u8, st.color_reg,
                st.alpha_in, st.alpha_op, st.alpha_bias, st.alpha_scale, st.alpha_clamp as u8, st.alpha_reg
            );
        }
    }
    for (i, t) in m.textures.iter().enumerate() {
        println!(
            "  tex {i} '{}' {} {}x{} wrap {}/{} filter {}/{} mips {}",
            t.name, gx::format_name(t.format), t.width, t.height, t.wrap_s, t.wrap_t, t.min_filter, t.mag_filter, t.mip_count
        );
    }
}
