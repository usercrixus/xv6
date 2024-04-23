#include "../type/param.h"
#include "../type/types.h"
#include "../defs.h"
#include "../x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "../processus/proc.h"
#include "elf.h"
#include "../userLand/user.h"
#include "../userLand/ulib.h"
#include "vm.h"

extern char data[];  // defined by kernel.ld
pageDirecoryEntry* kpgdir;

/*
Set up the Global Descriptor Table (GDT) for each CPU's kernel and user segment
descriptors.

The GDT is a data structure used by Intel x86-family processors to define the
characteristics of the various memory areas used during program execution. This
includes the base address, the limit, and access privileges like execute-read or
read-write.

In this case, we are setting up segments for both kernel mode (privileged) and
user mode (unprivileged).

in the context of a segment descriptor, 0 and 0xffffffff represent the "base"
and "limit" of the segment respectively.

   Base (0): This is the starting address of the segment in memory. A value of 0
means that the segment starts at the beginning of the address space.

   Limit (0xffffffff): This is the size of the segment, or more precisely, the
offset to the last addressable unit within the segment. 0xffffffff is the
maximum 32-bit value, and thus represents the maximum size of a segment in a
32-bit architecture. This means that the segment spans the entire address space.

In effect, these two values (0 and 0xffffffff) define segments that span the
entire 32-bit address space for both kernel and user mode. This is typical in
operating systems that use paging for memory management and only use
segmentation minimally, if at all. In such systems, each segment usually spans
the entire address space and the memory management is done through paging.

in xv6, and most modern operating systems, the segmentation is set up in what's
called a "flat" memory model. In this model, each segment starts at address 0
and extends to the maximum address, which means all segments overlap and cover
the exact same range of addresses.

The four segments in xv6 (kernel code, kernel data, user code, and user data)
are differentiated not by their base and limit values, but by their access
permissions and privilege levels.

The purpose of these segment descriptors is to allow the processor to switch
between different contexts (such as user mode and kernel mode) and to provide a
mechanism for enforcing access controls.

In a protected mode operating system like xv6, the processor uses the current
segment selector to determine which segment it's currently executing in. The
segment selector is a value that's loaded into a segment register (such as CS
for code, DS for data, etc.).

When the processor needs to switch between user mode and kernel mode, it can do
so by changing the segment selector in the appropriate segment register. For
example, when an interrupt or system call occurs, the processor switches from
the user code segment to the kernel code segment. And when the kernel is
finished handling the interrupt or system call, it switches back to the user
code segment before returning control to the user program.

EG_KCODE is the kernel code segment. It is executable and readable.
SEG_KDATA is the kernel data segment. It is writable.
SEG_UCODE is the user code segment. It is executable and readable, and it can be
accessed from user mode.
SEG_UDATA is the user data segment. It is writable, and it can be accessed from
user mode.
*/
void seginit(void) {
    struct cpu* c;

    c = &cpus[cpuid()];
    /*
    setting the kernel code segment descriptor. It's setting the permissions of
    the segment to STA_X (executable segment) and STA_R (readable segment). The
    base of the segment is 0 and the limit is 0xffffffff (the full 4 GB address
    space).
    */
    c->gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, DPL_KERNEL);
    /*
    setting the kernel data segment descriptor. It's setting the permissions of
    the segment to STA_W (writable segment). The base of the segment is 0 and
    the limit is 0xffffffff.
    */
    c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, DPL_KERNEL);
    /*
    setting the user code segment descriptor. It's the same as the kernel code
    segment, but it sets the descriptor privilege level (DPL) to DPL_USER,
    indicating it's a user-level descriptor.
    */
    c->gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, DPL_USER);
    /*
    setting the user data segment descriptor. It's the same as the kernel data
    segment, but it sets the descriptor privilege level (DPL) to DPL_USER.
    */
    c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
    /*
    Load the new Global Descriptor Table. It takes as argument the address of
    the GDT and its size.
    */
    lgdt(c->gdt, sizeof(c->gdt));
}

/*
This function traverses the page table hierarchy to find the Page Table Entry
(PTE) that corresponds to a given virtual address (va).

Parameters:
- pgdir: Page directory, an array of Page Directory Entries (PDEs)
- va: Virtual address to look up
- alloc: Flag indicating whether new page table pages should be allocated if
necessary

Returns:
- Pointer to the PTE within the Page Table if found, or 0 if not found or unable
to allocate

Additional information:

In the x86 architecture, the page tables are organized in a hierarchical
structure. The top-level data structure is the Page Directory, which contains
entries pointing to the Page Tables. Each Page Table, in turn, contains entries
pointing to the actual physical memory pages.

imagine the memory :
0b11111111 11111111 11111111 11111111
page directory entry mask are the first 10 bit
0b1111111111 00000000 00000000 000000
page table entry mask include the next 10 bit
0b1111111111 1111111111 00000000 0000

if my va is 0b11100111 11011011 11111001 11101111
the page dir entry offset is :
0b11100111 11
then, the page dir will give me a page directory adress example :
0b11100011 11001011 10111011 11101011
this page directory adress contain a page table adress on 20 bit example :
0b11000111 10011001 10110000 00000000
this adress have to be shift by the va 20 first bit to obtain a page
then, we can link our page with a physical adress in the mappage function
example :
link physical adres 0xf54f 12f4a ac12 dc1c
with virtual adress :
0b11100111 11111001 11110000 00000000

pgdir-> PAGEDIR1-->PTE1 -> PhysMem1
                        -> PhysMem2
                        -> PhysMem3
                -->PTE2 -> PhysMem1
                        -> PhysMem2
                        -> PhysMem3
                -->PTE3 -> PhysMem1
                        -> PhysMem2
                        -> PhysMem3
     -> PAGEDIR2-->PTE1 -> PhysMem1
                        -> PhysMem2
                        -> PhysMem3
                -->PTE2 -> PhysMem1
                        -> PhysMem2
                        -> PhysMem3
                -->PTE3 -> PhysMem1
                        -> PhysMem2
                        -> PhysMem3
*/
static pageTableEntry* walkpgdir(pageDirecoryEntry* pgdir,
                                 const void* va,
                                 int alloc) {
    pageDirecoryEntry* pde;
    /*
    A Page Table is a table-like data structure that maps virtual addresses to
    physical addresses. Here 20bit
    */
    pageTableEntry* pgtab;
    /*
    Retrieves the Page Directory Entry (PDE) for the virtual address va by
    indexing into the pgdir array using the Page Directory Index (PDX) macro

    In xv6, a page directory in a 4KB page consists of 1024 page directory
    entries (PDEs). If pgdir is 4KB in size, it means it can hold 4KB /
    sizeof(pageDirectoryEntry) elements. Assuming the pageDirectoryEntry type is
    4 bytes (32 bits), the calculation would be as follows: Number of entries =
    4KB / 4 bytes = 1024 entries Therefore, in this case, the pgdir array can
    hold 1024 entries, with valid indices ranging from 0 to 1023. 0b11_1111_1111
    is the maximum value PDX(va) and 0b11_1111_1111 is equal to 1023. (So it's
    all right)
    */
    pde = &pgdir[PDX(va)];
    /*
    checks if the PDE is present
    */
    if (pde->present == 1) {
        /*
        If the PDE is present, it means that the corresponding Page Table is
        already allocated. In this case, it retrieves the address of the Page
        Table by masking the PTE_ADDR bits of the PDE and converting it to a
        virtual address using the P2V macro
        */
        pgtab = (pageTableEntry*)P2V((pde->physicalAdress << 12));
    } else {
        /*
        If the PDE is not present, it means that the corresponding Page Table is
        not allocated. In this case, it checks if alloc is set to 1 and if a new
        page table can be allocated using the kalloc() function. If either
        condition fails, it returns 0 to indicate an error or inability to
        allocate the page table.
        */
        if (!alloc || (pgtab = (pageTableEntry*)kalloc()) == 0)
            return 0;
        /*
        clears all the PTEs in the allocated page
        */
        memset(pgtab, 0, PGSIZE);
        /*
        It sets the PTE_P, PTE_W, and PTE_U flags to indicate that the page
        table is present, writable, and accessible in user mode
        */
        pde->permission = 1;
        pde->writable = 1;
        pde->present = 1;
        pde->physicalAdress = (V2P(pgtab) >> 12) & 0xfffff;
    }
    /*
    returns the address of the PTE within the Page Table that corresponds to the
    virtual address va
    Do not forget, pagetab type is pageTableEntry who is 32bit so 4byte. PTX
    shit on 10bit not on 20 bit as the first 10 bit are fixed because part of
    the tableDirectoryEntry. Si it is 4*1024 si 4KB == pageSize. A page table is
    1024 page of 4KB...
    */
    return &pgtab[PTX(va)];
}

/*
responsible for creating Page Table Entries (PTEs) in the page directory (pgdir)
for a range of virtual addresses starting at va and corresponding to physical
addresses starting at pa. The size parameter represents the size of the range,
which may not be page-aligned.
*/
static int mappages(pageDirecoryEntry* pgdir,
                    void* va,
                    uint size,
                    uint pa,
                    int perm) {
    pageTableEntry* pte;

    char* a = (char*)PGROUNDDOWN((uint)va);
    char* last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
    while (1) {
        if ((pte = walkpgdir(pgdir, a, 1)) == 0)
            return -1;
        /*
        checks if it's already marked as present (PTE_P flag). If it is, it
        indicates a remapping, which is considered an error, and the function
        panics.
        */
        if (pte->present)
            panic("remap");
        /*
        If the PTE doesn't exist or is not marked as present, the function sets
        the PTE with the physical address (pa), permission bits (perm), and the
        PTE_P flag to mark it as present.
        */
        //*pte = pa | perm | PTE_P;
        pte->physicalAdress = pa >> 12;
        pte->permission = (perm & 0b0100) > 2;
        pte->writable = (perm & 0b010) > 1;
        pte->present = 1;

        if (a == last)
            break;
        a += PGSIZE;
        pa += PGSIZE;
    }

    return 0;
}

/*
Represents the kernel's mappings, which are present in every process's page
table

Each element of the kmap array corresponds to a specific range of memory that
needs to be mapped in the kernel's page table.
*/
static struct kmap {
    /*
    represents the virtual address where the mapping starts. It is a pointer to
    the starting virtual address of the kernel mapping.
    */
    void* virt;
    /*
    represents the starting physical address of the corresponding memory region.
    It specifies the beginning address of the memory range that is being mapped.
    */
    uint phys_start;
    /*
    represents the ending physical address of the corresponding memory region.
    It specifies the last address of the memory range that is being mapped.
    */
    uint phys_end;
    /*
    represents the permissions or attributes associated with the mapping. It is
    an integer value that holds flags indicating the permissions of the mapped
    memory region, such as read-only (PTE_R), writeable (PTE_W), executable
    (PTE_X), etc.
    */
    int perm;
} kmap[] = {
    {(void*)KERNBASE, 0, EXTMEM, PTE_W},             // I/O space
    {(void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},  // kern text+rodata
    {(void*)data, V2P(data), PHYSTOP, PTE_W},        // kern data+memory
    {(void*)DEVSPACE, DEVSPACE, 0, PTE_W},           // more devices
};

/*
Responsible for setting up the kernel part of a page table.
*/
pageDirecoryEntry* setupkvm(void) {
    /*
    pointer pgdir of type pageDirectoryEntry, which represents a page directory
    entry. It will be used to store the starting address of the allocated page
    directory.
    */
    pageDirecoryEntry* pageDirectory;
    /*
    declares a pointer k of type struct kmap. The struct kmap is a data
    structure that contains information about the virtual and physical memory
    mappings for the kernel.
    */
    struct kmap* k;

    /*
    Recover a page, array of pde
    */
    if ((pageDirectory = (pageDirecoryEntry*)kalloc()) == 0)
        return 0;  // fail
    /*
    memset(pgdir, 0, PGSIZE);: This line initializes the allocated page
    directory by setting all its bytes to zero. This ensures that any
    uninitialized entries are properly initialized to zero.
    */
    memset(pageDirectory, 0, PGSIZE);

    /*
    there is a gap of unused memory between PHYSTOP and DEVSPACE in the xv6
    kernel. The reason for this gap is to provide a buffer zone or reserved
    space between the end of the general system memory and the start of the
    memory range allocated for devices.

    By having a gap between PHYSTOP and DEVSPACE, the kernel ensures that there
    is a clear distinction between the memory regions used for regular system
    operations and those reserved for devices. This separation helps prevent any
    unintended overlap or conflicts between device memory accesses and regular
    system memory accesses.

    Overall, the gap between PHYSTOP and DEVSPACE is a deliberate design choice
    to ensure proper isolation and separation of memory regions within the xv6
    kernel.
    */
    if (P2V(PHYSTOP) > (void*)DEVSPACE)
        panic("PHYSTOP too high");

    for (k = kmap; k < &kmap[NELEM(kmap)]; k++)
        /*
        create page table entries and map virtual addresses to physical
        addresses.
        */
        if (mappages(pageDirectory, k->virt, k->phys_end - k->phys_start,
                     k->phys_start, k->perm) < 0) {
            freevm(pageDirectory);
            return 0;  // fail
        }
    /*
    if all the virtual-to-physical mappings are successfully created, the
    function returns the address of the allocated page directory, indicating
    success.
    */
    return pageDirectory;
}

/*
Responsible for allocating memory for the kernel page table.
This function allocates memory for the kernel page table and sets up the
necessary page table entries to map the kernel's virtual address space.

Allows the kernel and scheduler processes to operate within their own address
space, separate from user processes, and enables efficient memory management and
access to kernel-specific resources.
*/
void kvmalloc(void) {
    /*
    allocates a page table for the kernel's address space by setting up a new
    page directory
    */
    kpgdir = setupkvm();
    /*
    switches the current address space to use the newly created page directory
    */
    switchkvm();
}

/*
Responsible for switching the hardware's page table register (CR3 register on
x86) to the kernel-only page table. It is typically called when no user process
is running and the system is operating in kernel mode.

By loading the physical address of the kernel page directory into the CR3
register, the hardware's memory management unit (MMU) is configured to use the
kernel-only page table, effectively switching the address translation mechanism
to operate in the kernel's virtual address space.
*/
void switchkvm(void) {
    lcr3(V2P(kpgdir));  // switch to the kernel page table
}

/*
The primary role of switchuvm (switch user virtual memory) is to prepare the
system's hardware to run a different process (p) in user mode. This involves
setting up the correct memory mapping and task state segment (TSS) for the
process.
*/
void switchuvm(struct proc* p) {
    if (p == 0)
        panic("switchuvm: no process");
    if (p->kstack == 0)
        panic("switchuvm: no kstack");
    if (p->pgdir == 0)
        panic("switchuvm: no pgdir");

    pushcli();
    mycpu()->gdt[SEG_TSS] =
        SEG16(STS_T32A, &mycpu()->ts, sizeof(mycpu()->ts) - 1, 0);
    mycpu()->gdt[SEG_TSS].s = 0;
    mycpu()->ts.ss0 = SEG_KDATA << 3;
    mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
    mycpu()->ts.iomb = (ushort)0xFFFF;
    ltr(SEG_TSS << 3);
    lcr3(V2P(p->pgdir));
    popcli();
}

/*
responsible for loading the initial user program (initcode) into the virtual
memory address space of a process

pgdir: It is a pointer to the page directory of the process. The page directory
is a data structure that maps the virtual addresses used by the process to the
corresponding physical addresses in memory. It is used to manage the process's
virtual memory.

init: It is a pointer to the start of the initcode program in memory. The
initcode program is the initial user program that will be loaded into the
process's virtual memory and executed when the process starts.

sz: It is the size of the initcode program. It represents the number of bytes
occupied by the initcode program in memory.
*/
void inituvm(pageDirecoryEntry* pgdir, char* init, uint sz) {
    /*
    checks if the size of the initcode program is larger than or equal to a page
    (PGSIZE). If it is, it panics with an error message indicating that the
    program is larger than a page, which is not supported.
    */
    if (sz >= PGSIZE)
        panic("inituvm: more than a page");

    char* mem = kalloc();
    /*
    clears the allocated page of memory by setting all its bytes to 0 using
    memset().
    */
    memset(mem, 0, PGSIZE);
    /*
    maps the allocated physical memory page to the virtual memory address 0 in
    the process's address space. It does this by calling the mappages()
    function, which maps a range of virtual addresses to a range of physical
    addresses in the page directory. In this case, it maps the virtual addresses
    from 0 to PGSIZE to the physical address obtained from V2P(mem) (which
    converts the virtual address to a physical address) with read-write (PTE_W)
    and user-accessible (PTE_U) permissions.
    */
    mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W | PTE_U);
    memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int loaduvm(pageDirecoryEntry* pgdir,
            char* addr,
            struct inode* ip,
            uint offset,
            uint sz) {
    uint i, pa, n;
    pageTableEntry* pte;

    if ((uint)addr % PGSIZE != 0)
        panic("loaduvm: addr must be page aligned");
    for (i = 0; i < sz; i += PGSIZE) {
        if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0)
            panic("loaduvm: address should exist");
        pa = pte->physicalAdress << 12;
        if (sz - i < PGSIZE)
            n = sz - i;
        else
            n = PGSIZE;
        if (readi(ip, P2V(pa), offset + i, n) != n)
            return -1;
    }
    return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm(pageDirecoryEntry* pgdir, uint oldsz, uint newsz) {
    char* mem;
    uint a;

    if (newsz >= KERNBASE)
        return 0;
    if (newsz < oldsz)
        return oldsz;

    a = PGROUNDUP(oldsz);
    for (; a < newsz; a += PGSIZE) {
        mem = kalloc();
        if (mem == 0) {
            cprintf("allocuvm out of memory\n");
            deallocuvm(pgdir, newsz, oldsz);
            return 0;
        }
        memset(mem, 0, PGSIZE);
        if (mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W | PTE_U) < 0) {
            cprintf("allocuvm out of memory (2)\n");
            deallocuvm(pgdir, newsz, oldsz);
            kfree(mem);
            return 0;
        }
    }
    return newsz;
}

/*
Deallocate user virtual memory pages in the specified memory range, starting
from oldsz and ending at newsz. It walks through the page directory and page
tables, frees the corresponding physical memory pages, and updates the page
table entries to reflect the deallocation.

    pgdir: It is a pointer to the page directory (pageDirectoryEntry*). The page
directory is the top-level structure that holds page table entries (PTEs) and
maps virtual addresses to physical addresses.

    oldsz: It is an unsigned integer (uint) representing the old size of the
memory that needs to be deallocated. This value specifies the upper limit of the
memory range to be considered for deallocation.

    newsz: It is also an unsigned integer (uint) representing the new size of
the memory. This value specifies the lower limit of the memory range that should
not be deallocated.

*/
int deallocuvm(pageDirecoryEntry* pgdir, uint oldsz, uint newsz) {
    pageTableEntry* pte;  // page table entry
    uint pa;              // physical adress

    /*
    means there is no deallocation required, so it returns the oldsz value.
    */
    if (newsz >= oldsz)
        return oldsz;
    /*
    reminder :
    0b00000000000000000001000000000000 = pgsize
    PDX(a) + 0b00000000010000000000000000000000 = PDX(a) + 1
    */
    for (uint a = PGROUNDUP(newsz); a < oldsz; a += PGSIZE) {
        pte = walkpgdir(pgdir, (char*)a, 0);
        if (!pte)
            /*
            aligns a to the start of the next page directory entry. This
            ensures that when you loop back and increment a by PGSIZE again,
            you are moving to the next page within the next page directory
            entry.

            This optimization is possible because the page table entries within
            a Page Directory Entry are contiguous, so by skipping to the next
            PDE, it avoids unnecessary iterations and checks within the current
            PDE.
            */
            a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
        /*
        checks if the page table entry (pte) is present
        */
        else if (pte->present != 0) {
            /*
            extracts the physical address from the page table entry
            */
            pa = pte->physicalAdress << 12;

            if (pa == 0)
                panic("kfree");
            /*
            Converts the physical address pa to its corresponding virtual
            address v using the P2V macro. This step is necessary because the
            kfree function expects a virtual address to free the memory.
            */
            char* v = P2V(pa);
            kfree(v);
            /*
            Clears the page table entry to indicate that it is no longer present
            in physical memory.
            */
            pte->physicalAdress = 0;
            pte->present = 0;
            pte->permission = 0;
            pte->writable = 0;
        }
    }
    return newsz;
}

/*
is responsible for freeing a page table and all the physical memory pages
associated with it in the user part of the address space.
*/
void freevm(pageDirecoryEntry* pgdir) {
    uint i;
    /*
    checks if the provided page directory (pgdir) is valid (not null). If the
    page directory is null, it raises a panic, indicating an error.
    */
    if (pgdir == 0)
        panic("freevm: no pgdir");

    deallocuvm(pgdir, KERNBASE, 0);
    /*
    deallocuvm(pgdir, KERNBASE, 0) deallocates the user memory, while the loop
    deallocates the page tables used for mapping that memory.
    NPDENTRIES=1024 ; 1024*32 = 32768 bit = 32768/8 octet = 4096 octet = 4KB
    (page size)
    */
    for (i = 0; i < NPDENTRIES; i++) {
        /*
        Indicating that the corresponding page table exists and is mapped in the
        address space.
        */
        if ((pgdir + i)->present) {
            /*
            retrieves the virtual address corresponding to the page table entry
            */
            char* v = P2V((pgdir + i)->physicalAdress << 12);
            kfree(v);
        }
    }
    kfree((char*)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void clearpteu(pageDirecoryEntry* pgdir, char* uva) {
    pageTableEntry* pte;

    pte = walkpgdir(pgdir, uva, 0);
    if (pte == 0)
        panic("clearpteu");
    pte->permission = 0;
}

// Given a parent process's page table, create a copy
// of it for a child.
pageDirecoryEntry* copyuvm(pageDirecoryEntry* pgdir, uint sz) {
    pageDirecoryEntry* d;
    pageTableEntry* pte;
    uint pa, i, flags;
    char* mem;

    if ((d = setupkvm()) == 0)
        return 0;
    for (i = 0; i < sz; i += PGSIZE) {
        if ((pte = walkpgdir(pgdir, (void*)i, 0)) == 0)
            panic("copyuvm: pte should exist");
        if (!pte->present)
            panic("copyuvm: page not present");
        pa = pte->physicalAdress << 12;
        flags = pte->present | (pte->writable << 1) | (pte->permission << 2);
        if ((mem = kalloc()) == 0)
            goto bad;
        memmove(mem, (char*)P2V(pa), PGSIZE);
        if (mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
            kfree(mem);
            goto bad;
        }
    }
    return d;

bad:
    freevm(d);
    return 0;
}

//  Map user virtual address to kernel address.
char* uva2ka(pageDirecoryEntry* pgdir, char* uva) {
    pageTableEntry* pte;

    pte = walkpgdir(pgdir, uva, 0);
    if (pte->present == 0)
        return 0;
    if (pte->permission == 0)
        return 0;
    return (char*)P2V(pte->physicalAdress << 12);
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int copyout(pageDirecoryEntry* pgdir, uint va, void* p, uint len) {
    char *buf, *pa0;
    uint n, va0;

    buf = (char*)p;
    while (len > 0) {
        va0 = (uint)PGROUNDDOWN(va);
        pa0 = uva2ka(pgdir, (char*)va0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (va - va0);
        if (n > len)
            n = len;
        memmove(pa0 + (va - va0), buf, n);
        len -= n;
        buf += n;
        va = va0 + PGSIZE;
    }
    return 0;
}
