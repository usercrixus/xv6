// Multiprocessor support
// Search memory for MP description structures.
// http://developer.intel.com/design/pentium/datashts/24201606.pdf

#include "./type/types.h"
#include "defs.h"
#include "type/param.h"
#include "./memory/memlayout.h"
#include "mp.h"
#include "x86.h"
#include "./memory/mmu.h"
#include "./processus/proc.h"
#include "userLand/ulib.h"
#include "drivers/lapic.h"

/*
reserves memory to store multiple instances of struct cpu, allowing each element
of the array to represent a different CPU in the system. This provides a
convenient way to manage and access CPU-specific information and state for
various purposes, such as initialization, scheduling, or parallel execution.
*/
struct cpu cpus[NCPU];
/*
Number of cpu found in the system.
Since ncpu is declared as a global variable, it is implicitly initialized to 0.
The same rule applies to the other global variables cpus and ioapicid.
*/
int ncpu;
/*
Unlike the Local APIC, which is associated with each processor in struct cpu
cpus[NCPU], there is usually a single IOAPIC in the system that handles
interrupts from multiple I/O devices.
*/
uchar ioapicid;

/*
calculates the sum of byte values in a given memory region. It initializes the
sum to zero and iteratively adds the byte values to the sum. The final sum is
then returned as the result of the function.

takes two arguments: addr, which is a pointer to the memory region to calculate
the sum, and len, which specifies the length of the memory region.

In the provided sum function, where the variable sum is an int, the arithmetic
overflow will occur when the sum exceeds the maximum value that an int can hold.
When this overflow happens, the value will wrap around to the minimum value that
an int can hold, 0. the maximum value that an int can hold is typically
2147483647 (2^31 - 1)
*/
static uchar checksum(uchar* addr, int len) {
    int sum;

    sum = 0;
    for (int i = 0; i < len; i++)
        sum += addr[i];
    return sum;
}

/*
search for an MP (Multiprocessor) structure within a given memory range.

takes two arguments: a, which is the starting address of the memory range to
search, and len, which specifies the length of the memory range.
*/
static struct mp* isAnMpFPStruct(uint a, int len) {
    uchar* addr = P2V(a);   // start address of the memory range
    uchar* e = addr + len;  // end address of the memory range
    while (addr < e) {
        if (memcmp(addr, "_MP_", 4) == 0 &&
            checksum(addr, sizeof(struct mp)) == 0) {
            return (struct mp*)addr;
        }
        addr += sizeof(struct mp);
    }
    return 0;
}

/*
responsible for searching and locating the MP (Multiprocessor) Floating Pointer
Structure, which contains information about the system's multiprocessing
capabilities. The function looks for the MP structure in three possible
locations: the Extended BIOS Data Area (EBDA), the last kilobyte of system base
memory, or the BIOS ROM between addresses 0xE0000 and 0xFFFFF.
*/
static struct mp* mpsearch(void) {
    /*
    will be used to store the address of the MP Floating Pointer Structure.
    */
    struct mp* mp;
    /*
    Assigns the virtual address of the BIOS Data Area (BDA) to the bda pointer.
    The BDA contains important system information, including the addresses of
    the EBDA and base memory.
    */
    uchar* bda = (uchar*)P2V(0x400);

    /*
    used to store memory addresses.
    */
    uint p;
    /*
    Retrieves the EBDA base address from the BDA. The BDA stores the EBDA base
    address in two bytes, located at offsets 0x0F and 0x0E. The retrieved
    address is combined and left-shifted by 4 bits to form the EBDA base address
    (p).
    */
    if ((p = ((bda[0x0F] << 8) | bda[0x0E]) << 4)) {
        /*
        Check if a valid MP (Multiprocessor) Floating Pointer Structure is
        present at this adress
        */
        if ((mp = isAnMpFPStruct(p, 1024)))
            return mp;
    } else {
        p = ((bda[0x14] << 8) | bda[0x13]) << 4;
        /*
        Check if a valid MP (Multiprocessor) Floating Pointer Structure is
        present at this adress
        */
        if ((mp = isAnMpFPStruct(p - 1024, 1024)))
            return mp;
    }
    /*
    Check if a valid MP (Multiprocessor) Floating Pointer Structure is
    present at this adress
    */
    return isAnMpFPStruct(0xF0000, 0x10000);
}

/*
Responsible for searching and validating the MP (Multiprocessor) configuration
table.

The Multiprocessor Configuration Table is part of the Multiprocessor
Specification (MPS) for x86 architecture. It is designed to provide detailed
information about the hardware in a multiprocessor system.

The mpconf structure is located in memory at a specific address known to the
code. It contains fields that describe the size of the MP configuration
table (located just after itself) and other relevant information about the
system's MP configuration.
*/
static struct mpconf* mpconfig(struct mp* mp) {
    // Guard against missing MP floating pointer structure
    if (mp == 0 || mp->physaddr == 0)
        return 0;
    struct mpconf* conf = (struct mpconf*)P2V((uint)mp->physaddr);
    if (memcmp(conf, "PCMP", 4) != 0)
        return 0;
    if (conf->version != 1 && conf->version != 4)
        return 0;
    if (checksum((uchar*)conf, conf->length) != 0)
        return 0;

    return conf;
}

/*
Initialization process for Symmetric Multiprocessing (SMP) systems in the xv6
operating system. It is responsible for setting up and configuring the system's
multiple processors and I/O Advanced Programmable Interrupt Controllers
(IOAPICs).

It get some crucial information about processor hard coded in the low memory
(like bios). Save ioapic id, lapic id for each processor and lapic adress for
the main processor
*/
void mpinit(void) {
    struct mp* mp = mpsearch();
    /*
    returns a pointer to an mpconf structure.
    */
    struct mpconf* conf = mpconfig(mp);
    /*
    If the function returns 0, indicating an error, it triggers a panic.
    */
    if (conf == 0)
        panic("Expect to run on an SMP");
    /*
    set to the memory address of the local Advanced Programmable Interrupt
    Controller (APIC). The APIC is responsible for managing interrupts in a
    multiprocessor system.
    */
    lapic = (uint*)conf->lapicaddr;
    /*
    In memory, the MP configuration table is located just after the mpconf
    structure. The mpconf structure provides information about the layout and
    size of the MP configuration table, allowing the code to navigate and
    interpret the contents of the table.

    After the mpconf structure, the actual MP configuration table follows in
    memory. This table contains multiple entries, each representing information
    about a specific component of the system, such as processors, I/O devices,
    buses, interrupts, etc.
    */
    uchar* mPConfigurationTable = (uchar*)(conf + 1);
    /*
    calculates the end address of the MP configuration table by adding the
    length field of the mpconf structure to the memory address of the conf
    pointer.
    */
    uchar* endMPConfTab = (uchar*)conf + conf->length;

    struct mpproc* proc;
    struct mpioapic* ioapic;
    while (mPConfigurationTable < endMPConfTab) {
        switch (*mPConfigurationTable) {
            case MPPROC:
                proc = (struct mpproc*)mPConfigurationTable;
                if (ncpu < NCPU) {
                    cpus[ncpu].apicid = proc->apicid;
                    ncpu++;
                }
                mPConfigurationTable += sizeof(struct mpproc);
                continue;
            case MPIOAPIC:
                ioapic = (struct mpioapic*)mPConfigurationTable;
                ioapicid = ioapic->apicno;
                mPConfigurationTable += sizeof(struct mpioapic);
                continue;
            case MPBUS:
                /*
                The cases MPBUS, MPIOINTR, and MPLINTR are used to handle
                specific entries in the MP configuration table that are not
                relevant to the current processing within the loop.

                By incrementing the p pointer by 8 bytes, the code moves to the
                next entry in the MP configuration table, regardless of the
                specific type of entry.
                */
                mPConfigurationTable += 8;
                continue;
            case MPIOINTR:
                mPConfigurationTable += 8;
                continue;
            case MPLINTR:
                mPConfigurationTable += 8;
                continue;
            default:
                panic("Didn't find a suitable machine");
        }
    }
    /*
    The Integrated Micro Channel Redirect Port (IMCR) is used to control the
    routing of hardware interrupts in certain systems. By configuring the
    IMCR, you can specify how hardware interrupts from external devices are
    delivered to the processors in a multiprocessor system
    */
    if (mp->imcrp) {
        /*
        outb(0x22, 0x70) command is used to select the IMCR by sending the
        value 0x70 to the command port (0x22). This tells the system that
        subsequent I/O operations will be directed to the IMCR.
        */
        outb(0x22, 0x70);
        /*
        Once the IMCR is selected, you can use the outb(0x23, data) command
        to write a value (data) to the data port (0x23). This value
        represents the desired configuration or control information for the
        IMCR.

        So, port 0x22 is used for selecting the hardware device or register,
        and port 0x23 is used for writing data to that device or register.

        the purpose of this line is to set the least significant bit of the
        IMCR register to 1, effectively enabling the interrupt mask. By
        doing so, it prevents external interrupts from being processed by
        the system.
        */
        outb(0x23, inb(0x23) | 1);
    }
}
