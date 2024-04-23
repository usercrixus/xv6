// x86 trap and interrupt constants.

// Processor-defined:
#define T_DIVIDE 0  // divide error
#define T_DEBUG 1   // debug exception
#define T_NMI 2     // non-maskable interrupt
#define T_BRKPT 3   // breakpoint
#define T_OFLOW 4   // overflow
#define T_BOUND 5   // bounds check
#define T_ILLOP 6   // illegal opcode
#define T_DEVICE 7  // device not available
#define T_DBLFLT 8  // double fault
// #define T_COPROC      9      // reserved (not used since 486)
#define T_TSS 10    // invalid task switch segment
#define T_SEGNP 11  // segment not present
#define T_STACK 12  // stack exception
#define T_GPFLT 13  // general protection fault
#define T_PGFLT 14  // page fault
// #define T_RES        15      // reserved
#define T_FPERR 16    // floating point error
#define T_ALIGN 17    // aligment check
#define T_MCHK 18     // machine check
#define T_SIMDERR 19  // SIMD floating point error

/*
These are arbitrarily chosen, but with care not to overlap
processor defined exceptions or interrupt vectors.
Used in the context of system calls, specifically for software interrupts that
transition from user mode to kernel mode.
*/
#define T_SYSCALL 64
#define T_DEFAULT 500  // catchall

/*
The base number for the interrupt vector
In x86, the first 32 interrupt vectors (0-31) are reserved for exceptions that
are defined by the CPU - these include things like division by zero, page
faults, general protection faults, etc. The interrupt vector table starts with
these CPU exceptions, and then continues with IRQ lines from the Programmable
Interrupt Controller (PIC) or the APIC.

T_IRQ0 is set to 32 because it's the first available vector number after the CPU
reserved exception vectors. This makes T_IRQ0 + IRQ_n the vector for the n-th
hardware interrupt. The IRQ lines for devices, as mapped by the PIC or APIC,
would start from this point.

So, T_IRQ0 is essentially the base number for hardware interrupt vectors in the
system, and that's why it's set to 32. IRQ numbers from the PIC or APIC are then
added to this base to get the actual interrupt vector number for each device.
*/
#define T_IRQ0 32
/*
used to represent the interrupt request (IRQ) line for the timer.
In many computer systems, particularly those based on the x86 architecture,
various hardware devices are assigned different IRQ lines for raising
interrupts. The timer, which is crucial for operations like task scheduling and
time-keeping, is typically assigned to IRQ line 0.
*/
#define IRQ_TIMER 0
/*
In the context of APIC (Advanced Programmable Interrupt Controller) and
interrupt handling, vector 1 typically corresponds to the keyboard interrupt.
*/
#define IRQ_KBD 1
/*
represents the interrupt number associated with the COM1 serial port.

COM1 refers to the base address of the first serial port (COM1) on the system.
Serial ports, also known as communication ports or COM ports, are hardware
interfaces used for serial communication between a computer and external
devices.
*/
#define IRQ_COM1 4
/*
Represent the interrupt request line associated with the Integrated Drive
Electronics (IDE) interface. IDE, also known as ATA (Advanced Technology
Attachment), is a standard interface for connecting storage devices like hard
drives and optical drives to a computer's motherboard.
*/
#define IRQ_IDE 14
#define IRQ_ERROR 19
#define IRQ_SPURIOUS 31
