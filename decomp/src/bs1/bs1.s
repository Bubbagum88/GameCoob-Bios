# BS1: the first code the Gekko runs after reset (PAL 1.0).
#
# Hand-written assembly, not compiled C. It runs straight out of the IPL ROM
# at the reset vector 0xFFF00100, through the EXI descrambler, so every 32-byte
# line is fetched over the slow EXI bus. To make loops fast it first walks a
# chain of `b` instructions, one per cache line, which pulls the following
# lines into the instruction cache, and only then enters the loop. Those
# "prefetch hops" are marked below; they are why the code is broken into
# 32-byte pieces that branch over each other.
#
# Steps:
#   1. HID0/MSR, ARAM and MI setup, caches on, BATs and segment registers.
#   2. Pulse the PI reset lines.
#   3. Probe EXI channel 2 for an AD16 (the debug trace device on dev units).
#      If one answers, POST codes 0x01000000.. are written to it, and the
#      24 MiB of main memory is tested with 0xAAAAAAAA and 0x55555555.
#      A failing test hangs forever after reporting where it failed.
#   4. DMA 0x1AFF00 bytes from ROM 0x800 to physical 0x012FFFE0 over EXI 0,
#      so ROM 0x820 (BS2) lands at 0x81300000.
#   5. Disable the ROM (EXI0 CSR ROMDIS), unlock SI EXI clock, clear the
#      OS BI2 pointer and jump to BS2 at 0x81300000.
#
# Register use (no stack, no calling convention):
#   r2  EXI register base          r14 SI register base
#   r8  1 (TSTART mask)            r10 0 (deselect)
#   r15 AD16 word to write         r20 AD16 ID read back, r21 expected ID
#   r22 EXI2 CSR value for AD16    r25/r26 memtest base/pattern
#   r17-r19 failing 256 KiB blocks (bitmask), r27/r28 fail counts per bus
#   half, r29 highest failing address

.set HID0_INIT,     0x00110C64
.set MSR_FP,        0x00002000

.set AR_SIZE,       0x0C005012      # DSP interface: ARAM size / mode
.set MI_BASE,       0x0C004000      # memory interface
.set PI_BASE,       0xCC003000      # processor interface
.set SI_BASE,       0xCC006400
.set EXI_BASE,      0xCC006800
.set SI_EXILK,      0x3C

# EXI channel 0 (ROM) and channel 2 (AD16) registers, relative to EXI_BASE
.set EXI0_CSR,      0x00
.set EXI0_MAR,      0x04
.set EXI0_LENGTH,   0x08
.set EXI0_CR,       0x0C
.set EXI0_DATA,     0x10
.set EXI2_CSR,      0x28
.set EXI2_CR,       0x34
.set EXI2_DATA,     0x38

.set AD16_ID,       0x04120000
.set AD16_CMD_ID,   0x00000000
.set AD16_CMD_WRITE, 0xA0000000

.set MEM_SIZE,      0x01800000      # 24 MiB
.set BS2_ROM,       0x800           # copy starts 0x20 before BS2 itself
.set BS2_DMA_DEST,  0x012FFFE0
.set BS2_COPY_SIZE, 0x001AFF00
.set BS2_ENTRY,     0x81300000

.section .text, "ax"

.global __reset
__reset:
	lis r4, HID0_INIT@h
	addi r4, r4, HID0_INIT@l
	mthid0 r4
	lis r4, MSR_FP@h
	addi r4, r4, MSR_FP@l
	mtmsr r4

	# ARAM: size/mode 0x43, refresh 0x9C
	lis r4, AR_SIZE@h
	addi r4, r4, AR_SIZE@l
	li r5, 0x43
	sth r5, 0(r4)
	li r5, 0x9C
	sth r5, 8(r4)

	lis r3, MI_BASE@h
	ori r3, r3, MI_BASE@l
	li r4, 0x40
	sth r4, 0x26(r3)
	nop
	nop

	# HID0 |= ICE | DCE
	mfhid0 r3
	ori r4, r3, 0xC000
	mthid0 r4
	nop
	nop
	nop
	isync

	# Clear all BATs
	li r4, 0
	mtdbatu 0, r4
	mtdbatu 1, r4
	mtdbatu 2, r4
	mtdbatu 3, r4
	mtibatu 0, r4
	mtibatu 1, r4
	mtibatu 2, r4
	mtibatu 3, r4
	isync

	# Segment registers: T=1 (direct store), so only BATs translate
	lis r4, 0x8000
	addi r4, r4, 0
	mtsr 0, r4
	mtsr 1, r4
	mtsr 2, r4
	mtsr 3, r4
	mtsr 4, r4
	mtsr 5, r4
	mtsr 6, r4
	mtsr 7, r4
	mtsr 8, r4
	mtsr 9, r4
	mtsr 10, r4
	mtsr 11, r4
	mtsr 12, r4
	mtsr 13, r4
	mtsr 14, r4
	mtsr 15, r4

	# BAT0: 0x80000000 -> 0, 256 MiB, cached
	lis r4, 0
	addi r4, r4, 0x0002
	lis r3, 0x8000
	addi r3, r3, 0x1FFF
	mtdbatl 0, r4
	mtdbatu 0, r3
	isync
	mtibatl 0, r4
	mtibatu 0, r3
	isync

	# DBAT1: 0xC0000000 -> 0, 256 MiB, cache-inhibited and guarded
	lis r4, 0
	addi r4, r4, 0x002A
	lis r3, 0xC000
	addi r3, r3, 0x1FFF
	mtdbatl 1, r4
	mtdbatu 1, r3
	isync

	# BAT3: 0xFFF00000 -> 0xFFF00000, 1 MiB (this ROM)
	lis r4, 0xFFF0
	addi r4, r4, 0x0001
	lis r3, 0xFFF0
	addi r3, r3, 0x001F
	mtdbatl 3, r4
	mtdbatu 3, r3
	isync
	mtibatl 3, r4
	mtibatu 3, r3
	isync

	# MSR |= IR | DR
	mfmsr r4
	ori r4, r4, 0x0030
	mtmsr r4
	isync

	# PI: pulse the reset lines, holding them for 0x1124 timebase ticks
	lis r3, PI_BASE@h
	ori r3, r3, PI_BASE@l
	lis r4, 0x0245
	ori r4, r4, 0x248A
	stw r4, 0x30(r3)
	lwz r4, 0x24(r3)
	ori r4, r4, 1
	rlwinm r4, r4, 0, 31, 28
	stw r4, 0x24(r3)
	mftb r5
.Lreset_wait:
	mftb r6
	subf r7, r5, r6
	cmplwi r7, 0x1124
	blt .Lreset_wait
	ori r4, r4, 3
	stw r4, 0x24(r3)

	lis r14, SI_BASE@h
	ori r14, r14, SI_BASE@l
	li r4, 0
	stw r4, SI_EXILK(r14)

	# Probe for an AD16 on EXI channel 2: send the ID command, read 4 bytes
	lis r2, EXI_BASE@h
	ori r2, r2, EXI_BASE@l
	lis r22, 0
	ori r22, r22, 0xBA
	li r8, 1
	li r10, 0
	lis r21, AD16_ID@h
	ori r21, r21, AD16_ID@l
	lis r3, AD16_CMD_ID@h
	ori r3, r3, AD16_CMD_ID@l
	lis r7, 0
	ori r7, r7, 0x15
	stw r3, EXI2_DATA(r2)
	stw r22, EXI2_CSR(r2)
	lwz r16, EXI2_CSR(r2)
	stw r7, EXI2_CR(r2)
.Lid_cmd_wait:
	lwz r16, EXI2_CR(r2)
	and. r16, r16, r8
	bgt .Lid_cmd_wait
	lis r7, 0
	ori r7, r7, 0x31
	stw r7, EXI2_CR(r2)
.Lid_read_wait:
	lwz r16, EXI2_CR(r2)
	and. r16, r16, r8
	bgt .Lid_read_wait
	stw r10, EXI2_CSR(r2)
	lwz r16, EXI2_CSR(r2)
	lwz r16, EXI2_CSR(r2)
	lwz r20, EXI2_DATA(r2)
	b .Lhop_ad16_0
	.balign 32, 0

	# --- ad16_write, interleaved with its prefetch hops ---
.Lhop_ad16_0:
	b .Lhop_ad16_1
ad16_write_body:
	lis r3, AD16_CMD_WRITE@h
	ori r3, r3, AD16_CMD_WRITE@l
	lis r7, 0
	ori r7, r7, 0x05
	stw r3, EXI2_DATA(r2)
	stw r22, EXI2_CSR(r2)
	lwz r16, EXI2_CSR(r2)
	b .Lad16_write_1
.Lhop_ad16_1:
	b .Lhop_ad16_2
.Lad16_write_1:
	stw r7, EXI2_CR(r2)
.Lad16_cmd_wait:
	lwz r16, EXI2_CR(r2)
	and. r16, r16, r8
	bgt .Lad16_cmd_wait
	nop
	nop
	b .Lad16_write_2
.Lhop_ad16_2:
	b .Lhop_ad16_3
.Lad16_write_2:
	lis r7, 0
	ori r7, r7, 0x35
	stw r15, EXI2_DATA(r2)
	stw r7, EXI2_CR(r2)
.Lad16_data_wait:
	lwz r16, EXI2_CR(r2)
	and. r16, r16, r8
	b .Lad16_write_3
.Lhop_ad16_3:
	b .Lhop_ad16_4
.Lad16_write_3:
	bgt .Lad16_data_wait
	stw r10, EXI2_CSR(r2)
	lwz r16, EXI2_CSR(r2)
	lwz r16, EXI2_CSR(r2)
	blr
	.balign 32, 0

.Lhop_ad16_4:
	b .Lpost_1
	# Write r15 to the AD16, if there is one
ad16_write:
	cmplw r20, r21
	beq ad16_write_body
	blr

.Lpost_1:
	lis r15, 0x0100
	bl ad16_write
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	b .Lpost_2
	.balign 32, 0

.Lpost_2:
	nop
	nop
	nop
	nop
	nop
	lis r15, 0x0200
	bl ad16_write
	b .Lhop_mem_0

	# --- memtest, interleaved with its prefetch hops ---
.Lhop_mem_0:
	b .Lhop_mem_1
	# Fill MEM_SIZE bytes at r25 with r26, then read back and record misses
memtest:
	nop
	nop
	nop
	nop
	nop
	mr r23, r25
	lis r24, MEM_SIZE@h
	b .Lmemtest_1
.Lhop_mem_1:
	b .Lhop_mem_2
.Lmemtest_1:
	srwi r24, r24, 5
	mtctr r24
.Lfill_loop:
	stw r26, 0x00(r23)
	stw r26, 0x04(r23)
	stw r26, 0x08(r23)
	stw r26, 0x0C(r23)
	b .Lmemtest_2
.Lhop_mem_2:
	b .Lhop_mem_3
.Lmemtest_2:
	stw r26, 0x10(r23)
	stw r26, 0x14(r23)
	stw r26, 0x18(r23)
	stw r26, 0x1C(r23)
	addi r23, r23, 0x20
	bdnz .Lfill_loop
	b .Lmemtest_3
.Lhop_mem_3:
	b .Lhop_mem_4
.Lmemtest_3:
	mr r23, r25
	lis r24, MEM_SIZE@h
	srwi r24, r24, 2
	mtctr r24
.Lcheck_loop:
	lwz r15, 0(r23)
	cmplw r15, r26
	b .Lmemtest_4
.Lhop_mem_4:
	b .Lhop_mem_5
.Lmemtest_4:
	beq .Lcheck_next
	# Mark the failing 256 KiB block: bits 0-31 in r17, 32-63 in r18, 64-95 in r19
	srwi r15, r23, 18
	andi. r15, r15, 31
	li r16, 1
	slw r16, r16, r15
	or r17, r17, r16
	b .Lmemtest_5
.Lhop_mem_5:
	b .Lhop_mem_6
.Lmemtest_5:
	srwi r15, r23, 18
	addi r15, r15, -32
	andi. r15, r15, 31
	li r16, 1
	slw r16, r16, r15
	or r18, r18, r16
	b .Lmemtest_6
.Lhop_mem_6:
	b .Lhop_mem_7
.Lmemtest_6:
	srwi r15, r23, 18
	addi r15, r15, -64
	andi. r15, r15, 31
	li r16, 1
	slw r16, r16, r15
	or r19, r19, r16
	b .Lmemtest_7
.Lhop_mem_7:
	b .Lhop_mem_8
.Lmemtest_7:
	# Count it against the low or high half of the 64-bit bus
	clrlwi r15, r23, 28
	cmplwi r15, 8
	bge .Lfail_high
	addi r28, r28, 1
	b .Lcheck_next
.Lfail_high:
	addi r27, r27, 1
	b .Lmemtest_8
.Lhop_mem_8:
	b .Lmemtest_run
.Lmemtest_8:
	cmplw r29, r23
	bge .Lcheck_next
	mr r29, r23
.Lcheck_next:
	addi r23, r23, 4
	bdnz .Lcheck_loop
	blr

.Lmemtest_run:
	li r17, 0
	li r18, 0
	li r19, 0
	li r27, 0
	li r28, 0
	li r29, 0
	lis r15, 0x0300
	bl ad16_write
	b .Lhop_report_0
	.balign 32, 0

.Lhop_report_0:
	b .Lhop_report_1
	# Only with an AD16 attached: test all of memory with two patterns
.Lrun_memtest:
	lis r25, 0x8000
	lis r26, 0xAAAA
	ori r26, r26, 0xAAAA
	bl memtest
	not r26, r26
	bl memtest
	nop
	b .Lreport
.Lhop_report_1:
	b .Lhop_report_2
.Lreport:
	# 0x04000000 | 0x02000000 if high half failed | 0x01000000 if low half
	# failed | highest failing address / 4
	lis r15, 0x0400
	mr. r16, r27
	beq .Lreport_1
	oris r15, r15, 0x0200
.Lreport_1:
	mr. r16, r28
	beq .Lreport_2
	b .Lreport_3
.Lhop_report_2:
	b .Lmemtest_check
.Lreport_3:
	oris r15, r15, 0x0100
.Lreport_2:
	srwi r29, r29, 2
	or r15, r15, r29
	bl ad16_write
	nop
	nop
	b .Lmemtest_hang

.Lmemtest_check:
	cmplw r20, r21
	beq .Lrun_memtest
	# Bad memory: hang here forever
.Lmemtest_hang:
	mr. r16, r27
	bne .Lmemtest_hang
	mr. r16, r28
	bne .Lmemtest_hang

	# Copy BS2 out of ROM over EXI 0 in 1 KiB DMA chunks
	lis r2, EXI_BASE@h
	ori r2, r2, EXI_BASE@l
	lis r6, 0
	ori r6, r6, 0x150              # CSR: CS1 (ROM), 32 MHz
	lis r7, 0
	ori r7, r7, 0x35               # CR: imm write 4 bytes
	li r8, 1
	lis r9, 0
	ori r9, r9, 0x03               # CR: DMA read
	li r10, 0
	lis r11, 0
	ori r11, r11, 0x400            # chunk size
	lis r12, 0x0001
	ori r12, r12, 0x0000           # chunk size << 6 (ROM address step)
	lis r3, (BS2_ROM << 6)@h
	ori r3, r3, (BS2_ROM << 6)@l   # ROM read command: address << 6
	lis r4, BS2_DMA_DEST@h
	ori r4, r4, BS2_DMA_DEST@l
	lis r13, BS2_COPY_SIZE@h
	ori r13, r13, BS2_COPY_SIZE@l
	b .Lhop_copy_0
	.balign 32, 0

	# --- copy loop, interleaved with its prefetch hops ---
.Lhop_copy_0:
	b .Lhop_copy_1
.Lcopy_loop:
	cmpwi r13, 0
	beq .Lcopy_done
	mr r5, r11
	cmplw r13, r5
	bgt .Lcopy_1
	mr r5, r13
.Lcopy_1:
	stw r6, EXI0_CSR(r2)
	b .Lcopy_2
.Lhop_copy_1:
	b .Lhop_copy_2
.Lcopy_2:
	stw r3, EXI0_DATA(r2)
	lwz r16, EXI0_CSR(r2)
	stw r7, EXI0_CR(r2)
.Lcopy_cmd_wait:
	lwz r16, EXI0_CR(r2)
	and. r16, r16, r8
	bgt .Lcopy_cmd_wait
	b .Lcopy_3
.Lhop_copy_2:
	b .Lhop_copy_3
.Lcopy_3:
	stw r4, EXI0_MAR(r2)
	lwz r4, EXI0_MAR(r2)
	stw r5, EXI0_LENGTH(r2)
	lwz r5, EXI0_LENGTH(r2)
	stw r9, EXI0_CR(r2)
.Lcopy_dma_wait:
	lwz r16, EXI0_CR(r2)
	b .Lcopy_4
.Lhop_copy_3:
	b .Lhop_copy_4
.Lcopy_4:
	and. r16, r16, r8
	bgt .Lcopy_dma_wait
	stw r10, EXI0_CSR(r2)
	lwz r16, EXI0_CSR(r2)
	lwz r16, EXI0_CSR(r2)
	add r3, r3, r12
	b .Lcopy_5
.Lhop_copy_4:
	b .Lhop_copy_5
.Lcopy_5:
	add r4, r4, r11
	subf r13, r5, r13
	b .Lcopy_loop

.Lcopy_done:
	lis r4, BS2_ENTRY@h
	ori r4, r4, BS2_ENTRY@l
	mtlr r4
	b .Lcopy_done_1
.Lhop_copy_5:
	b .Lhop_copy_6
.Lcopy_done_1:
	# EXI0 CSR ROMDIS: the boot ROM can no longer be read
	lis r6, 0
	ori r6, r6, 0x2000
	stw r6, EXI0_CSR(r2)
	li r4, 1
	stw r4, SI_EXILK(r14)
	lwz r4, SI_EXILK(r14)
	b .Lcopy_done_2
.Lhop_copy_6:
	b .Lhop_copy_7
.Lcopy_done_2:
	lis r4, 0x8000
	li r3, 0
	stw r3, 0xF4(r4)               # OSBootInfo BI2 pointer
	blr                            # to BS2
	.balign 32, 0

	# Prefetch hops through the rest of the line chain, then into the loop
.Lhop_copy_7:
	b .Lhop_copy_8
	.balign 32, 0
.Lhop_copy_8:
	b .Lhop_copy_9
	.balign 32, 0
.Lhop_copy_9:
	b .Lhop_copy_10
	.balign 32, 0
.Lhop_copy_10:
	b .Lhop_copy_11
	.balign 32, 0
.Lhop_copy_11:
	b .Lhop_copy_12
	.balign 32, 0
.Lhop_copy_12:
	b .Lhop_copy_13
	.balign 32, 0
.Lhop_copy_13:
	b .Lcopy_loop
	.balign 32, 0
