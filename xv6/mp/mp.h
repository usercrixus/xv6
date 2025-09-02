/*
indicate that the processor is the bootstrap processor
*/
#define MPBOOT 0x02

// Table entry types :

#define MPPROC 0x00    // One per processor
#define MPBUS 0x01     // One per bus
#define MPIOAPIC 0x02  // One per I/O APIC
#define MPIOINTR 0x03  // One per bus interrupt source
#define MPLINTR 0x04   // One per system interrupt source

/*
In the context of the MP (Multiprocessor) specification, MP floating pointer
structure is used to locate and access the MP configuration table, which
contains information about the system's multiprocessing capabilities and
configuration and detailed information about each processor and bus in the
system..
*/
struct mp {
    /*
    A 4-byte signature that identifies the floating pointer structure. In the
    case of the MP specification, it is set to "_MP_"
    */
    uchar signature[4];
    /*
    The physical address (32-bit) of the MP configuration table.
    Use an integer here to avoid GCC's object-size based array-bounds
    diagnostics on opaque firmware pointers during -m32 builds.
    */
    uint physaddr;
    /*
    The length of the floating pointer structure in bytes. It is typically set
    to 1.
    */
    uchar length;
    /*
    The version number of the MP specification supported by the system. It
    provides information about the format and features of the MP configuration
    table.
    */
    uchar specrev;
    /*
    The checksum of all bytes in the floating pointer structure. The sum of all
    bytes in the structure, including the checksum field itself, should add up
    to 0.
    */
    uchar checksum;
    /*
    The system configuration type. It indicates the format and meaning of the MP
    configuration table.
    */
    uchar type;
    /*
    An indicator of whether the system supports the Integrated Micro Channel
    Redirect Port (IMCR) feature. It is set to 1 if the IMCR is present, and 0
    otherwise.
    */
    uchar imcrp;
    /*
    Reserved fields for future use. They are typically set to 0.
    */
    uchar reserved[3];
};

/*
represents the header of the MP (Multiprocessor) configuration table. It
contains various fields that provide information about the configuration and
characteristics of the multiprocessing system.

The MP (Multiprocessor) configuration table is a data structure that provides
detailed information about the multiprocessing capabilities and configuration of
a computer system.

The MP configuration table contains multiple entries, each describing a specific
processor or bus in the system. It provides essential information for managing
and utilizing multiple processors, such as their types, features, and
connectivity.
*/
struct mpconf {
    /*
    A 4-byte field that holds the signature of the MP configuration table
    header. In this case, it is set to "PCMP".
    */
    uchar signature[4];
    /*
    A 16-bit field that specifies the total length of the MP configuration
    table, including the header itself.
    */
    ushort length;
    /*
    An 8-bit field that indicates the version number of the MP specification
    supported by the system.
    */
    uchar version;
    /*
    An 8-bit field that represents the checksum of all bytes in the MP
    configuration table, including the header. The sum of all bytes, including
    the checksum field itself, should add up to zero.
    */
    uchar checksum;
    /*
    A 20-byte field that stores the product ID or name of the system.
    */
    uchar product[20];
    /*
    A pointer to an OEM-specific table. It points to additional configuration
    information provided by the OEM (Original Equipment Manufacturer) of the
    system.
    */
    uint* oemtable;
    /*
    A 16-bit field that specifies the length of the OEM table in bytes.
    */
    ushort oemlength;
    /*
    A 16-bit field that indicates the number of entries in the MP configuration
    table. Each entry provides information about a specific processor or bus in
    the system.
    */
    ushort entry;
    /*
    A pointer to the local Advanced Programmable Interrupt Controller (APIC)
    address. It specifies the memory location of the local APIC, which is
    responsible for managing interrupts in multiprocessor systems.
    */
    uint* lapicaddr;
    /*
    A 16-bit field that represents the length of an extended table, if present.
    The extended table provides additional configuration information beyond the
    basic MP configuration table.
    */
    ushort xlength;
    /*
    An 8-bit field that serves as the checksum for the extended table.
    */
    uchar xchecksum;
    /*
    A single byte reserved for future use. It is typically set to zero.
    */
    uchar reserved;
};

/*
represents an entry in the MP (Multiprocessor) processor table. It contains
information specific to an individual processor in the system
*/
struct mpproc {
    /*
    An 8-bit field that specifies the entry type. In this case, it is set to 0
    to indicate a processor table entry.
    */
    uchar type;
    /*
    An 8-bit field that represents the local APIC (Advanced Programmable
    Interrupt Controller) ID of the processor. The local APIC is responsible for
    managing interrupts on the specific processor.
    */
    uchar apicid;
    /*
    An 8-bit field that indicates the version of the local APIC associated with
    the processor.
    */
    uchar version;
    /*
    An 8-bit field that holds various flags or attributes related to the
    processor. In this case, there is a flag defined, MPBOOT (0x02), which
    indicates that this processor is the bootstrap processor.
    */
    uchar flags;
    /*
    A 4-byte field that represents the CPU signature. It typically contains
    information about the processor's manufacturer and model.
    */
    uchar signature[4];
    /*
    A 32-bit field that stores feature flags obtained from the CPUID
    instruction. CPUID is an instruction used to query the processor for its
    capabilities and features.
    */
    uint feature;
    /*
    An 8-byte field reserved for future use. It is typically set to zero.
    */
    uchar reserved[8];
};

/*
represents an entry in the MP (Multiprocessor) I/O APIC (Advanced Programmable
Interrupt Controller) table. It contains information specific to an I/O APIC in
the system.

I/O APIC (Advanced Programmable Interrupt Controller) facilitate the management
and distribution of interrupt requests in a multiprocessor system. It serves as
a central hub for handling and routing interrupts from various I/O devices to
the appropriate processors.
*/
struct mpioapic {
    /*
    An 8-bit field that specifies the entry type. In this case, it is set to 2
    to indicate an I/O APIC table entry.
    */
    uchar type;
    /*
    An 8-bit field that represents the I/O APIC ID. It identifies a specific I/O
    APIC in the system.
    */
    uchar apicno;
    /*
    An 8-bit field that indicates the version of the I/O APIC.
    */
    uchar version;
    /*
    An 8-bit field that holds various flags or attributes related to the I/O
    APIC.
    */
    uchar flags;
    /*
    A pointer to the address of the I/O APIC. It specifies the memory location
    where the I/O APIC is mapped.
    */
    uint* addr;
};
