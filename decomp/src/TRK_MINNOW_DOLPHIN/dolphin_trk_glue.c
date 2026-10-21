#include <types.h>

/* MetroTRK serial glue. BS2 only keeps the interrupt hook OSInit calls when
   the debug flag in BI2 is >= 2; the rest of the TRK is not linked. */

typedef int (*DBCommInitFunc)(volatile u8** inputPendingPtrRef, void* cb);
typedef int (*DBCommFunc)(void);
typedef int (*DBCommReadFunc)(u8* buffer, int length);
typedef int (*DBCommWriteFunc)(const u8* buffer, int length);

typedef struct DBCommTable {
    DBCommInitFunc initialize_func;  /* 0x00 */
    DBCommFunc init_interrupts_func; /* 0x04 */
    DBCommFunc peek_func;            /* 0x08 */
    DBCommReadFunc read_func;        /* 0x0C */
    DBCommWriteFunc write_func;      /* 0x10 */
    DBCommFunc open_func;            /* 0x14 */
    DBCommFunc close_func;           /* 0x18 */
} DBCommTable;

DBCommTable gDBCommTable = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void EnableEXI2Interrupts(void);

/* 8137EA4C */
void EnableMetroTRKInterrupts(void) {
    EnableEXI2Interrupts();
}

/* 8137EA6C */
void EnableEXI2Interrupts(void) {
    gDBCommTable.init_interrupts_func();
}
