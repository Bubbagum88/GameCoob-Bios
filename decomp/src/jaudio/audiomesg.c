#include "jaudio/audiomesg.h"

/**
 * @TODO: Documentation
 * BS2: 8133C240, size 0x18
 */
void Jac_InitMessageQueue(Jac_MessageQueue* queue, OSMessage* msgArray, s32 count)
{
	queue->msgArray   = msgArray;
	queue->msgCount   = count;
	queue->firstIndex = 0;
	queue->usedCount  = 0;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000044
 */
BOOL Jac_SendMessageBlock(Jac_MessageQueue* queue, OSMessage mesg)
{
	while (Jac_SendMessage(queue, mesg) != TRUE) { }
	return TRUE;
}

/**
 * @TODO: Documentation
 */
BOOL Jac_SendMessage(Jac_MessageQueue* queue, OSMessage msg)
{
	int mesgId;
	BOOL enable = OSDisableInterrupts();

	while (queue->msgCount <= queue->usedCount) {
		OSRestoreInterrupts(enable);
		return FALSE;
	}

	mesgId                  = (queue->msgCount - 1) & (queue->firstIndex + queue->usedCount);
	queue->msgArray[mesgId] = msg;
	queue->usedCount++;

	OSRestoreInterrupts(enable);

	return TRUE;
}

/**
 * @TODO: Documentation
 * BS2: 8133CB00, size 0x58
 */
BOOL Jac_ReceiveMessage(Jac_MessageQueue* queue, OSMessage* outMsg)
{
	BOOL enable = OSDisableInterrupts();
	BOOL res    = Jac_ReceiveMessage_Fast(queue, outMsg);
	OSRestoreInterrupts(enable);
	return res;
}

/**
 * @TODO: Documentation
 */
BOOL Jac_ReceiveMessage_Fast(Jac_MessageQueue* queue, OSMessage* outMsg)
{
	while (queue->usedCount == 0) {
		return FALSE;
	}

	if (outMsg) {
		*outMsg = queue->msgArray[queue->firstIndex];
	}

	queue->firstIndex = (queue->firstIndex + 1) & (queue->msgCount - 1);
	queue->usedCount--;

	return TRUE;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000088
 */
void Jac_JamMessage(Jac_MessageQueue*, OSMessage)
{
	TRAP_UNIMPLEMENTED;
}

// IPL-only functions in this unit with no Pikmin counterpart, still assembly:
//   fn_8133C260 (size 0x14)
//   fn_8133C280 (size 0x24)
//   fn_8133C2C0 (size 0x1c)
//   fn_8133C2E0 (size 0x1c)
//   fn_8133C300 (size 0x14)
//   fn_8133C320 (size 0x64)
//   fn_8133C3A0 (size 0x134)
//   fn_8133C4E0 (size 0x188)
//   fn_8133C680 (size 0x140)
//   fn_8133C7C0 (size 0x84)
//   fn_8133C860 (size 0xa8)
//   fn_8133C920 (size 0x7c)
//   fn_8133C9A0 (size 0x7c)
//   fn_8133CA20 (size 0xd8)
