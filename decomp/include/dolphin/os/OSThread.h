#ifndef _DOLPHIN_OSTHREAD_H_
#define _DOLPHIN_OSTHREAD_H_

#include <dolphin/os/OSContext.h>

#define OS_THREAD_SPECIFIC_MAX 2

typedef struct OSThread OSThread;
typedef struct OSMutex OSMutex;
typedef s32 OSPriority;

typedef struct OSThreadQueue {
    OSThread* head;
    OSThread* tail;
} OSThreadQueue;

typedef struct OSThreadLink {
    OSThread* next;
    OSThread* prev;
} OSThreadLink;

typedef struct OSMutexQueue {
    OSMutex* head;
    OSMutex* tail;
} OSMutexQueue;

typedef struct OSMutexLink {
    OSMutex* next;
    OSMutex* prev;
} OSMutexLink;

struct OSMutex {
    OSThreadQueue queue;
    OSThread* thread;
    s32 count;
    OSMutexLink link;
};

struct OSThread {
    OSContext context;
    u16 state;
    u16 attr;
    s32 suspend;
    OSPriority priority;
    OSPriority base;
    void* val;
    OSThreadQueue* queue;
    OSThreadLink link;
    OSThreadQueue queueJoin;
    OSMutex* mutex;
    OSMutexQueue queueMutex;
    OSThreadLink linkActive;
    u8* stackBase;
    u32* stackEnd;
};

#define OS_THREAD_STATE_READY 1
#define OS_THREAD_STATE_RUNNING 2
#define OS_THREAD_STATE_WAITING 4
#define OS_THREAD_STATE_MORIBUND 8

#define OS_THREAD_ATTR_DETACH 0x0001u

#define OS_PRIORITY_MIN 0
#define OS_PRIORITY_MAX 31
#define OS_PRIORITY_IDLE OS_PRIORITY_MAX

#define OS_THREAD_STACK_MAGIC 0xDEADBABE

void OSInitThreadQueue(OSThreadQueue* queue);
OSThread* OSGetCurrentThread(void);
s32 OSDisableScheduler(void);
s32 OSEnableScheduler(void);
void __OSReschedule(void);
BOOL OSCreateThread(OSThread* thread, void* (*func)(void*), void* param, void* stack, u32 stackSize,
                    OSPriority priority, u16 attr);
void OSExitThread(void* val);
void OSCancelThread(OSThread* thread);
s32 OSResumeThread(OSThread* thread);
s32 OSSuspendThread(OSThread* thread);
void OSSleepThread(OSThreadQueue* queue);
void OSWakeupThread(OSThreadQueue* queue);
OSPriority OSGetThreadPriority(OSThread* thread);
OSPriority __OSGetEffectivePriority(OSThread* thread);
void __OSUnlockAllMutex(OSThread* thread);

#endif
