#ifndef _JAUDIO_JAUDIO_TYPES_H
#define _JAUDIO_JAUDIO_TYPES_H

// Common definitions for the JAudio sources, which come from the Pikmin
// decompilation (github.com/projectPiki/pikmin, CC0) and use its types.h helpers.
// JAudio is built as C++ (-lang c++), like in Pikmin.

#include "types.h"
#include "macros.h"
#include <stddef.h>

#ifndef ATTRIBUTE_ALIGN
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#endif

#if defined(__cplusplus)
#define BEGIN_SCOPE_EXTERN_C extern "C" {
#define END_SCOPE_EXTERN_C   }
#else
#define BEGIN_SCOPE_EXTERN_C
#define END_SCOPE_EXTERN_C
#endif

#if defined(__cplusplus) && __cplusplus < 201103L
#define nullptr 0
#endif

// Pikmin marks parameters const only in non-matching builds; matching code has none.
#define immut

#define HALF_PI 1.5707964f

// Body of a function Pikmin's binary does not keep (the linker stripped it)
#if defined(__MWERKS__)
#define TRAP_UNIMPLEMENTED asm { trap } ((void)0)
#else
#define TRAP_UNIMPLEMENTED ((void)0)
#endif

// Uses a ternary to pad the stack by some number of words
#if defined(__MWERKS__)
#define STACK_PAD_TERNARY(expr, n) JAUDIO_REPEAT_##n((expr) ? "fake" : "fake")
#define JAUDIO_REPEAT_1(x) x
#define JAUDIO_REPEAT_2(x) x; x
#define JAUDIO_REPEAT_3(x) x; x; x
#define JAUDIO_REPEAT_4(x) x; x; x; x
#else
#define STACK_PAD_TERNARY(expr, n) ((void)0)
#endif

#endif
