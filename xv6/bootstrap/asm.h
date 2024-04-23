/*
Assembler macros used for setting up segment descriptors in x86 architecture.
These segment descriptors are a part of the segmentation mechanism in x86, which
is used for memory management and protection.
*/

/*
Defines a null segment descriptor. In x86 segmentation, the first segment
descriptor in the Global Descriptor Table (GDT) is traditionally a null
descriptor, serving as a placeholder and ensuring that a segment selector with
a value of 0 does not point to a valid segment.
*/
#define SEG_NULLASM \
    .word 0, 0;     \
    .byte 0, 0, 0, 0

/*
Constructs the 8-byte segment descriptor using the segment type, base address,
and limit provided as arguments.

The 0xC0 means the limit is in 4096-byte units and (for executable segments)
32-bit mode.
*/
#define SEG_ASM(type, base, lim)                      \
    .word(((lim) >> 12) & 0xffff), ((base) & 0xffff); \
    .byte(((base) >> 16) & 0xff), (0x90 | (type)),    \
        (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)

#define STA_X 0x8  // Executable segment
#define STA_W 0x2  // Writeable (non-executable segments)
#define STA_R 0x2  // Readable (executable segments)