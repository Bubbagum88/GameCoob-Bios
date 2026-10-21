#ifndef _DOLPHIN_GXREGS_H
#define _DOLPHIN_GXREGS_H

// Register indices for __cpReg/__piReg/__peReg/__memReg (from the Pikmin decomp hw_regs.h)

#define CP_STATUS                 (0)
#define CP_CONTROL                (1)
#define CP_CLEAR                  (2)
#define CP_PERF_SELECT            (3)
#define CP_04                     (4)
#define CP_05                     (5)
#define CP_06                     (6)
#define CP_07                     (7)
#define CP_08                     (8)
#define CP_09                     (9)
#define CP_10                     (10)
#define CP_11                     (11)
#define CP_12                     (12)
#define CP_13                     (13)
#define CP_14                     (14)
#define CP_15                     (15)
#define CP_FIFO_BASE_LO           (16)
#define CP_FIFO_BASE_HI           (17)
#define CP_FIFO_END_LO            (18)
#define CP_FIFO_END_HI            (19)
#define CP_FIFO_HI_WATERMARK_LO   (20)
#define CP_FIFO_HI_WATERMARK_HI   (21)
#define CP_FIFO_LO_WATERMARK_LO   (22)
#define CP_FIFO_LO_WATERMARK_HI   (23)
#define CP_FIFO_RW_DISTANCE_LO    (24)
#define CP_FIFO_RW_DISTANCE_HI    (25)
#define CP_FIFO_WRITE_POINTER_LO  (26)
#define CP_FIFO_WRITE_POINTER_HI  (27)
#define CP_FIFO_READ_POINTER_LO   (28)
#define CP_FIFO_READ_POINTER_HI   (29)
#define CP_FIFO_BP_LO             (30)
#define CP_FIFO_BP_HI             (31)
#define CP_XF_RASBUSY_LO          (32)
#define CP_XF_RASBUSY_HI          (33)
#define CP_XF_CLKS_L0             (34)
#define CP_XF_CLKS_HI             (35)
#define CP_XF_WAIT_IN_LO          (36)
#define CP_XF_WAIT_IN_HI          (37)
#define CP_XF_WAIT_OUT_LO         (38)
#define CP_XF_WAIT_OUT_HI         (39)
#define CP_VCACHE_METRIC_CHECK_LO (40)
#define CP_VCACHE_METRIC_CHECK_HI (41)
#define CP_VCACHE_METRIC_MISS_LO  (42)
#define CP_VCACHE_METRIC_MISS_HI  (43)
#define CP_VCACHE_METRIC_STALL_LO (44)
#define CP_VCACHE_METRIC_STALL_HI (45)
#define CP_CLKS_PER_VTX_IN_LO     (48)
#define CP_CLKS_PER_VTX_IN_HI     (49)
#define CP_CLKS_PER_VTX_OUT       (50)
#define PE_Z_CONFIG                      (0)
#define PE_ALPHA_CONFIG                  (1)
#define PE_DEST_ALPHA_CONFIG             (2)
#define PE_ALPHA_MODE                    (3)
#define PE_ALPHA_READ                    (4)
#define PE_CONTROL_REGISTER              (5)
#define PE_06                            (6)
#define PE_TOKEN                         (7)
#define PE_BB_LEFT                       (8)  // bounding box
#define PE_BB_RIGHT                      (9)  // bounding box
#define PE_BB_TOP                        (10) // bounding box
#define PE_BB_BOTTOM                     (11) // bounding box
#define PE_PERF_ZCOMP_INPUT_ZCOMPLOC_LO  (12)
#define PE_PERF_ZCOMP_INPUT_ZCOMPLOC_HI  (13)
#define PE_PERF_ZCOMP_OUTPUT_ZCOMPLOC_LO (14)
#define PE_PERF_ZCOMP_OUTPUT_ZCOMPLOC_HI (15)
#define PE_PERF_ZCOMP_INPUT_LO           (16)
#define PE_PERF_ZCOMP_INPUT_HI           (17)
#define PE_PERF_ZCOMP_OUTPUT_LO          (18)
#define PE_PERF_ZCOMP_OUTPUT_HI          (19)
#define PE_PERF_BLEND_INPUT_LO           (20)
#define PE_PERF_BLEND_INPUT_HI           (21)
#define PE_PERF_EFB_COPY_CLOCKS_LO       (22)
#define PE_PERF_EFB_COPY_CLOCKS_HI       (23)
#define PI_INTRPT_SRC  (0) // interrupt cause
#define PI_INTRPT_MASK (1) // interrupt mask
#define PI_FIFO_START  (3) // FIFO base start
#define PI_FIFO_END    (4) // FIFO base end
#define PI_FIFO_PTR    (5) // FIFO current write pointer
#define PI_FIFO_RESET  (6)
#define PI_07          (7)
#define PI_08          (8)
#define PI_RESETCODE   (9) // reset code, used by OSReset
#define PI_10          (10)
#define PI_FLIPPER_REV (11)
#define PI_12          (12)
#define PI_INTRPT_ERR       (0x1)     // GP runtime error
#define PI_INTRPT_RSW       (0x2)     // reset switch
#define PI_INTRPT_DVD       (0x4)     // DVD/DI interrupt
#define PI_INTRPT_SI        (0x8)     // serial/controller interrupt
#define PI_INTRPT_EXI       (0x10)    // external mem interrupt
#define PI_INTRPT_AI        (0x20)    // audio streaming interrupt
#define PI_INTRPT_DSP       (0x40)    // digital signal proc interrupt
#define PI_INTRPT_MEM       (0x80)    // memory interface interrupt
#define PI_INTRPT_VI        (0x100)   // video interface interrupt
#define PI_INTRPT_PE_TOKEN  (0x200)   // pixel engine token
#define PI_INTRPT_PE_FINISH (0x400)   // pixel engine finish
#define PI_INTRPT_CP        (0x800)   // command FIFO
#define PI_INTRPT_DEBUG     (0x1000)  // external debugger
#define PI_INTRPT_HSP       (0x2000)  // high speed port
#define PI_INTRPT_RSWST     (0x10000) // reset switch state (1 when pressed)
#define MEM_PROT_0_FIRST   (0) // protected region 0
#define MEM_PROT_0_LAST    (1) // protected region 0
#define MEM_PROT_1_FIRST   (2) // protected region 1
#define MEM_PROT_1_LAST    (3) // protected region 1
#define MEM_PROT_2_FIRST   (4) // protected region 2
#define MEM_PROT_2_LAST    (5) // protected region 2
#define MEM_PROT_3_FIRST   (6) // protected region 3
#define MEM_PROT_3_LAST    (7) // protected region 3
#define MEM_PROT_TYPE      (8) // protection type
#define MEM_09             (9)
#define MEM_10             (10)
#define MEM_11             (11)
#define MEM_12             (12)
#define MEM_13             (13)
#define MEM_INTRPT_MASK    (14) // interrupt mask
#define MEM_INTRPT_SRC     (15) // interrupt cause
#define MEM_INTRPT_FLAG    (16) // set when interrupt happens
#define MEM_INTRPT_ADDR_LO (17) // address that caused interrupt
#define MEM_INTRPT_ADDR_HI (18) // address that caused interrupt
#define MEM_19             (19)
#define MEM_20             (20) // unknown memory flag, set in __OSInitMemoryProtection
#define MEM_21             (21)
#define MEM_22             (22)
#define MEM_23             (23)
#define MEM_24             (24)
#define MEM_TIMER0_HI      (25)
#define MEM_TIMER0_LO      (26)
#define MEM_TIMER1_HI      (27)
#define MEM_TIMER1_LO      (28)
#define MEM_TIMER2_HI      (29)
#define MEM_TIMER2_LO      (30)
#define MEM_TIMER3_HI      (31)
#define MEM_TIMER3_LO      (32)
#define MEM_TIMER4_HI      (33)
#define MEM_TIMER4_LO      (34)
#define MEM_TIMER5_HI      (35)
#define MEM_TIMER5_LO      (36)
#define MEM_TIMER6_HI      (37)
#define MEM_TIMER6_LO      (38)
#define MEM_TIMER7_HI      (39)
#define MEM_TIMER7_LO      (40)
#define MEM_TIMER8_HI      (41)
#define MEM_TIMER8_LO      (42)
#define MEM_TIMER9_HI      (43)
#define MEM_TIMER9_LO      (44)

#endif
