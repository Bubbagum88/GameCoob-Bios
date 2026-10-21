#ifndef _MSL_TYPES_H
#define _MSL_TYPES_H

/* types.h for the Runtime/MSL_C/TRK libraries (include/stl is searched first
   for them). Their sources come from the Pikmin decomp (projectPiki/pikmin,
   CC0), which uses the same MSL build as BS2. */

#include <dolphin/types.h>

typedef unsigned int uint;
typedef long double f128;
typedef volatile f128 vf128;

#define MACRO_NOTHING
#define MACRO_COMMA ,
#define TO_STRING(x) STRINGIFY(x)
#define STRINGIFY(x) #x

#ifdef __cplusplus
#define BEGIN_SCOPE_EXTERN_C extern "C" {
#define END_SCOPE_EXTERN_C }
#else
#define BEGIN_SCOPE_EXTERN_C
#define END_SCOPE_EXTERN_C
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ARRAY_SIZE(o) (sizeof((o)) / sizeof(*(o)))
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))

// Metrowerks C/C++ language extensions
#if defined(__MWERKS__)
#define AT_ADDRESS(addr) : (addr)
#define WEAK            __declspec(weak)
#define DECL_SECT(name) __declspec(section name)
#define ASM             asm
#else
#define AT_ADDRESS(addr)
#define WEAK
#define DECL_SECT(name)
#define ASM
#endif

#define INIT  DECL_SECT(".init")
#define CTORS DECL_SECT(".ctors")

// If an unimplemented function is used by mistake (e.g. in modding), we should trap on that.
#if defined(__MWERKS__) // clang-format off
#define TRAP_UNIMPLEMENTED asm { trap } ((void)0)
#elif defined(_MSC_VER)
#define TRAP_UNIMPLEMENTED __asm { ud2 } __assume(0)
#else
#define TRAP_UNIMPLEMENTED ((void)0) /* Replace me */
#endif // clang-format on

// Documenting MetroWerks intrinsic functions for PowerPC and placing them behind macros for portability.
#if defined(__MWERKS__)
#define __mwerks_eieio()                  /* void   */ __eieio()                  //
#define __mwerks_sync()                   /* void   */ __sync()                   //
#define __mwerks_isync()                  /* void   */ __isync()                  //
#define __mwerks_abs(value)               /* int    */ __abs(value)               // int
#define __mwerks_labs(value)              /* long   */ __labs(value)              // long
#define __mwerks_fabs(value)              /* float  */ __fabs(value)              // float
#define __mwerks_fnabs(value)             /* float  */ __fnabs(value)             // float
#define __mwerks_cntlzw(value)            /* int    */ __cntlzw(value)            // int
#define __mwerks_lhbrx(base, idx)         /* int    */ __lhbrx(base, idx)         // void*, int
#define __mwerks_lwbrx(base, idx)         /* int    */ __lwbrx(base, idx)         // void*, int
#define __mwerks_sthbrx(value, base, idx) /* void   */ __sthbrx(value, base, idx) // unsigned short, void*, int
#define __mwerks_stwbrx(value, base, idx) /* void   */ __stwbrx(value, base, idx) // unsigned int, void*, int
#define __mwerks_dcbf(base, offset)       /* void   */ __dcbf(base, offset)       // void*, int
#define __mwerks_dcbt(base, offset)       /* void   */ __dcbt(base, offset)       // void*, int
#define __mwerks_dcbst(base, offset)      /* void   */ __dcbst(base, offset)      // void*, int
#define __mwerks_dcbtst(base, offset)     /* void   */ __dcbtst(base, offset)     // void*, int
#define __mwerks_dcbz(base, offset)       /* void   */ __dcbz(base, offset)       // void*, int
#define __mwerks_mulhw(lhs, rhs)          /* int    */ __mulhw(lhs, rhs)          // int, int
#define __mwerks_mulhwu(lhs, rhs)         /* uint   */ __mulhwu(lhs, rhs)         // uint, uint
#define __mwerks_divw(dividend, divisor)  /* int    */ __divw(dividend, divisor)  // int, int
#define __mwerks_divwu(dividend, divisor) /* uint   */ __divwu(dividend, divisor) // uint, uint
#define __mwerks_fmadd(p1, p2, p3)        /* double */ __fmadd(p1, p2, p3)        // double, double, double
#define __mwerks_fmsub(p1, p2, p3)        /* double */ __fmsub(p1, p2, p3)        // double, double, double
#define __mwerks_fnmadd(p1, p2, p3)       /* double */ __fnmadd(p1, p2, p3)       // double, double, double
#define __mwerks_fnmsub(p1, p2, p3)       /* double */ __fnmsub(p1, p2, p3)       // double, double, double
#define __mwerks_fsel(A, C, B)            /* double */ __fsel(A, C, B)            // double, double, double
#define __mwerks_fmadds(p1, p2, p3)       /* float  */ __fmadds(p1, p2, p3)       // float, float, float
#define __mwerks_fmsubs(p1, p2, p3)       /* float  */ __fmsubs(p1, p2, p3)       // float, float, float
#define __mwerks_fnmadds(p1, p2, p3)      /* float  */ __fnmadds(p1, p2, p3)      // float, float, float
#define __mwerks_fnmsubs(p1, p2, p3)      /* float  */ __fnmsubs(p1, p2, p3)      // float, float, float
#define __mwerks_fres(value)              /* float  */ __fres(value)              // float
#define __mwerks_fabsf(value)             /* float  */ __fabsf(value)             // float
#define __mwerks_fnabsf(value)            /* float  */ __fnabsf(value)            // float
#define __mwerks_frsqrte(value)           /* double */ __frsqrte(value)           // double
#define __mwerks_mffs()                   /* double */ __mffs()                   //
#define __mwerks_setflm(value)            /* float  */ __setflm(value)            // float
#define __mwerks_rlwinm(S, SH, MB, ME)    /* int    */ __rlwinm(S, SH, MB, ME)    // int, int, int, int
#define __mwerks_rlwnm(S, SH, MB, ME)     /* int    */ __rlwnm(S, SH, MB, ME)     // int, int, int, int
#define __mwerks_rlwimi(A, S, SH, MB, ME) /* int    */ __rlwimi(A, S, SH, MB, ME) // int, int, int, int, int
#define __mwerks_strcpy(dest, src)        /* char*  */ __strcpy(dest, src)        // char*, const char*
#define __mwerks_memcpy(dest, src, size)  /* void*  */ __memcpy(dest, src, size)  // void*, const void*, size_t
#define __mwerks_alloca(size)             /* void*  */ __alloca(size)             // unsigned long
#define __mwerks_va_setup(args)           /* void   */ __va_setup(args)           // void*
#define __mwerks_builtin_va_info(args)    /* void   */ __builtin_va_info(args)    // va_list*
#endif

#endif
