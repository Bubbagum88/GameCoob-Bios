#ifndef _DOLPHIN_OSERROR_H_
#define _DOLPHIN_OSERROR_H_

#include <dolphin/os/OSContext.h>

typedef u16 OSError;
typedef void (*OSErrorHandler)(OSError error, OSContext* context, ...);

#define OS_ERROR_MACHINE_CHECK 1

OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler);

#endif
