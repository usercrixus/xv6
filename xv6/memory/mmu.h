#ifndef mmu
#define mmu

/*
This file contains definitions for the x86 memory management unit (MMU).
*/

#define FL_IF 0x00000200    // Interrupt Enable
#define CR0_PE 0x00000001   // Protection Enable
#define CR0_WP 0x00010000   // Write Protect
#define CR0_PG 0x80000000   // Paging
#define CR4_PSE 0x00000010  // Page size extension
#define SEG_KCODE 1         // kernel code
#define SEG_KDATA 2         // kernel data+stack
#define SEG_UCODE 3         // user code
#define SEG_UDATA 4         // user data+stack
#define SEG_TSS 5           // this process's task state

// cpu->gdt[NSEGS] holds the above segments.
#define NSEGS 6

/*
The DPL, or Descriptor Privilege Level, determines the privilege level of a
segment. It can range from 0 to 3, with 0 being the highest privilege level
(typically used for the operating system) and 3 being the lowest privilege level
(typically used for user applications). This constant sets the DPL to user
level, which is 3.
*/
#define DPL_USER 0x3
#define DPL_KERNEL 0x0  // Kernel DPL

/*
The 'Executable' bit in the Type field of the segment descriptor. If this bit is
set, the segment is an executable segment, meaning that it contains code that
can be run.
Generally speaking, an executable segment (STA_X) is usually also set to be
readable (STA_R). This is because if a segment contains code (is executable),
the processor needs to be able to read that code in order to execute it. So
typically, you would see these two flags used together when setting up an
executable segment.
*/
#define STA_X 0x8
/*
The 'Write' bit in the Type field of the segment descriptor for data segments.
If this bit is set, the segment is writable, meaning that the processor can
write data to it.
*/
#define STA_W 0x2
/*
The 'Read' bit in the Type field of the segment descriptor for code segments. If
this bit is set, the segment is readable, meaning that the processor can read
data from it. For code segments, being "readable" also means that the CPU is
allowed to read instructions from this segment.
*/
#define STA_R 0x2

// System segment type bits
#define STS_T32A 0x9  // Available 32-bit TSS
#define STS_IG32 0xE  // 32-bit Interrupt Gate
#define STS_TG32 0xF  // 32-bit Trap Gate

// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/

/*
page directory index
get the first 10bit of a 32 bit adress
*/
#define PDX(va) (((uint)(va) >> PDXSHIFT) & 0x3FF)

/*
page table index
get the first 20bit of a 32 bit adress
*/
#define PTX(va) (((uint)(va) >> PTXSHIFT) & 0x3FF)

/*
used to construct a virtual address from the page directory index (d), table
index (t), and offset (o)

Example :
PGADDR(3, 2, 4096) = ((3 << 22) | (2 << 12) | 4096)
                   = (3145728 | 8192 | 4096)
                   = 3158016

So, in this example, the PGADDR macro constructs a virtual address of 3158016 by
combining the page directory index, table index, and offset according to the
specified shift values.

*/
#define PGADDR(d, t, o) ((uint)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

//--------Page directory and page table constants-----------
#define NPDENTRIES 1024  // # directory entries per page directory
#define NPTENTRIES 1024  // # PTEs per page table
#define PGSIZE 4096      // bytes mapped by a page

#define PTXSHIFT 12  // offset of PTX in a linear address
#define PDXSHIFT 22  // offset of PDX in a linear address
//--------Page directory and page table constants-----------

//--------------------Page Alignment------------------------
#define PGROUNDUP(sz)      \
    (((sz) + PGSIZE - 1) & \
     ~(PGSIZE - 1))  // rounds up sz to the nearest multiple of PGSIZE.
#define PGROUNDDOWN(a) \
    (((a)) & ~(PGSIZE - 1))  // rounds down a to the nearest multiple of PGSIZE.
//--------------------Page Alignment------------------------

//---------Page table/directory entry flags-----------
/*
PTE_P stands for Present and indicates whether the page or page table is
currently in physical memory. If this flag is not set, the entry is considered
invalid and the processor generates a page fault when an attempt is made to
access it.
*/
#define PTE_P 0x001
/*
PTE_W stands for Writeable and indicates whether the memory associated with the
page or page table can be written to. If this flag is not set, the memory is
read-only.
*/
#define PTE_W 0x002
/*
PTE_U stands for User and indicates whether the memory can be accessed by
user-level processes. If this flag is not set, the memory can only be accessed
by kernel-level processes.
*/
#define PTE_U 0x004
/*
PTE_PS stands for Page Size and indicates whether the entry is a page table
entry or a page directory entry. If this flag is set, the entry is a page
directory entry, and if it is not set, the entry is a page table entry.
*/
#define PTE_PS 0x080
//---------Page table/directory entry flags-----------

/*
extracts the address part of the entry by masking the lower 12 bits which
represent the offset within the page. In other words, this macro gives you the
physical address of the page frame that is being described by the entry. This
assumes that the page frame size is 4KB, which is the default size used by most
modern systems.
*/
#define PTE_ADDR(pte) ((uint)(pte) & ~0xFFF)
/*
extracts the flags part of the entry by masking the address part.
The flags indicate various attributes of the page, such as whether it is present
in physical memory (PTE_P), whether it is writeable (PTE_W), whether it can be
accessed by user-level code (PTE_U), and whether it uses a larger page size
(PTE_PS
*/
#define PTE_FLAGS(pte) ((uint)(pte) & 0xFFF)

#ifndef __ASSEMBLER__

/*
Segment descriptor, which is used in x86 processors to define the
characteristics and memory layout of a segment.
Please refer to GDT (Global Descriptor Table) for more details.
*/
struct segdesc {
    /*
    The low bits of the segment limit. The segment limit specifies the size of
    the segment in bytes.
    */
    uint lim_15_0 : 16;
    /*
    The low bits of the segment base address. The segment base address is the
    starting address of the segment in memory.
    */
    uint base_15_0 : 16;
    /*
    Represents the middle bits of the segment base address.
    */
    uint base_23_16 : 8;
    /*
    Specifies the segment type using constants defined as STS_ (Segment Type
    Status) values. The segment type determines the access rights and behavior
    of the segment, such as code segment, data segment, system segment, etc.
    */
    uint type : 4;
    /*
    Indicates whether the segment is for system use (0) or application use (1).
    System segments are used by the operating system, while application segments
    are used by user programs.
    */
    uint s : 1;
    /*
    Represents the Descriptor Privilege Level, which specifies the privilege
    level required to access the segment. Privilege levels range from 0 to 3,
    with 0 being the highest privilege (kernel mode) and 3 being the lowest
    privilege (user mode).
    */
    uint dpl : 2;
    /*
    Indicates whether the segment is present in memory (1) or not (0). If the
    segment is not present, accessing it will trigger a segment fault.
    */
    uint p : 1;
    /*
    Represents the high bits of the segment limit.
    */
    uint lim_19_16 : 4;
    /*
    Unused and available for software use. It can be used by the operating
    system or applications for additional purposes.
    */
    uint avl : 1;
    /*
    Reserved and should be set to 0.
    */
    uint rsv1 : 1;
    /*
    Indicates whether the segment is a 16-bit segment (0) or a 32-bit segment
    (1). It affects how memory addresses are interpreted within the segment.
    */
    uint db : 1;
    /*
    Secifies the granularity of the segment limit. When set to 1, the segment
    limit is scaled by 4K (4096 bytes), giving a larger range of memory that can
    be addressed.
    */
    uint g : 1;
    /*
    Represents the high bits of the segment base address.
    */
    uint base_31_24 : 8;
};

/*
Used to generate a segment descriptor that follows the format required by the
x86 architecture for Global Descriptor Table (GDT) and Local Descriptor Table
(LDT) entries.

    type: This field describes the type of the segment. It can represent code or
data and has attributes like "readable/writable" or "executable".

    base: This field defines the 32 bit starting address of the segment.

    lim: This field contains the limit of the segment. The actual limit is
calculated based on the lim value and granularity bit (G bit). If G bit is 0,
the limit is in bytes; if it's 1, the limit is in blocks of 4KB.

    dpl: This field stands for Descriptor Privilege Level. It has a range of
0-3, with 0 being the highest privilege level (kernel) and 3 being the lowest
(user programs).

Structure format :
Bits        Field        Description
31 - 24     BASE[31:24]  Upper 8 bits of the base address of the segment.
23 - 22     Flags        Flags for granularity (G) and operand size (D).
21 - 20     Limit[19:16] Upper 4 bits of the limit of the segment.
19 - 16     Not used     Reserved space.
15 - 12     AVL          Available for use by system software.
11 - 10     Segment Len  Reserved for future use.
9 - 8       DPL          Descriptor Privilege Level.
7           S            Descriptor type.
6 - 0       Type         Contains flags to define the type of the segment.
*/
#define SEG(type, base, lim, dpl)                                            \
    (struct segdesc) {                                                       \
        ((lim) >> 12) & 0xffff, (uint)(base) & 0xffff,                       \
            ((uint)(base) >> 16) & 0xff, type, 1, dpl, 1, (uint)(lim) >> 28, \
            0, 0, 1, 1, (uint)(base) >> 24                                   \
    }
#define SEG16(type, base, lim, dpl)                                            \
    (struct segdesc) {                                                         \
        (lim) & 0xffff, (uint)(base) & 0xffff, ((uint)(base) >> 16) & 0xff,    \
            type, 1, dpl, 1, (uint)(lim) >> 16, 0, 0, 1, 0, (uint)(base) >> 24 \
    }

/*
pte_t is a typedef in C programming that stands for Page Table Entry Type.
It is an unsigned integer type used to represent a page table or page directory
entry in an operating system's memory management unit (MMU).
*/
typedef uint pte_t;

/*
The taskstate struct describes the format of the task state segment (TSS) used
in x86 operating systems to store the state of a task during a context switch.
The struct includes fields for storing the state of the CPU registers,
such as eax, ecx, edx, ebx, esi, edi, esp, and ebp,
as well as segment selectors such as es, cs, ss, ds, fs, and gs.
It also includes fields for storing the stack pointers and more.
*/
struct taskstate {
    uint link;   // Old ts selector
    uint esp0;   // Stack pointers and segment selectors
    ushort ss0;  //   after an increase in privilege level
    ushort padding1;
    uint* esp1;
    ushort ss1;
    ushort padding2;
    uint* esp2;
    ushort ss2;
    ushort padding3;
    void* cr3;  // Page directory base
    uint* eip;  // Saved state from last task switch
    uint eflags;
    uint eax;  // More saved state (registers)
    uint ecx;
    uint edx;
    uint ebx;
    uint* esp;
    uint* ebp;
    uint esi;
    uint edi;
    ushort es;  // Even more saved state (segment selectors)
    ushort padding4;
    ushort cs;
    ushort padding5;
    ushort ss;
    ushort padding6;
    ushort ds;
    ushort padding7;
    ushort fs;
    ushort padding8;
    ushort gs;
    ushort padding9;
    ushort ldt;
    ushort padding10;
    ushort t;     // Trap on task switch
    ushort iomb;  // I/O map base address
};

/*
This is a C struct definition for a gatedesc structure used in operating system
development to represent various types of interrupt and trap gates in the
Interrupt Descriptor Table (IDT). The structure consists of several bit fields
that are used to store different pieces of information about the gate. Interupt
descriptor table is often like : struct gatedesc idt[256];

off_15_0 and off_31_16:
These fields store the lower 16 bits (off_15_0) and the higher 16 bits
(off_31_16) of the offset (address) of the interrupt handler function. When
combined, they form the 32-bit address of the interrupt or exception handler in
memory. This is where the CPU will jump to handle the interrupt or exception.
*/
struct gatedesc {
    uint off_15_0 : 16;   // low 16 bits of offset in segment
    uint cs : 16;         // code segment selector
    uint args : 5;        // args, 0 for interrupt/trap gates
    uint rsv1 : 3;        // reserved(should be zero)
    uint type : 4;        // type(STS_{IG32,TG32})
    uint s : 1;           // must be 0 (system)
    uint dpl : 2;         // descriptor(meaning new) privilege level
    uint p : 1;           // Present
    uint off_31_16 : 16;  // high bits of offset in segment
};

/*
used to set up an interrupt or trap gate descriptor in the Interrupt Descriptor
Table (IDT). The IDT is a data structure used by the processor to handle
interrupts and exceptions in the operating system.

The macro takes the following parameters:

    gate: The gate descriptor structure to be set up.
    istrap: A flag indicating whether the gate is a trap (exception) gate (1) or
an interrupt gate (0).
    sel: The code segment selector for the interrupt/trap handler.
    off: The offset in the code segment where the interrupt/trap handler is
located.
    d: The Descriptor Privilege Level (DPL) required for software to invoke
this gate explicitly using an int instruction.

The macro then initializes the various fields of the gate descriptor structure
as follows:

    off_15_0: The lower 16 bits of the offset, indicating the address of the
handler within the code segment.
    cs: The code segment selector.
    args: Reserved field, set to 0.
    rsv1: Reserved field, set to 0.
    type: The type of the gate, either STS_TG32 for trap gate or STS_IG32 for
interrupt gate.
    s: Descriptor type flag, set to 0.
    dpl: The Descriptor Privilege Level required to invoke the gate.
    p: Present flag, set to 1 to mark the gate as valid.
    off_31_16: The upper 16 bits of the offset, indicating the high part of the
handler's address.
*/
#define SETGATE(gate, istrap, sel, off, d)            \
    {                                                 \
        (gate).off_15_0 = (uint)(off) & 0xffff;       \
        (gate).cs = (sel);                            \
        (gate).args = 0;                              \
        (gate).rsv1 = 0;                              \
        (gate).type = (istrap) ? STS_TG32 : STS_IG32; \
        (gate).s = 0;                                 \
        (gate).dpl = (d);                             \
        (gate).p = 1;                                 \
        (gate).off_31_16 = (uint)(off) >> 16;         \
    }

#endif
#endif