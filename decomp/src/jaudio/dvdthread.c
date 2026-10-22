// Function bodies also follow ac-decomp's jaudio_NES/internal/dvdthread.c (CC0).
#include "jaudio/dvdthread.h"
#include "jaudio/sdk.h"
#include "jaudio/aictrl.h"
#include "jaudio/dummyprobe.h"
#include "jaudio/sample.h"
#include <stddef.h>

static char audio_root_path[32] = "";

static OSMessageQueue mq;
static s32 msgbuf[0x80];
static u8 CALLSTACK[0x8000];

static u32 mq_init;
static size_t buffersize;
static u32 buffers;
static size_t next_buffersize;
static u8* next_buffertop;
static u32 next_buffers;
static u32 cur_q;
static OSThreadQueue dvdt_sleep;
static BOOL DVDT_PAUSE_FLAG;

static ErrorCallback error_callback;
static u8* ADVD_BUFFER[2];
static u32 buffer_load;

static void __Alloc_DVDBuffer();
static void __UpdateBuffer();
static void __WriteBufferSize(u8*, u32, u32);

/**
 * @TODO: Documentation
 * BS2: 8133A7C0, size 0x60
 */
static void* GetCallStack()
{
	void* ret;
	BOOL enable = OSDisableInterrupts();

	ret = &CALLSTACK[cur_q++ * 0x100];

	if (cur_q == 0x80) {
		cur_q = 0;
	}

	OSRestoreInterrupts(enable);

	return ret;
}

/**
 * @TODO: Documentation
 * BS2: 8133A820, size 0xac
 */
static s32 DVDReadMutex(DVDFileInfo* fileInfo, void* addr, s32 len, s32 offs, immut char* errorArg)
{
	if (DVDT_PAUSE_FLAG == TRUE) {
		Console_printf("\201\364Jac/DVDT::DVDThread Paused!\n");
		OSSleepThread(&dvdt_sleep);
		Console_printf("\201\364Jac/DVDT::DVDThread UnPaused!\n");
	}

	while (TRUE) {
		if (DVDReadPrio(fileInfo, addr, len, offs, 2) != -1 || error_callback == NULL) {
			break;
		}
		error_callback(errorArg, (u8*)addr);
	}
}

/**
 * @TODO: Documentation
 * BS2: 8133A8E0, size 0x80
 */
void DVDT_ExtendPath(char* dst, immut char* ext)
{
	if (*audio_root_path != '\0') {
		strcpy(dst, audio_root_path);
		if (*ext == '/') {
			strcat(dst, ext + 1);
		} else {
			strcat(dst, ext);
		}
	} else {
		strcpy(dst, ext);
	}
}

/**
 * @TODO: Documentation
 * BS2: 8133A960, size 0x7c
 */
s32 DVDT_AddTaskHigh(TaskCallback callback, void* stack, size_t len)
{
	if (mq_init == FALSE) {
		return 0;
	}

	TaskCallback* cstack = (TaskCallback*)GetCallStack();

	Jac_bcopy(stack, cstack + 1, len);

	*cstack = callback;
	OSJamMessage(&mq, (OSMessage)cstack, OS_MESSAGE_BLOCK);

	return 1;
}

/**
 * @TODO: Documentation
 * BS2: 8133A9E0, size 0x7c
 */
s32 DVDT_AddTask(TaskCallback callback, void* stack, size_t len)
{
	if (mq_init == FALSE) {
		return 0;
	}

	TaskCallback* cstack = (TaskCallback*)GetCallStack();

	Jac_bcopy(stack, cstack + 1, len);

	*cstack = callback;
	OSSendMessage(&mq, (OSMessage)cstack, OS_MESSAGE_BLOCK);

	return 1;
}

/**
 * @TODO: Documentation
 * BS2: 8133AA60, size 0x3c
 */
void jac_dvdproc_init()
{
	OSInitMessageQueue(&mq, (OSMessage*)msgbuf, ARRAY_SIZE(msgbuf));
	mq_init = 1;
}

/**
 * @TODO: Documentation
 * BS2: 8133AAA0, size 0x98
 */
void* jac_dvdproc(void*)
{
	// "the DVD thread has started"
	OSReport("\201\364Jac/DVDT::DVD\203X\203\214\203b\203h\202\252\213N\223\256\202\265\202\275\n");
	__Alloc_DVDBuffer();
	TaskCallback* callback;

	u8* buf = (u8*)OSAlloc2(0x10000);

	OSInitThreadQueue(&dvdt_sleep);
	OSMessage msg;
	while (TRUE) {
		while (TRUE) {
			OSReceiveMessage(&mq, &msg, OS_MESSAGE_BLOCK);
			callback = (TaskCallback*)msg;
			__UpdateBuffer();

			if (buffersize == 0) {
				__WriteBufferSize(buf, 2, 0x8000);
			}
			if (callback != NULL) {
				break;
			};
		}
		(*callback)(callback + 1);
	}
}

/**
 * @TODO: Documentation
 * BS2: 8133AB40, size 0x90
 */
static void __DoError(DVDCall* call, u32 type)
{
	switch (type) {
	case 0:
		// "can't open the file"
		OSReport("\201\364Jac/DVDT:: \203t\203@\203C\203\213\202\252\203I\201[\203v\203\223\202\305\202\253\202\334\202\271\202\361\n");
		break;
	case 1:
		// "the file size is 0"
		OSReport("\201\364Jac/DVDT:: \203t\203@\203C\203\213\203T\203C\203Y\202\252\202O\202\305\202\267\n");
		break;
	}

	if (call->callbackStatus != NULL) {
		*call->callbackStatus = -1;
	}

	if (call->callback != NULL) {
		call->callback(-1);
	}
}

/**
 * @TODO: Documentation
 * BS2: 8133ABE0, size 0x44
 */
static void __DoFinish(DVDCall* call, u32 status)
{
	if (call->callbackStatus != NULL) {
		*call->callbackStatus = status;
	}

	if (call->callback != NULL) {
		call->callback(call->owner);
	}
}

/**
 * @TODO: Documentation
 * @note UNUSED in BS2 (stripped), kept for its string
 */
s32 DVDT_LoadtoDRAM_Main(void* dvdCall)
{
	s32 readStatus = 0;
	DVDCall* call  = (DVDCall*)dvdCall;
	DVDFileInfo finfo;

	if (Jac_DVDOpen(call->fileName, &finfo) == FALSE) {
		__DoError(call, 0);
		return -1;
	}
	u32 infoLength = finfo.length;
	if (infoLength == 0) {
		__DoError(call, 1);
		return -1;
	}

	if (call->length == 0) {
		call->length = infoLength;
	}

	DCInvalidateRange((void*)call->dst, call->length);
	readStatus |= DVDReadMutex(&finfo, (void*)call->dst, call->length, call->src, call->fileName);
	DVDClose(&finfo);
	__DoFinish(call, readStatus);
	return 0;
}

/**
 * @TODO: Documentation
 * @note UNUSED in BS2 (stripped), kept for its string
 */
s32 DVDT_LoadtoDRAM(u32 owner, immut char* name, u32 dst, u32 src, u32 length, u32* status, Jac_DVDCallback callback)
{
	DVDCall call;
	void* cb = (void*)&call;

	call.owner = owner;
	if (owner == 0xFFFFFFFF) {
		// "you shouldn't use this ID"
		OSReport("\201\364Jac/DVDT::\202\261\202\314ID\202\360\216g\202\244\202\314\202\315\202\346\202\353\202\265\202\255\202\310\202\242\n");
	}
	DVDT_ExtendPath(((DVDCall*)cb)->fileName, name);

	call.dst            = dst;
	call.callbackStatus = status;
	if (status != 0) {
		*status = 0;
	}

	call.callback = callback;
	call.src      = src;
	call.length   = length;

	DVDT_AddTask(DVDT_LoadtoDRAM_Main, cb, 0x58);

	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 8133AC40, size 0x38
 */
static void __Alloc_DVDBuffer()
{
	if (buffersize == 0) {
		int i;

		for (i = 0; i < buffers; i++) {
			ADVD_BUFFER[i] = 0;
		}
	}
}

/**
 * @TODO: Documentation
 * BS2: 8133AC80, size 0x30
 */
static void __WriteBufferSize(u8* buf, u32 numBuffers, u32 size)
{
	buffersize = size;
	buffers    = numBuffers;

	int i;
	int j = numBuffers;

	for (i = 0; i < numBuffers; i++, j--) {
		ADVD_BUFFER[i] = buf;
		buf += size;
	}
}

/**
 * @TODO: Documentation
 * BS2: 8133ACC0, size 0x40
 */
static void __UpdateBuffer()
{
	if (next_buffers != 0) {
		__WriteBufferSize(next_buffertop, next_buffers, next_buffersize);
		next_buffers   = 0;
		next_buffertop = NULL;
	}
}

static vu32 buffer_full;

/**
 * @TODO: Documentation
 * BS2: 8133AD00, size 0x10
 */
static void ARAM_DMAfinish(u32)
{
	buffer_full--;
}

/**
 * @TODO: Documentation
 * BS2: 8133AD20, size 0x264
 */
s32 DVDT_LoadtoARAM_Main(void* dvdCall)
{
	DVDCall* call        = (DVDCall*)dvdCall;
	static int arq_index = 0;
	static DVDFileInfo finfo;
	static ARQRequest req[4];

	if (!Jac_DVDOpen(call->fileName, &finfo)) {
		__DoError(call, 0);
		return -1;
	}

	u32 len = finfo.length;
	if (len == 0) {
		__DoError(call, 1);
		return -1;
	}

	if (call->length == 0) {
		call->length = len;
	}

	OSReport("\201\364Jac/DVDT::DVD\201\250ARAM ([%s] %d bytes)\n", call->fileName, call->length);
	u32 start = OSGetTick();

	while (call->length != 0) {
		u32 readSize;
		u8* buf     = ADVD_BUFFER[buffer_load];
		buffer_load = (buffer_load + 1) % buffers;
		while (buffer_full == buffers)
			;

		if (call->length < buffersize) {
			readSize = ALIGN_NEXT(call->length, 32);
			len      = DVDReadMutex(&finfo, buf, readSize, call->src, call->fileName);

			call->length = 0;
		} else {
			readSize = buffersize;
			len      = DVDReadMutex(&finfo, buf, readSize, call->src, call->fileName);

			call->src += buffersize;
			call->length -= buffersize;
		}

		ARQPostRequest(&req[arq_index], 0x12345678, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH, (u32)buf, call->dst, readSize,
		               ARAM_DMAfinish);
		buffer_full++;
		arq_index++;
		arq_index &= 3;
		call->dst += readSize;
	}

	DVDClose(&finfo);

	while (buffer_full != 0)
		;

	OSReport("\201\364Jac/DVDT::          Finish (%d msec)\n", OSTicksToMilliseconds(OSGetTick() - start));

	__DoFinish(call, len);

	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 8133AFA0, size 0xa8
 */
s32 DVDT_LoadtoARAM(u32 owner, immut char* path, u32 dst, u32 src, u32 length, u32* status, Jac_DVDCallback callback)
{
	DVDCall call;
	void* cb = (void*)&call;

	call.owner = owner;
	if (owner == 0xFFFFFFFF) {
		// "you shouldn't use this ID"
		OSReport("\201\364Jac/DVDT::\202\261\202\314ID\202\360\216g\202\244\202\314\202\315\202\346\202\353\202\265\202\255\202\310\202\242\n");
	}
	DVDT_ExtendPath(((DVDCall*)cb)->fileName, path);

	call.dst            = dst;
	call.callbackStatus = status;
	if (status != 0) {
		*status = 0;
	}

	call.callback = callback;
	call.src      = src;
	call.length   = length;

	DVDT_AddTask(DVDT_LoadtoARAM_Main, cb, 0x58);

	return 0;
}

static vu32 buffer_full2;

/**
 * @TODO: Documentation
 * BS2: 8133B060, size 0x10
 */
static void ARAM_DMAfinish2(u32)
{
	buffer_full2--;
}

/**
 * @TODO: Documentation
 * BS2: 8133B080, size 0x7c
 */
s32 DVDT_ARAMtoDRAM_Main(void* arg)
{
	static DVDFileInfo finfo;
	static ARQRequest req;
	DVDCall* call = (DVDCall*)arg;

	buffer_full2++;

	ARQPostRequest(&req, (u32)call, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH, call->src, call->dst, call->length, ARAM_DMAfinish2);

	while (buffer_full2 != 0)
		;

	__DoFinish(call, call->length);

	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 8133B100, size 0x7c
 */
s32 DVDT_DRAMtoARAM_Main(void* arg)
{
	static DVDFileInfo finfo;
	static ARQRequest req;
	DVDCall* call = (DVDCall*)arg;

	buffer_full2++;

	ARQPostRequest(&req, (u32)call, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH, call->dst, call->src, call->length, ARAM_DMAfinish2);

	while (buffer_full2 != 0)
		;

	__DoFinish(call, call->length);

	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 8133B180, size 0x98
 */
s32 DVDT_ARAMtoDRAM(u32 owner, u32 dst, u32 src, u32 length, u32* status, Jac_DVDCallback callback)
{
	STACK_PAD_VAR(2);
	DVDCall call;
	void* cb = (void*)&call;

	call.owner = owner;
	if (owner == 0xFFFFFFFF) {
		// "you shouldn't use this ID"
		OSReport("\201\364Jac/DVDT::\202\261\202\314ID\202\360\216g\202\244\202\314\202\315\202\346\202\353\202\265\202\255\202\310\202\242\n");
	}
	call.dst            = dst;
	call.callbackStatus = status;

	if (status != 0) {
		*status = 0;
	}

	call.callback = callback;
	call.src      = src;
	call.length   = length;

	DVDT_AddTaskHigh(DVDT_ARAMtoDRAM_Main, cb, 0x58);

	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 8133B220, size 0x98
 */
s32 DVDT_DRAMtoARAM(u32 owner, u32 dst, u32 src, u32 length, u32* status, Jac_DVDCallback callback)
{
	STACK_PAD_VAR(2);
	DVDCall call;
	void* cb = (void*)&call;

	call.owner = owner;
	if (owner == 0xFFFFFFFF) {
		// "you shouldn't use this ID"
		OSReport("\201\364Jac/DVDT::\202\261\202\314ID\202\360\216g\202\244\202\314\202\315\202\346\202\353\202\265\202\255\202\310\202\242\n");
	}
	call.dst            = dst;
	call.callbackStatus = status;

	if (status != 0) {
		*status = 0;
	}

	call.callback = callback;
	call.src      = src;
	call.length   = length;

	DVDT_AddTaskHigh(DVDT_DRAMtoARAM_Main, cb, 0x58);

	return 0;
}

/**
 * @TODO: Documentation
 * BS2: 8133B2C0, size 0x84
 */
s32 DVDT_CheckFile(immut char* file)
{
	static DVDFileInfo finfo;

	char path[64];
	DVDT_ExtendPath(path, file);

	if (!Jac_DVDOpen(path, &finfo)) {
		// "failed to check the size of file %s"
		Console_printf("\201\364\203t\203@\203C\203\213 %s \203T\203C\203Y\203`\203F\203b\203N\202\311\216\270\224s\202\265\202\334\202\265\202\275\n", file);
		return 0;
	}
	u32 len = finfo.length;

	DVDClose(&finfo);

	return len;
}

/**
 * @TODO: Documentation
 * BS2: 8133B360, size 0x74
 */
s32 Jac_CheckFile(immut char* file)
{
	static DVDFileInfo finfo;

	if (!Jac_DVDOpen(file, &finfo)) {
		// "failed to check the size of file %s"
		Console_printf("\201\364\203t\203@\203C\203\213 %s \203T\203C\203Y\203`\203F\203b\203N\202\311\216\270\224s\202\265\202\334\202\265\202\275\n", file);
		return 0;
	}
	u32 len = finfo.length;

	DVDClose(&finfo);

	return len;
}

/**
 * @TODO: Documentation
 * @note UNUSED in BS2, but its static still takes up .bss
 */
void Jac_LoadFile(immut char*, u8*)
{
	static DVDFileInfo finfo;
}

static u32 dvdfile_dics;
static char dvd_file[32][64];
static u32 dvd_entrynum[32];

/**
 * @TODO: Documentation
 * BS2: 8133B3E0, size 0x120
 */
s32 Jac_RegisterFastOpen(immut char* file)
{
	int num;
	if (strlen(file) > 63) {
		return -1;
	}

	int i;
	for (i = 0; i < dvdfile_dics; i++) {
		if (!strcmp(dvd_file[i], file)) {
			return dvd_entrynum[i];
		}
	}
	if (dvdfile_dics == 32) {
		return -1;
	}

	num = DVDConvertPathToEntrynum(file);

	if (num != -1) {
		strcpy(dvd_file[dvdfile_dics], file);
		dvd_entrynum[dvdfile_dics] = num;
		OSReport("\201\364Jac/DVDT :: Registered file %s, (No.%d ID.%d )\n", file, dvdfile_dics, num);
		dvdfile_dics++;
	} else {
		OSReport("\201\364Jac/DVDT :: Cannot find file to regist %s \n", file);
	}
	return num;
}

/**
 * @TODO: Documentation
 * BS2: 8133B500, size 0x50
 */
BOOL Jac_DVDOpen(immut char* name, DVDFileInfo* info)
{
	int entry = Jac_RegisterFastOpen(name);

	if (entry == -1) {
		return DVDOpen(name, info);
	} else {
		return DVDFastOpen(entry, info);
	}
}
