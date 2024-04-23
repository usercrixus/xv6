// Memory layout

/*
start address of the extended memory region.

The extended memory region refers to the portion of physical memory that is
available for general-purpose use beyond the kernel's memory space. It starts at
the address specified by EXTMEM and extends up to the maximum limit of the
available physical memory.
*/
#define EXTMEM 0x100000
/*
PHYSTOP is a macro defined to represent the topmost physical memory address that
can be used by the operating system. It defines the boundary between usable
memory and reserved memory.
By defining PHYSTOP, the operating system can ensure that it does not use memory
beyond this address, as it may be reserved for hardware or other purposes. It
helps in managing and allocating the available physical memory within the
specified range.
*/
#define PHYSTOP 0xE000000
/*
DEVSPACE is a macro defined to represent the base address of the memory space
reserved for device mappings. Devices, such as peripherals and hardware
registers, are typically mapped to specific memory addresses for communication
and control.
By defining DEVSPACE, the operating system can easily access and interact with
devices by using memory-mapped I/O techniques. Memory-mapped I/O allows devices
to be treated as if they were regular memory locations, enabling read and write
operations to control the device's functionality.
*/
#define DEVSPACE 0xFE000000
/*
First kernel virtual address
*/
#define KERNBASE 0x80000000
/*
KERNLINK is a constant that holds the virtual address where the kernel is
linked, serving as a convenient reference point for accessing the kernel's code
and data in the virtual memory space.
*/
#define KERNLINK (KERNBASE + EXTMEM)

/*
This macro takes a virtual address and converts it to a physical address by
subtracting KERNBASE.
*/
#define V2P(a) (((uint)(a)) - KERNBASE)
/*
This macro takes a physical address and converts it to a virtual address by
adding KERNBASE.

The purpose of the P2V macro is to provide a convenient way to translate
physical addresses (which represent locations in physical memory) to their
corresponding virtual addresses within the kernel's virtual memory space.

The P2V macro is not intended for converting physical addresses to user virtual
addresses. User virtual addresses are handled differently, typically through the
use of page tables and address translation mechanisms specific to user
processes.
*/
#define P2V(a) ((void*)(((char*)(a)) + KERNBASE))
#define V2P_WO(x) ((x)-KERNBASE)    // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNBASE)  // same as P2V, but without casts
