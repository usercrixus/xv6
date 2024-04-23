#ifndef TRAP_H
#define TRAP_H

#include "../x86.h"

extern uint ticks;
extern struct spinlock tickslock;
/*
responsible for initializing the interrupt descriptor table (IDT) and associated
data structures used for handling interrupts and exceptions in the xv6 operating
system.
*/
void tvinit(void);
/*
Responsible for initializing the Interrupt Descriptor Table (IDT).

Typically called during the early stages of operating system initialization.
Setting up the IDT is a critical step in enabling the operating system to handle
interrupts and exceptions properly, which are essential for tasks like
responding to hardware signals, managing system calls, and handling faults and
errors.
*/
void idtinit(void);
/*
Called from trapasm.S

handle various types of traps, including system calls, hardware interrupts, and
exceptions.

The trap function is invoked in response to the INT T_SYSCALL instruction (INT
64)

When INT is executed, the CPU uses the IDT to find the appropriate interrupt
handler for the given interrupt number. This handler is often an entry point in
the kernel, which eventually leads to the invocation of the trap function in
xv6.
*/
void trap(struct trapframe* tf);

#endif