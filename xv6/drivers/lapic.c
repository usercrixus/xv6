/*
Location: Typically a separate chip or integrated into the chipset on the
motherboard.

Function: Manages interrupts that come from I/O devices like disk controllers,
network cards, and USB controllers.

Purpose: Designed to route I/O interrupts to the appropriate processor,
particularly in systems with multiple processors or cores.

Flexibility: Allows for more complex routing of interrupts than what was
possible with the older PIC (Programmable Interrupt Controller) system, which
the IOAPIC has largely replaced in modern systems.

Scalability: Supports a larger number of interrupt lines, which is essential in
systems with many I/O devices.
*/

#include "../type/param.h"
#include "../type/types.h"
#include "../defs.h"
#include "../type/date.h"
#include "../memory/memlayout.h"
#include "../systemCall/traps.h"
#include "../memory/mmu.h"
#include "../x86.h"
#include "../userLand/ulib.h"
#include "lapic.h"

/*
The APIC is responsible for managing interrupts in a multiprocessor system.
uint* lapic is initialized in mp.c
*/
volatile uint* lapic;

/*
used to write a value to a specific register of the local Advanced Programmable
Interrupt Controller (APIC). It takes two arguments: index, which represents the
register index to write to, and value, which is the value to be written to the
register.
*/
static void lapicw(int index, int value) {
    lapic[index] = value;
    lapic[ID];  // wait for write to finish, by reading
}

void lapicinit(void) {
    if (!lapic)
        return;

    /*
    Enable local APIC; set spurious interrupt vector.
    */
    lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

    /*
    Configure the Local APIC Timer

    The timer is a hardware component that counts down at the bus frequency
    from the initial count value set in lapic[TICR]. Once the timer reaches 0,
    it generates an interrupt.

    In xv6, the timer is set to operate in periodic mode, which means it will
    repeatedly count down from the initial count value and issue interrupts at
    regular intervals.

    To configure the timer:
    1. Set the Timer Divide Configuration Register (TDCR) to divide the counts
    by 1 (X1), ensuring that the timer operates at the full bus frequency.
    */
    lapicw(TDCR, X1);

    /*
    2. Configure the TIMER entry in the Local APIC's interrupt controller. Set
    it to operate in periodic mode and specify the interrupt vector number for
    the timer interrupts.
    */
    lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
    /*
    3. Set the initial count value for the timer. In this case, the initial
    count is set to 10,000,000. The specific value used determines the frequency
    of the timer interrupts.
    */
    lapicw(TICR, 10000000);

    // Disable logical interrupt lines.
    lapicw(LINT0, MASKED);
    lapicw(LINT1, MASKED);

    /*
    Disable performance counter overflow interrupts on machines that provide
    that interrupt entry.

    Checks if the version of the Local APIC is 4 or higher. lapic[VER]
    retrieves the version number from the Local APIC version register. The
    version number is in bits 16 to 23 of the version register (hence the >> 16
    shift and the & 0xFF mask). APIC versions 4 and above have an entry for
    performance counter overflow interrupts.
    */
    if (((lapic[VER] >> 16) & 0xFF) >= 4)

        /*
        If the version is high enough, it then masks (disables) performance
        counter overflow interrupts by writing the MASKED value to the PCINT
        (Performance Counter Interrupt) register.
        */
        lapicw(PCINT, MASKED);

    // Map error interrupt to IRQ_ERROR.
    lapicw(ERROR, T_IRQ0 + IRQ_ERROR);

    /*
    Clearing the ESR. The ESR is cleared by writing any value to it. But it has
    a quirk: the Intel manual specifies that it requires back-to-back writes to
    actually clear it. This is why you see two writes of 0 to ESR consecutively.
    */
    lapicw(ESR, 0);
    lapicw(ESR, 0);

    /*
    Acknowledge any outstanding interrupts. This is necessary to inform
    the Local Advanced Programmable Interrupt Controller (LAPIC) that the
    current interrupt has been processed, so that it can issue the next
    interrupt if there are any pending.

    If this step was not performed, the LAPIC would still consider the current
    interrupt as being serviced and would not issue the next interrupt, even if
    there were other interrupts pending, leading to a potential stall in the
    system.
    */
    lapicw(EOI, 0);

    /*
    Send an Inter-Processor Interrupt (IPI) to all other processors in the
    system to synchronize their arbitration IDs

    In a multiprocessing system like a multi-core CPU, the arbitration ID is a
    unique identifier used by the Advanced Programmable Interrupt Controller
    (APIC) to identify each processor. This ID allows the APIC to manage and
    direct interrupts to the appropriate processor.

    Arbitration is a process by which the system determines which processor gets
    to handle a particular operation or task when multiple processors request to
    do so. It's necessary in a multi-processor system to ensure orderly and
    efficient processing, and to prevent conflicts.

    When the system boots up, each processor is given an unique arbitration ID,
    which it uses to participate in the arbitration process. The APIC can then
    use these IDs to send interrupts to specific processors, or to all
    processors at once (broadcast).

    Synchronising arbitration IDs means to make sure all the processors in the
    system have been assigned unique IDs and are ready to handle interrupts.

    writing to the ICRHI (Interrupt Command Register High) to specify that the
    IPI should not be sent to a specific processor ID. Setting it to 0 means
    that the destination is not specific.
    */
    lapicw(ICRHI, 0);
    /*
    writing to the ICRLO (Interrupt Command Register Low) to specify the type of
    interrupt and the mode of delivery.
    */
    lapicw(ICRLO, BCAST | INIT | LEVEL);
    /*
    Keep looping as long as the DELIVS bit in the ICRLO register is set.
    */
    while (lapic[ICRLO] & DELIVS)
        ;

    /*
    Setting the TPR to zero essentially means that no interrupts are being
    masked and all priority levels of interrupts are allowed to interrupt the
    processor. This does not yet enable interrupts on the processor itself,
    which is usually controlled by a separate mechanism (like the IF, or
    Interrupt Flag, bit in the processor's EFLAGS register).

    However, setting the TPR to zero enables the APIC to receive all levels of
    interrupts and pass them to the processor when the processor is ready to
    receive them. It tells the APIC that the processor is ready to handle the
    interrupts of all priorities.

    In other words, this line of code is enabling the reception of interrupts on
    the APIC itself, but it does not enable the actual delivery of these
    interrupts to the processor.
    */
    lapicw(TPR, 0);
}

int lapicid(void) {
    if (!lapic)
        return 0;
    return lapic[ID] >> 24;
}

void lapiceoi(void) {
    if (lapic)
        lapicw(EOI, 0);
}

void microdelay(int us) {}

/*
represents the I/O port used to read from and write to the CMOS registers. When
data is written to this port, it specifies which CMOS register to access.
*/
#define CMOS_PORT 0x70
/*
represents the I/O port used to read the data from the CMOS register specified
by the CMOS_PORT. When data is read from this port, it retrieves the value
stored in the corresponding CMOS register.
*/
#define CMOS_RETURN 0x71

void lapicstartap(uchar apicid, uint addr) {
    /*
    The CMOS (Complementary Metal-Oxide-Semiconductor) is a special memory chip
    that stores system configuration data, including information about hardware
    devices and settings. The CMOS shutdown code and warm reset vector are two
    specific values stored in the CMOS memory that play a role in the system
    initialization process

    CMOS Shutdown Code:

    The CMOS shutdown code is a value that determines the type of system
    shutdown to be performed. In this code segment, the value 0x0A is written to
    the CMOS port 0xF (offset 0xF) to set the shutdown code. The purpose of
    setting the shutdown code is to specify how the system should behave when it
    is powered off or restarted. In the context of CMOS shutdown, the "type of
    system shutdown" refers to different ways in which the system can be powered
    off or restarted. The CMOS shutdown code is a specific value that is written
    to a CMOS register to indicate the desired type of shutdown.

    CMOS shutdown code is set by writing a value to the CMOS port 0xF. The value
    0x0A is written to indicate the shutdown code.
    */
    outb(CMOS_PORT, 0xF);
    outb(CMOS_PORT + 1, 0x0A);
    /*
    represents the warm reset vector. The warm reset vector is a data structure
    used by the system to define the address to which the processor should jump
    when a warm reset is triggered.

    A warm reset, also known as a soft reset or a warm reboot, is a type of
    system reset or restart that is performed without completely powering off
    the computer. Unlike a cold reset or cold boot, which involves shutting down
    the system and then powering it back on, a warm reset preserves the system's
    power supply and some of its state, allowing for a quicker restart.

    During a warm reset, the computer's hardware and software are reset to their
    initial or default states, similar to a fresh boot. However, the power to
    the system remains on, so the process is faster than a cold reset. A warm
    reset is often used to recover from system errors or to initiate a restart
    after installing software updates or making configuration changes.

    The line wrv = (ushort*)P2V((0x40 << 4 | 0x67)); calculates the virtual
    address of the warm reset vector by shifting the value 0x40 four bits to the
    left and bitwise ORing it with 0x67. the warm reset vector refers to a
    specific hardware address in memory.
    */
    ushort* wrv = (ushort*)P2V((0x40 << 4 | 0x67));
    /*
    the entry code for the APs is being set as the warm reset vector to instruct
    the APs to start executing that code upon warm reset.
    */
    wrv[0] = 0;
    wrv[1] = addr >> 4;

    /*
    Five next step are part of the Universal startup algorithm, which is a
    sequence to start up additional processors (APs) in a multiprocessor system.
    The code performs the following actions:
    */
    /*
    writes the APIC ID of the target processor to the high-order bits of the ICR
    (Interrupt Command Register) High register. The APIC ID is left-shifted by
    24 bits to set the appropriate bits in the register.
    */
    lapicw(ICRHI, apicid << 24);
    /*
    writes the value INIT | LEVEL | ASSERT to the ICR (Interrupt Command
    Register) Low register. The INIT bit indicates that an INIT (initialization)
    interrupt should be sent to the target processor. The LEVEL bit specifies
    that the interrupt is level-triggered, and the ASSERT bit indicates that the
    interrupt should be asserted.
    */
    lapicw(ICRLO, INIT | LEVEL | ASSERT);
    microdelay(200);
    /*
    clears the ASSERT bit in the ICR Low register, deasserting the INIT
    interrupt. The target processor starts the reset sequence and prepares to
    transition to the next step.
    */
    lapicw(ICRLO, INIT | LEVEL);
    microdelay(100);

    /*
    The loop sends a startup IPI (Inter-Processor Interrupt) to the target CPU
    specified by apicid in order to initiate the execution of code at the
    address addr on that CPU. The entryother.S
    */
    for (int i = 0; i < 2; i++) {
        lapicw(ICRHI, apicid << 24);
        /*
        The STARTUP flag indicates that the interrupt is a startup IPI, and
        (addr >> 12) sets the vector field of the interrupt command to the
        address of the code to be executed on the target CPU.

        The purpose of sending the startup IPI twice is part of the official
        Intel algorithm, even though regular hardware is expected to only accept
        a startup IPI when it is in the halted state due to an INIT. The second
        startup IPI is often ignored by the hardware, but it is included as a
        precautionary measure.
        */
        lapicw(ICRLO, STARTUP | (addr >> 12));
        microdelay(200);
    }
}
