#ifndef x86
#define x86

#include "type/types.h"

/*
Contains a series of inline assembly functions and a structure definition
specific to the x86 architecture. The inline assembly functions are used to
allow C code to use special x86 instructions that are not easily accessible
through regular C code.
*/

/*
Reads a byte from the specified I/O port and returns it.
*/
static inline uchar inb(ushort port) {
    uchar data;

    asm volatile("in %1,%0" : "=a"(data) : "d"(port));
    return data;
}

/*
Reads a sequence of doublewords from the specified I/O port and stores them at
the specified address.
*/
static inline void insl(int port, void* addr, int cnt) {
    asm volatile("cld; rep insl"
                 : "=D"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "memory", "cc");
}

/*
Writes a byte to the specified I/O port.
*/
static inline void outb(ushort port, uchar data) {
    asm volatile("out %0,%1" : : "a"(data), "d"(port));
}

/*
Writes a word to the specified I/O port.
*/
static inline void outw(ushort port, ushort data) {
    asm volatile("out %0,%1" : : "a"(data), "d"(port));
}

/*
Writes a sequence of doublewords to the specified I/O port from the specified
address.
*/
static inline void outsl(int port, const void* addr, int cnt) {
    asm volatile("cld; rep outsl"
                 : "=S"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "cc");
}

/*
Stores a byte value repeatedly in memory.

The stosb instruction is used to fill a block of memory with a specified value.
The instruction takes three operands: the destination memory address (addr), the
value to be stored (data), and the number of bytes to be stored (cnt).

The cld instruction clears the direction flag, which ensures that the stosb
instruction writes to memory in ascending order. The rep prefix before the stosb
instruction repeats the stosb instruction cnt times. The stosb instruction
increments the addr pointer by 1 byte (8 bits) each time it stores the specified
data.

tips :
In inline assembly, the capital letter refers to the 32-bit register and the
lowercase letter refers to the 16-bit register D refers to the 32-bit
general-purpose register EDX, while d refers to its lower 16-bit portion, DX. C
refers to the 32-bit register ECX and c refers to the 16-bit register CX. A
refers to the 32-bit register EAX, while a refers to the 16-bit register AX.

By including "memory" as a clobber, we tell the compiler that the operation
modifies memory, so it must take that into account when generating the code.

The "cc" clobber is used to inform the compiler that the operation modifies the
condition code register (eflags). The rep instruction is a prefix for repeating
the following instruction ecx times, and it modifies the condition codes (CF,
PF, AF, ZF, SF, and OF) depending on the result of the previous iteration. By
including "cc" as a clobber, we tell the compiler that the operation modifies
the condition codes, so it must take that into account when generating the code.

stosb(0, 1, 5) mean :
Address:  | 0x00 | 0x01 | 0x02 | 0x03 | 0x04 |
Value:    | 0x01 | 0x01 | 0x01 | 0x01 | 0x01 |

Memory is byte-addressable, meaning that each byte of memory has its own unique
address. So the byte at address 0x0 is the first byte, and the byte at address
0x1 is the second byte, and so on.
*/
static inline void stosb(void* addr, int data, int cnt) {
    asm volatile("cld; rep stosb"
                 : "=D"(addr), "=c"(cnt)
                 : "0"(addr), "1"(cnt), "a"(data)
                 : "memory", "cc");
}

/*
Stores a doubleword value repeatedly in memory.
*/
static inline void stosl(void* addr, int data, int cnt) {
    asm volatile("cld; rep stosl"
                 : "=D"(addr), "=c"(cnt)
                 : "0"(addr), "1"(cnt), "a"(data)
                 : "memory", "cc");
}

struct segdesc;

/*
Loads the Global Descriptor Table (GDT) register with the specified GDT table.
*/
static inline void lgdt(struct segdesc* p, int size) {
    volatile ushort pd[3];

    pd[0] = size - 1;
    pd[1] = (uint)p;
    pd[2] = (uint)p >> 16;

    asm volatile("lgdt (%0)" : : "r"(pd));
}

struct gatedesc;

/*
Loads the Interrupt Descriptor Table (IDT) register with the specified IDT
table.
*/
static inline void lidt(struct gatedesc* p, int size) {
    volatile ushort pd[3];

    pd[0] = size - 1;
    pd[1] = (uint)p;
    pd[2] = (uint)p >> 16;

    asm volatile("lidt (%0)" : : "r"(pd));
}

/*
Loads the Task Register (TR) with the specified segment selector.
*/
static inline void ltr(ushort sel) {
    asm volatile("ltr %0" : : "r"(sel));
}

/*
Reads the current value of the EFLAGS register.
*/
static inline uint readeflags(void) {
    uint eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

/*
Loads the GS segment register with the specified value.
*/
static inline void loadgs(ushort v) {
    asm volatile("movw %0, %%gs" : : "r"(v));
}

/*
Disables interrupts.
*/
static inline void cli(void) {
    asm volatile("cli");
}

/*
Enables interrupts.
*/
static inline void sti(void) {
    asm volatile("sti");
}

/*
Atomically exchanges the value at the specified memory address with the
specified value.
*/
static inline uint xchg(volatile uint* addr, uint newval) {
    uint result;

    // The + in "+m" denotes a read-modify-write operand.
    asm volatile("lock; xchgl %0, %1"
                 : "+m"(*addr), "=a"(result)
                 : "1"(newval)
                 : "cc");
    return result;
}

/*
Reads the value of the CR2 control register, which contains the address of the
last page fault.
*/
static inline uint rcr2(void) {
    uint val;
    asm volatile("movl %%cr2,%0" : "=r"(val));
    return val;
}

/*
Loads the CR3 control register with the specified value.
*/
static inline void lcr3(uint val) {
    asm volatile("movl %0,%%cr3" : : "r"(val));
}

/*
This structure represents the part of the trap frame that is usually saved
automatically by the x86 hardware when a trap occurs

When a trap occurs, the CPU automatically pushes eip, cs, eflags, and, if
there's a change in privilege level (like from user mode to kernel mode), esp
and ss onto the stack.
*/
struct TrapframeHardware {
    /*
    EIP stands for "Extended Instruction Pointer" and is a register used in x86
    architecture.

    The Instruction Pointer points to the memory address of the next instruction
    to be executed by the CPU. When the processor fetches an instruction from
    memory, it increments the Instruction Pointer to point to the next
    instruction. This allows the CPU to sequentially execute instructions in the
    correct order.
    */
    uint eip;
    /*
    Code segment, which specifies the memory segment containing the code being
    executed.
    */
    ushort cs;
    ushort padding5;
    /*
    Value of the EFLAGS register at the time of a trap or interrupt.

    The EFLAGS register consists of multiple flags, each representing a specific
    condition or mode of operation. Some of the commonly used flags stored in
    the EFLAGS register include: Carry Flag (CF): Indicates if the last
    arithmetic operation resulted in a carry or borrow. Zero Flag (ZF):
    Indicates if the result of the last arithmetic or logical operation was
    zero. Sign Flag (SF): Indicates the sign (positive or negative) of the
    result of the last arithmetic operation. Overflow Flag (OF): Indicates if
    the last arithmetic operation resulted in an overflow. Interrupt Enable Flag
    (IF): Controls whether interrupts are enabled or disabled. Direction Flag
    (DF): Determines the direction of string instructions (e.g., rep movsb) for
    copying data.
    */
    uint eflags;

    // below herex86 only when crossing rings, such as from user to kernel
    /*
    used when transitioning between different privilege levels (e.g., from user
    mode to kernel mode). It stores the stack pointer value for the new stack
    that will be used after the privilege level change.

    Let's say we have a user process executing in user mode, and an interrupt
    occurs that requires a transition to kernel mode. The esp field in the trap
    frame will hold the stack pointer value for the new stack that will be used
    after the privilege level change.

    For instance, when an interrupt occurs, the processor saves the current
    state (registers, flags, etc.) onto the user stack. Then, it switches to the
    kernel stack and sets the esp field in the trap frame to the stack pointer
    value of the kernel stack. This allows the kernel to resume execution at the
    appropriate point after handling the interrupt.
    */
    uint esp;
    /*
    Trapframe structure, ushort ss stores the value of the stack segment
    register at the time of a trap or interrupt. The SS register represents the
    base address of the stack segment, indicating where the stack starts in
    memory. The ESP register, on the other hand, points to the current top of
    the stack. As soon as values are pushed onto the stack, the ESP decreases
    and points to the new top of the stack. So, under normal circumstances, the
    ESP will be lower than the SS register.
    */
    ushort ss;
    ushort padding6;
};

/*
This structure represents the part of the trap frame that is manually saved by
the operating system's trap handler.
*/
struct TrapframeSystem {
    /*
    Registers as pushed by pusha
    General-purpose registers of the x86 architecture. Here's a brief
    explanation of each register:
    */

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
    (Base Pointer): It is commonly used as a frame pointer to point to the base
    of the current stack frame for function calls and local variable access.
    */
    uint ebp;
    /*
    (Old Stack Pointer): useless & ignored
    */
    uint oesp;
    /*
    (Base Register): It is a general-purpose register often used as a base
    pointer for memory access, holding addresses or pointers to data.
    */
    uint ebx;
    /*
    (Data Register): It is a general-purpose register used for holding data
    values or serving as a counter in loops or arithmetic operations.
    */
    uint edx;
    /*
    Counter Register): It is a general-purpose register used for holding loop
    counters, shift counts, or other temporary values.
    */
    uint ecx;
    /*
    The number that identifies the system call is loaded into the eax register.
    Each system call has a unique number which the kernel uses to identify what
    action to perform.

    Upon completion, the system call typically returns a value in the eax
    register, such as a success/error code.
    */
    uint eax;

    // rest of trap frame
    /*
    In the x86 architecture, gs, fs, es, ds, and cs are all segment registers
    used for segment selection. Each register is associated with a specific
    segment, and they are used to determine the memory segments that
    instructions access during program execution.

    Here's a brief overview of their roles:
    cs (Code Segment) points to the segment that contains the currently
    executing code. It is used for fetching instructions. ds (Data Segment)
    points to the segment that holds the data accessed by instructions without
    an explicit segment override. It is used for accessing global and static
    data. es (Extra Segment), fs (Additional Segment), and gs (General Segment)
    are general-purpose segment registers that can be used for various purposes
    depending on the specific requirements of the operating system or
    application.

    Example with cs :
    In a context where all segments have the same base address, the code segment
    register (cs, and gs, fs, es, ds) still plays a significant role. While it
    is true that in a flat memory model, where all segments have the same base
    address, the segment base address is effectively zero for all segments, the
    cs register is still used for specifying the current code segment. Even
    though the segment base address is the same for all segments, the cs
    register is used to differentiate between different segments based on their
    segment selectors. each segment has its own segment selector, which is a
    combination of an index into the Global Descriptor Table (GDT) or Local
    Descriptor Table (LDT) and additional flags.

    So, the segment registers such as gs, fs, es, ds, and cs can still be useful
    for providing different levels of access permissions, including read-only
    and write access.

    See the GDT documentation for more details.
    */

    /*
    gs (General Segment) is a general-purpose segment register that can be used
    for various purposes,
    */
    ushort gs;
    ushort padding1;
    /*
    fs (Additional Segment) is typically used as an additional segment register
    for specific purposes.
    */
    ushort fs;
    ushort padding2;
    /*
    es (Extra Segment) is an extra segment register that can be used for various
    purposes, similar to gs and fs
    */
    ushort es;
    ushort padding3;
    /*
    cs (Code Segment) is used to hold the segment selector for the code segment,
    while ds (Data Segment) is used to hold the segment selector for the data
    segment. Refer to cs definition for more informations
    */
    ushort ds;
    ushort padding4;
    /*
    Stores the trap number, which indicates the type of trap or interrupt that
    occurred.
    */
    uint trapno;

    // below here defined by x86 hardware
    /*
    Error codes associated with certain traps or interrupts. Not all traps or
    interrupts generate an error code, so this field may not be used in some
    cases. Let's consider the example of a page fault (trap number 14). Here are
    three possible error codes (err) that can be associated with the page fault
    trap: Error code 0: This indicates a page fault caused by a non-present
    page. It means that the program attempted to access a memory page that is
    not currently mapped in the page table, leading to a page fault. Error code
    1: This indicates a page fault caused by a read access violation. It means
    that the program attempted to read from a memory page that it does not have
    the necessary permissions to access, resulting in a page fault. Error code
    2: This indicates a page fault caused by a write access violation. It means
    that the program attempted to write to a memory page that it does not have
    the necessary permissions to modify, resulting in a page fault.
    */
    uint err;
};

/*
Layout of the trap frame built on the stack by the hardware and by trapasm.S,
and passed to trap().

The trapframe is a data structure used to store the state of a process or thread
when it encounters an exception or an interrupt, such as a system call, page
fault, or hardware interrupt. When a trap or interrupt occurs, the current state
of the executing process or thread is saved in the trapframe, including register
values, program counter, stack pointer, and other relevant information. The
trapframe allows the operating system to handle the exception or interrupt and
resume execution of the process or thread from where it was interrupted,
ensuring that the process maintains its state and can continue execution
correctly. The trapframe is typically used during context switches, exception
handling, and interrupt handling.
*/
struct trapframe {
    struct TrapframeSystem trapframeSystem;
    struct TrapframeHardware trapframeHardware;
};

#endif