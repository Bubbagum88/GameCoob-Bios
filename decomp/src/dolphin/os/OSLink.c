#include <dolphin/types.h>

typedef struct OSModuleInfo OSModuleInfo;

typedef struct OSModuleQueue {
    OSModuleInfo* head;
    OSModuleInfo* tail;
} OSModuleQueue;

extern OSModuleQueue __OSModuleList : 0x800030C8;
extern const void* __OSStringTable : 0x800030D0;

void __OSModuleInit(void) {
    __OSModuleList.head = __OSModuleList.tail = 0;
    __OSStringTable = 0;
}
