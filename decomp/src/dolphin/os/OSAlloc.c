#include <dolphin/os.h>

#define ALIGNMENT 32
#define InRange(cell, arenaStart, arenaEnd) ((u32)(arenaStart) <= (u32)(cell) && (u32)(cell) < (u32)(arenaEnd))
#define OFFSET(n, a) (((u32)(n)) & ((a)-1))
#define TRUNC(n, a) (((u32)(n)) & ~((a)-1))
#define ROUND(n, a) (((u32)(n) + (a)-1) & ~((a)-1))
#define HEADERSIZE ((u32)ROUND(sizeof(Cell), ALIGNMENT))
#define MINOBJSIZE (HEADERSIZE + ALIGNMENT)

typedef struct Cell {
    struct Cell* prev;
    struct Cell* next;
    long size;
} Cell;

typedef struct HeapDesc {
    long size;
    Cell* free;
    Cell* allocated;
} HeapDesc;

volatile OSHeapHandle __OSCurrHeap = -1;

static HeapDesc* HeapArray;
static int NumHeaps;
static void* ArenaStart;
static void* ArenaEnd;

static Cell* DLAddFront(Cell* list, Cell* cell) {
    cell->next = list;
    cell->prev = NULL;
    if (list) {
        list->prev = cell;
    }
    return cell;
}

static Cell* DLLookup(Cell* list, Cell* cell) {
    for (; list; list = list->next) {
        if (list == cell) {
            return list;
        }
    }
    return NULL;
}

static Cell* DLExtract(Cell* list, Cell* cell) {
    if (cell->next) {
        cell->next->prev = cell->prev;
    }
    if (cell->prev == NULL) {
        return cell->next;
    } else {
        cell->prev->next = cell->next;
        return list;
    }
}

static Cell* DLInsert(Cell* list, Cell* cell) {
    Cell* prev;
    Cell* next;

    for (next = list, prev = NULL; next; prev = next, next = next->next) {
        if (cell <= next) {
            break;
        }
    }

    cell->next = next;
    cell->prev = prev;
    if (next) {
        next->prev = cell;
        if ((char*)cell + cell->size == (char*)next) {
            cell->size += next->size;
            cell->next = next = next->next;
            if (next) {
                next->prev = cell;
            }
        }
    }
    if (prev) {
        prev->next = cell;
        if ((char*)prev + prev->size == (char*)cell) {
            prev->size += cell->size;
            prev->next = next;
            if (next) {
                next->prev = prev;
            }
        }
        return list;
    } else {
        return cell;
    }
}

static BOOL DLOverlap(Cell* list, void* start, void* end) {
    Cell* cell;

    for (cell = list; cell; cell = cell->next) {
        if (((char*)start <= (char*)cell && (char*)cell < (char*)end) ||
            ((char*)start < (char*)cell + cell->size && (char*)cell + cell->size <= (char*)end)) {
            return TRUE;
        }
    }
    return FALSE;
}

static long DLSize(Cell* list) {
    Cell* cell;
    long size;

    size = 0;
    for (cell = list; cell; cell = cell->next) {
        size += cell->size;
    }
    return size;
}

void* OSAllocFromHeap(OSHeapHandle heap, u32 size) {
    HeapDesc* hd;
    Cell* cell;
    Cell* newCell;
    long leftoverSize;

    hd = &HeapArray[heap];
    size += HEADERSIZE;
    size = ROUND(size, ALIGNMENT);

    for (cell = hd->free; cell != NULL; cell = cell->next) {
        if ((long)size <= cell->size) {
            break;
        }
    }

    if (cell == NULL) {
        return NULL;
    }

    leftoverSize = cell->size - (long)size;
    if (leftoverSize < MINOBJSIZE) {
        hd->free = DLExtract(hd->free, cell);
    } else {
        cell->size = (long)size;
        newCell = (Cell*)((char*)cell + size);
        newCell->size = leftoverSize;
        newCell->prev = cell->prev;
        newCell->next = cell->next;
        if (newCell->next != NULL) {
            newCell->next->prev = newCell;
        }
        if (newCell->prev != NULL) {
            newCell->prev->next = newCell;
        } else {
            hd->free = newCell;
        }
    }

    hd->allocated = DLAddFront(hd->allocated, cell);
    return (void*)((char*)cell + HEADERSIZE);
}

void* OSAllocFixed(void** rstart, void** rend) {
    OSHeapHandle i;
    Cell* cell;
    Cell* newCell;
    HeapDesc* hd;
    void* start = (void*)TRUNC(*rstart, ALIGNMENT);
    void* end = (void*)ROUND(*rend, ALIGNMENT);

    for (i = 0; i < NumHeaps; i++) {
        hd = &HeapArray[i];
        if (hd->size < 0) {
            continue;
        }
        if (DLOverlap(hd->allocated, start, end)) {
            return NULL;
        }
    }

    for (i = 0; i < NumHeaps; i++) {
        hd = &HeapArray[i];
        if (hd->size < 0) {
            continue;
        }
        for (cell = hd->free; cell; cell = cell->next) {
            void* cellEnd = (char*)cell + cell->size;

            if ((char*)cellEnd <= (char*)start) {
                continue;
            }
            if ((char*)end <= (char*)cell) {
                break;
            }
            if ((char*)cell < (char*)start - HEADERSIZE && (char*)start < (char*)cellEnd) {
                if ((char*)cellEnd < (char*)end + MINOBJSIZE) {
                    end = cellEnd;
                }
            }
            if ((char*)start - HEADERSIZE <= (char*)cell && (char*)cell < (char*)end) {
                if ((char*)cellEnd < (char*)end + MINOBJSIZE) {
                    end = cellEnd;
                }
            }
            if ((char*)cell < (char*)start - HEADERSIZE) {
                cell->size = (char*)start - HEADERSIZE - (char*)cell;
                if ((char*)end < (char*)cellEnd) {
                    newCell = (Cell*)end;
                    newCell->next = cell->next;
                    if (newCell->next) {
                        newCell->next->prev = newCell;
                    }
                    newCell->prev = cell;
                    cell->next = newCell;
                    newCell->size = (char*)cellEnd - (char*)end;
                }
            } else {
                if ((char*)cellEnd < (char*)end + MINOBJSIZE) {
                    hd->free = DLExtract(hd->free, cell);
                } else {
                    newCell = (Cell*)end;
                    newCell->size = (char*)cellEnd - (char*)end;
                    newCell->next = cell->next;
                    if (newCell->next) {
                        newCell->next->prev = newCell;
                    }
                    newCell->prev = cell->prev;
                    if (newCell->prev) {
                        newCell->prev->next = newCell;
                    } else {
                        hd->free = newCell;
                    }
                }
            }
        }
    }

    *rstart = start;
    *rend = end;
    return start;
}

void OSFreeToHeap(OSHeapHandle heap, void* ptr) {
    HeapDesc* hd;
    Cell* cell;

    cell = (Cell*)((char*)ptr - HEADERSIZE);
    hd = &HeapArray[heap];
    hd->allocated = DLExtract(hd->allocated, cell);
    hd->free = DLInsert(hd->free, cell);
}

OSHeapHandle OSSetCurrentHeap(OSHeapHandle heap) {
    OSHeapHandle prev;

    prev = __OSCurrHeap;
    __OSCurrHeap = heap;
    return prev;
}

void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps) {
    u32 arraySize;
    OSHeapHandle i;

    arraySize = sizeof(HeapDesc) * maxHeaps;
    HeapArray = (HeapDesc*)arenaStart;
    NumHeaps = maxHeaps;

    for (i = 0; i < NumHeaps; i++) {
        HeapDesc* hd = &HeapArray[i];

        hd->size = -1;
        hd->free = hd->allocated = NULL;
    }

    __OSCurrHeap = -1;

    arenaStart = (void*)((char*)HeapArray + arraySize);
    arenaStart = (void*)ROUND(arenaStart, ALIGNMENT);

    ArenaStart = arenaStart;
    ArenaEnd = (void*)TRUNC(arenaEnd, ALIGNMENT);

    return arenaStart;
}

OSHeapHandle OSCreateHeap(void* start, void* end) {
    OSHeapHandle heap;
    HeapDesc* hd;
    Cell* cell;

    start = (void*)ROUND(start, ALIGNMENT);
    end = (void*)TRUNC(end, ALIGNMENT);

    for (heap = 0; heap < NumHeaps; heap++) {
        hd = &HeapArray[heap];
        if (hd->size < 0) {
            hd->size = (char*)end - (char*)start;

            cell = (Cell*)start;
            cell->prev = NULL;
            cell->next = NULL;
            cell->size = hd->size;

            hd->free = cell;
            hd->allocated = 0;
            return heap;
        }
    }
    return -1;
}

void OSDestroyHeap(OSHeapHandle heap) {
    HeapDesc* hd;

    hd = &HeapArray[heap];
    hd->size = -1;
}

void OSAddToHeap(OSHeapHandle heap, void* start, void* end) {
    HeapDesc* hd;
    Cell* cell;

    hd = &HeapArray[heap];

    start = (void*)ROUND(start, ALIGNMENT);
    end = (void*)TRUNC(end, ALIGNMENT);

    cell = (Cell*)start;
    cell->size = (char*)end - (char*)start;
    hd->size += cell->size;
    hd->free = DLInsert(hd->free, cell);
}

#define CHECK(line, condition)                                                                     \
    if (!(condition)) {                                                                            \
        OSReport("OSCheckHeap: Failed " #condition " in %d", line);                                \
        return -1;                                                                                 \
    }

long OSCheckHeap(OSHeapHandle heap) {
    HeapDesc* hd;
    Cell* cell;
    long total = 0;
    long free = 0;

    CHECK(__LINE__, HeapArray);
    CHECK(__LINE__, 0 <= heap && heap < NumHeaps);

    hd = &HeapArray[heap];
    CHECK(__LINE__, 0 <= hd->size);

    CHECK(__LINE__, hd->allocated == NULL || hd->allocated->prev == NULL);
    for (cell = hd->allocated; cell; cell = cell->next) {
        CHECK(__LINE__, InRange(cell, ArenaStart, ArenaEnd));
        CHECK(__LINE__, OFFSET(cell, ALIGNMENT) == 0);
        CHECK(__LINE__, cell->next == NULL || cell->next->prev == cell);
        CHECK(__LINE__, MINOBJSIZE <= cell->size);
        CHECK(__LINE__, OFFSET(cell->size, ALIGNMENT) == 0);

        total += cell->size;
        CHECK(__LINE__, 0 < total && total <= hd->size);
    }

    CHECK(__LINE__, hd->free == NULL || hd->free->prev == NULL);
    for (cell = hd->free; cell; cell = cell->next) {
        CHECK(__LINE__, InRange(cell, ArenaStart, ArenaEnd));
        CHECK(__LINE__, OFFSET(cell, ALIGNMENT) == 0);
        CHECK(__LINE__, cell->next == NULL || cell->next->prev == cell);
        CHECK(__LINE__, MINOBJSIZE <= cell->size);
        CHECK(__LINE__, OFFSET(cell->size, ALIGNMENT) == 0);
        CHECK(__LINE__, cell->next == NULL || (char*) cell + cell->size < (char*) cell->next);

        total += cell->size;
        free += cell->size - HEADERSIZE;
        CHECK(__LINE__, 0 < total && total <= hd->size);
    }

    CHECK(__LINE__, total == hd->size);

    return free;
}

void OSDumpHeap(OSHeapHandle heap) {
    HeapDesc* hd;
    Cell* cell;

    OSReport("\nOSDumpHeap(%d):\n", heap);

    hd = &HeapArray[heap];
    if (hd->size < 0) {
        OSReport("--------Inactive\n");
        return;
    }

    OSReport("addr\tsize\t\tend\tprev\tnext\n");
    OSReport("--------Allocated\n");
    for (cell = hd->allocated; cell; cell = cell->next) {
        OSReport("%x\t%d\t%x\t%x\t%x\n", cell, cell->size, (char*)cell + cell->size, cell->prev, cell->next);
    }

    OSReport("--------Free\n");
    for (cell = hd->free; cell; cell = cell->next) {
        OSReport("%x\t%d\t%x\t%x\t%x\n", cell, cell->size, (char*)cell + cell->size, cell->prev, cell->next);
    }
}
