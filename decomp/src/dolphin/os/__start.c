// From the Pikmin decomp (projectPiki/pikmin, CC0), cut down to the IPL's
// older startup: no TRK hook, no OSInit, argc/argv go to main in r3/r4.

#include <dolphin/os.h>
#include <dolphin/db.h>
#include <string.h>

#pragma section code_type ".init"

#define BOOTINFO2_ADDR 0x800000F4
#define ARENAHI_ADDR   0x80000034

typedef struct __rom_copy_info {
	char* rom;
	char* addr;
	u32 size;
} __rom_copy_info;

typedef struct __bss_init_info {
	char* addr;
	u32 size;
} __bss_init_info;

__declspec(section ".init") extern __rom_copy_info _rom_copy_info[];
__declspec(section ".init") extern __bss_init_info _bss_init_info[];
__declspec(section ".init") extern char _stack_addr[];
__declspec(section ".init") extern char _SDA_BASE_[];
__declspec(section ".init") extern char _SDA2_BASE_[];

extern void __init_hardware(void);
extern void __flush_cache(void* addr, int size);
extern void __init_user(void);
extern int main(int argc, char* argv[]);
extern void exit(int);
static void __init_registers(void);
static void __init_data(void);

asm void __start(void)
{
	nofralloc
	bl      __init_registers
	bl      __init_hardware
	li      r0, -1
	stwu    r1, -8 (r1)
	stw     r0, 4 (r1)
	stw     r0, 0 (r1)
	bl      __init_data
	lis     r6,     BOOTINFO2_ADDR @ha
	addi    r6, r6, BOOTINFO2_ADDR @l
	lwz     r5, 0 (r6)
	cmplwi  r5, 0
	beq+    _no_args
	addi    r6, r5, 8
	lwz     r6, 0 (r6)
	cmplwi  r6, 0
	beq+    _no_args
	add     r6, r5, r6
	lwz     r3, 0 (r6)
	cmplwi  r3, 0
	beq     _no_args
	addi    r4, r6, 4
	mtctr   r3

_loop:
	addi    r6, r6, 4
	lwz     r7, 0 (r6)
	add     r7, r7, r5
	stw     r7, 0 (r6)
	bdnz    _loop
	lis     r5,     ARENAHI_ADDR @ha
	addi    r5, r5, ARENAHI_ADDR @l
	rlwinm  r7, r4, 0, 0, 26
	stw     r7, 0 (r5)
	b       _end_of_parseargs

_no_args:
	li      r3, 0
	li      r4, 0

_end_of_parseargs:
	bl      DBInit
	bl      __init_user
	bl      main
	b       exit
}

asm static void __init_registers(void)
{
	nofralloc
	lis     r1,      _stack_addr @h
	ori     r1, r1,  _stack_addr @l
	lis     r2,      _SDA2_BASE_ @h
	ori     r2, r2,  _SDA2_BASE_ @l
	lis     r13,      _SDA_BASE_ @h
	ori     r13, r13, _SDA_BASE_ @l
	blr
}

static void __copy_rom_section(void* dst, const void* src, u32 size)
{
	if (size && (dst != src)) {
		memcpy(dst, src, size);
		__flush_cache(dst, size);
	}
}

static void __init_bss_section(void* dst, u32 size)
{
	if (size) {
		memset(dst, 0, size);
	}
}

static void __init_data(void)
{
	__rom_copy_info* dci;
	__bss_init_info* bii;

	dci = _rom_copy_info;
	while (TRUE) {
		if (dci->size == 0)
			break;
		__copy_rom_section(dci->addr, dci->rom, dci->size);
		dci++;
	}

	bii = _bss_init_info;
	while (TRUE) {
		if (bii->size == 0)
			break;
		__init_bss_section(bii->addr, bii->size);
		bii++;
	}
}
