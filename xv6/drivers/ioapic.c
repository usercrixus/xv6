/*
Default physical address of IO APIC
*/
#define IOAPIC 0xFEC00000
/*
refers to the register offset used to read the identification information of the
IO APIC (Input/Output Advanced Programmable Interrupt Controller). It is the
register that holds the ID value of the IO APIC device.
*/
#define REG_ID 0x00  // Register index: ID
/*
Refers to the register offset used to read the version information of the IO
APIC (Input/Output Advanced Programmable Interrupt Controller). It is the
register that holds the version number and other relevant information about the
IO APIC device.
*/
#define REG_VER 0x01
/*
Offset value used to access the IOAPIC's interrupt table.
*/
#define REG_TABLE 0x10
/*
Constant that represents the interrupt mask value to disable the interrupt
*/
#define INT_DISABLED 0x00010000
#define INT_LEVEL 0x00008000      // Level-triggered (vs edge-)
#define INT_ACTIVELOW 0x00002000  // Active low (vs high)
#define INT_LOGICAL 0x00000800    // Destination is CPU id (vs APIC ID)

/*
Location: Integrated into each processor or core in modern x86 systems.

Function: Manages interrupts that are local to the processor. This includes both
internal and external interrupts.

Purpose: Its primary role is to handle interrupts that are specific to the
processor it resides in, such as inter-processor interrupts (IPIs), timer
interrupts, and performance monitoring interrupts.

Advantage: Allows for more efficient handling of interrupts in multi-core or
multi-processor systems by reducing the need for all interrupts to go through a
single point.

Control: Each LAPIC can be independently programmed and controlled.
*/
#include ".././type/types.h"
#include "../defs.h"
#include "../systemCall/traps.h"

/*
Variable that can be used to interact with the IO APIC (Input/Output Advanced
Programmable Interrupt Controller) device.

The IO APIC is a hardware component responsible for managing interrupts in a
computer system. It receives interrupt signals from various sources and
distributes them to the appropriate processors in a multiprocessor system.
*/
volatile struct ioapic* ioapic;

/*
Describing the memory-mapped I/O (MMIO) structure for interacting with the IO
APIC (Input/Output Advanced Programmable Interrupt Controller). The IO APIC is a
hardware device responsible for routing and managing interrupts in a
multiprocessor system.
*/
struct ioapic {
    /*
    This member is used to write the desired register number of the IO APIC.
    When a value is written to reg, it instructs the IO APIC to select the
    corresponding register for read or write operations.
    */
    uint reg;
    /*
    Offset to respect the IOAPIC hardware requirement
    */
    uint pad[3];
    /*
    This member holds the data value associated with the selected register.
    After setting the desired register in reg, you can read or write the
    corresponding data value using this member.
    */
    uint data;
};

/*
Reads the value from the specified register of the IOAPIC. It takes an input
parameter reg representing the register offset.

The hardware implementation of the IOAPIC (Input/Output Advanced Programmable
Interrupt Controller) handles the setting of the data value when the reg member
is modified.
*/
static uint ioapicread(int reg) {
    ioapic->reg = reg;
    return ioapic->data;
}

/*
Writes the specified data to the specified register of the IOAPIC. It takes two
input parameters: reg representing the register offset and data representing the
data to be written.
*/
static void ioapicwrite(int reg, uint data) {
    ioapic->reg = reg;
    ioapic->data = data;
}

/*
Initializes the I/O Advanced Programmable Interrupt Controller (IOAPIC). The
IOAPIC is responsible for managing interrupts from the system's input/output
devices to the processor(s). Manage and route interrupts from various devices to
the appropriate processors in a multiprocessor system.
*/
void ioapicinit(void) {
    /*
    assigns the IOAPIC's base address to the ioapic pointer using the IOAPIC
    constant. IOAPIC is a well-defined address. In this case, the IOAPIC
    constant is used to represent that base address.
    */
    ioapic = (volatile struct ioapic*)IOAPIC;
    /*
    determine the maximum number of interrupts supported by the IO APIC
    */
    int maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
    /*
    Store the IOAPIC's ID
    */
    int id = ioapicread(REG_ID) >> 24;
    /*
    The MP Floating Pointer (MPFP) structure contains information about the
    system's multiprocessing configuration, including the IDs of the IO APIC
    devices present in the system. The IOAPIC ID in the MPFP should match the
    actual IO APIC ID obtained during initialization. If they do not match, it
    indicates a discrepancy between the expected and actual hardware
    configuration, suggesting that the system is not properly configured as a
    multiprocessor system.
    */
    if (id != ioapicid)
        cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");

    /*
    initialize each interrupt entry in the IOAPIC's interrupt redirection table.
    */
    for (int i = 0; i <= maxintr; i++) {
        /*
        Writes the interrupt mask and the corresponding interrupt vector number
        to the IOAPIC's register at the offset REG_TABLE + 2 * i. The interrupt
        mask (INT_DISABLED) ensures the interrupt is initially disabled, and the
        interrupt vector number (T_IRQ0 + i) specifies the corresponding IRQ
        number for that interrupt.

        The reason for using REG_TABLE + 2 * i as the offset is based on the
        IOAPIC's interrupt redirection table structure. Each interrupt entry in
        the table consists of two 32-bit registers: the first register holds the
        interrupt mask and the interrupt vector number, and the second register
        holds the destination field.
        */
        ioapicwrite(REG_TABLE + 2 * i, INT_DISABLED | (T_IRQ0 + i));
        /*
        Writes 0 to the IOAPIC's register at the offset REG_TABLE + 2 * i + 1,
        which sets the destination field to 0 (not routed to any CPUs).

        The reason for using REG_TABLE + 2 * i as the offset is based on the
        IOAPIC's interrupt redirection table structure. Each interrupt entry in
        the table consists of two 32-bit registers: the first register holds the
        interrupt mask and the interrupt vector number, and the second register
        holds the destination field.
        */
        ioapicwrite(REG_TABLE + 2 * i + 1, 0);
    }
}

/*
Responsible for enabling a specific interrupt on the IO APIC (Input/Output
Advanced Programmable Interrupt Controller). The IO APIC is a component that
manages interrupts in a system, including distributing interrupts to different
processors.

The function takes two parameters: irq and cpunum.
    - irq represents the interrupt number or line that needs to be enabled.
    - cpunum specifies the CPU number or APIC ID to which the interrupt should
be routed.
*/
void ioapicenable(int irq, int cpunum) {
    /*
    writes to the IO APIC's I/O register to set the interrupt configuration for
    the specified interrupt number (irq). REG_TABLE + 2 * irq calculates the
    offset address of the interrupt entry in the IO APIC's register table.
    T_IRQ0 + irq determines the interrupt vector number for the specified
    interrupt. T_IRQ0 is a constant defined in the system, representing the base
    vector number for external interrupts.
    */
    ioapicwrite(REG_TABLE + 2 * irq, T_IRQ0 + irq);
    /*
    writes to the IO APIC's I/O register to configure the routing of the
    interrupt to the specified CPU (cpunum). REG_TABLE + 2 * irq + 1 calculates
    the offset address of the interrupt entry's high bits in the IO APIC's
    register table. cpunum << 24 shifts the cpunum value 24 bits to the left to
    set it in the appropriate position for CPU routing.
    */
    ioapicwrite(REG_TABLE + 2 * irq + 1, cpunum << 24);
}
