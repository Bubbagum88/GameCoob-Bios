#ifndef _DOLPHIN_OSEXCEPTION_H_
#define _DOLPHIN_OSEXCEPTION_H_

#include <dolphin/os/OSContext.h>

typedef u8 __OSException;
typedef void (*__OSExceptionHandler)(__OSException exception, OSContext* context);

#define __OS_EXCEPTION_DECREMENTER 8

__OSExceptionHandler __OSSetExceptionHandler(__OSException exception, __OSExceptionHandler handler);
__OSExceptionHandler __OSGetExceptionHandler(__OSException exception);

#endif
