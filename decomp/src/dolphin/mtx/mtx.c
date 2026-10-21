#include <dolphin/mtx.h>

static f32 Unit01[] = { 0.0f, 1.0f };

void C_MTXIdentity(Mtx mtx) {
    mtx[0][0] = 1.0f;
    mtx[0][1] = 0.0f;
    mtx[0][2] = 0.0f;
    mtx[0][3] = 0.0f;

    mtx[1][0] = 0.0f;
    mtx[1][1] = 1.0f;
    mtx[1][2] = 0.0f;
    mtx[1][3] = 0.0f;

    mtx[2][0] = 0.0f;
    mtx[2][1] = 0.0f;
    mtx[2][2] = 1.0f;
    mtx[2][3] = 0.0f;
}

void PSMTXIdentity(register Mtx m) {
    register f32 zero_c = 0.0f;
    register f32 one_c = 1.0f;
    register f32 c_01;
    register f32 c_10;

    asm {
        psq_st zero_c, 8(m), 0, 0
        ps_merge01 c_01, zero_c, one_c
        psq_st zero_c, 24(m), 0, 0
        ps_merge10 c_10, one_c, zero_c
        psq_st zero_c, 32(m), 0, 0
        psq_st c_01, 16(m), 0, 0
        psq_st c_10, 0(m), 0, 0
        psq_st c_10, 40(m), 0, 0
    }
}

asm void PSMTXCopy(const register Mtx src, register Mtx dst) {
    nofralloc
    psq_l fp0, 0(src), 0, 0
    psq_st fp0, 0(dst), 0, 0
    psq_l fp1, 8(src), 0, 0
    psq_st fp1, 8(dst), 0, 0
    psq_l fp2, 16(src), 0, 0
    psq_st fp2, 16(dst), 0, 0
    psq_l fp3, 24(src), 0, 0
    psq_st fp3, 24(dst), 0, 0
    psq_l fp4, 32(src), 0, 0
    psq_st fp4, 32(dst), 0, 0
    psq_l fp5, 40(src), 0, 0
    psq_st fp5, 40(dst), 0, 0
    blr
}

asm void PSMTXConcat(const register Mtx mA, const register Mtx mB, register Mtx mAB) {
    nofralloc
    stwu r1, -64(r1)
    psq_l fp0, 0(mA), 0, 0
    stfd fp14, 8(r1)
    psq_l fp6, 0(mB), 0, 0
    lis r6, Unit01@ha
    psq_l fp7, 8(mB), 0, 0
    stfd fp15, 16(r1)
    addi r6, r6, Unit01@l
    stfd fp31, 40(r1)
    psq_l fp8, 16(mB), 0, 0
    ps_muls0 fp12, fp6, fp0
    psq_l fp2, 16(mA), 0, 0
    ps_muls0 fp13, fp7, fp0
    psq_l fp31, 0(r6), 0, 0
    ps_muls0 fp14, fp6, fp2
    psq_l fp9, 24(mB), 0, 0
    ps_muls0 fp15, fp7, fp2
    psq_l fp1, 8(mA), 0, 0
    ps_madds1 fp12, fp8, fp0, fp12
    psq_l fp3, 24(mA), 0, 0
    ps_madds1 fp14, fp8, fp2, fp14
    psq_l fp10, 32(mB), 0, 0
    ps_madds1 fp13, fp9, fp0, fp13
    psq_l fp11, 40(mB), 0, 0
    ps_madds1 fp15, fp9, fp2, fp15
    psq_l fp4, 32(mA), 0, 0
    psq_l fp5, 40(mA), 0, 0
    ps_madds0 fp12, fp10, fp1, fp12
    ps_madds0 fp13, fp11, fp1, fp13
    ps_madds0 fp14, fp10, fp3, fp14
    ps_madds0 fp15, fp11, fp3, fp15
    psq_st fp12, 0(mAB), 0, 0
    ps_muls0 fp2, fp6, fp4
    ps_madds1 fp13, fp31, fp1, fp13
    ps_muls0 fp0, fp7, fp4
    psq_st fp14, 16(mAB), 0, 0
    ps_madds1 fp15, fp31, fp3, fp15
    psq_st fp13, 8(mAB), 0, 0
    ps_madds1 fp2, fp8, fp4, fp2
    ps_madds1 fp0, fp9, fp4, fp0
    ps_madds0 fp2, fp10, fp5, fp2
    lfd fp14, 8(r1)
    psq_st fp15, 24(mAB), 0, 0
    ps_madds0 fp0, fp11, fp5, fp0
    psq_st fp2, 32(mAB), 0, 0
    ps_madds1 fp0, fp31, fp5, fp0
    lfd fp15, 16(r1)
    psq_st fp0, 40(mAB), 0, 0
    lfd fp31, 40(r1)
    addi r1, r1, 64
    blr
}

void PSMTXTranspose(const register Mtx src, register Mtx xPose) {
    register f32 c_zero = 0.0f;
    register f32 row0a, row1a, row0b, row1b;
    register f32 trns0, trns1, trns2;

    asm {
        psq_l row0a, 0(src), 0, 0
        stfs c_zero, 44(xPose)
        psq_l row1a, 16(src), 0, 0
        ps_merge00 trns0, row0a, row1a
        psq_l row0b, 8(src), 1, 0
        ps_merge11 trns1, row0a, row1a
        psq_l row1b, 24(src), 1, 0
        psq_st trns0, 0(xPose), 0, 0
        psq_l row0a, 32(src), 0, 0
        ps_merge00 trns2, row0b, row1b
        psq_st trns1, 16(xPose), 0, 0
        ps_merge00 trns0, row0a, c_zero
        psq_st trns2, 32(xPose), 0, 0
        ps_merge10 trns1, row0a, c_zero
        psq_st trns0, 8(xPose), 0, 0
        lfs row0b, 40(src)
        psq_st trns1, 24(xPose), 0, 0
        stfs row0b, 40(xPose)
    }
}

asm u32 PSMTXInverse(const register Mtx src, register Mtx inv) {
    nofralloc
    psq_l fp0, 0(src), 1, 0
    psq_l fp1, 4(src), 0, 0
    psq_l fp2, 16(src), 1, 0
    ps_merge10 fp6, fp1, fp0
    psq_l fp3, 20(src), 0, 0
    psq_l fp4, 32(src), 1, 0
    ps_merge10 fp7, fp3, fp2
    psq_l fp5, 36(src), 0, 0
    ps_mul fp11, fp3, fp6
    ps_mul fp13, fp5, fp7
    ps_merge10 fp8, fp5, fp4
    ps_msub fp11, fp1, fp7, fp11
    ps_mul fp12, fp1, fp8
    ps_msub fp13, fp3, fp8, fp13
    ps_mul fp10, fp3, fp4
    ps_msub fp12, fp5, fp6, fp12
    ps_mul fp9, fp0, fp5
    ps_mul fp8, fp1, fp2
    ps_sub fp6, fp6, fp6
    ps_msub fp10, fp2, fp5, fp10
    ps_mul fp7, fp0, fp13
    ps_msub fp9, fp1, fp4, fp9
    ps_madd fp7, fp2, fp12, fp7
    ps_msub fp8, fp0, fp3, fp8
    ps_madd fp7, fp4, fp11, fp7
    ps_cmpo0 cr0, fp7, fp6
    bne _regular
    li r3, 0
    blr
_regular:
    ps_res fp0, fp7
    ps_add fp6, fp0, fp0
    ps_mul fp5, fp0, fp0
    ps_nmsub fp0, fp7, fp5, fp6
    lfs fp1, 12(src)
    ps_muls0 fp13, fp13, fp0
    lfs fp2, 28(src)
    ps_muls0 fp12, fp12, fp0
    lfs fp3, 44(src)
    ps_muls0 fp11, fp11, fp0
    ps_merge00 fp5, fp13, fp12
    ps_muls0 fp10, fp10, fp0
    ps_merge11 fp4, fp13, fp12
    ps_muls0 fp9, fp9, fp0
    psq_st fp5, 0(inv), 0, 0
    ps_mul fp6, fp13, fp1
    psq_st fp4, 16(inv), 0, 0
    ps_muls0 fp8, fp8, fp0
    ps_madd fp6, fp12, fp2, fp6
    psq_st fp10, 32(inv), 1, 0
    ps_nmadd fp6, fp11, fp3, fp6
    psq_st fp9, 36(inv), 1, 0
    ps_mul fp7, fp10, fp1
    ps_merge00 fp5, fp11, fp6
    psq_st fp8, 40(inv), 1, 0
    ps_merge11 fp4, fp11, fp6
    psq_st fp5, 8(inv), 0, 0
    ps_madd fp7, fp9, fp2, fp7
    psq_st fp4, 24(inv), 0, 0
    ps_nmadd fp7, fp8, fp3, fp7
    li r3, 1
    psq_st fp7, 44(inv), 1, 0
    blr
}

void PSMTXTrans(register Mtx m, register f32 xT, register f32 yT, register f32 zT) {
    register f32 c0 = 0.0f;
    register f32 c1 = 1.0f;

    asm {
        stfs xT, 12(m)
        stfs yT, 28(m)
        psq_st c0, 4(m), 0, 0
        psq_st c0, 32(m), 0, 0
        stfs c0, 16(m)
        stfs c1, 20(m)
        stfs c0, 24(m)
        stfs c1, 40(m)
        stfs zT, 44(m)
        stfs c1, 0(m)
    }
}

void PSMTXScale(register Mtx m, register f32 xS, register f32 yS, register f32 zS) {
    register f32 c0 = 0.0f;

    asm {
        stfs xS, 0(m)
        psq_st c0, 4(m), 0, 0
        psq_st c0, 12(m), 0, 0
        stfs yS, 20(m)
        psq_st c0, 24(m), 0, 0
        psq_st c0, 32(m), 0, 0
        stfs zS, 40(m)
        stfs c0, 44(m)
    }
}

void C_MTXLookAt(Mtx m, const Point3d* camPos, const Vec* camUp, const Point3d* target) {
    Vec vLook, vRight, vUp;

    vLook.x = camPos->x - target->x;
    vLook.y = camPos->y - target->y;
    vLook.z = camPos->z - target->z;
    VECNormalize(&vLook, &vLook);

    VECCrossProduct(camUp, &vLook, &vRight);
    VECNormalize(&vRight, &vRight);

    VECCrossProduct(&vLook, &vRight, &vUp);

    m[0][0] = vRight.x;
    m[0][1] = vRight.y;
    m[0][2] = vRight.z;
    m[0][3] = -(camPos->x * vRight.x + camPos->y * vRight.y + camPos->z * vRight.z);

    m[1][0] = vUp.x;
    m[1][1] = vUp.y;
    m[1][2] = vUp.z;
    m[1][3] = -(camPos->x * vUp.x + camPos->y * vUp.y + camPos->z * vUp.z);

    m[2][0] = vLook.x;
    m[2][1] = vLook.y;
    m[2][2] = vLook.z;
    m[2][3] = -(camPos->x * vLook.x + camPos->y * vLook.y + camPos->z * vLook.z);
}
