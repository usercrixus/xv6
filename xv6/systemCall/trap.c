#include "../type/types.h"
#include "defs.h"
#include "../type/param.h"
#include "../memory/memlayout.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "x86.h"
#include "traps.h"
#include "../synchronization/spinlock.h"
#include "../userLand/user.h"
#include "syscall.h"
#include "trap.h"
#include "../drivers/uart.h"
#include "../drivers/lapic.h"

/*
The Interrupt Descriptor Table (IDT) is a data structure used by the x86
architecture to implement an interrupt vector table. The IDT is used by the
processor to determine the correct response to interrupts and exceptions. Each
entry in the IDT is an "interrupt gate" or "trap gate" that tells the processor
where to find the code to execute when a particular interrupt or exception
occurs.
*/
struct gatedesc idt[256];
/*
in vectors.S (generate by vectors.py): array of 256 entry pointers
*/
extern uint vectors[];
uint ticks;
struct spinlock tickslock;

void tvinit(void) {
    /*
    This loop initializes the entries in the IDT. Each entry in the IDT
    represents an interrupt or exception that the system can handle.

    idt[i] refers to the IDT entry at index i.
    0 represents the trap type, indicating that it's not a trap gate.
    SEG_KCODE << 3 specifies the segment selector for the kernel code segment.
    vectors[i] is the address of the interrupt or exception handler function.
    0 sets the privilege level of the handler to kernel mode.
    */
    for (int i = 0; i < 256; i++)
        SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);

    /*
    specifically sets the IDT entry for the system call interrupt.

    idt[T_SYSCALL] refers to the IDT entry for the system call interrupt.
    1 represents the trap type, indicating that it's a trap gate.
    SEG_KCODE << 3 specifies the segment selector for the kernel code segment.
    vectors[T_SYSCALL] is the address of the system call handler function.
    DPL_USER sets the privilege level of the handler to user mode, allowing user
    processes to make system calls.

    By setting the system call IDT entry, the system enables user processes to
    invoke privileged operations through system calls.
    */
    SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);
    /*
    initializes the tickslock spinlock used for synchronization related to
    timekeeping.

    Timekeeping refers to the measurement and management of time in a computer
    system. It involves tracking and maintaining accurate time information for
    various purposes, such as scheduling tasks, synchronization, event handling,
    and performance monitoring.
    */
    initlock(&tickslock, "time");
}

void idtinit(void) {
    lidt(idt, sizeof(idt));
}

void trap(struct trapframe* tf) {
    // Force process exit if it has been killed
    if (myproc() && myproc()->killed)
        exit();

    /*
    The switch statement handles different types of hardware interrupts,
    identified by tf->trapno:
    */
    switch (tf->trapframeSystem.trapno) {
        /*
        first checks if the trap is a system call (tf->trapno == T_SYSCALL).
        If it is, it performs the system call handling:
        */
        case T_SYSCALL:
            myproc()->tf = tf;
            syscall();
            break;
        case T_IRQ0 + IRQ_TIMER:
            if (cpuid() == 0) {
                acquire(&tickslock);
                ticks++;
                wakeup(&ticks);
                release(&tickslock);
            }
            lapiceoi();

            // Force process to give up CPU on clock tick.
            if (myproc() && myproc()->state == RUNNING)
                yield();
            break;
        case T_IRQ0 + IRQ_IDE:
            ideintr();
            lapiceoi();
            break;
        case T_IRQ0 + IRQ_KBD:
            kbdintr();
            lapiceoi();
            break;
        case T_IRQ0 + IRQ_COM1:
            uartintr();
            lapiceoi();
            break;
        case T_IRQ0 + IRQ_SPURIOUS:
            cprintf("cpu%d: spurious interrupt at %x:%x\n", cpuid(),
                    tf->trapframeHardware.cs, tf->trapframeHardware.eip);
            lapiceoi();
            break;
        /*
        handle unexpected traps, i.e., traps that are not specifically accounted
        for in the preceding case statements
        */
        default:
            /*
            Checks where the trap occurred - in kernel space or user space.
            */
            if (myproc() == 0 || (tf->trapframeHardware.cs & 3) == 0) {
                /*
                In kernel :
                In an operating system, any unexpected trap in kernel mode is
                considered a critical error because the kernel is supposed to be
                bug-free and fully in control of what it executes. An unexpected
                trap in kernel mode suggests a bug or serious issue in the
                kernel.
                */
                cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
                        tf->trapframeSystem.trapno, cpuid(),
                        tf->trapframeHardware.eip, rcr2());
                panic("trap");
            } else {
                // In user space, assume process misbehaved.
                cprintf(
                    "pid %d %s: trap %d err %d on cpu %d "
                    "eip 0x%x addr 0x%x--kill proc\n",
                    myproc()->pid, myproc()->name, tf->trapframeSystem.trapno,
                    tf->trapframeSystem.err, cpuid(), tf->trapframeHardware.eip,
                    rcr2());
                // Marks the current process as killed
                myproc()->killed = 1;
            }
    }

    // Force process exit if it has been killed
    if (myproc() && myproc()->killed)
        exit();
}
