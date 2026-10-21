#include <dolphin/os.h>

#define OS_TIME_MONTH_MAX 12
#define OS_TIME_WEEK_DAY_MAX 7
#define OS_TIME_YEAR_DAY_MAX 365

#define SECS_IN_MIN ((s64)60)
#define SECS_IN_HOUR (SECS_IN_MIN * 60)
#define SECS_IN_DAY (SECS_IN_HOUR * 24)
#define SECS_IN_YEAR (SECS_IN_DAY * 365)

#define BIAS (2000 * 365 + (2000 + 3) / 4 - (2000 - 1) / 100 + (2000 - 1) / 400)

#define OS_TIME_ADJUST (*(OSTime*)0x800030D8)

int EXIProbeReset(void);

static int YearDays[OS_TIME_MONTH_MAX] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
static int LeapYearDays[OS_TIME_MONTH_MAX] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

asm OSTime OSGetTime(void) {
    nofralloc
@1
    mftbu r3
    mftb r4
    mftbu r5
    cmpw r3, r5
    bne @1
    blr
}

asm OSTick OSGetTick(void) {
    nofralloc
    mftb r3
    blr
}

static asm void __SetTime(register OSTime time) {
    nofralloc
    li r5, 0
    mttbl r5
    mttbu r3
    mttbl r4
    blr
}

void __OSSetTime(OSTime time) {
    BOOL enabled;
    OSTime* timeAdjustAddr = (OSTime*)0x800030D8;

    enabled = OSDisableInterrupts();
    *timeAdjustAddr += OSGetTime() - time;
    __SetTime(time);
    EXIProbeReset();
    OSRestoreInterrupts(enabled);
}

OSTime __OSGetSystemTime(void) {
    BOOL enabled;
    OSTime* timeAdjustAddr = (OSTime*)0x800030D8;
    OSTime result;

    enabled = OSDisableInterrupts();
    result = *timeAdjustAddr + OSGetTime();
    OSRestoreInterrupts(enabled);
    return result;
}

static inline int IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static inline int GetYearDays(int year, int mon) {
    int* md = IsLeapYear(year) ? LeapYearDays : YearDays;

    return md[mon];
}

static inline int GetLeapDays(int year) {
    if (year < 1) {
        return 0;
    }
    return (year + 3) / 4 - (year - 1) / 100 + (year - 1) / 400;
}

static void GetDates(int days, OSCalendarTime* td) {
    int year;
    int n;
    int month;
    int* p_days;

    td->wday = (days + 6) % OS_TIME_WEEK_DAY_MAX;

    for (year = days / OS_TIME_YEAR_DAY_MAX; days < (n = year * OS_TIME_YEAR_DAY_MAX + GetLeapDays(year)); year--) {
        ;
    }
    days -= n;
    td->year = year;
    td->yday = days;

    p_days = IsLeapYear(year) ? LeapYearDays : YearDays;
    for (month = OS_TIME_MONTH_MAX; days < p_days[--month];) {
        ;
    }
    td->mon = month;
    td->mday = days - p_days[month] + 1;
}

void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* td) {
    int days;
    int secs;
    OSTime d;

    d = ticks % OSSecondsToTicks(1);
    if (d < 0) {
        d += OSSecondsToTicks(1);
    }
    td->usec = (int)(OSTicksToMicroseconds(d) % 1000);
    td->msec = (int)(OSTicksToMilliseconds(d) % 1000);

    ticks -= d;
    days = (int)(OSTicksToSeconds(ticks) / SECS_IN_DAY + BIAS);
    secs = (int)(OSTicksToSeconds(ticks) % SECS_IN_DAY);
    if (secs < 0) {
        days -= 1;
        secs += SECS_IN_DAY;
    }

    GetDates(days, td);
    td->hour = secs / 60 / 60;
    td->min = (secs / 60) % 60;
    td->sec = secs % 60;
}

OSTime OSCalendarTimeToTicks(OSCalendarTime* td) {
    OSTime secs;
    int ov_mon;
    int mon;
    int year;

    ov_mon = td->mon / OS_TIME_MONTH_MAX;
    mon = td->mon - (ov_mon * OS_TIME_MONTH_MAX);
    if (mon < 0) {
        mon += OS_TIME_MONTH_MAX;
        ov_mon--;
    }
    year = td->year + ov_mon;

    secs = SECS_IN_YEAR * year + SECS_IN_DAY * (GetLeapDays(year) + GetYearDays(year, mon) + td->mday - 1) +
           SECS_IN_HOUR * td->hour + SECS_IN_MIN * td->min + td->sec - (OSTime)0xEB1E1BF80ULL;

    return OSSecondsToTicks(secs) + OSMillisecondsToTicks((OSTime)td->msec) +
           OSMicrosecondsToTicks((OSTime)td->usec);
}
