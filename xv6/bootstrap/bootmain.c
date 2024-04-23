/*
Designed for loading an ELF (Executable and Linkable Format) kernel image from
disk into memory in a protected 32-bit mode environment.

Part of the boot block, along with bootasm.S, which calls bootmain(). bootasm.S
has put the processor into protected 32-bit mode. bootmain() loads an ELF kernel
image from the disk starting at sector 1 and then jumps to the kernel entry
routine.
*/

#include "../type/types.h"
#include "./processus/elf.h"
#include "x86.h"
#include "../memory/memlayout.h"

#define SECTSIZE 512

/*
used to wait until the disk drive is ready for data transfer. This function is
crucial when performing read operations from the disk.
*/
void waitdisk(void) {
    /*
    Port 0x1F7: This port is the disk's status register when interfacing with an
    ATA (AT Attachment) disk drive in x86 systems. The status register contains
    several flags indicating the state of the disk.

    0x40 in binary is 01000000, where the second most significant bit represents
    the "ready" status of the drive. The loop waits until the drive is ready and
    not busy.
    */
    while ((inb(0x1F7) & 0xC0) != 0x40)
        ;
}

/*
Read a single sector from the disk into a specified memory location (dst). It
operates by sending commands to the disk controller and then reading the data
from the disk into memory.

dst is the destination memory address where the data read from the disk will be
stored.

offset represents the logical block addressing (LBA) of the sector on the disk.
In simple terms, offset is the index of the disk sector that the boot loader
intends to read. For example, an offset of 1 would typically refer to the first
sector of the disk (sector numbers usually start from 0).
*/
void readsect(void* dst, uint offset) {
    waitdisk();
    /*
    Sets the sector count to 1. This tells the disk controller that we want to
    read only one sector.
    */
    outb(0x1F2, 1);
    /*
    Sets the low 8 bits of the sector number to read.
    */
    outb(0x1F3, offset);
    outb(0x1F4, offset >> 8);   // Sets the next 8 bits of the sector number.
    outb(0x1F5, offset >> 16);  // Sets the next 8 bits of the sector number.
    /*
    Sets the high bits of the sector number. The 0xE0 masks the high bits to set
    up for a read operation and to select the master disk.
    */
    outb(0x1F6, (offset >> 24) | 0xE0);
    /*
    Sends the read command (0x20) to the disk controller.
    */
    outb(0x1F7, 0x20);

    waitdisk();
    /*
    Reads data from the disk. insl is an instruction to read a 32-bit value
    from the I/O port. Here, it reads from port 0x1F0 (data port) into the
    destination address dst. The number of 32-bit values to read is calculated
    by dividing the sector size by 4 (since each insl reads 4 bytes).
    */
    insl(0x1F0, dst, SECTSIZE / 4);
}

/*
responsible for reading a specified number of bytes from the kernel stored on
the disk into a physical memory address.

pa Specifies the physical address (pa) in memory where the data read from the
disk should be stored.

count indicates the number of bytes (count) to read from the disk.

offset Specifies the offset on the disk, in bytes, where the reading should
begin.
*/
void readseg(uchar* pa, uint count, uint byteOffset) {
    /*
    Determines the end physical address up to which data will be read.
    */
    uchar* endPhysicalAdress = pa + count;
    /*
    Adjusts the physical address backward to the nearest sector boundary.
    */
    pa -= byteOffset % SECTSIZE;
    /*
    Converts the byte offset to a sector number, since disk operations are based
    on sector numbers. The kernel is assumed to start at sector 1, so 1 is
    added.
    */
    uint setctorOffset = (byteOffset / SECTSIZE) + 1;
    /*
    reads data from the disk into memory
    */
    while (pa < endPhysicalAdress) {
        readsect(pa, setctorOffset);
        pa += SECTSIZE;
        setctorOffset++;
    }
}

void bootmain(void) {
    /*
    Pointer to elfhdr structure (ELF header), which contains information about
    the ELF executable.
    */
    struct elfhdr* elf = (struct elfhdr*)0x10000;
    /*
    The boot loader reads the first page (4096 bytes) from the disk into a
    scratch space at address 0x10000. This space contains the ELF header and the
    program headers.
    */
    readseg((uchar*)elf, 4096, 0);
    /*
    The boot loader checks if the loaded header has the correct ELF magic
    number. If not, it returns, and the error is handled by bootasm.S.
    */
    if (elf->magic != ELF_MAGIC)
        return;  // let bootasm.S handle error

    /*
    initializes a pointer programHeader to the first program header in the ELF
    file.
    */
    struct proghdr* programHeader = (struct proghdr*)((uchar*)elf + elf->phoff);
    /*
    endProgramHeader point just past the last program header.
    */
    struct proghdr* endProgramHeader = programHeader + elf->phnum;

    /*
    load each segment of the kernel (or another program) from the disk into
    memory, as specified in the program headers of an ELF (Executable and
    Linkable Format) file.
    */
    while (programHeader < endProgramHeader) {
        /*
        Gets the physical address (paddr) where this segment should be loaded.
        */
        uchar* pa = (uchar*)programHeader->paddr;
        readseg(pa, programHeader->filesz, programHeader->off);
        /*
        If the segment's memory size (memsz) is larger than the file size
        (filesz), this indicates there's additional memory space allocated for
        the segment that isn't filled by the ELF file. This extra space needs to
        be zeroed.
        */
        if (programHeader->memsz > programHeader->filesz)
            stosb(pa + programHeader->filesz, 0,
                  programHeader->memsz - programHeader->filesz);
        programHeader++;
    }
    /*
    Function pointer to the entry point of the kernel.
    */
    void (*entry)(void) = (void (*)(void))(elf->entry);
    entry();
    // Does not return!
}