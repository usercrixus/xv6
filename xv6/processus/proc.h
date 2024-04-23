#ifndef proc_h
#define proc_h

#include "../memory/mmu.h"
#include "../type/types.h"

/*
Per-CPU state
The cpu structure is used to maintain the per-CPU state of the operating system
kernel The cpu structure is used to store various CPU-specific information and
state that is needed by the kernel to manage and schedule processes efficiently.
*/
struct cpu {
    /*
     Holds the local Advanced Programmable Interrupt Controller (APIC) ID of the
     CPU.

     An Advanced Programmable Interrupt Controller (APIC) is a feature found in
     modern CPUs that provides more advanced and flexible interrupt handling
     capabilities than the traditional 8259 PIC (Programmable Interrupt
     Controller) found in earlier CPUs. The APIC is typically used in
     multi-processor systems to manage interrupts between processors, and can
     also be used to assign priorities to interrupts, allowing for more
     efficient handling of interrupts in the system Each processor in a
     multi-processor system has its own local APIC, which is identified by a
     unique APIC ID. The APIC ID is used to route interrupts to the correct
     processor in the system. When an interrupt is generated, the interrupt
     controller determines which processor the interrupt is intended for based
     on the APIC ID of the destination processor and routes the interrupt
     accordingly.
    */
    uchar apicid;
    /*
    Holds a pointer to the context structure, which is used by the swtch()
    function to enter the scheduler and switch between different processes.
    Refer to the context struct definition for more details.
    */
    struct context* scheduler;
    /*
    Is used for saving the CPU registers when an interrupt occurs.
    Refer to the taskstate struct definition for more details.
    */
    struct taskstate ts;
    /*
    Array of segdesc structures, which define the global descriptor table (GDT)
    used by the x86 architecture to manage memory segmentation. Refer to the
    segdesc struct definition for more details.
    */
    struct segdesc gdt[NSEGS];
    /*
    This field is an unsigned integer that indicates whether the CPU has started
    or not.
    */
    volatile uint started;
    /*
    The ncli field in the kernel represents the depth of nesting of the cli and
    sti instructions (handled by pushcli function and popcli function) When a
    critical section is entered, the cli instruction is executed to disable
    interrupts, and the ncli field is incremented to indicate that the number of
    times cli has been called. When the critical section is exited, the sti
    instruction is executed to re-enable interrupts, and the ncli field is
    decremented to indicate that sti has been called. If ncli is zero,
    interrupts are enabled, and if ncli is non-zero, interrupts are disabled.
    This field is used to ensure that nested critical sections are handled
    correctly,
    */
    int ncli;
    /*
    intena is used to keep track of whether interrupts were enabled or not
    before the outermost call to pushcli(). This is important because the
    popcli() function should only re-enable interrupts if they were enabled
    before the outermost call to pushcli().
    */
    int intena;
    /*
    Pointer to the proc structure representing the process that is currently
    running on the CPU. If the CPU is idle, this field is set to null.
    */
    struct proc* proc;
};

extern struct cpu cpus[NCPU];
extern int ncpu;

/*
The struct is used to save and restore the CPU state, which includes register
values, during a context switch. A context switch is the process of saving the
current state of a thread or process and restoring the state of another thread
or process so that it can continue executing. The saved registers are for kernel
context switches, and not all the segment registers need to be saved because
they are constant across kernel contexts. Additionally, %eax, %ecx, and %edx do
not need to be saved because the x86 convention is that the caller has saved
them. The contexts are stored at the bottom of the stack they describe, and the
stack pointer is the address of the context. When a context switch occurs, the
kernel saves the current thread's context on its own kernel stack and switches
to the stack of the thread that will be scheduled to run next. The saved context
of the thread being switched in is loaded from its kernel stack, and the
register values are reset to its saved values, including the stack pointer.
*/
struct context {
    /*
    (Destination Index): It is used for holding a destination address or index
    during memory operations, string operations, or data transfers.
    */
    uint edi;
    /*
    (Source Index): It is used for holding a source address or index during
    memory operations, string operations, or data transfers.
    */
    uint esi;
    /*
    Base Register): It is a general-purpose register often used as a base
    pointer for memory access, holding addresses or pointers to data.
    */
    uint ebx;
    /*
    (Base Pointer): It is commonly used as a frame pointer to point to the base
    of the current stack frame for function calls and local variable access.
    */
    uint ebp;
    /*
    (Instruction Pointer): It is a special-purpose register in the x86
    architecture that holds the memory address of the next instruction to be
    executed.
    */
    uint eip;
};

/*
Enumeration type that represents the different states a process can be in.
Each state is assigned a unique identifier, which allows for easier
representation and comparison of process states in the code.
*/
enum procstate {
    /*
    In an operating system like xv6, the process table is used to keep track of
    all active processes. Each entry in the process table represents a process
    slot. When a process terminates or is no longer needed, its corresponding
    process slot in the process table becomes available for reuse. Initially,
    when the operating system starts up, all process slots in the process table
    are in the UNUSED state, indicating that they are not currently in use.
    These slots are considered available for new processes to occupy.
    */
    UNUSED,
    /*
    This state represents a newly created process that has been allocated a
    process slot but has not yet been fully initialized or started.
    */
    EMBRYO,
    /*
    Waiting for an event or condition to occur. It is not eligible for execution
    until the event it is waiting for happens. During the EMBRYO state, the
    operating system performs various initialization tasks for the process. This
    may include allocating resources, setting up the process's address space,
    assigning a process ID (PID), initializing data structures, and establishing
    the necessary environment for execution.
    */
    SLEEPING,
    /*
    Ready to be scheduled and executed by the CPU. They are waiting to be
    assigned a CPU for execution. Let's say you have a process that performs
    some I/O operation, such as reading data from a disk. When the process
    initiates the I/O operation, it may enter the SLEEPING state because it is
    waiting for the data to be read from the disk. The process cannot continue
    executing until the I/O operation completes and the data is available.
    */
    RUNNABLE,
    /*
    Indicates that a process is currently being executed by a CPU. There can
    only be one process in the RUNNING state at a time on each CPU. Once the I/O
    operation completes and the data is read from the disk, the process is woken
    up by the operating system and transitioned back to the RUNNABLE state. At
    this point, it becomes eligible for execution again and can continue its
    execution from where it left off.
    */
    RUNNING,
    /*
    When a process has finished execution but has not yet been cleaned up by its
    parent process, it enters the ZOMBIE state. The process remains in this
    state until its parent retrieves its exit status, at which point it is
    removed from the system.
    */
    ZOMBIE
};

/*
Represents the per-process state, containing various fields that store
information about a process in an operating system. Struct proc encapsulates the
essential information and state associated with a process, allowing the
operating system to manage and control its execution, memory, and resources.
*/
struct proc {
    /*
    Holds the size of the process's memory in bytes. It indicates the amount of
    memory allocated to the process.
    */
    uint sz;
    /*
    Pointer to the page directory, which is a data structure used by the
    operating system to manage the process's virtual memory.
    */
    pageDirecoryEntry* pgdir;
    /*
    Points to the bottom of the kernel stack allocated for the process.
    The kernel stack is used to store the execution context of the process while
    it is running in kernel mode. The kernel stack is a separate stack allocated
    for each process while it is running in kernel mode. It is distinct from the
    user stack used by the process when executing in user mode. The kernel stack
    is used to store the execution context of the process while it is in kernel
    mode. Similarly, there would be another pointer, often called ustack, which
    represents the user stack.
    */
    char* kstack;
    /*
    This field represents the current state of the process, such as "RUNNING,"
    "SLEEPING," "ZOMBIE," etc. It is an enumeration type that defines different
    process states.
    */
    enum procstate state;
    /*
    Stores the process ID (PID), a unique identifier assigned to each process by
    the operating system.
    */
    int pid;
    /*
    Points to the parent process of the current process. It maintains the
    process hierarchy in the system.
    */
    struct proc* parent;
    /*
    Holds the trap frame for the current system call.
    Trapframe is a data structure used to store the state of a process or thread
    when it encounters an exception or an interrupt, such as a system call, page
    fault, or hardware interrupt.
    */
    struct trapframe* tf;
    /*
    Pointer to the context structure associated with the process.
    The context stores the CPU state, such as register values, when the process
    is not currently running.
    */
    struct context* context;
    /*
    Used when a process is sleeping. If it is non-zero, it indicates that the
    process is sleeping on a specific channel.
    */
    void* chan;
    /*
    Set to a non-zero value if the process has been killed or terminated.
    */
    int killed;
    /*
    Array of file pointers representing the open files for the process. It
    stores references to the files opened by the process.
    */
    struct file* ofile[NOFILE];
    /*
    Points to the current directory of the process. It keeps track of the
    process's working directory
    */
    struct inode* cwd;
    /*
    Character array that holds the name of the process. It is typically used for
    debugging or identification purposes
    */
    char name[16];
};

/*
Additionnal information :

While there may be some overlapping fields between the context and trapframe
structures (such as the general-purpose registers), they serve different
purposes and are used in different contexts. The context structure is more
focused on managing thread or process switching, while the trapframe structure
is more focused on handling interrupts and exceptions. Having separate
structures allows for more flexibility and clarity in managing the different
aspects of CPU state and interrupt handling within the operating system.
*/

#endif