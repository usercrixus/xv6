/*
Simple PIO-based (non-DMA) IDE driver code.

ide.c is typically a source file in an operating system's codebase that deals
with interacting with IDE (Integrated Drive Electronics) hardware. IDE is a
standard interface for connecting storage devices, like hard drives and optical
drives, to a computer's motherboard.
*/

#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "../memory/memlayout.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "../x86.h"
#include "../systemCall/traps.h"
#include "../synchronization/spinlock.h"
#include "../synchronization/sleeplock.h"
#include "../fileSystem/fs.h"
#include "../fileSystem/buf.h"
#include "../userLand/user.h"

/*
A sector is the smallest addressable unit on a disk. It represents a fixed-size
portion of the disk's surface and is typically 512 bytes in size. Sectors are
numbered sequentially from the start of the disk, allowing for direct access to
specific sectors using their sector numbers.

In disk storage systems, a sector is used to define an addressable unit, whereas
a block is used to represent a specific size of data.
*/
#define SECTOR_SIZE 512
#define IDE_BSY 0x80
#define IDE_DRDY 0x40
/*
The bit pattern for the Device Fault (DF) bit in the status register. The DF bit
is set when a device fault occurs during a disk operation. A device fault
indicates an error condition related to the disk drive, such as a CRC (Cyclic
Redundancy Check) error or a data transmission problem.
*/
#define IDE_DF 0x20
/*
The bit pattern for the Error (ERR) bit in the status register. The ERR bit is
set when an error occurs during a disk operation. It indicates various types of
errors, such as a command error, data error, or media error.
*/
#define IDE_ERR 0x01
/*
The IDE command code for a single-sector read operation. When this command is
sent to the IDE controller, it instructs it to read a single sector from the
disk.
*/
#define IDE_CMD_READ 0x20
/*
The IDE command code for a single-sector write operation. When this command is
sent to the IDE controller, it instructs it to write a single sector to the
disk.
*/
#define IDE_CMD_WRITE 0x30
/*
The IDE command code for a multiple-sector read operation. When this command is
sent to the IDE controller, it instructs it to read multiple consecutive sectors
from the disk.
*/
#define IDE_CMD_RDMUL 0xc4
/*
The IDE command code for a multiple-sector write operation. When this command is
sent to the IDE controller, it instructs it to write multiple consecutive
sectors to the disk.
*/
#define IDE_CMD_WRMUL 0xc5
/*
used to protect the IDE disk operation queue (idequeue). When a process needs to
perform a disk operation, it must acquire the idelock before it can access the
idequeue. This ensures that no two processes can access or modify the idequeue
at the same time, thereby preventing race conditions and data corruption.
*/
static struct spinlock idelock;
/*
pointer to the head of a queue (a linked list) of buffer structures (struct buf)
that represent disk blocks that need to be read from or written to the IDE disk.
*/
static struct buf* idequeue;

/*
utility function used in the xv6 operating system to wait for the IDE
(Integrated Drive Electronics) controller to become ready and check for any
potential errors during disk I/O operations.

Argument: int checkerr
    This argument determines whether error checking is enabled or disabled
    during the wait operation. If checkerr is non-zero, it indicates that error
    checking is enabled. If checkerr is zero, error checking is disabled, and
    the function will not perform error checks.

Return Value:
    The function returns an integer value.
    If the function completes successfully without any errors or if error
    checking is disabled (checkerr is zero), it returns 0 to indicate successful
    completion. If error checking is enabled (checkerr is non-zero) and an error
    or device fault occurs during the operation, it returns -1 to indicate the
    error condition.
*/
static int idewait(int checkerr) {
    int r;
    /*
    enters a loop that repeatedly reads the status register of the IDE
    controller (at port 0x1f7) and checks if the busy (IDE_BSY) and drive ready
    (IDE_DRDY) bits are set. The loop continues until the drive ready bit is
    set, indicating that the IDE controller is no longer busy and ready for the
    next command. The status register is read by inb which reads a byte from the
    specified I/O port.
    */
    while (((r = inb(0x1f7)) & (IDE_BSY | IDE_DRDY)) != IDE_DRDY) {
        // wait
    }
    /*
    executed if the checkerr parameter is non-zero (indicating that error
    checking is enabled) and either the device fault (IDE_DF) or error (IDE_ERR)
    bits are set in the status register. It checks if there was a device fault
    or error during the operation.
    */
    if (checkerr && (r & (IDE_DF | IDE_ERR)) != 0)
        return -1;

    return 0;
}

/*
IDE, which stands for Integrated Drive Electronics, refers to a standard
interface for connecting storage devices like hard drives and optical drives to
a computer's motherboard. The IDE interface is also known by the name ATA
(Advanced Technology Attachment). It was a very common interface used in older
computers before being largely replaced by SATA (Serial ATA) in newer systems.
*/
void ideinit(void) {
    initlock(&idelock, "ide");
    ioapicenable(IRQ_IDE, ncpu - 1);
    idewait(0);
}

/*
Responsible for starting an IDE disk I/O request for the specified buffer b.
This function assumes that the caller already holds the idelock, which is used
to synchronize disk access.
*/
static void idestart(struct buf* b) {
    /*
    Checks if b is a valid buffer. If b is NULL, it indicates an error, and the
    system panics.

    checks if the blockno field of b is within the valid range of disk blocks.
    FSSIZE represents the total number of disk blocks in the file system. If
    blockno is out of bounds, it indicates an error, and the system panics.
    */
    if (b == 0)
        panic("idestart");
    if (b->blockno >= FSSIZE)
        panic("incorrect blockno");
    /*
    calculates the number of disk sectors per block. BSIZE represents the size
    of a disk block, and SECTOR_SIZE represents the size of a disk sector. The
    sector size is typically smaller than or equal to the block size in disk
    storage systems.
    */
    int sector_per_block = BSIZE / SECTOR_SIZE;
    /*
    calculates the starting sector of the disk I/O request based on the block
    number and the number of sectors per block.
    */
    int sector = b->blockno * sector_per_block;
    /*
    determine the appropriate IDE command to use based on the number of sectors
    per block. If there's only one sector per block, the command will be either
    IDE_CMD_READ or IDE_CMD_WRITE, otherwise, it will be IDE_CMD_RDMUL or
    IDE_CMD_WRMUL.
    */
    int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ : IDE_CMD_RDMUL;
    int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

    /*
    checks if the number of sectors per block exceeds the limit of 7. If it
    does, it indicates an error, and the system panics.
    */
    if (sector_per_block > 7)
        panic("idestart");

    idewait(0);
    /*
    writes a value of 0 to the control register (port 0x3f6) of the IDE
    controller. Writing to this register generates an interrupt, which is a way
    to notify the IDE controller that there is a new command to be processed.
    */
    outb(0x3f6, 0);
    /*
    writes the number of sectors per block to the sector count register (port
    0x1f2) of the IDE controller. It specifies the number of sectors to be
    transferred in the upcoming disk I/O operation.
    */
    outb(0x1f2, sector_per_block);
    /*
    writes the low byte of the sector address to the low-order data register
    (port 0x1f3) of the IDE controller. It sets the starting sector of the disk
    I/O operation.
    */
    outb(0x1f3, sector & 0xff);
    /*
    writes the middle byte of the sector address to the middle-order data
    register (port 0x1f4) of the IDE controller. It specifies the middle bits of
    the sector address.
    */
    outb(0x1f4, (sector >> 8) & 0xff);
    /*
    writes the high byte of the sector address to the high-order data register
    (port 0x1f5) of the IDE controller. It sets the high bits of the sector
    address.
    */
    outb(0x1f5, (sector >> 16) & 0xff);
    /*
    writes the device and head information to the device/head register (port
    0x1f6) of the IDE controller. It sets the device, head, and high bits of the
    sector address.

    The value 0xe0 sets the highest three bits of the register to indicate that
    it is an IDE device.
    ((b->dev & 1) << 4) sets the fourth bit of the register based on the device
    number (b->dev).
    ((sector >> 24) & 0x0f) sets the lower four bits of the register based on
    the high bits of the sector address.
    */
    outb(0x1f6, 0xe0 | ((b->dev & 1) << 4) | ((sector >> 24) & 0x0f));
    /*
    Responsible for initiating a disk I/O operation based on the state of the b
    buffer structure.
    */
    if (b->flags & B_DIRTY) {
        /*
        If the buffer is dirty (modified), this line sends the write command
        (write_cmd) to the command register (port 0x1f7) of the IDE controller.
        It instructs the IDE controller to perform a write operation.
        */
        outb(0x1f7, write_cmd);
        /*
        After sending the write command, this line performs the actual data
        transfer from the buffer to the disk. It uses the outsl function to
        write a block of data (BSIZE bytes) from the buffer's data field to the
        data register (port 0x1f0) of the IDE controller. The BSIZE / 4 argument
        indicates the number of 32-bit (4-byte) words to transfer.
        */
        outsl(0x1f0, b->data, BSIZE / 4);
    } else {
        /*
        If the buffer is not dirty, indicating that it is a read operation, this
        line sends the read command (read_cmd) to the command register (port
        0x1f7) of the IDE controller. It instructs the IDE controller to perform
        a read operation.
        */
        outb(0x1f7, read_cmd);
    }
}

// Interrupt handler.
void ideintr(void) {
    struct buf* b;

    // First queued buffer is the active request.
    acquire(&idelock);

    if ((b = idequeue) == 0) {
        release(&idelock);
        return;
    }
    idequeue = b->qnext;

    // Read data if needed.
    if (!(b->flags & B_DIRTY) && idewait(1) >= 0)
        insl(0x1f0, b->data, BSIZE / 4);

    // Wake process waiting for this buf.
    b->flags |= B_VALID;
    b->flags &= ~B_DIRTY;
    wakeup(b);

    // Start disk on next buf in queue.
    if (idequeue != 0)
        idestart(idequeue);

    release(&idelock);
}

/*
responsible for initiating disk read and write operations

IDE stands for Integrated Drive Electronics. It's a standard interface for
connecting a motherboard to storage devices such as hard drives and CD-ROM/DVD
drives. The IDE standard was widely used in the past, but has been largely
replaced by SATA (Serial ATA) in more modern systems.

In IDE, the disk controller, which manages data transfers between the computer
and the disk, is integrated into the disk drive itself, rather than being part
of the computer's motherboard. This made IDE drives relatively easy to install
and use.

In the context of xv6 or other operating systems, "IDE" often refers to the disk
drive that the operating system is using for storage. The operating system needs
to be able to read data from and write data to this drive, so it includes code
for interacting with the drive through the IDE interface.

struct buf* b : that is passed to this function is the buffer for the disk block
that needs to be read from or written to. The function checks the flags to
determine whether a read or write operation is necessary, initiates the
operation, and waits for it to complete.
*/
void iderw(struct buf* b) {
    /*
    used later to traverse and modify the IDE device queue.
    */
    struct buf* pp;
    /*
    Checks whether the lock on the buffer b is held. If not, it causes a kernel
    panic because the buffer should be locked when iderw is called to ensure
    thread-safety.

    checks if the buffer is already marked as valid and not dirty. If it is,
    then there's nothing to do because the buffer already contains the correct
    data, and the function causes a kernel panic.

    checks if the device for the buffer is not 0 (the first IDE device) and if
    the second IDE device is not present. If these conditions are met, it means
    that there's an attempt to access a non-existent device, and the function
    causes a kernel panic.
    */
    if (!holdingsleep(&b->lock))
        panic("iderw: buf not locked");
    if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
        panic("iderw: nothing to do");

    acquire(&idelock);

    /*
    appends the buffer b to the IDE device queue (idequeue).
    */
    pp = idequeue;
    b->qnext = 0;
    if (!pp) {
        idequeue = b;
    } else {
        while (pp->qnext) {
            pp = pp->qnext;
        }
        pp->qnext = b;
    }

    // Start disk if necessary.
    if (idequeue == b)
        idestart(idequeue);

    // Wait for request to finish.
    while ((b->flags & (B_VALID | B_DIRTY)) != B_VALID) {
        sleep(b, &idelock);
    }

    release(&idelock);
}
