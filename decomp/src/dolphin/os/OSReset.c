#include <dolphin/os.h>
#include <dolphin/hw_regs.h>

typedef struct OSResetFunctionQueue {
    OSResetFunctionInfo* head;
    OSResetFunctionInfo* tail;
} OSResetFunctionQueue;

#define OS_RESETCODE_RESTART 0x80000000

static OSResetFunctionQueue ResetFunctionQueue;

void OSRegisterResetFunction(OSResetFunctionInfo* info) {
    OSResetFunctionInfo* tmp;
    OSResetFunctionInfo* iter;

    for (iter = ResetFunctionQueue.head; iter && iter->priority <= info->priority; iter = iter->next)
        ;

    if (iter == NULL) {
        tmp = ResetFunctionQueue.tail;
        if (tmp == NULL) {
            ResetFunctionQueue.head = info;
        } else {
            tmp->next = info;
        }
        info->prev = tmp;
        info->next = NULL;
        ResetFunctionQueue.tail = info;
        return;
    }

    info->next = iter;
    tmp = iter->prev;
    iter->prev = info;
    info->prev = tmp;
    if (tmp == NULL) {
        ResetFunctionQueue.head = info;
        return;
    }
    tmp->next = info;
}

// The IPL build has this function unoptimized
#pragma push
#pragma optimization_level 0
#pragma peephole off
u32 OSGetResetCode(void) {
    if (*(u8*)0x800030E2) {
        return OS_RESETCODE_RESTART;
    }
    return ((__PIRegs[PI_RESETCODE] & ~7) >> 3);
}
#pragma pop
