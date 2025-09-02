/*
The kalloc.c file is responsible for memory allocation in the kernel (Allocates
4096-byte pages)
*/

#include "../type/types.h"
#include "defs.h"
#include "../type/param.h"
#include "memlayout.h"
#include "mmu.h"
#include "../synchronization/spinlock.h"
#include "../userLand/user.h"
#include "../userLand/ulib.h"

/*
First address after kernel loaded from ELF file. This variable is defined in the
linker script.
*/
extern char end[];

/*
struct run represents a free memory page in the kernel's memory allocator.
It is a small structure that contains a single field called next, which is a
pointer to the next free page. The kernel's memory allocator maintains a linked
list of free memory pages by chaining together struct run instances using their
next pointers.
*/
struct run {
    struct run* next;
};

/*
represents the kernel memory allocator and includes fields such as a spin lock
(lock) for synchronization, a flag (use_lock) to determine if the lock should be
used, and a pointer (freelist) to the list of free memory blocks available for
allocation.
*/
struct {
    struct spinlock lock;
    /*
    The use_lock field in the kmem structure is a flag that indicates whether
    the memory allocator (kalloc(), kfree(), etc.) should use a lock to ensure
    mutual exclusion when accessing the kernel memory.
    */
    int use_lock;
    struct run* freelist;
} kmem;

/*
The freerange function takes a start and end virtual address range as input and
iterates through the range, freeing all the memory pages within it. The kfree()
function is responsible for actually freeing the memory
*/
void freerange(void* virtualMemoryStart, void* virtualMemoryEnd) {
    char* memory = (char*)PGROUNDUP((uint)virtualMemoryStart);

    while (memory < (char*)virtualMemoryEnd) {
        kfree(memory);
        memory += PGSIZE;
    }
}
/*
Creates the kernel pages directories, which are a 4KB linked list pages. This
4KB pages will be an array of PDEs (Page Directory Entries), which are 32-bit
structures containing a pointer to another 4KB page representing a page table.

The kernel pages directories are responsible for mapping the virtual addresses
used by the kernel to their corresponding physical addresse. It provides the
necessary level of indirection to efficiently manage and access kernel memory.

The kernel page directory is responsible for mapping the virtual addresses used
by the kernel to their corresponding physical addresses. It allows the kernel to
efficiently manage and access its memory.

The function kinit1 initializes the kernel's memory management system by setting
up the page directories. It takes two parameters, vstart and vend, which specify
the start and end virtual addresses of the kernel memory. These parameters are
used to calculate the size of the kernel memory region.
*/
void kinit1(void* virtualMemoryStart, void* virtualMemoryEnd) {
    initlock(&kmem.lock, "kmem");
    /*
    By setting kmem.use_lock to 0 during initialization, it indicates that the
    subsequent memory management operations performed by the kernel do not
    require locking. Since there is no concurrent execution yet,
    */
    kmem.use_lock = 0;
    freerange(virtualMemoryStart, virtualMemoryEnd);
}

/*
responsible for initializing the kernel memory allocator. It takes two
parameters: vstart and vend, which specify the start and end addresses of the
kernel's virtual memory space.
*/
void kinit2(void* vstart, void* vend) {
    freerange(vstart, vend);
    kmem.use_lock = 1;
}

/*
Free the page of physical memory pointed at by v,
which normally should have been returned by a
call to kalloc().  (The exception is when
initializing the allocator; see kinit above.)

expects a virtual adress to free the memory
*/
void kfree(char* virtualMemory) {
    if ((uint)virtualMemory % PGSIZE || virtualMemory < end ||
        V2P(virtualMemory) >= PHYSTOP)
        panic("kfree");
    /*
    The purpose of filling the memory with junk using memset(v, 1, PGSIZE)
    function is to help catch any dangling references to the memory that was
    just freed. When a memory block is freed, its contents are not immediately
    erased or zeroed out, so it is possible that a pointer to that memory block
    could still be valid and used after the memory has been freed. By filling
    the memory with junk, any attempt to use a dangling reference to the freed
    memory will result in unexpected behavior and likely a crash, making it
    easier to catch and debug the problem. In brief, it's intended to trigger an
    error by filling the freed memory with a non-zero value. This can help catch
    dangling references.
    */
    memset(virtualMemory, 1, PGSIZE);

    if (kmem.use_lock)
        acquire(&kmem.lock);
    /*
    add a memory adress to the linked list struct run
    */
    struct run* newHead = (struct run*)virtualMemory;
    newHead->next = kmem.freelist;
    kmem.freelist = newHead;
    if (kmem.use_lock)
        release(&kmem.lock);
}

/*
Allocate one 4096-byte page of physical memory.
Returns a pointer that the kernel can use.
Returns 0 if the memory cannot be allocated.
*/
char* kalloc(void) {
    struct run* r;

    if (kmem.use_lock)
        acquire(&kmem.lock);

    r = kmem.freelist;
    if (r)
        kmem.freelist = r->next;

    if (kmem.use_lock)
        release(&kmem.lock);

    return (char*)r;
}
