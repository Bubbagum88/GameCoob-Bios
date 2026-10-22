#include "jaudio/heapctrl.h"

#include "jaudio/dummyrom.h"

#include "jaudio/sdk.h"
#include <stddef.h>

// BS2's heapctrl.c has no DMA buffer, heap delete or garbage collection:
// Pikmin's ARAM/DRAM DMA helpers, Jac_DeleteHeap and Jac_ShowHeap are not here.

static u32 global_id = 0;

/**
 * @TODO: Documentation
 * @note UNUSED Size: 00002C
 */
void Jac_GetUnlockHeap(jaheap_*)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 00001C
 */
void Jac_CheckAlloc(jaheap_*)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * BS2: 81353580, size 0x44
 */
void Jac_InitHeap(jaheap_* heap)
{
	heap->startAddress     = 0;
	heap->usedSize         = 0;
	heap->size             = 0;
	heap->heapId           = global_id++;
	heap->isRootHeap       = 0;
	heap->childCount       = 0;
	heap->firstChild       = 0;
	heap->parent           = NULL;
	heap->nextSibling      = 0;
	heap->groupOwner       = 0;
	heap->firstGroupedHeap = 0;
	heap->nextGroupedHeap  = 0;
}

/**
 * @TODO: Documentation
 * BS2: 813535E0, size 0x38
 */
void Jac_SelfInitHeap(jaheap_* heap, u32 startAddr, u32 size, u32 memType)
{
	heap->startAddress     = startAddr;
	heap->size             = size;
	heap->usedSize         = 0;
	heap->isRootHeap       = 0;
	heap->memoryType       = memType;
	heap->childCount       = 0;
	heap->firstChild       = NULL;
	heap->parent           = NULL;
	heap->nextSibling      = 0;
	heap->groupOwner       = 0;
	heap->firstGroupedHeap = 0;
	heap->nextGroupedHeap  = 0;
}

/**
 * @TODO: Documentation
 * BS2: 81353620, size 0x100
 */
BOOL Jac_SelfAllocHeap(jaheap_* parent, jaheap_* heap, u32 size, u32 startAddr)
{
	if (parent->startAddress && parent->startAddress != -1) {
		return FALSE;
	}

	parent->startAddress = startAddr;
	parent->size         = size;
	parent->usedSize     = 0;
	parent->isRootHeap   = 0;
	parent->memoryType   = heap->memoryType;
	parent->childCount   = 0;
	parent->firstChild   = nullptr;
	parent->parent       = heap;

	jaheap_* temp = heap->firstChild;
	if (temp == NULL) {
		heap->firstChild    = parent;
		parent->nextSibling = nullptr;
		heap->usedSize      = parent->startAddress - heap->startAddress + parent->size;
	} else {
		jaheap_* temp2 = heap->firstChild;
		if (parent->startAddress < heap->firstChild->startAddress) {
			parent->nextSibling = heap->firstChild;
			heap->firstChild    = parent;
		} else {
			while (TRUE) {
				if (!(temp = temp2->nextSibling)) {
					parent->nextSibling = NULL;
					temp2->nextSibling  = parent;
					heap->usedSize      = parent->startAddress - heap->startAddress + parent->size;
					break;
				}
				if (parent->startAddress < temp->startAddress) {
					parent->nextSibling = temp;
					temp2->nextSibling  = parent;
					break;
				}
				temp2 = temp;
			}
		}
	}

	heap->childCount++;
	return TRUE;
}

/**
 * @TODO: Documentation
 * BS2: 81353720, size 0x38
 */
BOOL Jac_SetGroupHeap(jaheap_* heapA, jaheap_* heapB)
{
	if (heapA->groupOwner || heapA->nextGroupedHeap) {
		return FALSE;
	}

	heapA->groupOwner       = heapB;
	heapA->nextGroupedHeap  = heapB->firstGroupedHeap;
	heapB->firstGroupedHeap = heapA;
	return TRUE;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 00000C
 */
void Jac_CutdownHeap(jaheap_*)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * BS2: 81353760, size 0x5c
 */
void Jac_InitMotherHeap(jaheap_* heap, u32 startAddr, u32 size, u8 memType)
{
	heap->startAddress     = startAddr + 0x1f & 0xffffffe0;
	heap->usedSize         = 0;
	heap->size             = size - (startAddr & 0x1f);
	heap->heapId           = global_id++;
	heap->isRootHeap       = 1;
	heap->memoryType       = memType;
	heap->childCount       = 0;
	heap->firstChild       = NULL;
	heap->parent           = NULL;
	heap->nextSibling      = NULL;
	heap->groupOwner       = NULL;
	heap->firstGroupedHeap = NULL;
	heap->nextGroupedHeap  = NULL;
}

/**
 * @TODO: Documentation
 * BS2: 813537C0, size 0x1b4
 */
BOOL Jac_AllocHeap(jaheap_* heap, jaheap_* parent, u32 size)
{
	u32 y;
	jaheap_* temp;
	jaheap_* temp2;
	jaheap_* temp3;
	jaheap_* result;
	u32 t;
	u32 max;
	u32 x;

	u32 fixedSize = OSRoundUp32B(size);

	if (parent->startAddress == 0) {
		return FALSE;
	}

	if (heap->startAddress && heap->startAddress != -1) {
		return FALSE;
	}

	if (parent->size - parent->usedSize < fixedSize) {
		temp   = parent->firstChild;
		y      = parent->startAddress;
		result = NULL;
		max    = 0xfffffff;
		while (TRUE) {
			if (temp == NULL) {
				break;
			}
			x = temp->startAddress - y;
			if (x >= fixedSize) {
				x -= fixedSize;

				if (x < max) {
					result = temp;
					t      = y;
					max    = x;
				}
			}

			y    = temp->startAddress + temp->size;
			temp = temp->nextSibling;
		}

		if (result == 0) {
			return FALSE;
		}

		if (result == parent->firstChild) {
			heap->nextSibling  = parent->firstChild;
			parent->firstChild = heap;
		} else {
			temp3 = parent->firstChild;
			while (TRUE) {
				if (temp3->nextSibling == result) {
					heap->nextSibling  = temp3->nextSibling;
					temp3->nextSibling = heap;
					break;
				}
				temp3 = temp3->nextSibling;
			}
		}

		heap->startAddress = t;
		heap->size         = fixedSize;
		heap->usedSize     = 0;
		heap->isRootHeap   = 0;
		heap->memoryType   = parent->memoryType;
		heap->childCount   = 0;
		heap->firstChild   = NULL;
		heap->parent       = parent;
		parent->childCount++;
		return TRUE;
	}

	heap->startAddress = parent->startAddress + parent->usedSize;
	heap->size         = fixedSize;
	heap->usedSize     = 0;
	heap->isRootHeap   = 0;
	heap->memoryType   = parent->memoryType;
	heap->childCount   = 0;
	heap->firstChild   = NULL;
	heap->parent       = parent;

	temp2 = parent->firstChild;
	!temp2;
	if (temp2 == NULL) {
		parent->firstChild = heap;
		heap->nextSibling  = NULL;
	} else {
		while (TRUE) {
			if (temp2->nextSibling == NULL) {
				temp2->nextSibling = heap;
				break;
			}
			temp2 = temp2->nextSibling;
		}
	}

	heap->nextSibling = NULL;
	parent->usedSize += fixedSize;
	parent->childCount++;
	return TRUE;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000058
 */
void Jac_CheckFreeHeap_Total(jaheap_*)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 00005C
 */
void Jac_CheckFreeHeap_Linear(jaheap_*)
{
	TRAP_UNIMPLEMENTED;
}
