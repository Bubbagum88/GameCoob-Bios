//! BS2's own small J3D model renderer, "gMR" (`gMRdraw.c`, 0x81303CAC..
//! 0x813053F8, and the material half of `gMRtexture.c`, 0x81305F48..
//! 0x81306A3C), drawing through [`Gx`].
//!
//! The model data comes from [`crate::ipl::j3d`], which reads the same J3D1
//! blocks `gMRbuild.c` relocates (joints, materials and shapes through
//! their remap tables, the INF1 tree as [`Model::nodes`]). Two things
//! differ from BS2 on purpose:
//!
//! * Shapes are sent in immediate mode from the decoded triangles instead
//!   of `GXCallDisplayList` over the vertex arrays (`fn_81303DE4`), so the
//!   vertex formats and arrays are not set up here.
//! * Textures come decoded ([`FastTex`]) instead of through
//!   `J3D_LoadTexture`'s texture objects.

use super::mtx::{self, Srt};
use super::render::Prepared;
use crate::ipl::j3d::{Model, Node, NODE_JOINT, NODE_MATERIAL, NODE_SHAPE};
use crate::sdk::gx::*;
use crate::sdk::mtx::{self as psmtx, Mtx};
use alloc::vec;
use alloc::vec::Vec;
use core::sync::atomic::{AtomicU16, Ordering};

/// `lbl_814ACFD8`: the material the last material node loaded. It is a
/// global, so a packet that starts at a shape node uses whatever material
/// was loaded before, even from another model.
static CUR_MAT: AtomicU16 = AtomicU16::new(0xFFFF);

/// `GMRInstance`: one placed model.
#[derive(Clone)]
pub struct Instance {
    /// Model-view matrix of each joint, from [`calc_joints`] or
    /// [`animate_joints`].
    pub joint_mtx: Vec<Mtx>,
    /// `baseMtx`: model matrix, then model-view after [`set_view_mtx`].
    pub base: Mtx,
    /// Alpha below 255 adds a stage that scales alpha and turns z off.
    pub alpha: u8,
    /// `matAnm`: material color 0 of each material from the color
    /// animation, in place of the model's (`gmr_AnimMaterials`).
    pub mat_anm: Option<Vec<[u8; 4]>>,
    /// `(material, i, color)`: TEV colors a caller swaps in for a draw by
    /// pointing `model->material[m].tevColor[i]` at its own (the menu
    /// icons' tints, the mosaic cubes' colors, the memory card blocks').
    pub tev_color: Vec<(u16, usize, [i16; 4])>,
    /// A material whose first texture a caller clears (`texNo[0] = 0xFFFF`)
    /// so that it draws with whatever TEXMAP0 holds (a memory card file's
    /// icon on its block).
    pub no_tex0: Option<u16>,
}

impl Instance {
    /// `J3D_InitModel`: joint matrices for `m`, alpha 255.
    pub fn new(m: &Model) -> Instance {
        Instance { joint_mtx: vec![psmtx::IDENTITY; m.joints.len()], base: psmtx::IDENTITY, alpha: 255, mat_anm: None, tev_color: Vec::new(), no_tex0: None }
    }
}

/// `J3D_SetModelMtx` (0x813053A8): `m` with its columns scaled by `scale`.
pub fn set_model_mtx(inst: &mut Instance, m: &Mtx, scale: [f32; 3]) {
    inst.base = *m;
    mtx::scale_cols(&mut inst.base, scale);
}

/// `J3D_SetViewMtx` (0x813053E4): `view · base`.
pub fn set_view_mtx(inst: &mut Instance, view: &Mtx) {
    inst.base = psmtx::concat(view, &inst.base);
}

/// `fn_81303CAC`: joint `idx`'s matrix from its transform, under `cur`.
fn joint_mtx(inst: &mut Instance, cur: &mut Mtx, idx: u16, xf: &Srt) {
    let mut m = mtx::rot_trans(xf.r[0] as i32, xf.r[1] as i32, xf.r[2] as i32, xf.t);
    if xf.s != [1.0; 3] {
        mtx::scale_cols(&mut m, xf.s);
    }
    *cur = psmtx::concat(cur, &m);
    if let Some(j) = inst.joint_mtx.get_mut(idx as usize) {
        *j = *cur;
    }
}

/// The joint's rest transform, as `fn_8130555C` copies it.
fn rest(m: &Model, idx: u16) -> Srt {
    match m.joints.get(idx as usize) {
        Some(j) => Srt { s: j.scale, r: j.rotation, t: j.translation },
        None => Srt { s: [1.0; 3], r: [0; 3], t: [0.0; 3] },
    }
}

/// `fn_8130402C` / `fn_81304240`: walk the tree, keeping the parent's
/// matrix across children and siblings.
fn walk_joints(m: &Model, inst: &mut Instance, cur: &mut Mtx, node: u16, xf: &dyn Fn(u16) -> Srt) {
    let Some(&n) = m.nodes.get(node as usize) else { return };
    let save = *cur;
    if n.ty == NODE_JOINT {
        joint_mtx(inst, cur, n.index, &xf(n.index));
    }
    if let Some(c) = n.child {
        walk_joints(m, inst, cur, c, xf);
    }
    *cur = save;
    if let Some(nx) = n.next {
        walk_joints(m, inst, cur, nx, xf);
    }
}

/// `J3D_CalcJoints` (0x813041F0): joint matrices in the rest pose.
pub fn calc_joints(m: &Model, inst: &mut Instance) {
    let mut cur = inst.base;
    walk_joints(m, inst, &mut cur, 0, &|j| rest(m, j));
}

/// `J3D_AnimateJoints` (0x813043BC): joint matrices from the joint
/// animation (`fn_81306CE0`, here `xf`, which returns `None` for a joint
/// the animation leaves alone) and, when `colors` is given, material color
/// 0 from the color animation (`fn_81306F14`).
pub fn animate_joints(m: &Model, inst: &mut Instance, xf: &dyn Fn(u16) -> Option<Srt>, colors: Option<&dyn Fn(u16) -> Option<[u8; 4]>>) {
    let mut cur = inst.base;
    walk_joints(m, inst, &mut cur, 0, &|j| xf(j).unwrap_or_else(|| rest(m, j)));
    if let Some(colors) = colors {
        let anm = (0..m.materials.len() as u16)
            .map(|i| colors(i).unwrap_or_else(|| m.materials[i as usize].mat_colors[0]))
            .collect();
        inst.mat_anm = Some(anm);
    }
}

/// `fn_81304CE8`: the GX state the models are drawn over. Called once a
/// frame before the scene's models (not before the logo text, which gets
/// whatever the previous frame left).
pub fn setup(gx: &mut Gx) {
    gx.set_cull_mode(GX_CULL_BACK);
    gx.set_color_update(true);
    gx.set_alpha_update(true);
    gx.set_blend_mode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA);
    for s in 0..16 {
        gx.set_tev_order(s, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
    }
    gx.set_tev_op(0, TevMode::Replace);
    for i in 0..10 {
        gx.load_pos_mtx_imm(&psmtx::IDENTITY, GX_PNMTX0 + i * 3);
        gx.load_nrm_mtx_imm(&psmtx::IDENTITY, GX_PNMTX0 + i * 3);
        gx.load_tex_mtx_imm(&psmtx::IDENTITY, GX_TEXMTX0 + i * 3);
    }
    gx.set_chan_mat_color(GX_COLOR0A0, [0xFF; 4]);
    gx.set_chan_mat_color(GX_COLOR1A1, [0xFF; 4]);
    gx.set_num_chans(1);
    gx.set_chan_ctrl(GX_COLOR0A0, false, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    gx.set_chan_ctrl(GX_COLOR1A1, false, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    gx.set_num_tex_gens(0);
    for i in 0..8 {
        gx.set_tex_coord_gen(i, GX_TG_MTX2X4, GX_TG_TEX0 + i, GX_IDENTITY);
    }
    gx.clear_vtx_desc();
    gx.set_alpha_compare(GX_ALWAYS, 0, 1, GX_ALWAYS, 0);
}

/// `fn_813061EC`: load all GX state of material `index`.
fn load_material<'a>(gx: &mut Gx<'a>, p: &'a Prepared, inst: &Instance, index: u16) {
    let m = &p.model;
    let Some(mat) = m.materials.get(index as usize) else { return };
    // MAT1 table indices, 0xFFFF where the material leaves a value unset.
    let set = |o: usize| mat.raw.get(o..o + 2).is_some_and(|b| b != [0xFF, 0xFF]);

    if mat.cull != 0xFF {
        gx.set_cull_mode(mat.cull);
    }
    if mat.chan_count != 0xFF {
        gx.set_num_chans(mat.chan_count);
    }
    if set(0x08) {
        let c = inst.mat_anm.as_ref().and_then(|a| a.get(index as usize)).copied().unwrap_or(mat.mat_colors[0]);
        gx.set_chan_mat_color(GX_COLOR0A0, c);
    }
    if set(0x0A) {
        gx.set_chan_mat_color(GX_COLOR1A1, mat.mat_colors[1]);
    }
    // The stored bytes are passed one argument along, with the ambient
    // source fixed to the register.
    for (k, chan) in [GX_COLOR0, GX_ALPHA0, GX_COLOR1, GX_ALPHA1].into_iter().enumerate() {
        if let Some(c) = mat.chans[k] {
            gx.set_chan_ctrl(chan, c.lighting, GX_SRC_REG, c.mat_src, c.light_mask, c.diffuse_fn, c.attn_fn);
        }
    }
    gx.set_num_tex_gens(mat.tex_gens.len() as u8);
    for (i, g) in mat.tex_gens.iter().enumerate() {
        gx.set_tex_coord_gen(i as u8, g.kind, g.src, g.mtx as u32);
    }
    for (i, t) in mat.tex_mtx.iter().enumerate() {
        if let Some(t) = t {
            gx.load_tex_mtx_imm(&t.m, GX_TEXMTX0 + i as u32 * 3);
        }
    }
    let stages = mat.tev_stages.len() as u8;
    gx.set_num_tev_stages(stages);
    for (i, c) in mat.tev_colors.iter().enumerate() {
        let c = inst.tev_color.iter().find(|t| t.0 == index && t.1 == i).map(|t| t.2).or(*c);
        if let Some(c) = c {
            gx.set_tev_color_s10(i as u8 + 1, c);
        }
    }
    for i in 0..8u8 {
        if let Some(st) = mat.tev_stages.get(i as usize) {
            gx.set_tev_kcolor_sel(i, GX_TEV_KCSEL_1_4);
            gx.set_tev_kalpha_sel(i, GX_TEV_KASEL_1);
            // Every IPL stage is a full setup (op 0xFF), never GXSetTevOp.
            gx.set_tev_color_in(i, st.color_in[0], st.color_in[1], st.color_in[2], st.color_in[3]);
            gx.set_tev_color_op(i, st.color_op, st.color_bias, st.color_scale, st.color_clamp, st.color_reg);
            gx.set_tev_alpha_in(i, st.alpha_in[0], st.alpha_in[1], st.alpha_in[2], st.alpha_in[3]);
            gx.set_tev_alpha_op(i, st.alpha_op, st.alpha_bias, st.alpha_scale, st.alpha_clamp, st.alpha_reg);
        }
        match mat.tev_orders.get(i as usize) {
            Some(o) => gx.set_tev_order(i, o.texcoord, o.texmap, o.channel),
            None => gx.set_tev_order(i, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL),
        }
        let tex = mat.textures[i as usize].filter(|_| !(i == 0 && inst.no_tex0 == Some(index)));
        if let Some(t) = tex.and_then(|t| p.tex(t)) {
            gx.load_tex_obj(t, i);
        }
    }
    if inst.alpha != 0xFF {
        // One more stage: alpha times the instance alpha (KCOLOR0's).
        gx.set_num_tev_stages(stages + 1);
        gx.set_tev_kcolor(GX_KCOLOR0, [0, 0, 0, inst.alpha]);
        gx.set_tev_kalpha_sel(stages, GX_TEV_KASEL_K0_A);
        gx.set_tev_color_in(stages, GX_CC_ZERO, GX_CC_ONE, GX_CC_CPREV, GX_CC_ZERO);
        gx.set_tev_color_op(stages, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
        gx.set_tev_alpha_in(stages, GX_CA_ZERO, GX_CA_KONST, GX_CA_APREV, GX_CA_ZERO);
        gx.set_tev_alpha_op(stages, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, true, GX_TEVPREV);
        gx.set_tev_order(stages, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        gx.set_z_mode(false, GX_LEQUAL, false);
    } else {
        gx.set_tev_kcolor(GX_KCOLOR0, [0, 0, 0, 0xFF]);
        if mat.flag & 1 == 1 {
            gx.set_z_mode(true, GX_LEQUAL, true);
        } else {
            gx.set_z_mode(false, GX_LEQUAL, false);
        }
    }
}

/// `fn_81304694`: the 3x3 part replaced by the lengths of its columns, so
/// the shape keeps its scale but faces the camera.
fn billboard(m: &mut Mtx) {
    let len = |c: usize| libm::sqrtf(m[0][c] * m[0][c] + m[1][c] * m[1][c] + m[2][c] * m[2][c]);
    let (sx, sy, sz) = (len(0), len(1), len(2));
    m[0][..3].copy_from_slice(&[sx, 0.0, 0.0]);
    m[1][..3].copy_from_slice(&[0.0, sy, 0.0]);
    m[2][..3].copy_from_slice(&[0.0, 0.0, sz]);
}

/// `fn_81304510`: for each normal-sourced texgen of material `mat`, load
/// its texture matrix times the shape's model-view rotation (environment
/// mapping in view space).
fn load_env_tex_mtx(gx: &mut Gx, m: &Model, mat: u16, view: &Mtx) {
    let Some(mat) = m.materials.get(mat as usize) else { return };
    let mut used = [false; 10];
    for g in &mat.tex_gens {
        if g.src == GX_TG_NRM {
            if let Some(u) = (g.mtx as u32).checked_sub(GX_TEXMTX0).and_then(|i| used.get_mut((i / 3) as usize)) {
                *u = true;
            }
        }
    }
    let mut rot = *view;
    for row in rot.iter_mut() {
        row[3] = 0.0;
    }
    for (j, t) in mat.tex_mtx.iter().enumerate() {
        if let (Some(t), true) = (t, used[j]) {
            gx.load_tex_mtx_imm(&psmtx::concat(&t.m, &rot), GX_TEXMTX0 + j as u32 * 3);
        }
    }
}

/// `fn_81304854`: draw shape `index` with the joint matrix of its first
/// packet.
fn draw_shape(gx: &mut Gx, m: &Model, inst: &mut Instance, index: u16, mat: u16) {
    let Some(shape) = m.shapes.get(index as usize) else { return };
    let Some(mtx) = inst.joint_mtx.get_mut(shape.draw_joint as usize) else { return };
    if shape.matrix_type == 1 {
        billboard(mtx);
    }
    let mtx = *mtx;
    // MtxInverse33 gives the inverse transpose; on a singular matrix the C
    // leaves its (uninitialized) stack matrix, here identity.
    let nrm = mtx::inverse33(&mtx).unwrap_or(psmtx::IDENTITY);
    gx.load_pos_mtx_imm(&mtx, GX_PNMTX0);
    gx.load_nrm_mtx_imm(&nrm, GX_PNMTX0);
    load_env_tex_mtx(gx, m, mat, &mtx);

    // GXCallDisplayList, from the decoded triangles.
    gx.begin(GX_TRIANGLES, 0, shape.indices.len() as u16);
    for &i in &shape.indices {
        let Some(v) = shape.vertices.get(i as usize) else { continue };
        gx.position3f32(v.pos[0], v.pos[1], v.pos[2]);
        if shape.has_normal {
            gx.normal3f32(v.normal[0], v.normal[1], v.normal[2]);
        }
        if shape.has_color {
            gx.color1u32(u32::from_be_bytes(v.color));
        }
        if shape.has_uv {
            gx.tex_coord2f32(v.uv[0], v.uv[1]);
        }
    }
    gx.end();
}

/// `fn_81304AF8`: draw the subtree at `node` and the siblings after it.
fn draw_nodes<'a>(gx: &mut Gx<'a>, p: &'a Prepared, inst: &mut Instance, node: u16) {
    let Some(&Node { ty, index, child, next }) = p.model.nodes.get(node as usize) else { return };
    if ty == NODE_SHAPE {
        draw_shape(gx, &p.model, inst, index, CUR_MAT.load(Ordering::Relaxed));
    } else if ty == NODE_MATERIAL {
        load_material(gx, p, inst, index);
        CUR_MAT.store(index, Ordering::Relaxed);
    }
    if let Some(c) = child {
        draw_nodes(gx, p, inst, c);
    }
    if let Some(n) = next {
        draw_nodes(gx, p, inst, n);
    }
}

/// `J3D_Draw` (0x81305204): the whole model.
pub fn draw<'a>(gx: &mut Gx<'a>, p: &'a Prepared, inst: &mut Instance) {
    draw_nodes(gx, p, inst, 0);
}

/// `J3D_DrawPacket` (0x81305248): the subtree at `node` (BS2 passes it as a
/// byte offset into the node array, 0x18 bytes a node) and its later
/// siblings.
pub fn draw_packet<'a>(gx: &mut Gx<'a>, p: &'a Prepared, inst: &mut Instance, node: u16) {
    draw_nodes(gx, p, inst, node);
}
