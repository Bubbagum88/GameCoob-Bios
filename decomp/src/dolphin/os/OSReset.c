#include <dolphin/os.h>
#include <dolphin/hw_regs.h>

typedef BOOL (*OSResetFunction)(BOOL final);
typedef struct OSResetFunctionInfo OSResetFunctionInfo;

struct OSResetFunctionInfo {
    OSResetFunction func;
    u32 priority;
    OSResetFunctionInfo* next;
    OSResetFunctionInfo* prev;
};

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

u32 OSGetResetCode(void) {
    u32 code;

    if (*(u8*)0x800030E2) {
        code = OS_RESETCODE_RESTART;
    } else {
        code = (__PIRegs[9] & ~7) >> 3;
    }
    return code;
}
