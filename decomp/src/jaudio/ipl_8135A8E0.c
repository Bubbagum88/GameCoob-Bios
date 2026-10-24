#include "jaudio/jaudio_types.h"
#include "jaudio/aictrl.h"
#include "jaudio/aramcall.h"
#include "jaudio/audiothread.h"
#include "jaudio/bankread.h"
#include "jaudio/cmdstack.h"
#include "jaudio/connect.h"
#include "jaudio/driverinterface.h"
#include "jaudio/dspbuf.h"
#include "jaudio/dummyprobe.h"
#include "jaudio/dvdthread.h"
#include "jaudio/fxinterface.h"
#include "jaudio/heapctrl.h"
#include "jaudio/ja_calc.h"
#include "jaudio/jammain_2.h"
#include "jaudio/oneshot.h"
#include "jaudio/seqsetup.h"
#include "jaudio/virload.h"
#include "jaudio/waveread.h"
#include <dolphin/os.h>
#include <string.h>

// The IPL's sound glue: the BS2 code drives JAudio only through these.
// Built as C++ like the rest of JAudio, but the entry points are plain C names.

void* ARAlloc(u32 size);
int Check_GlobalActiveChannel();

BEGIN_SCOPE_EXTERN_C

// Track the IPL's sequence plays on.
extern seqp_ lbl_815D3C68;

static char sBankName[0x20] = "Banks/ipl.bnk";
static char sWaveName[0x20] = "Banks/ipl.ws";
static FxlineConfig sFxConfig[4] = {
	{ 1, 0x0, 0x6000, 0x1, 0x0000, 50, { 0, 0, 0, 500, 1000, 4000, 7000, 8000 } },
	{ 1, 0x0, 0x0000, 0x1, 0x6000, 50, { 0, 0, 0, 500, 1000, 4000, 7000, 8000 } },
	{ 2, 0xA, 0x3FFF, 0xB, 0x0000, 40, { 0, 120, 250, 500, 1000, 4000, 4000, 4000 } },
	{ 0, 0xA, 0x1000, 0xB, 0x0000, 0, { 0, 0, 0, 500, 1000, 4000, 7000, 8000 } },
};

static s32 sSeqTrack;

void fn_8135B2A0(void);
u8* fn_8135B360(immut char* path);
u32 fn_8135B460(immut char* path);
s32 fn_8135B540(u32 id);

/* 0x8135A8E0: the spin sound of the cube; mode 1 is the far side of the turn */
void Sound_UpdateSpin(u8 mode, f32 speed)
{
	f32 s;
	f32 vol;
	f32 b;
	f32 a;
	f32 pitch;
	seqp_* track;
	u32 unused;

	if (speed > 1.0f) {
		speed = 1.0f;
	}
	s   = sqrtf2(speed);
	vol = 32767.0f - 32767.0f * s;
	track = Jam_GetTrackHandle(0x21002);
	if (mode == 1) {
		vol = 0.0f;
	}
	Jam_WritePortAppDirect(track, 1, vol);
	if (s != 0.0f) {
		track = Jam_GetTrackHandle(0x21003);
		switch (mode) {
		case 0:
			a     = 2048.0f * (1.6f * (0.1f + s));
			b     = 2048.0f * (0.1f + s);
			pitch = 10.0f / s;
			if (pitch > 100.0f) {
				pitch = 100.0f;
			}
			break;
		case 1:
			a     = 2048.0f * (3.0f * (0.15f + s));
			b     = 2048.0f * (3.6f * (0.15f + s));
			pitch = 10.0f / s;
			if (pitch > 100.0f) {
				pitch = 100.0f;
			}
			break;
		}
		Jam_WritePortAppDirect(track, 3, a);
		Jam_WritePortAppDirect(track, 2, b);
		if (s > 0.6f) {
			Jam_WritePortAppDirect(track, 0, (u8)pitch);
		}
	}
}

/* 0x8135AA80 */
void Sound_Play(u32 id)
{
	static s32 mode = 1;
	static u32 spin = 0;
	u8 pad[8];

	if (id < 3) {
		Jam_WritePortAppDirect(Jam_GetTrackHandle(0x21002), 0, id);
		spin = 0;
	} else if (id == 23) {
		spin++;
		if (spin > 20) {
			spin = 0;
		}
		if (spin > 10) {
			Sound_UpdateSpin(1, (21 - spin) / 10.0f);
		} else {
			Sound_UpdateSpin(0, spin / 10.0f);
		}
	} else if (id < 23) {
		Jam_WritePortAppDirect(Jam_GetTrackHandle(0x20002), 0, id);
	} else {
		Jam_WritePortAppDirect(Jam_GetTrackHandle(0x20001), 0, id - 5);
	}
}

/* 0x8135ABE0 */
void Sound_StopAmbient(void)
{
	Jam_WritePortAppDirect(Jam_GetTrackHandle(0x21001), 1, 0);
}

/* 0x8135AC20 */
void Sound_StartAmbient(u32 id)
{
	u8 pad[8];
	static s32 a = 1;
	fn_8135B540(id);
	static s32 b = 0;

	if (id == 0) {
		Jam_WritePortAppDirect(Jam_GetTrackHandle(0x21001), 0, 1);
	}
}

/* 0x8135ACC0: ARAM loader for the archive files */
u32 fn_8135ACC0(immut char* name, u32 ofs, u32 size, u32* status, jaheap_* heap)
{
	u8* src;
	u32 dst;
	volatile u32 done;

	if (strcmp(name, "ipl_1.aw") == 0) {
		OSReport("DVDコール(%s) ofs %d size %d\n", name, ofs, size);
		return 0;
	}
	OSReport("読み出しコール(%s) ofs %d size %d\n", name, ofs, size);
	src = fn_8135B360(name);
	if (size == 0) {
		size = fn_8135B460(name);
	}
	dst = (u32)ARAlloc(size);
	DVDT_DRAMtoARAM(0x123456, (u32)src, dst, size, (u32*)&done, NULL);
	while (done == 0) { }
	OSReport("♪Janus/BANK/Init:: Virtual波形 %s(%x)を ARAM %xへ 読みこみました\n", name, src, dst);
	OSReport("読み出しコールエンド(%s)\n", name);
	heap->startAddress = dst;
	*status = 1;
	return dst;
}

/* 0x8135ADE0 */
void fn_8135ADE0(void)
{
	u8* data;
	u32 state;
	u32 i;
	char* name;
	u8 pad[8];

	Bank_Init();
	Console_printf("\n♪Janus/BANK/Init:: バンクの初期ローディングを行います\n");
	for (i = 0; i < 0x100; i++) {
		switch (i) {
		case 0:
			name  = sBankName;
			state = 2;
			break;
		default:
			state = 0;
			break;
		}
		if (state == 2) {
			data  = fn_8135B360(name);
			state = 1;
			Console_printf("♪Janus/BANK/Init:: Virtualバンク %sを読みこみました\n", name);
		}
		if (state != 0) {
			Console_printf("♪Janus/BANK/Init:: ロードが終了しました。ポインタ接続処理を開始します\n");
			Bank_Regist(data, i);
		}
	}
	Console_printf("♪Janus/BANK/Init:: バンクの初期ローディングが終了しました\n");
}

/* 0x8135AEC0 */
void fn_8135AEC0(void)
{
	u8* data;
	u32 state;
	u32 i;
	char* name;
	u8 pad[8];

	Wavegroup_Init();
	Console_printf("\n♪Janus/Wave/Init:: ウェーブシステムバンクの初期化を行います\n");
	for (i = 0; i < 0x100; i++) {
		switch (i) {
		case 0:
			name  = sWaveName;
			state = 2;
			break;
		default:
			state = 0;
			break;
		}
		if (state == 2) {
			data  = fn_8135B360(name);
			state = 1;
			Console_printf("♪Janus/BANK/Init:: Virtualウェーブバンク %sを読みこみました\n", name);
		}
		if (state != 0) {
			Console_printf("♪Janus/Wave/Init:: ロードが終了しました。ポインタ接続処理を開始します\n");
			Wavegroup_Regist(data, i);
		}
	}
	Console_printf("♪Janus/Wave/Init:: バンクの初期ローディングが終了しました\n");
}

/* 0x8135AFA0 */
void fn_8135AFA0(void)
{
	static const char name[] = "iplrom.com";
	u32 size = fn_8135B460(name);
	u8* data = fn_8135B360(name);

	sSeqTrack = Jaq_SetSeqData(&lbl_815D3C68, data, size, 0);
	Jaq_StartSeq(sSeqTrack);
}

/* 0x8135B020 */
void fn_8135B020(void)
{
	Jaq_Reset();
	OSReport("Test::DVD Bank&Wave Read\n");
	Jac_ConnectTableInit();
	Jam_InitRegistTrack();
	Jac_RegisterARAMCallback(fn_8135ACC0);
	fn_8135AEC0();
	WaveScene_Set(0, 0);
	fn_8135ADE0();
	fn_8135AFA0();
}

/* 0x8135B080 */
void fn_8135B080(void)
{
	OSReport("----アーカイブ 事前登録---------\n");
}

/* 0x8135B0C0: start JAudio for the IPL */
void fn_8135B0C0(void* heap, s32 heapSize, u32 aramSize)
{
	u32 i;

	StartAudioThread(heap, heapSize, aramSize, 3);
	fn_8135B080();
	fn_8135B020();
	Jac_Portcmd_Init();
	Jac_SetMixerLevel(0.8f, 4.0f);
	for (i = 0; i < 4; i++) {
		DFX_SetFxLine(i, i < 3 ? (s16*)OSAlloc2(sFxConfig[i].circularBufferSize * 0xA0) : NULL, &sFxConfig[i]);
	}
}

/* 0x8135B160 */
void Sound_SetOutputMode(u32 mode)
{
	Jac_SetOutputMode(mode);
}

/* 0x8135B180: restart the IPL sequence */
void fn_8135B180(void)
{
	u32 count;
	u32 unused;

	Jaq_StopSeq(sSeqTrack);
	count = Jac_GetCurrentSCounter();
	while (count == Jac_GetCurrentSCounter()) { }
	jcs_* handle = Get_GlobalHandle();
	AllStop_1Shot(handle);
	FlushRelease_1Shot(handle);
	fn_8135AFA0();
	fn_8135B2A0();
	OSReport("Reset-ReEntry\n");
}

/* 0x8135B200: stop all sound */
void fn_8135B200(void)
{
	BOOL level = OSDisableInterrupts();
	Jaq_StopSeq(sSeqTrack);
	jcs_* handle = Get_GlobalHandle();
	AllStop_1Shot(handle);
	FlushRelease_1Shot(handle);
	OSRestoreInterrupts(level);
	fn_8135B2A0();
}

/* 0x8135B260 */
void LCEnable(void)
{
	BOOL level = OSDisableInterrupts();
	StopAudioThread();
	OSRestoreInterrupts(level);
}

static s32 sQuietFrames;

/* 0x8135B2A0 */
void fn_8135B2A0(void)
{
	sQuietFrames = 0;
}

/* 0x8135B2C0: TRUE once no channel has played for 30 polls */
BOOL fn_8135B2C0(void)
{
	BOOL level = OSDisableInterrupts();
	u32 active = Check_GlobalActiveChannel();
	OSRestoreInterrupts(level);
	OSReport("Remain Chan. %d\n", active);
	if (active == 0) {
		sQuietFrames++;
		if (sQuietFrames >= 30) {
			return TRUE;
		}
		return FALSE;
	}
	sQuietFrames = 0;
	return FALSE;
}

END_SCOPE_EXTERN_C
