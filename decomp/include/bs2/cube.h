#ifndef BS2_CUBE_H
#define BS2_CUBE_H

#include "types.h"
#include "bs2/mtx.h"
#include "bs2/j3d.h"

/* Rolling-cube intro ("no disc" boot animation). PAL 1.0 addresses. */

#define CUBE_GRID 32
#define CUBE_FACES 3

typedef enum CubeDir {
    CUBE_DIR_XPOS = 0,   /* x + 1 */
    CUBE_DIR_YPOS = 1,   /* y + 1 */
    CUBE_DIR_XNEG = 2,   /* x - 1 */
    CUBE_DIR_YNEG = 3,   /* y - 1 */
    CUBE_DIR_FACE0 = 4,  /* 4..6: roll over an edge onto face (dir - 4) */
    CUBE_DIR_RISE = 7,   /* path finished, cube lifts off */
    CUBE_DIR_END = 8     /* path table terminator */
} CubeDir;

typedef enum CubePhase {
    CUBE_PHASE_FALL = 0,
    CUBE_PHASE_PATH = 1,
    CUBE_PHASE_RISE = 2,
    CUBE_PHASE_DROP = 3,
    CUBE_PHASE_LOGO = 4
} CubePhase;

/* One entry of the path table at 0x81381160 */
typedef struct CubePathStep {
    u8 dir;      /* CubeDir */
    u8 group;    /* nonzero: leave a visible tile */
    u8 life;     /* steps before the tile fades, 0xFF = forever */
    u8 pad;
} CubePathStep;

/* One cell of the tile grid at 0x814AFD58: [face][x][y] */
typedef struct CubeTile {
    u8 life;     /* 0x0 */
    u8 group;    /* 0x1 */
    u8 state;    /* 0x2: 0 idle, 1 fading in, 2 fading out */
    u8 pad;
    u32 color;   /* 0x4 */
} CubeTile;

/* 0x814B5D58 */
typedef struct CubeState {
    s32 x;              /* 0x00 grid column, starts at 16 */
    s32 y;              /* 0x04 grid row, starts at 18 */
    s8 dir;             /* 0x08 current CubeDir */
    s8 face;            /* 0x09 face plane 0..2 */
    u8 unkA;
    u8 alpha;           /* 0x0B */
    u32 color;          /* 0x0C RGBA */
    u16 flags;          /* 0x10 */
    s16 stepFrame;      /* 0x12 */
    s16 sideFrames;     /* 0x14 frames for a 90 degree roll (8) */
    s16 cornerFrames;   /* 0x16 frames for a 180 degree edge roll (15) */
    s16 stepDelay;      /* 0x18 */
    s16 stepDoneFrame;  /* 0x1A */
    u8 unk1C;
    u8 step;            /* 0x1D index into the path table */
    u8 tileInset;       /* 0x1E */
    u8 done;            /* 0x1F */
    u8 holdFall;        /* 0x20 */
    u8 fallFrame;       /* 0x21 */
    u8 fallFrames;      /* 0x22 (4) */
    u8 fallDelay;       /* 0x23 (13) */
    f32 fallVel;        /* 0x24 */
    f32 fallAccel;      /* 0x28 = 2h / n^2 */
    f32 fallHeight0;    /* 0x2C (100) */
    f32 fallHeight;     /* 0x30 */
    u8 riseFrame;       /* 0x34 */
    u8 riseFrames;      /* 0x35 (15) */
    u8 riseDelay;       /* 0x36 (6) */
    u8 riseTurns;       /* 0x37 quarter turns during the rise (6) */
    f32 riseHeight;     /* 0x38 (130) */
    u8 dropFrame;       /* 0x3C */
    u8 dropFrames;      /* 0x3D (8) */
    u8 dropDelay;       /* 0x3E (17) */
    f32 dropVel;        /* 0x40 */
    f32 dropAccel;      /* 0x44 */
    f32 dropHeight0;    /* 0x48 */
    f32 dropHeight;     /* 0x4C */
    f32 dropVel0;       /* 0x50 (30) */
    u8 logoFrames;      /* 0x54 (33) */
    u8 logoFrame;       /* 0x55 */
    u8 textFrame;       /* 0x56 */
    u8 textFrames;      /* 0x57 (50) */
    u8 textDelay;       /* 0x58 (4) */
    u8 size;            /* 0x59 cube edge length (54) */
    u8 tileR, tileG, tileB; /* 0x5A..0x5C (100, 80, 190) */
    s16 jingleFrame;    /* 0x5E */
    s16 jingleDelay[3]; /* 0x60 normal, Z on all pads, Z on pad 1 */
} CubeState;

CubeTile *Cube_GetGrid(void);
CubeState *Cube_Get(void);
void Cube_Init(BOOL first);
MtxPtr Cube_GetMtx(void);
void Cube_Draw(void);
s32 Cube_Update(BOOL playJingle);
BOOL Cube_IsRunning(void);
void Cube_DrawBig(u8 alpha);
void Cube_InitModels(void);
J3DModelInstance *Cube_GetModel(void);

#endif
