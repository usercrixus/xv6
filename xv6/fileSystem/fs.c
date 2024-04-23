// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "stat.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "../synchronization/spinlock.h"
#include "../synchronization/sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "file.h"
#include "../userLand/ulib.h"
#include "../synchronization/spinlock.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

/*
there should be one superblock per disk device, but we run with only one device
*/
struct superblock sb;
/*
Represents the inode cache in the file system.
*/
struct {
    /*
    a spin lock used to protect the inode cache from concurrent access by
    multiple threads
    */
    struct spinlock lock;
    /*
    array of inodes that represents the actual cache of inodes in the file
    system.
    */
    struct inode inode[NINODE];
} icache;

/*
read the superblock from the filesystem.

int dev: This is the device identifier from which the superblock is to be read.
In a file system, different devices can be represented by different integers.
For example, the hard disk could be represented by one number, and a USB stick
could be represented by another.

struct superblock* sb: This is a pointer to a superblock structure. The
superblock is a data structure in a filesystem that contains information about
the filesystem itself (like the size of the filesystem, the number of inodes it
contains, etc.). The function will fill this structure with the information read
from the device.
*/
void readsb(int dev, struct superblock* sb) {
    /*
    In the context of file systems, particularly in Unix-like operating systems
    such as xv6, the first block (block 0) on the disk is typically reserved for
    boot-related information. The second block (block 1) typically contains the
    superblock, which holds critical information about the file system.
    */
    struct buf* bp = bread(dev, 1);
    memmove(sb, bp->data, sizeof(*sb));
    brelse(bp);
}

/*
used to zero out the contents of a block on a storage device.
*/
static void bzero(int dev, int bno) {
    /*
    read the block with the specified device (dev) and block number (bno) into a
    buffer cache entry (bp). The bread function retrieves the block from the
    storage device and stores it in memory for manipulation.
    */
    struct buf* bp = bread(dev, bno);
    /*
    The memset function is then used to set all bytes within the data field of
    the buffer cache entry (bp->data) to 0. This effectively zeroes out the
    block's contents, preparing it to be written back to the storage device.
    */
    memset(bp->data, 0, BSIZE);
    /*
    write the modified block to the log. This step is typically part of a file
    system's logging mechanism to ensure durability and consistency of disk
    writes.
    */
    log_write(bp);
    brelse(bp);
}

/*
Responsible for allocating a new disk block on the file system.
*/
static uint balloc(uint dev) {
    /*
    used to represent the bitmask for checking block availability.
    */
    int m;
    /*
    used to hold the buffer of the bitmap block being processed.
    */
    struct buf* bp = 0;
    for (int b = 0; b < sb.size; b += BPB) {
        /*
        get the bitmap
        */
        bp = bread(dev, BBLOCK(b, sb));
        for (int bi = 0; bi < BPB && b + bi < sb.size; bi++) {
            /*
            calculates the bitmask m by shifting the value 1 left by (bi % 8)
            bits. This creates a bitmask with only the bit at position (bi % 8)
            set to 1.
            */
            m = 1 << (bi % 8);
            /*
            checks if the corresponding bit within the byte of the bitmap block
            is zero, indicating that the block is free.
            */
            if ((bp->data[bi / 8] & m) == 0) {
                /*
                sets the corresponding bit within the byte to 1 by performing a
                bitwise OR operation between the byte bp->data[bi / 8] and the
                bitmask m. This marks the block as in use.
                */
                bp->data[bi / 8] |= m;
                log_write(bp);
                brelse(bp);
                bzero(dev, b + bi);
                return b + bi;
            }
        }
        brelse(bp);
    }
    panic("balloc: out of blocks");
}

/*
Responsible for freeing a disk block in the file system.

dev : represents the device number
b : the block number to be freed.
*/
static void bfree(int dev, uint b) {
    /*
    reads the block bitmap disk block associated with the block number b by
    calling the bread function.
    */
    struct buf* bp = bread(dev, BBLOCK(b, sb));
    /*
    calculates the bit index bi within the block bitmap for the given block
    number b
    */
    int bi = b % BPB;
    /*
    calculates a mask m by left-shifting the value 1 by the bit index within a
    byte (bi % 8). The modulo operation % is used to ensure that the value of bi
    is in the range of 0 to 7, representing the bits within a byte.
    */
    int m = 1 << (bi % 8);
    /*
    checks if the bit representing the block in the block bitmap is already
    clear (indicating a free block).
    */
    if ((bp->data[bi / 8] & m) == 0)
        panic("freeing free block");
    /*
    If the block is not free, it clears the corresponding bit in the block
    bitmap. The expression ~m performs a bitwise NOT operation on the mask m,
    creating a mask where all bits are set except the one represented by m.
    Then, the bitwise AND operation &= clears the corresponding bit in the byte
    of the block bitmap.
    */
    bp->data[bi / 8] &= ~m;
    log_write(bp);
    brelse(bp);
}

/*
Responsible for initializing the file system's in-memory inode cache and other
related data structures

An inode cache, sometimes referred to as an "icache", is a memory-based data
structure used by the operating system to reduce the number and frequency of
disk accesses when looking up inode information.

Inodes (index nodes) are key data structures in Unix-based file systems such as
Ext2/3/4, XFS, and others. An inode contains metadata about a file or directory,
such as its size, timestamp, owner, permission, type (whether it's a file,
directory, link, etc.), and the location of the data blocks that actually
contain the file's data. Whenever the OS needs to access a file, it must first
read the file's inode to find out where the file's data is located on disk.
*/
void iinit(int dev) {
    initlock(&icache.lock, "icache");
    /*
    iterates over the array of inodes in the inode cache (icache.inode) and
    initializes a sleep lock for each inode.
    */
    for (int i = 0; i < NINODE; i++) {
        initsleeplock(&icache.inode[i].lock, "inode");
    }

    readsb(dev, &sb);
    /*
    print the stat of the disk device
    */
    cprintf(
        "sb: size %d nblocks %d ninodes %d nlog %d logstart %d inodestart %d "
        "bmap start %d\n",
        sb.size, sb.nblocks, sb.ninodes, sb.nlog, sb.logstart, sb.inodestart,
        sb.bmapstart);
}

/*
Used to find and return the in-memory copy of an inode with a specified inode
number (inum) on a given device (dev). It operates on the inode cache (icache)
and does not lock or read the inode from disk. The function follows a caching
mechanism to optimize inode access and avoid redundant disk operations.
*/
static struct inode* iget(uint dev, uint inum) {
    struct inode* emptyICache = 0;

    acquire(&icache.lock);

    emptyICache = 0;
    for (struct inode* ip = &icache.inode[0]; ip < &icache.inode[NINODE];
         ip++) {
        if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            ip->ref++;
            release(&icache.lock);
            return ip;
        }
        if (emptyICache == 0 && ip->ref == 0)
            emptyICache = ip;
    }

    /*
    If no empty slot was found, it indicates that the inode cache is full, and
    an error is triggered (panic("iget: no inodes")).
    */
    if (emptyICache == 0)
        panic("iget: no inodes");

    /*
    Set the empty inode cache to the found inum
    */
    emptyICache->dev = dev;
    emptyICache->inum = inum;
    emptyICache->ref = 1;
    emptyICache->valid = 0;
    release(&icache.lock);

    return emptyICache;
}

/*
Responsible for allocating a new inode on the specified device (dev) and marking
it as allocated by setting its type to the specified type
*/
struct inode* ialloc(uint dev, short type) {
    struct buf* bp;
    struct dinode* dip;

    for (int inum = 1; inum < sb.ninodes; inum++) {
        bp = bread(dev, IBLOCK(inum, sb));
        dip = (struct dinode*)bp->data + inum % INODE_PER_BLOCK;
        /*
        checks if the type field of the dinode struct is zero, indicating a free
        inode.
        */
        if (dip->type == 0) {
            /*
            clears the memory occupied by the dinode struct by setting all its
            bytes to zero
            */
            memset(dip, 0, sizeof(*dip));
            dip->type = type;
            log_write(bp);
            brelse(bp);
            return iget(dev, inum);
        }
        brelse(bp);
    }
    panic("ialloc: no inodes");
}

/*
responsible for updating the on-disk representation of an inode to reflect the
changes made to its corresponding in-memory inode. It ensures that any
modifications made to the ip (in-memory inode) are persisted on disk.
*/
void iupdate(struct inode* ip) {
    struct buf* bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    /*
    obtains a pointer (dip) to the specific inode within the block. This is
    achieved by casting the data pointer of the buffer (bp->data) to a pointer
    to struct dinode and adding the offset within the block calculated using
    ip->inum % INODE_PER_BLOCK.
    */
    struct dinode* dip = (struct dinode*)bp->data + ip->inum % INODE_PER_BLOCK;
    dip->type = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->nlink = ip->nlink;
    dip->size = ip->size;
    memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
    log_write(bp);
    brelse(bp);
}

/*
Used to increment the reference count of an inode (ip). It is commonly used to
increase the reference count when a new reference to the same inode is needed,
following the ip = idup(ip1) idiom.
*/
struct inode* idup(struct inode* ip) {
    acquire(&icache.lock);
    ip->ref++;
    release(&icache.lock);
    return ip;
}

/*
lock the given inode (ip) and ensure that its data is loaded into memory.

In a file system, an inode represents a file or a directory, and it contains
important metadata about the file, such as its type, size, permissions, and disk
block addresses. When a thread wants to access or modify an inode, it needs to
acquire a lock to ensure exclusive access and prevent data corruption due to
concurrent modifications.
*/
void ilock(struct inode* ip) {
    if (ip == 0 || ip->ref < 1)
        panic("ilock");

    acquiresleep(&ip->lock);

    if (ip->valid == 0) {
        struct buf* bp = bread(ip->dev, IBLOCK(ip->inum, sb));
        struct dinode* dip =
            (struct dinode*)bp->data + ip->inum % INODE_PER_BLOCK;
        if (dip->type == 0)
            panic("ilock: no type");
        ip->type = dip->type;
        ip->major = dip->major;
        ip->minor = dip->minor;
        ip->nlink = dip->nlink;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        brelse(bp);
        ip->valid = 1;
    }
}

/*
Responsible for releasing the lock on the given inode (ip). It is used to unlock
an inode after it has been locked to allow other processes to access it.
*/
void iunlock(struct inode* ip) {
    /*
    ip is NULL, indicating an invalid inode

    If the calling process does not hold the lock on the inode
    (holdingsleep(&ip->lock) returns false).

    If the reference count of the inode is less than 1, indicating an
    inconsistent state.
    */
    if (ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
        panic("iunlock");

    /*
    the inode is in a valid state and the calling process holds the lock. The
    function proceeds to release the sleep lock on the inode, allowing other
    processes to acquire it.
    */
    releasesleep(&ip->lock);
}

/*
Responsible for truncating (discarding the contents of) an inode. It is called
when the inode has no links to it (no directory entries referring to it) and has
no in-memory reference to it (is not an open file or current directory). The
purpose of this function is to free the blocks associated with the inode's file
contents on disk.
*/
static void itrunc(struct inode* ip) {
    /*
    The loop iterates over the direct block pointers of the inode (ip->addrs[0]
    to ip->addrs[NDIRECT-1]). NDIRECT is a constant that represents the number
    of direct block pointers in the inode.
    */
    for (int i = 0; i < NDIRECT; i++) {
        /*
        For each direct block pointer, the code checks if it is allocated (not
        zero). If the block pointer is not zero, it means that the block is
        allocated and holds file data.
        */
        if (ip->addrs[i]) {
            /*
            Free the block on the device (ip->dev). The bfree function is
            responsible for marking the block as free in the file system's block
            bitmap.
            */
            bfree(ip->dev, ip->addrs[i]);
            /*
            ip->addrs[i] is set to zero to indicate that the block is no longer
            associated with the inode.
            */
            ip->addrs[i] = 0;
        }
    }

    if (ip->addrs[NDIRECT]) {
        struct buf* bp = bread(ip->dev, ip->addrs[NDIRECT]);
        uint* a = (uint*)bp->data;
        for (int j = 0; j < NINDIRECT; j++) {
            if (a[j])
                bfree(ip->dev, a[j]);
        }
        brelse(bp);
        bfree(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }

    ip->size = 0;
    iupdate(ip);
}

/*
Responsible for dropping a reference to an in-memory inode and performing the
necessary cleanup operations.
*/
void iput(struct inode* ip) {
    acquiresleep(&ip->lock);
    /*
    Checks if the inode is valid (loaded from disk) and has no links to it. If
    both conditions are true, it means the inode is no longer needed and can be
    freed.
    */
    if (ip->valid && ip->nlink == 0) {
        acquire(&icache.lock);
        int r = ip->ref;
        release(&icache.lock);
        /*
        Checks if the reference count is 1, indicating that the current
        reference being dropped is the last one.
        */
        if (r == 1) {
            itrunc(ip);
            ip->type = 0;
            iupdate(ip);
            ip->valid = 0;
        }
    }
    releasesleep(&ip->lock);

    acquire(&icache.lock);
    ip->ref--;
    release(&icache.lock);
}

/*
a convenience function that combines unlocking and putting an inode.
*/
void iunlockput(struct inode* ip) {
    iunlock(ip);
    iput(ip);
}

/*
responsible for mapping a logical block number (bn) to a disk block address in
the inode's block allocation. It ensures that the inode's data blocks are
properly allocated and returns the address of the requested block.

struct inode* ip: It is a pointer to the inode for which the disk block address
is being determined. The inode contains information about the file or directory,
including its block allocation.

uint bn: It is the block number within the file's block allocation that we want
to map to a disk block address. This block number represents the bnth block
within the file.

returns the address (block number) of the allocated block corresponding to the
given block number bn in the inode ip. The address represents the location of
the block on the disk.
*/
static uint bmap(struct inode* ip, uint bn) {
    /*
    store the disk block address.
    */
    uint addr;

    if (bn < NDIRECT) {
        /*
        checks if the block is already allocated by accessing the corresponding
        entry in ip->addrs[bn]. If the block is not allocated (i.e., the entry
        is 0), it allocates a new block using the balloc function and assigns
        the returned block address to both ip->addrs[bn] and addr. This ensures
        that the inode's block allocation and the local addr variable are
        updated with the allocated block address.
        */
        if ((addr = ip->addrs[bn]) == 0)
            ip->addrs[bn] = addr = balloc(ip->dev);
        return addr;
    }

    bn -= NDIRECT;
    if (bn < NINDIRECT) {
        /*
        Checks if the indirect block has been allocated for the file.
        */
        if ((addr = ip->addrs[NDIRECT]) == 0)
            /*
            If the indirect block has not been allocated (ip->addrs[NDIRECT] ==
            0), it allocates a new block using balloc(ip->dev), which returns
            the disk block address of the newly allocated block.
            */
            ip->addrs[NDIRECT] = addr = balloc(ip->dev);

        /*
        reads the indirect block from disk into a buffer using bread(ip->dev,
        addr), where addr is the disk block address of the indirect block.
        */
        struct buf* bp = bread(ip->dev, addr);
        uint* a = (uint*)bp->data;
        /*
        checks if the block address at index bn in the indirect block array is
        0, indicating that the block has not been allocated yet.
        */
        if ((addr = a[bn]) == 0) {
            /*
            allocates a new block
            */
            a[bn] = addr = balloc(ip->dev);
            log_write(bp);
        }
        brelse(bp);
        return addr;
    }

    panic("bmap: out of range");
}

/*
copy the file system metadata (stat information) from an inode structure to a
struct stat object. The stati function is called when retrieving the file
information for a specific inode, which is typically used by system calls or
file-related operations.
*/
void stati(struct inode* ip, struct stat* st) {
    st->dev = ip->dev;
    st->ino = ip->inum;
    st->type = ip->type;
    st->nlink = ip->nlink;
    st->size = ip->size;
}

/*
read data from a file represented by the given inode (ip). It handles reading
data from both regular files and device files. For regular files, it reads data
from the disk blocks associated with the inode. For device files, it calls the
appropriate device driver's read function to read data from the device.

Arguments:

ip: A pointer to the inode structure representing the file from which to read
the data.

dst: A pointer to the destination buffer where the read data will be stored.

off: The starting offset within the file from which to begin reading.

n: The number of bytes to read.

return : number of beat read or -1
*/
int readi(struct inode* ip, char* dst, uint off, uint n) {
    uint m;
    struct buf* bp;

    if (ip->type == T_DEV) {
        /*
        checks if the ip->major field, which represents the major device number,
        is within a valid range. If it's less than 0 or greater than or equal to
        the maximum number of devices (NDEV), it indicates an invalid major
        number. In such cases, the function returns an error value of -1 to
        indicate failure.

        Next, it checks if the device's read function is defined and not NULL.
        */
        if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
            return -1;
        return devsw[ip->major].read(ip, dst, n);
    }

    /*
    Handle the offset and number of bit to read condition
    */
    if (off > ip->size || n < 0)
        return -1;
    if (off + n > ip->size)
        n = ip->size - off;

    /*
    iterates over the range of bytes to be read, starting from tot = 0 and
    continuing until tot < n. The loop increments the offset (off), the
    destination pointer (dst), and the total bytes read (tot) by the size of
    each chunk (m).
    */
    for (uint tot = 0; tot < n; tot += m, off += m, dst += m) {
        /*
        reads the block that contains the current offset off
        */
        bp = bread(ip->dev, bmap(ip, off / BSIZE));
        /*
        minimum value between the remaining bytes to be read (n - tot) and the
        number of bytes available in the current block (BSIZE - off % BSIZE).
        This ensures that we only read up to the remaining bytes in the file or
        the remaining space in the current block.
        */
        m = min(n - tot, BSIZE - off % BSIZE);
        /*
        The data from the current block is copied to the destination buffer dst
        */
        memmove(dst, bp->data + off % BSIZE, m);
        brelse(bp);
    }
    return n;
}

/*
write data to an inode in a file system.

ip: Pointer to the inode structure representing the file or directory to write
to.

src: Pointer to the source buffer containing the data to be written.

off: Offset within the file at which the writing should start.

n: Number of bytes to write.

return : number of byte written or -1
*/
int writei(struct inode* ip, char* src, uint off, uint n) {
    if (ip->type == T_DEV) {
        if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
            return -1;
        return devsw[ip->major].write(ip, src, n);
    }

    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > MAXFILE * BSIZE)
        return -1;

    uint m;
    struct buf* bp;
    for (uint tot = 0; tot < n; tot += m, off += m, src += m) {
        bp = bread(ip->dev, bmap(ip, off / BSIZE));
        /*
        n - tot: It represents the remaining number of bytes to be written.

        BSIZE - off % BSIZE: It represents the available space in the current
        block. Here, off % BSIZE calculates the current offset within the block,
        and subtracting it from BSIZE gives the remaining space in the block.

        By taking the minimum value between these two quantities, the code
        ensures that only the amount of data that can fit in the current block
        is written.
        */
        m = min(n - tot, BSIZE - off % BSIZE);
        memmove(bp->data + off % BSIZE, src, m);
        log_write(bp);
        brelse(bp);
    }

    /*
    update the inode info and write it to the disk
    */
    if (n > 0 && off > ip->size) {
        ip->size = off;
        iupdate(ip);
    }
    return n;
}

/*
used to compare two directory entry names (s and t) for equality. It is commonly
used in file system operations involving directories.
*/
int namecmp(const char* s, const char* t) {
    return strncmp(s, t, DIRSIZ);
}

/*
Used to look for a directory entry with a specific name within a directory. It
searches for a matching entry by iterating over the directory's data blocks and
comparing the names of the entries.

struct inode* dp: This is a pointer to the inode of the directory in which the
lookup is performed. The function assumes that dp points to a valid directory
inode.

char* name: This is the name of the directory entry being searched for. The
function will compare this name with the names of directory entries in the
directory to find a match.

uint* poff: This is a pointer to a variable that will store the byte offset of
the matching directory entry. If a match is found, the function will update the
value at the memory location pointed to by poff with the offset. If poff is
NULL, no offset value will be stored.
*/
struct inode* dirlookup(struct inode* dp, char* name, uint* poff) {
    struct dirent de;

    if (dp->type != T_DIR)
        panic("dirlookup not DIR");

    /*
    Loop that iterates over the data blocks of the directory. It uses the off
    variable to keep track of the byte offset within the directory.
    */
    for (uint off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlookup read");
        if (de.inum == 0)
            continue;
        /*
        If the name of the read directory entry matches the specified name using
        the namecmp function, it indicates a successful match.
        */
        if (namecmp(name, de.name) == 0) {
            /*
            If the poff pointer is provided (not NULL), it sets the value at
            poff to the current offset off. This allows the caller to obtain the
            byte offset of the matching directory entry if needed.
            */
            if (poff)
                *poff = off;

            return iget(dp->dev, de.inum);
        }
    }

    return 0;
}

/*
add a new directory entry with the specified name and inode number into the
directory represented by the dp inode.

dp: The dp inode represents the directory where the new directory entry will be
added.

name: The name is the name of the new directory entry that will be added to the
directory dp.

inum: The inum is the inode number of the file or directory that the new
directory entry will point to.
*/
int dirlink(struct inode* dp, char* name, uint inum) {
    struct inode* ip;

    /*
    check if a directory entry with the same name already exists in the
    directory represented by the dp inode
    */
    if ((ip = dirlookup(dp, name, 0)) != 0) {
        iput(ip);
        return -1;
    }

    struct dirent de;
    int off;
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlink read");
        /*
        the entry is empty and available for use.
        */
        if (de.inum == 0)
            break;
    }

    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    if (writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
        panic("dirlink");

    return 0;
}

/*
Used to extract the next path element from a given path and copy it into the
name buffer. It returns a pointer to the element following the copied one in the
path.
*/
static char* skipelem(char* path, char* name) {
    char* s;
    int len;
    /*
    skips any leading slashes in the path by incrementing the path pointer until
    the first non-slash character is encountered.
    */
    while (*path == '/')
        path++;
    /*
    If the first non-slash character is a null terminator (end of string), it
    means there are no more names to remove, so it returns 0.
    */
    if (*path == 0)
        return 0;
    s = path;
    /*
    advances the path pointer until the next slash or null terminator is
    encountered, indicating the end of the current path element.
    */
    while (*path != '/' && *path != 0)
        path++;
    len = path - s;
    /*
    If the length is greater than or equal to the maximum directory entry size
    (DIRSIZ), it means the path element cannot fit entirely in the name buffer.
    In this case, it only copies the first DIRSIZ characters of the path element
    into name.
    */
    if (len >= DIRSIZ)
        memmove(name, s, DIRSIZ);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }

    while (*path == '/')
        path++;

    return path;
}

/*
Responsible for looking up and returning the inode corresponding to a given path
name. It supports two modes of operation: when nameiparent is non-zero, it
returns the inode for the parent directory and copies the final path element
into the name buffer; otherwise, it returns the inode for the specified path.
The inode is always a directory.

    path: This is a pointer to a string representing the path name. It specifies
the file or directory location in the file system that you want to look up.

    nameiparent: This is an integer flag that determines the behavior of the
function. If nameiparent is non-zero, the function will return the inode of the
parent directory of the final path element and copy the final path element into
the name buffer. If nameiparent is zero, the function will return the inode of
the final path element itself.

    name: This is a pointer to a character buffer that will be used to store the
final path element if nameiparent is non-zero. It should have enough room to
accommodate the name, which has a maximum length of DIRSIZ bytes.
*/
static struct inode* namex(char* path, int nameiparent, char* name) {
    /*
    The ip pointer will be used to traverse the path, while next will hold the
    inode of the next directory in the path.
    */
    struct inode *ip, *next;
    /*
    If the first character of the path is '/', it indicates an absolute path, so
    the function gets the inode for the root directory using iget(ROOTDEV,
    ROOTINO). Otherwise, it duplicates the current working directory of the
    calling process (myproc()->cwd) by calling idup.
    */
    if (*path == '/')
        ip = iget(ROOTDEV, ROOTINO);
    else
        ip = idup(myproc()->cwd);

    /*
    Iterates over the path components in path by calling the skipelem function.
    It updates the path variable to point to the next path component and assigns
    the current component to the name variable. The loop continues until there
    are no more path components to process.
    */
    while ((path = skipelem(path, name)) != 0) {
        ilock(ip);
        /*
        if the inode's type is not a directory. If it's not a directory, it
        means the path component being processed is not a valid directory name.
        In this case, the function releases the lock on the current inode
        (iunlockput(ip)) and returns 0 to indicate failure.
        */
        if (ip->type != T_DIR) {
            /*
            if you don't understant why call iput, think that you are in the
            directory ./ and that you do something like rm ./ (you will
            understand when studying iput).
            */
            iunlockput(ip);
            return 0;
        }
        /*
        if the nameiparent flag is set (indicating that the caller wants the
        parent inode) and if the current path component is the last component in
        the path (i.e., the path ends after this component). If both conditions
        are met, the function releases the lock on the current inode
        (iunlock(ip)) and returns the current inode ip.
        */
        if (nameiparent && *path == 0) {
            // Stop one level early.
            iunlock(ip);
            return ip;
        }
        /*
        Calls the dirlookup function to find the inode corresponding to the
        current path component (name) within the directory represented by the
        current inode (ip). If the lookup fails (returns 0), indicating that the
        path component doesn't exist, the function releases the lock on the
        current inode (iunlockput(ip)) and returns 0 to indicate failure.
        */
        if ((next = dirlookup(ip, name, 0)) == 0) {
            iunlockput(ip);
            return 0;
        }
        iunlockput(ip);
        /*
        Assigns the next inode (corresponding to the next path component) to the
        ip variable, preparing it for the next iteration of the loop.
        */
        ip = next;
    }

    return ip;
}

/*
The purpose of the namei function is to provide a convenient interface to
resolve a given path into an inode. It initializes a character array to hold the
names extracted from the path and then passes the arguments to the namex
function to perform the actual path resolution.
*/
struct inode* namei(char* path) {
    char name[DIRSIZ];
    return namex(path, 0, name);
}

/*
The purpose of the namei function is to provide a convenient interface to
resolve a given path into an inode. It initializes a character array to hold the
names extracted from the path and then passes the arguments to the namex
function to perform the actual path resolution.
*/
struct inode* nameiparent(char* path, char* name) {
    return namex(path, 1, name);
}
