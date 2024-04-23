#include ".././type/types.h"
/*
Each block of memory, whether free or allocated, starts with this header. When a
block is free, the ptr field is used to link it to other free blocks. The size
field is always used to store the size of the block.
*/

typedef struct memoryHeapNodeHeader {
    /*
    In the context of a memory allocator, this is typically used to form a
    linked list of free blocks. When the block is in the free list, this
    pointer is used to point to the next free block.
    */
    struct memoryHeapNodeHeader* ptr;
    /*
    represents the size of the memory block. This size is typically used by
    the memory allocator to keep track of the size of each allocated or free
    block.

    This size is in Header Units (here 64bit))
    */
    uint size;
} MemoryHeapNodeHeader;

/*
Allocates a block of memory of at least nbytes bytes and returns a pointer to
the allocated memory.
*/
void* malloc(uint);
/*
Free a block of memory previously allocated by malloc (or a similar function)
and return it to the free list for future use.

Parameter: ap, a pointer to the block of memory to be freed.
*/
void free(void* blockToFreed);