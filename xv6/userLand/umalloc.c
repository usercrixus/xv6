/*
Schema of memory allocation :

linked list of memory block, each number on the left correspond to a memory
adress. On the right to the size of the memory block :
|_1Block10_|->|_50Block50_|->|_800Block10_|->|_1500Block80_|

free :
we want to free a block of memory. example, the block |_1000Block10_|
the original list is like :
|_1Block10_|->|_50Block50_|->|_800Block10_|->|_1500Block80_|
we will insert the block between |_800Block_| and |_1500Block_|
|_1Block10_|->|_50Block50_|->|_800Block10_|->|_1000Block10_|->|_1500Block80_|

special case :
imagine now that we have :
|_1Block10_|->|_50Block50_|->|_800Block100_|->|_1500Block80_|
and that we want to insert :
|_900Block600_|

the new list will look like :
|_1Block10_|->|_50Block50_|->|_800Block100_|->|_900Block600_|->|_1500Block80_|

but here |_800Block100_|->|_900Block600_| can be merged as :
|_800Block700_|
so we have :
|_1Block10_|->|_50Block50_|->|_800Block700_|->|_1500Block80_|
Again, |_800Block700_|->|_1500Block80_| can be merged :
|_1Block10_|->|_50Block50_|->|_800Block780_|

This is a graphic representation of the free() algorithm and the basics to
understand the whole file.
*/

#include ".././type/types.h"
#include "../fileSystem/stat.h"
#include "user.h"
#include "../type/param.h"
#include "./umalloc.h"

/*
This static variable base acts as the initial element of the free list.
*/
static MemoryHeapNodeHeader base = {(void*)-1, -1};

void free(void* blockToFreed) {
    MemoryHeapNodeHeader* memoryNodeHeaderToFreed =
        (MemoryHeapNodeHeader*)blockToFreed - 1;

    MemoryHeapNodeHeader* adjacentPoint = &base;

    while (adjacentPoint->ptr && adjacentPoint->ptr < memoryNodeHeaderToFreed) {
        adjacentPoint = adjacentPoint->ptr;
    }

    if (!adjacentPoint->ptr) {
        // end of the list
        adjacentPoint->ptr = memoryNodeHeaderToFreed;
        memoryNodeHeaderToFreed->ptr = (void*)-1;
    } else {
        if (memoryNodeHeaderToFreed ==
            adjacentPoint + adjacentPoint->size + 1) {
            // they are perfectly adjacent on the left, so we merge them
            adjacentPoint->size += memoryNodeHeaderToFreed->size + 1;
        }

        if (memoryNodeHeaderToFreed + memoryNodeHeaderToFreed->size + 1 ==
            adjacentPoint->ptr) {
            // they are perfectly adjacent on the right, again we merge them
            adjacentPoint->size += adjacentPoint->ptr->size + 1;
            if (adjacentPoint->ptr->ptr) {
                adjacentPoint->ptr = adjacentPoint->ptr->ptr;
            } else {
                adjacentPoint->ptr = (void*)-1;
            }
        }
    }
}

/*
extend the heap when the memory allocator needs more space to satisfy an
allocation request.

Parameter: memoryUnit, the number of Header units to allocate.
Returns: A pointer to the updated free list or 0 for failure.
*/
static MemoryHeapNodeHeader* morecore(uint memoryUnit) {
    /*
    at least a certain minimum amount of memory is requested. Here, it's set to
    4096 units. If memoryUnit is less than 4096, it's set to 4096. This is a
    typical optimization to avoid frequent system calls for small amounts of
    memory.
    */
    if (memoryUnit < 4096)
        memoryUnit = 4096;
    char* p = sbrk(memoryUnit * sizeof(MemoryHeapNodeHeader));
    if (p == (char*)-1)
        return 0;  // Error
    /*
    Set the new pointer and the new size of the free memory
    */
    MemoryHeapNodeHeader* memoryNode = (MemoryHeapNodeHeader*)p;
    memoryNode->size = memoryUnit;
    /*
    The new block is then added to the free list using the free function. The
    pointer passed to free is offset by one Header unit because free expects a
    pointer to the memory area just after the Header.
    */
    free((void*)(memoryNode + 1));

    return &base;
}

void* malloc(uint nbytes) {
    /*
    nunits is the number of Header-sized units needed to satisfy the request
    (including space for the Header itself).
    */
    uint nunits = (nbytes + sizeof(MemoryHeapNodeHeader) - 1) /
                      sizeof(MemoryHeapNodeHeader) +
                  1;

    MemoryHeapNodeHeader* matchingBlock = &base;
    while (matchingBlock->ptr && matchingBlock->size < nunits) {
        matchingBlock = matchingBlock->ptr;
    }

    if (matchingBlock->size > nunits) {
        uint shift = matchingBlock->size - nunits;
        matchingBlock->size -= nunits;
        return (void*)(matchingBlock + shift);
    } else {
        morecore(nunits);
        return malloc(nbytes);
    }
}