#ifndef _DOLPHIN_OSMESSAGE_H_
#define _DOLPHIN_OSMESSAGE_H_

#include <dolphin/os/OSThread.h>

typedef void* OSMessage;

typedef struct OSMessageQueue {
    OSThreadQueue queueSend;
    OSThreadQueue queueReceive;
    OSMessage* msgArray;
    s32 msgCount;
    s32 firstIndex;
    s32 usedCount;
} OSMessageQueue;

#define OS_MESSAGE_NOBLOCK 0
#define OS_MESSAGE_BLOCK 1

void OSInitMessageQueue(OSMessageQueue* mq, OSMessage* msgArray, s32 msgCount);
BOOL OSSendMessage(OSMessageQueue* mq, OSMessage msg, s32 flags);
BOOL OSJamMessage(OSMessageQueue* mq, OSMessage msg, s32 flags);
BOOL OSReceiveMessage(OSMessageQueue* mq, OSMessage* msg, s32 flags);

#endif
