// BS2 model renderer: scene graph tree helpers, joint and shape setup
// (PAL 1.0 BS2, 0x813053F8..0x81305824).

#include <dolphin/os.h>
#include "bs2/j3d.h"

void* HeapAlloc(u32 size); /* 0x81307E40 */

void fn_81305454(J3DNode* sib, J3DNode* node);

void fn_813053F8(J3DNode* node) {
    node->parent = NULL;
    node->child = NULL;
    node->prev = NULL;
    node->next = NULL;
}

void fn_81305410(J3DNode* parent, J3DNode* node) {
    if (parent != NULL) {
        if (parent->child == NULL) {
            parent->child = node;
            node->parent = parent;
        } else {
            fn_81305454(parent->child, node);
        }
    }
}

/* recursion is expanded one level deeper here than in fn_813054D4 */
#pragma push
#pragma inline_depth(3)
void fn_81305454(J3DNode* sib, J3DNode* node) {
    if (sib->next == NULL) {
        sib->next = node;
        node->prev = sib;
    } else {
        fn_81305454(sib->next, node);
    }
}
#pragma pop

J3DNode* fn_813054D4(J3DNode* node) {
    J3DNode* ret;
    if (node->parent != NULL) {
        ret = node->parent;
    } else if (node->prev != NULL) {
        ret = fn_813054D4(node->prev);
    }
    return ret;
}

void fn_8130555C(J3DJointBlock* blk, J3DJoint* out) {
    s32 i;
    for (i = 0; i < blk->count; i++) {
        out->flags = blk->joints[blk->remap[i]].flags;
        out->xf = blk->joints[blk->remap[i]].xf;
        out++;
    }
}

void fn_813055F4(J3DShapeBlock* blk, J3DShape* out) {
    s32 j;
    s32 i;
    for (i = 0; i < blk->count; i++) {
        out[i].mtxType = blk->shapes[blk->remap[i]].mtxType;
        out[i].numPackets = blk->shapes[blk->remap[i]].numPackets;
        if (out[i].numPackets > 2) {
            OSPanic("gMRshape.c", 25, "ERROR: This renderer doesn't support Enveloped Model.\n");
        }
        out[i].vtxDesc = blk->vtxDesc + blk->shapes[blk->remap[i]].vtxDescOffset;
        out[i].mtxGroups = HeapAlloc(out[i].numPackets * sizeof(J3DShapeMtxGroup));
        out[i].packets = HeapAlloc(out[i].numPackets * sizeof(J3DShapePacket));
        for (j = 0; j < out[i].numPackets; j++) {
            out[i].mtxGroups[j].unk0 = blk->mtxData[blk->shapes[blk->remap[i]].firstMtxData + j].unk0;
            out[i].mtxGroups[j].count = blk->mtxData[blk->shapes[blk->remap[i]].firstMtxData + j].count;
            out[i].mtxGroups[j].table = blk->mtxTable + blk->mtxData[blk->shapes[blk->remap[i]].firstMtxData + j].first;
            out[i].packets[j].size = blk->packetLoc[blk->shapes[blk->remap[i]].firstPacket + j].size;
            out[i].packets[j].dl = blk->dlData + blk->packetLoc[blk->shapes[blk->remap[i]].firstPacket + j].offset;
        }
    }
}
