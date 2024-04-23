#include "./type/types.h"
#include "defs.h"
#include "type/param.h"
#include "./memory/memlayout.h"
#include "./memory/mmu.h"
#include "./processus/proc.h"
#include "x86.h"
#include "userLand/user.h"
#include "userLand/ulib.h"
#include "./systemCall/trap.h"
#include "drivers/uart.h"
#include "drivers/lapic.h"
#include "memory/vm.h"

/*
First address after kernel loaded from ELF file. This
This variable is defined in the linker script.

During the boot process, the kernel image is loaded from the ELF file into
memory, and the end variable is used to calculate the size of the kernel image.
By subtracting the address of the first byte of the kernel image from the
address of the end variable, the size of the kernel image can be determined.

In a typical operating system, the kernel code and data reside in a separate
region of physical memory that is protected from user processes. The kernel
needs to be mapped into the virtual address space of each process so that the
kernel code and data can be accessed by the process, but the mapping must be
done in a way that prevents user processes from modifying kernel memory.

In xv6, the kernel is mapped into the virtual address space starting at a fixed
virtual address called KERNBASE. This makes it easy for the kernel to access its
own code and data, and also simplifies the implementation of system calls and
other kernel operations.
*/
extern char end[];

/*
This code defines the boot page table used by the kernel during the boot process
in the entry.S and entryother.S files. The entrypgdir variable is an array of
pageDirectoryEntry entries, which are page directory entries that are used to
map virtual addresses to physical addresses.

The __attribute__((__aligned__(PGSIZE))) attribute ensures that the entrypgdir
array is aligned on a page boundary. This is necessary because page directories
and page tables must start on page boundaries.

The first entry in the entrypgdir array maps virtual addresses [0, 4MB) to
physical addresses [0, 4MB) with the PTE_PS flag set. This enables 4MB pages,
which means that instead of using small 4KB pages, the kernel will use large 4MB
pages for mapping this range of virtual addresses.

The second entry in the entrypgdir array maps virtual addresses [KERNBASE,
KERNBASE+4MB) to physical addresses [0, 4MB) with the PTE_PS flag set. The
KERNBASE >> PDXSHIFT expression is used to calculate the index of this entry in
the entrypgdir array. The KERNBASE constant is the virtual address at which the
kernel is mapped, and PDXSHIFT is the number of bits to shift the virtual
address right to get the index of the corresponding page directory entry. This
mapping is necessary to ensure that the kernel code can execute from its mapped
physical address in memory.

More info :
Page tables for mapping kernel memory use 4-Megabyte pages, but page tables for
user memory use 4-Kilobyte pages. This is because the kernel has a larger
address space than a typical user process, so using larger pages allows for more
efficient address translation. The user page table is set up later in the boot
process, after the kernel is initialized and before the first user process is
run. This is done by the userinit() function in the proc.c file.
*/
__attribute__((__aligned__(PGSIZE)))
pageDirecoryEntry entrypgdir[NPDENTRIES] = {
    [0] =
        {
            .present = 1,     // or 1, depending on whether the page is present
            .writable = 1,    // or 1, to indicate if the page is writable
            .permission = 1,  // or 1, for the permission setting
            .padding = 0b10000,  // look PTE_PS macro
            .physicalAdress = 0  // Set the physical address (20 bits)
        },
    // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
    [KERNBASE >> PDXSHIFT] = {
        .present = 1,        // or 1, depending on whether the page is present
        .writable = 1,       // or 1, to indicate if the page is writable
        .permission = 1,     // or 1, for the permission setting
        .padding = 0b10000,  // look PTE_PS macro
        .physicalAdress = 0  // Set the physical address (20 bits)
    }};
/*
Common CPU setup code.

The mpmain() function is the common CPU setup code that runs on all CPUs,
including the boot CPU and the non-boot CPUs (APs).

When an AP processor is started by the boot CPU, it enters the mpenter()
function, which sets up the page table and initializes the local APIC before
calling mpmain().

mpmain() first prints a message to the console indicating that a new CPU is
starting up. It then initializes the Interrupt Descriptor Table (IDT) by calling
idtinit(). After that, it sets the started flag of the struct cpu associated
with the current CPU using the xchg() instruction. This flag is used to notify
the boot CPU that this AP is ready to execute tasks. Finally, scheduler() is
called to start the execution of processes on the current CPU.

In summary, mpmain() is the main entry point for non-boot CPUs, responsible for
initializing the necessary CPU resources before executing the scheduler to start
running processes.
*/
static void mpmain(void) {
    cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
    idtinit();                     // load idt register
    xchg(&(mycpu()->started), 1);  // tell startothers() we're up
    scheduler();                   // start running processes
}

/*
Other CPUs jump here from entryother.S.

The mpenter() function is called by the non-boot processors after they have
finished setting up their basic CPU initialization.

When an additional processor, known as a non-boot (AP) processor, is added to
the system, it starts by executing the code in the entryother.S assembly file,
which is loaded into memory at a fixed address 0x7000. The entryother.S code
performs some basic initialization, sets up a stack, and then jumps to the
mpenter() function.

The mpenter() function sets up the kernel page table, the segment descriptors,
and initializes the local APIC. After that, it calls the mpmain() function,
which is the entry point for running the scheduler and starting to execute user
processes.

Essentially, the mpenter() function is responsible for preparing a non-boot
processor to start executing kernel code, by setting up various essential kernel
data structures and then jumping to the main kernel function mpmain().
*/
static void mpenter(void) {
    switchkvm();
    seginit();
    lapicinit();
    mpmain();
}

/*
Start the non-boot (AP) processors.

In a multiprocessor system, a non-boot processor is a secondary processor that
is not the first processor (boot processor) to be started up by the system. The
boot processor is responsible for initializing the system, loading the operating
system kernel, and starting the first user-level process. Once the boot
processor has completed its initialization tasks, it wakes up the non-boot
processors to start running parallel with it and perform computations. These
non-boot processors are also referred to as Application Processors (APs).
*/
static void startothers(void) {
    /*
    _binary_entryother_start refers to the start address of the "entryother"
    binary code.
    _binary_entryother_size refers to the size of the "entryother" binary code.

    Refer to entryother.S and Make files
    */
    extern uchar _binary_entryother_start[], _binary_entryother_size[];
    /*
    will be used to store the start address of the "entryother" binary code.
    */
    uchar* code;
    /*
    used to store a pointer to the CPU structure of each processor.
    */
    struct cpu* c;
    /*
    used to store the stack pointer for each processor.
    */
    char* stack;

    /*
    Copying the code of the entryother.S file to a specific memory location
    (0x7000). The linker has placed the image of entryother.S in
    _binary_entryother_start.
    */
    code = P2V(0x7000);
    /*
    copy the contents of _binary_entryother_start to the code memory location.
    _binary_entryother_start is a symbol generated by the build process,
    representing the start address of the entryother.S file's binary image.
    _binary_entryother_size is another symbol representing the size of the
    binary image.
    */
    memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);

    for (c = cpus; c < cpus + ncpu; c++) {
        if (c == mycpu())  // We've started already.
            continue;

        // Tell entryother.S what stack to use, where to enter, and what
        // pgdir to use. We cannot use kpgdir yet, because the AP processor
        // is running in low  memory, so we use entrypgdir for the APs too.

        /*
        Allocates a kernel stack for the secondary processor (AP). The kalloc()
        function is used to allocate a new stack from the kernel's memory pool.
        */
        stack = kalloc();
        /*
        Sets the value at the memory location (code - 4) to the top of the
        allocated stack. Here, code refers to the starting address of the code
        segment (entryother.S), and (code - 4) points to the location just
        before the code segment. The stack pointer is stored at this location,
        and it is set to stack + KSTACKSIZE, indicating the top of the stack.
        */
        *(void**)(code - 4) = stack + KSTACKSIZE;
        /*
        Sets the value at the memory location (code - 8) to the address of the
        mpenter function. This indicates the entry point for the secondary
        processor (AP) after it starts running.

        The mpenter function is not directly called at the address (code - 8).
        Instead, the address (code - 8) is stored in the entry code of the
        secondary processors (APs) to serve as the initial instruction pointer.
        When the APs start executing, they fetch the instruction at the entry
        address, which is (code - 8), and this instruction points to the mpenter
        function.
        */
        *(void (**)(void))(code - 8) = mpenter;
        /*
        Sets the value at the memory location (code - 12) to the physical
        address of the page directory (entrypgdir). The V2P() macro is used to
        convert the virtual address of the page directory to its corresponding
        physical address.
        */
        *(int**)(code - 12) = (void*)V2P(entrypgdir);

        lapicstartap(c->apicid, V2P(code));

        /*
        wait for cpu to finish mpmain(). mpmain is called from mpenter and
        mpenter from entryother.S.
        */
        while (c->started == 0)
            ;
    }
}

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int main(void) {
    /*
    In kernel.ld, kernel is linked at adress 0x80100000 which is
    KERNBASE+0x100000. Linker define virtual adress. P2V is KERNBASE+4MB. So, we
    can say that the kernel is 4MB size minus 0x100000

    The dynamic memory of the kernel can be calculated as 4MB minus the size of
    the kernel code and data, which is (end - KERNBASE). This calculation
    accounts for the actual memory used by the kernel, allowing the remaining
    portion of the 4MB region to be available for dynamic kernel memory
    allocations.

    Reason for leaving a gap between the linker address and the kernel's virtual
    address is to provide space for the Basic Input/Output System (BIOS) and
    other firmware code. By leaving a gap between the linker address and the
    kernel's virtual address, it allows the firmware to occupy its designated
    memory region without overlapping with the kernel. This ensures that the
    firmware code and the kernel do not interfere with each other and allows
    them to coexist peacefully in memory.
    */
    kinit1(end, P2V(4 * 1024 * 1024));
    kvmalloc();
    mpinit();
    lapicinit();
    seginit();
    picinit();
    ioapicinit();
    consoleinit();
    uartinit();  // can be deleted if you dev an azerty switch keyboard
    pinit();     // can be optimized ?
    tvinit();
    binit();
    fileinit();  // can opti ?
    ideinit();
    /*
    start other processors (non-boot processor)
    */
    startothers();
    /*
    Must come after startothers()
    */
    kinit2(P2V(4 * 1024 * 1024), P2V(PHYSTOP));
    userinit();  // first user process
    mpmain();    // finish this processor's setup
}
