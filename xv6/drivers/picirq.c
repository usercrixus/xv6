#include "../type/types.h"
#include "../x86.h"
#include "../systemCall/traps.h"

// I/O Addresses of the two programmable interrupt controllers
#define IO_PIC1 0x20  // Master (IRQs 0-7)
#define IO_PIC2 0xA0  // Slave (IRQs 8-15)

/*
Desable the 8259A interrupt controllers. Xv6 assumes SMP hardware.

The Programmable Interrupt Controller (PIC) is a device used to combine several
sources of interrupt onto one or more CPU lines, while allowing priority levels
to be assigned to its interrupt outputs. The initial PIC was the 8259A and it
was used in early computers, but it's been largely replaced in modern systems by
Advanced Programmable Interrupt Controllers (APICs) which offer better support
for multiprocessor systems.

xv6 assumes that the system has Symmetric MultiProcessing (SMP) hardware,
and in such a system, interrupt handling is typically done through the APIC, not
the older PIC. Disabling the PIC prevents it from interfering with the APIC's
handling of interrupts.
*/
void picinit(void) {
    /*
    writes the value 0xFF to the data port of PIC1. The data port of the PIC is
    used to set the interrupt mask register, and a value of 0xFF effectively
    disables all interrupts handled by PIC1.

    IO_PIC1 + 1 is the command port of the master PIC. It is used for sending
    commands and control signals to the PIC.
    IO_PIC1 + 0 is the data port of the master PIC. It is used for reading or
    writing data to/from the PIC.
    */
    outb(IO_PIC1 + 1, 0xFF);
    /*
    Similarly, disables all interrupts handled by PIC2.
    */
    outb(IO_PIC2 + 1, 0xFF);
}