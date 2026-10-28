// Memory Card blocks (PAL 1.0 BS2, 0x8131EDBC..0x81321730): every file of
// both cards is a small glass block on a 4 x 4 window of the grid. Blocks
// fly in from the far corner when the screen opens, zoom when chosen, fly
// to the other card on Copy / Move and shrink and burst into 12 pieces on
// Erase. The block tint shows the card size.
//

#include <dolphin/gx.h>
#include "bs2/menu.h"
#include "bs2/menu_fx_cube.h"
#include "bs2/mtx.h"

#define CARD_BLOCK_BURST 12

/* One block (0x5C) */
typedef struct CardBlock {
    s16 start;             /* 0x00 frame the flight starts */
    s16 pad02[2];
    s16 frame;             /* 0x06 */
    s16 alpha;             /* 0x08 0..255 */
    s16 zoomFrame;         /* 0x0A */
    s16 end;               /* 0x0C frame the flight ends */
    s16 alphaTarget;       /* 0x0E 255: fade in, 0: fade out */
    s16 zoomLen;           /* 0x10 */
    f32 zoom;              /* 0x14 */
    f32 zoomMin;           /* 0x18 */
    f32 zoomMax;           /* 0x1C */
    Vec pos;               /* 0x20 */
    Vec vel;               /* 0x2C */
    Vec v0;                /* 0x38 flight: start velocity */
    Vec p0;                /* 0x44 flight: start position */
    Vec p1;                /* 0x50 flight: end position */
} CardBlock;

/* Sort order entry of the UI (8 bytes) */
typedef struct CardOrder {
    s32 file;
    u32 key;
} CardOrder;

static CardBlock sBlocks[2][CARD_MAX_FILES];
static J3DModel sFileModel;
static J3DModel sEmptyModel;
static J3DModelInstance sEmptyInst[2][CARD_MAX_FILES];
static J3DModelInstance sFileInst[2][CARD_MAX_FILES];
static J3DModelInstance sFlyInst;
static J3DModelInstance sBurstInst[CARD_BLOCK_BURST];
static CardBlock sFly;
static s16 sBurstFrame[CARD_BLOCK_BURST];
static s16 sBurstLen[CARD_BLOCK_BURST];
static s16 sBurstDelay[CARD_BLOCK_BURST];
static Mtx sFlyMtx;
static s16 sBurstAngle[CARD_BLOCK_BURST];

static s16 (*sGridX)[16];
static s16 (*sGridY)[16];
static u8 sOpSlot;
static u16 sOpDest;
static u16 sOpFile;
static u16 sOpKind;
static BOOL sFlying;
static BOOL sEntering;
static BOOL sErasing;
static BOOL sBursting;
static s16 sWobble;

s32 Card_GetSlot(void);
s32 Card_GetSelectedFile(void);
BOOL CardDraw_IconFrameCount(s32 slot, s32 file);
void CardBlock_Init(CardBlock* b);
void CardBlock_Rewind(CardBlock* b);
void CardBlock_Finish(CardBlock* b);
void CardBlock_CalcMtx(void);
void CardBlock_CalcBurst(s32 i);
BOOL fn_81320ABC(void);
GXColorS10* CardBlock_Color(u32 size, s32 kind, s32 slot);
void CardBlock_DrawFile(J3DModelInstance* inst, u8 slot, s32 file);
void CardBlock_DrawEmpty(J3DModelInstance* inst, u8 slot, s32 file);
void CardBlock_DrawBurst(J3DModelInstance* inst);
void CardBlock_DrawSlot(u8 slot, u8 alpha);

/* a by-value copy through this keeps the temp -> arg copy pair */
static inline Vec* CardVecRef(Vec* v) { (void)v; return v; }

/* 0x8131EDBC */
void CardBlock_Init(CardBlock* b) {
    Vec zero = {0.0f, 0.0f, 0.0f};
    CardParams* params = Card_GetParams();

    CardBlock_Rewind(b);
    b->pos = zero;
    b->vel = zero;
    b->p0 = zero;
    b->v0 = zero;
    b->p1 = zero;
    b->alphaTarget = 0;
    b->alpha = 0;
    b->zoomFrame = 0;
    b->zoomLen = 6;
    b->zoomMax = params->iconZoom;
    b->zoomMin = params->iconZoomSel;
    b->zoom = b->zoomMin;
}

/* 0x8131EEC4 */
void fn_8131EEC4(BOOL first, s16 (*gridX)[16], s16 (*gridY)[16]) {
    int slot;
    int i;
    int j;

    if (first) {
        sFileModel.data = ResGet(0x57);
        sEmptyModel.data = ResGet(0x58);
        J3D_LoadModelData(&sFileModel);
        J3D_LoadModelData(&sEmptyModel);
        for (slot = 0; slot < 2; slot++) {
            for (i = 0; i < CARD_MAX_FILES; i++) {
                sEmptyInst[slot][i].model = &sFileModel;
                sFileInst[slot][i].model = &sEmptyModel;
                J3D_InitModel(&sEmptyInst[slot][i], 0);
                J3D_InitModel(&sFileInst[slot][i], 0);
            }
        }
        sFlyInst.model = &sEmptyModel;
        J3D_InitModel(&sFlyInst, 0);
        for (j = 0; j < CARD_BLOCK_BURST; j++) {
            sBurstInst[j].model = &sEmptyModel;
            J3D_InitModel(&sBurstInst[j], 0);
        }
    }
    sGridX = gridX;
    sGridY = gridY;
    for (slot = 0; slot < 2; slot++) {
        for (i = 0; i < CARD_MAX_FILES; i++) {
            CardBlock_Init(&sBlocks[slot][i]);
        }
    }
    sFlying = FALSE;
    sEntering = TRUE;
    sErasing = FALSE;
    sBursting = FALSE;
    sWobble = 0;
    CardBlock_Init(&sFly);
}

/* 0x8131F048 */
/* "if (v) {}" keeps the param slot at the frame bottom without losing alias info */
static inline Vec* CardVecId(Vec* v) { if (v) {} return v; }
static inline SRT* CardSRTId(SRT* s) { if (s) {} return s; }
#define VEC_SETQ(dst, X, Y, Z)             \
    do {                                   \
        Vec v_ = {0.0f, 0.0f, 0.0f};       \
        CardVecId(&v_)->x = X;             \
        v_.y = Y;                          \
        v_.z = Z;                          \
        dst = v_;                          \
    } while (0)
/* the inline return gives the f2 -> f28 copy of t */
static inline f32 CardBlockT(CardBlock* b) { return (b->zoom - b->zoomMin) / (b->zoomMax - b->zoomMin); }
void CardBlock_CalcMtx(void) {
    Mtx m;
    Mtx base;
    Mtx rot;
    SRT srt;
    Vec scale;
    Vec pos;
    f32 t;
    MenuWork* menu;
    CardParams* params;
    CardFile (*files)[CARD_MAX_FILES];
    int slot;
    int i;
    s16 w;

    menu = Menu_Get();
    params = Card_GetParams();
    {
        SRT s_ = {1.0f, 1.0f, 1.0f, 0, 0, 0, 0, -292.0f, 224.0f, 0.0f};
        srt = *CardSRTId(&s_);
    }
    files = CardThread_GetFiles();
    w = sWobble;
    MtxFromSRT(&srt, base);
    for (slot = 0; slot < 2; slot++) {
        for (i = 0; i < CARD_MAX_FILES; i++) {
            VEC_SETQ(scale, sBlocks[slot][i].zoom, sBlocks[slot][i].zoom, sBlocks[slot][i].zoom);
            if (slot == sOpSlot && i == sOpFile && sFlying) {
                if (!sErasing) {
                    t = (sFly.zoom - sFly.zoomMin) / (sFly.zoomMax - sFly.zoomMin);
                    VEC_SETQ(pos, sFly.pos.x / 16.0f, -sFly.pos.y / 16.0f, 2.0f * t);
                }
            } else {
                t = CardBlockT(&sBlocks[slot][i]);
                VEC_SETQ(pos, sBlocks[slot][i].pos.x / 16.0f, -sBlocks[slot][i].pos.y / 16.0f, 2.0f * t);
            }
            MtxRot(t * menu->wobbleAmp[0] * CosS(w * 70), t * menu->wobbleAmp[1] * CosS(menu->wobblePhaseRY + w * 35),
                   t * menu->wobbleAmp[2] * CosS(w * 35), rot);
            PSMTXTrans(m, pos.x + t * params->f64_ * SinS(menu->wobblePhaseTX + w * 35),
                       pos.y - t * params->f68_ * SinS(w * 70), pos.z);
            PSMTXConcat(m, base, m);
            PSMTXConcat(m, rot, m);
            if (files[slot][i].used) {
                J3D_SetModelMtx(&sFileInst[slot][i], m, scale);
                J3D_SetViewMtx(&sFileInst[slot][i], fn_81302F94());
                J3D_CalcJoints(&sFileInst[slot][i]);
            } else {
                J3D_SetModelMtx(&sEmptyInst[slot][i], m, scale);
                J3D_SetViewMtx(&sEmptyInst[slot][i], fn_81302F94());
                J3D_CalcJoints(&sEmptyInst[slot][i]);
            }
        }
    }
    if (sErasing) {
        scale = (Vec){sFly.zoom, sFly.zoom, sFly.zoom};
        t = (sFly.zoom - sFly.zoomMin) / (sFly.zoomMax - sFly.zoomMin);
        pos = (Vec){sFly.pos.x / 16.0f, -sFly.pos.y / 16.0f, 2.0f * t};
        MtxRot(t * menu->wobbleAmp[0] * CosS(w * 70), t * menu->wobbleAmp[1] * CosS(menu->wobblePhaseRY + w * 35),
               t * menu->wobbleAmp[2] * CosS(w * 35), rot);
        PSMTXTrans(m, pos.x + t * params->f64_ * SinS(menu->wobblePhaseTX + w * 35),
                   pos.y - t * params->f68_ * SinS(w * 70), pos.z);
        PSMTXConcat(m, base, m);
        PSMTXConcat(m, rot, m);
        if (!sBursting) {
            J3D_SetModelMtx(&sFlyInst, m, scale);
            J3D_SetViewMtx(&sFlyInst, fn_81302F94());
            J3D_CalcJoints(&sFlyInst);
        } else {
            PSMTXCopy(m, sFlyMtx);
        }
    }
}

static inline void CardBlock_Move(CardBlock* b) {
    Vec p;
    Vec zero = {0.0f, 0.0f, 0.0f};

    if (b->frame >= b->start) {
        VecHermite(&b->p0, &b->v0, &b->p1, &zero, &p, b->frame, b->start, b->end);
    } else {
        p = b->p0;
    }
    PSVECSubtract(&p, &b->pos, &b->vel);
    b->pos = p;
    b->frame++;
    if (b->frame > b->end) {
        b->frame = b->end;
    }
}

/* 0x8131F770: one piece of the Erase burst */
static inline void CardBlock_SetBurstMtx(J3DModelInstance* inst, Mtx m) {
    Vec* s;
    Vec scale = {0.3f, 0.3f, 0.3f};
    s = &scale;
    J3D_SetModelMtx(inst, m, *s);
}

/* y comes first: the args are evaluated right to left, so SinS runs before CosS */
static inline void CardBurst_Trans(Mtx m, f32 y, f32 x) {
    PSMTXTrans(m, x, y, 0.0f);
}

void CardBlock_CalcBurst(s32 i) {
    Mtx m;
    u8 pad[16];
    f32 t;

    if (sBurstFrame[i] > sBurstDelay[i]) {
        if (sBurstFrame[i] - sBurstDelay[i] <= sBurstLen[i]) {
            t = (f32)(sBurstFrame[i] - sBurstDelay[i]) / (f32)sBurstLen[i];
        } else {
            t = 0.0f;
        }
    } else {
        t = 0.0f;
    }
    CardBurst_Trans(m, 48.0f * (0.25f + 0.75f * t) * CosS(sBurstAngle[i]),
                    48.0f * (0.25f + 0.75f * t) * SinS(sBurstAngle[i]));
    PSMTXConcat(sFlyMtx, m, m);
    CardBlock_SetBurstMtx(&sBurstInst[i], m);
    J3D_SetViewMtx(&sBurstInst[i], fn_81302F94());
    J3D_CalcJoints(&sBurstInst[i]);
}

/* 0x8131F8F8 */
/* the erased block shrinks; TRUE once it is gone */
static inline BOOL CardBlock_Shrink(void) {
    BOOL done = FALSE;

    sFly.zoom = Hermite(sFly.zoomFrame, 0.0f, sFly.zoomMin, 0.0f, sFly.zoomLen, sFly.zoomMax, 0.0f);
    sFly.zoomFrame--;
    if (sFly.zoomFrame > sFly.zoomLen) {
        sFly.zoomFrame = sFly.zoomLen;
    }
    if (sFly.zoomFrame < 0) {
        sFly.zoomFrame = 0;
        done = TRUE;
    }
    return done;
}

/* the Erase burst starts: random delays and directions */
static inline void CardBlock_StartBurst(void) {
    int i;

    for (i = 0; i < CARD_BLOCK_BURST; i++) {
        sBurstFrame[i] = 0;
        sBurstLen[i] = 10;
        sBurstDelay[i] = Rand() & 3;
        sBurstAngle[i] = (u16)Rand();
    }
}

/* one frame of the Erase burst; TRUE once every piece is done */
static inline BOOL CardBlock_StepBurst(void) {
    BOOL done;
    int i;
    int n;

    done = FALSE;
    n = 0;
    for (i = 0; i < CARD_BLOCK_BURST; i++) {
        sBurstFrame[i]++;
        if (sBurstFrame[i] > sBurstDelay[i] && sBurstFrame[i] - sBurstDelay[i] > sBurstLen[i]) {
            sBurstFrame[i] = sBurstLen[i] + sBurstDelay[i];
            n++;
        }
        CardBlock_CalcBurst(i);
    }
    if (n == CARD_BLOCK_BURST) {
        done = TRUE;
    }
    return done;
}

void fn_8131F8F8(void) {
    CardBlock* b;
    int slot;
    int i;
    int step;
    int n;

    if (!CardIcon_IsSettled()) {
        return;
    }
    CardBlock_CalcMtx();
    if (sErasing) {
        if (sFlying) {
            sFly.zoom = Hermite(sFly.zoomFrame, 0.0f, sFly.zoomMin, 0.0f, sFly.zoomLen, sFly.zoomMax, 0.0f);
            sFly.zoomFrame--;
            if (sFly.zoomFrame > sFly.zoomLen) {
                sFly.zoomFrame = sFly.zoomLen;
            }
            if (sFly.zoomFrame < 0) {
                sFly.zoomFrame = 0;
            }
        } else {
            if (CardBlock_Shrink()) {
                if (!sBursting) {
                    CardBlock_StartBurst();
                }
                sBursting = TRUE;
                if (CardBlock_StepBurst()) {
                    sBursting = FALSE;
                    sErasing = FALSE;
                }
            }
        }
    } else {
        for (slot = 0; slot < 2; slot++) {
            for (i = 0; i < CARD_MAX_FILES; i++) {
                b = &sBlocks[slot][i];
                if (sEntering) {
                    step = 25;
                } else {
                    step = 8;
                }
                if (b->frame >= b->start) {
                    if (b->alphaTarget == 255) {
                        b->alpha += step;
                    } else {
                        b->alpha -= step;
                    }
                }
                if (b->alpha > 255) {
                    b->alpha = 255;
                }
                if (b->alpha < 0) {
                    b->alpha = 0;
                }
                CardBlock_Move(b);
                b->zoom = Hermite(b->zoomFrame, 0.0f, b->zoomMin, 0.0f, b->zoomLen, b->zoomMax, 0.0f);
                if (slot == Card_GetSlot() && i == Card_GetSelectedFile() && !sEntering) {
                    b->zoomFrame++;
                } else {
                    b->zoomFrame--;
                }
                if (b->zoomFrame > b->zoomLen) {
                    b->zoomFrame = b->zoomLen;
                }
                if (b->zoomFrame < 0) {
                    b->zoomFrame = 0;
                }
            }
        }
        if (sFlying) {
            CardBlock_Move(&sFly);
            if (sFly.frame >= sFly.end - 5) {
                sFly.p0 = sFly.pos;
                sFly.v0 = sFly.vel;
                if (sFly.p1.x >= 4672.0f) {
                    sFly.p1.x = sFly.p1.x - 2.0f * (sFly.p1.x - 4672.0f);
                } else {
                    sFly.p1.x = sFly.p1.x + 2.0f * (4672.0f - sFly.p1.x);
                }
                if (sFly.p1.y >= 3200.0f) {
                    sFly.p1.y = sFly.p1.y - 2.0f * (sFly.p1.y - 3200.0f);
                } else {
                    sFly.p1.y = sFly.p1.y + 2.0f * (3200.0f - sFly.p1.y);
                }
                sFly.frame = 0;
                sFly.end = 180;
            }
        }
    }
    if (sEntering && fn_81320ABC()) {
        for (slot = 0; slot < 2; slot++) {
            for (i = 0; i < CARD_MAX_FILES; i++) {
                CardBlock_Finish(&sBlocks[slot][i]);
            }
        }
        sEntering = FALSE;
    }
    sWobble += 7;
}

/* 0x8132005C */
inline void CardBlock_Rewind(CardBlock* b) {
    b->start = 0;
    b->frame = b->start;
    b->end = 40;
}

/* 0x81320078 */
inline void CardBlock_Finish(CardBlock* b) {
    b->start = 0;
    b->end = 40;
    b->frame = b->end;
}

/* 0x81320094: start Copy / Move (the block flies off) or Erase */
void fn_81320094(u8 slot, u16 dest, u16 file, u16 kind) {
    Vec p1;
    u8 pad[4];
    Vec pos;
    Vec vel;
    u16 a;
    static const Vec kZero = {0.0f, 0.0f, 0.0f};

    sOpDest = dest;
    sOpSlot = slot;
    sOpFile = file;
    sOpKind = kind;
    if ((u16)(kind - 2) <= 2) {
        if (kind != 4) {
            sFly = sBlocks[slot][file];
            pos = sBlocks[slot][file].pos;
            sFly.p0 = pos;
            vel = sBlocks[slot][file].vel;
            sFly.v0 = vel;
            sFly.pos = pos;
            sFly.vel = vel;
            sBlocks[slot][file].alpha = 0;
            sBlocks[slot][file].alphaTarget = 0;
            a = Rand();
            p1 = kZero;
            p1.x = 16.0f * (292.0f + 40.0f * SinS(a));
            p1.y = 16.0f * (200.0f + 40.0f * CosS(a));
            sFly.p1 = p1;
            sFly.frame = 0;
            sFly.end = 40;
        } else {
            sFly = sBlocks[slot][file];
            sBlocks[slot][file].alpha = 0;
            sBlocks[slot][file].alphaTarget = 0;
            sFly.zoomFrame = 20;
            sFly.zoomLen = 20;
            sFly.zoomMin = 0.0f;
            sErasing = TRUE;
            sBursting = FALSE;
        }
        sFlying = TRUE;
    }
}

/* 0x8132030C: the flying block lands on the other card as file `file` */
void fn_8132030C(u8 file) {
    u8 pad[4];
    if ((u16)(sOpKind - 2) <= 2) {
        sFlying = FALSE;
        if (sOpKind != 4) {
            sBlocks[sOpSlot ^ 1][file].pos = sFly.pos;
            if (sOpKind == 2) {
                sBlocks[sOpSlot][sOpFile].pos = sFly.pos;
            }
            if (sOpDest < 8) {
                Vec v = {0.0f, 150.0f, 0.0f};
                sBlocks[sOpSlot ^ 1][file].vel = *CardVecRef(&v);
            } else {
                Vec v = {0.0f, -150.0f, 0.0f};
                sBlocks[sOpSlot ^ 1][file].vel = *CardVecRef(&v);
            }
            sBlocks[sOpSlot ^ 1][file].zoom = sBlocks[sOpSlot ^ 1][file].zoomMax;
            sBlocks[sOpSlot ^ 1][file].zoomFrame = sBlocks[sOpSlot ^ 1][file].zoomLen;
            sBlocks[sOpSlot ^ 1][file].alpha = 255;
            sBlocks[sOpSlot ^ 1][file].alphaTarget = 255;
            sBlocks[sOpSlot ^ 1][file].end = 60;
        }
        if (sOpKind != 2) {
            sBlocks[sOpSlot][sOpFile].alpha = 0;
            sBlocks[sOpSlot][sOpFile].alphaTarget = 0;
        }
    }
}

/* 0x813204B0: the operation failed, the block goes back home */
void fn_813204B0(void) {
    Vec pos;

    if (sFlying) {
        sFlying = FALSE;
        if (sErasing) {
            sErasing = FALSE;
            sBursting = FALSE;
        }
        pos = sFly.pos;
        sBlocks[sOpSlot][sOpFile].pos = pos;
        sBlocks[sOpSlot][sOpFile].p0 = pos;
        sBlocks[sOpSlot][sOpFile].frame = 0;
        sBlocks[sOpSlot][sOpFile].end = 40;
        sBlocks[sOpSlot][sOpFile].alpha = 255;
        sBlocks[sOpSlot][sOpFile].alphaTarget = 255;
    }
}

/* 0x8132056C: send every block to its cell (or just off the window) */
void fn_8132056C(CardOrder (*order)[CARD_MAX_FILES], s8* row) {
    s16* gridY;
    s16* gridX;
    CardBlock* b;
    CardOrder* o;
    CardBlock* blocks;
    u32 slot;
    u32 i;

    for (slot = 0; slot < 2; slot++) {
        o = order[slot];
        blocks = sBlocks[slot];
        gridX = sGridX[slot];
        gridY = sGridY[slot];
        for (i = 0; i < CARD_MAX_FILES; i++) {
            b = &blocks[o[i].file];
            CardBlock_Rewind(b);
            if (i >= row[slot] * 4 && i <= row[slot] * 4 + 15) {
                b->alphaTarget = 255;
                b->p0 = b->pos;
                b->v0 = b->vel;
                b->p1.x = gridX[i - row[slot] * 4];
                b->p1.y = gridY[i - row[slot] * 4];
                b->p1.z = 0.0f;
            } else {
                b->alphaTarget = 0;
                b->p0 = b->pos;
                b->v0 = b->vel;
                if (i < row[slot] * 4) {
                    b->p1.x = gridX[i & 3];
                    b->p1.y = gridY[i & 3] - 512.0f;
                }
                if (i > row[slot] * 4 + 15) {
                    b->p1.x = gridX[(i & 3) + 12];
                    b->p1.y = 512.0f + gridY[(i & 3) + 12];
                }
                b->p1.z = 0.0f;
            }
        }
    }
}

/* 0x813207C8 */
void CardIcon_ResetFiles(void) {
    int slot;
    int i;
    s32 a;
    s32 delay;

    for (slot = 0; slot < 2; slot++) {
        for (i = 0; i < CARD_MAX_FILES; i++) {
            {
                Vec start = {4672.0f, 3584.0f, 0.0f};
                sBlocks[slot][i].pos = *CardVecRef(&start);
            }
            a = (u16)Rand();
            {
                Vec vel = {0.0f, 0.0f, 0.0f};
                vel.x = 350.0f * SinS(a);
                vel.y = 350.0f * CosS(a);
                sBlocks[slot][i].vel = *CardVecRef(&vel);
            }
            sBlocks[slot][i].p0 = sBlocks[slot][i].pos;
            sBlocks[slot][i].v0 = sBlocks[slot][i].vel;
            sBlocks[slot][i].alpha = 0;
            sBlocks[slot][i].frame = 0;
            delay = Rand() & 15;
            sBlocks[slot][i].start = delay;
            sBlocks[slot][i].end = delay + 33;
        }
    }
    sEntering = TRUE;
}

/* 0x81320920: a slot with no usable card collapses its blocks to the middle */
void fn_81320920(s32 slot) {
    Vec c;
    s16 unused[10];
    u32 n;
    u32 i;

    n = 0;
    for (i = 0; i < CARD_MAX_FILES; i++) {
        sBlocks[slot][i].alphaTarget = 0;
        if (sBlocks[slot][i].alpha == 0) {
            n++;
        }
    }
    if (n == CARD_MAX_FILES) {
        Vec zero = {0.0f, 0.0f, 0.0f};
        Vec vzero = {0.0f, 0.0f, 0.0f};
        for (i = 0; i < CARD_MAX_FILES; i++) {
            c = zero;
            c.x = (sGridX[slot][5] + sGridX[slot][6]) / 2;
            c.y = (sGridY[slot][5] + sGridY[slot][9]) / 2;
            sBlocks[slot][i].pos = c;
            sBlocks[slot][i].vel = vzero;
            sBlocks[slot][i].p0 = sBlocks[slot][i].pos;
            sBlocks[slot][i].v0 = sBlocks[slot][i].vel;
            sBlocks[slot][i].alpha = 0;
            sBlocks[slot][i].frame = 0;
        }
    }
}

static inline BOOL CardBlock_IsHome(CardBlock* b) {
    return b->frame == b->end ? TRUE : FALSE;
}

static inline void CardBlock_SetStart(void) {
    CardSlot* cards;
    u32 slot;
    u32 i;

    cards = CardThread_GetSlots();
    for (slot = 0; slot < 2; slot++) {
        if (cards[slot].state == CARD_SLOT_READY) {
            for (i = 0; i < CARD_MAX_FILES; i++) {
                sBlocks[slot][i].p0 = sBlocks[slot][i].pos;
                sBlocks[slot][i].v0 = sBlocks[slot][i].vel;
            }
        }
    }
}

/* 0x81320ABC: every block is home and the screen has faded in */
BOOL fn_81320ABC(void) {
    int slot;
    int i;
    int n;
    CardSlot* cards;

    n = 0;
    cards = CardThread_GetSlots();
    if (!Menu_CardFadeDone()) {
        return FALSE;
    }
    for (slot = 0; slot < 2; slot++) {
        if (cards[slot].state == CARD_SLOT_READY) {
            for (i = 0; i < CARD_MAX_FILES; i++) {
                if (CardBlock_IsHome(&sBlocks[slot][i])) {
                    n++;
                }
            }
        } else {
            n += CARD_MAX_FILES;
        }
    }
    if (n == CARD_MAX_FILES * 2 && !sErasing) {
        CardBlock_SetStart();
        return TRUE;
    }
    return FALSE;
}

/* 0x81320DD0: block tint by card size; kind 0/1: file (chosen / not), 2/3: empty */
GXColorS10* CardBlock_Color(u32 size, s32 kind, s32 slot) {
    static GXColorS10 c59_3 = {0, 20, 60, 255};
    static GXColorS10 c59_2 = {30, 80, 100, 255};
    static GXColorS10 c59_1 = {20, 50, 200, 255};
    static GXColorS10 c59_0 = {100, 150, 255, 255};
    static GXColorS10 c123_3 = {0, 37, 0, 255};
    static GXColorS10 c123_2 = {50, 120, 80, 255};
    static GXColorS10 c123_1 = {0, 105, 70, 255};
    static GXColorS10 c123_0 = {100, 255, 110, 255};
    static GXColorS10 c251_3 = {28, 34, 0, 255};
    static GXColorS10 c251_2 = {90, 110, 0, 255};
    static GXColorS10 c251_1 = {80, 120, 0, 255};
    static GXColorS10 c251_0 = {200, 255, 75, 255};
    static GXColorS10 c507_3 = {50, 30, 0, 255};
    static GXColorS10 c507_2 = {100, 80, 0, 255};
    static GXColorS10 c507_1 = {160, 100, 0, 255};
    static GXColorS10 c507_0 = {255, 180, 50, 255};
    static GXColorS10 c1019_3 = {50, 20, 0, 255};
    static GXColorS10 c1019_2 = {150, 60, 50, 255};
    static GXColorS10 c1019_1 = {160, 45, 50, 255};
    static GXColorS10 c1019_0 = {255, 110, 100, 255};
    static GXColorS10 c2043_3 = {45, 0, 60, 255};
    static GXColorS10 c2043_2 = {100, 40, 90, 255};
    static GXColorS10 c2043_1 = {90, 30, 170, 255};
    static GXColorS10 c2043_0 = {200, 80, 190, 255};
    GXColorS10* c;

    switch (size) {
    case 0x2000:
        switch (kind) {
        case 0:
            c = &c59_0;
            break;
        case 1:
            c = &c59_1;
            break;
        case 2:
            c = &c59_2;
            break;
        case 3:
            c = &c59_3;
            break;
        }
        break;
    case 0x4000:
        switch (kind) {
        case 0:
            c = &c123_0;
            break;
        case 1:
            c = &c123_1;
            break;
        case 2:
            c = &c123_2;
            break;
        case 3:
            c = &c123_3;
            break;
        }
        break;
    case 0x8000:
        switch (kind) {
        case 0:
            c = &c251_0;
            break;
        case 1:
            c = &c251_1;
            break;
        case 2:
            c = &c251_2;
            break;
        case 3:
            c = &c251_3;
            break;
        }
        break;
    case 0x10000:
        switch (kind) {
        case 0:
            c = &c507_0;
            break;
        case 1:
            c = &c507_1;
            break;
        case 2:
            c = &c507_2;
            break;
        case 3:
            c = &c507_3;
            break;
        }
        break;
    case 0x20000:
        switch (kind) {
        case 0:
            c = &c1019_0;
            break;
        case 1:
            c = &c1019_1;
            break;
        case 2:
            c = &c1019_2;
            break;
        case 3:
            c = &c1019_3;
            break;
        }
        break;
    case 0x40000:
        switch (kind) {
        case 0:
            c = &c2043_0;
            break;
        case 1:
            c = &c2043_1;
            break;
        case 2:
            c = &c2043_2;
            break;
        case 3:
            c = &c2043_3;
            break;
        }
        break;
    }
    return c;
}

/* 0x81320FD8 */
void CardBlock_DrawFile(J3DModelInstance* inst, u8 slot, s32 file) {
    CardSlot* cards;
    GXColorS10* save0;
    GXColorS10* save2;
    u16 save;

    cards = CardThread_GetSlots();
    if (inst->alpha > 0) {
        save0 = inst->model->material[0].tevColor;
        save2 = inst->model->material[2].tevColor;
        if (slot == Card_GetSlot() && file == Card_GetSelectedFile() && !sEntering) {
            inst->model->material[0].tevColor = CardBlock_Color(cards[slot].sectorSize, 0, slot);
            inst->model->material[2].tevColor = CardBlock_Color(cards[slot].sectorSize, 0, slot);
        } else {
            inst->model->material[0].tevColor = CardBlock_Color(cards[slot].sectorSize, 1, slot);
            inst->model->material[2].tevColor = CardBlock_Color(cards[slot].sectorSize, 1, slot);
        }
        J3D_DrawPacket(inst, inst->model->packets + 0x30);
        J3D_DrawPacket(inst, inst->model->packets + 0xF0);
        inst->model->material[0].tevColor = save0;
        inst->model->material[2].tevColor = save2;
        if (CardDraw_IconFrameCount(slot, file)) {
            save = inst->model->material[1].flags;
            inst->model->material[1].flags = 0xFFFF;
            J3D_DrawPacket(inst, inst->model->packets + 0x90);
            inst->model->material[1].flags = save;
        }
    }
}

/* 0x81321168 */
void CardBlock_DrawEmpty(J3DModelInstance* inst, u8 slot, s32 file) {
    CardSlot* cards;
    GXColorS10* save0;
    GXColorS10* save1;

    cards = CardThread_GetSlots();
    if (inst->alpha > 0) {
        save0 = inst->model->material[0].tevColor;
        save1 = inst->model->material[1].tevColor;
        if (slot == Card_GetSlot() && file == Card_GetSelectedFile() && !sEntering) {
            inst->model->material[0].tevColor = CardBlock_Color(cards[slot].sectorSize, 2, slot);
            inst->model->material[1].tevColor = CardBlock_Color(cards[slot].sectorSize, 2, slot);
        } else {
            inst->model->material[0].tevColor = CardBlock_Color(cards[slot].sectorSize, 3, slot);
            inst->model->material[1].tevColor = CardBlock_Color(cards[slot].sectorSize, 3, slot);
        }
        J3D_Draw(inst);
        inst->model->material[0].tevColor = save0;
        inst->model->material[1].tevColor = save1;
    }
}

/* 0x8132128C */
void CardBlock_DrawBurst(J3DModelInstance* inst) {
    CardSlot* cards;
    GXColorS10* save0;
    GXColorS10* save2;

    cards = CardThread_GetSlots();
    if (inst->alpha > 0) {
        save0 = inst->model->material[0].tevColor;
        save2 = inst->model->material[2].tevColor;
        inst->model->material[0].tevColor = CardBlock_Color(cards[sOpSlot].sectorSize, 1, sOpSlot);
        inst->model->material[2].tevColor = CardBlock_Color(cards[sOpSlot].sectorSize, 1, sOpSlot);
        J3D_DrawPacket(inst, inst->model->packets + 0x30);
        J3D_DrawPacket(inst, inst->model->packets + 0xF0);
        inst->model->material[0].tevColor = save0;
        inst->model->material[2].tevColor = save2;
    }
}

/* the Erase burst pieces, faded out over their lifetime */
static inline void CardBlock_DrawBursts(u8 alpha) {
    int i;
    u8 a;

    for (i = 0; i < CARD_BLOCK_BURST; i++) {
        if (sBurstFrame[i] > sBurstDelay[i]) {
            if (sBurstFrame[i] - sBurstDelay[i] <= sBurstLen[i]) {
                a = (alpha * (sBurstDelay[i] + (sBurstLen[i] - sBurstFrame[i]))) / sBurstLen[i];
            } else {
                a = 0;
            }
        } else {
            a = 0;
        }
        if (a) {
            sBurstInst[i].alpha = a;
            CardBlock_DrawBurst(&sBurstInst[i]);
        }
    }
}

/* 0x81321378: one slot's blocks; the chosen one and the flying one last */
void CardBlock_DrawSlot(u8 slot, u8 alpha) {
    CardFile (*files)[CARD_MAX_FILES];
    int i;
    s32 sel;

    files = CardThread_GetFiles();
    for (i = 0; i < CARD_MAX_FILES; i++) {
        if (slot == sOpSlot && i == sOpFile && sFlying) {
            sFileInst[slot][i].alpha = 0;
        } else if (slot == Card_GetSlot() && i == Card_GetSelectedFile() && !sEntering) {
            sFileInst[slot][i].alpha = 0;
        } else {
            sFileInst[slot][i].alpha = (u8)((alpha * sBlocks[slot][i].alpha) / 255);
        }
        sEmptyInst[slot][i].alpha = (u8)((alpha * sBlocks[slot][i].alpha) / 255);
        if (files[slot][i].used) {
            CardBlock_DrawFile(&sFileInst[slot][i], slot, i);
        } else {
            CardBlock_DrawEmpty(&sEmptyInst[slot][i], slot, i);
        }
    }
    if (sFlying) {
        if (!sErasing) {
            slot = sOpSlot;
            if (files[slot][sOpFile].used) {
                sFileInst[slot][sOpFile].alpha = (u8)((sFly.alpha * alpha) / 255);
                CardBlock_DrawFile(&sFileInst[slot][sOpFile], slot, sOpFile);
            }
        }
    } else if (!sEntering) {
        slot = Card_GetSlot();
        sel = Card_GetSelectedFile();
        if (files[slot][sel].used) {
            sFileInst[slot][sel].alpha = (u8)((alpha * sBlocks[slot][sel].alpha) / 255);
            CardBlock_DrawFile(&sFileInst[slot][sel], slot, sel);
        }
    }
    if (sErasing) {
        if (!sBursting) {
            sFlyInst.alpha = (u8)((sFly.alpha * alpha) / 255);
            CardBlock_DrawFile(&sFlyInst, sOpSlot, -1);
        } else {
            CardBlock_DrawBursts(alpha);
        }
    }
}

/* 0x813216F4 */
void fn_813216F4(u8 slot, u8 alpha) {
    CardBlock_DrawSlot(slot, alpha);
}

/* 0x81321714 */
BOOL fn_81321714(void) {
    if (sEntering) {
        return FALSE;
    }
    return TRUE;
}
