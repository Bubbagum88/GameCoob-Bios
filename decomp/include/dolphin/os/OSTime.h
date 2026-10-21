#ifndef _DOLPHIN_OSTIME_H_
#define _DOLPHIN_OSTIME_H_

#include <dolphin/types.h>

typedef s64 OSTime;
typedef u32 OSTick;

typedef struct OSCalendarTime {
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int wday;
    int yday;
    int msec;
    int usec;
} OSCalendarTime;

extern u32 __OSBusClock : 0x800000F8;
#define OS_BUS_CLOCK __OSBusClock
#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)

#define OSTicksToSeconds(ticks) ((ticks) / OS_TIMER_CLOCK)
#define OSTicksToMilliseconds(ticks) ((ticks) / (OS_TIMER_CLOCK / 1000))
#define OSTicksToMicroseconds(ticks) (((ticks)*8) / (OS_TIMER_CLOCK / 125000))
#define OSSecondsToTicks(sec) ((sec)*OS_TIMER_CLOCK)
#define OSMillisecondsToTicks(msec) ((msec) * (OS_TIMER_CLOCK / 1000))
#define OSMicrosecondsToTicks(usec) (((usec) * (OS_TIMER_CLOCK / 125000)) / 8)

OSTime OSGetTime(void);
OSTick OSGetTick(void);
OSTime __OSGetSystemTime(void);
OSTime __OSTimeToSystemTime(OSTime time);
void __OSSetTime(OSTime time);
void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* td);
OSTime OSCalendarTimeToTicks(OSCalendarTime* td);

#endif
