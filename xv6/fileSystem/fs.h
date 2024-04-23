#ifndef fs_h
#define fs_h

// On-disk file system format.
// Both the kernel and user programs use this header file.

#define ROOTINO 1  // root i-number
/*
the file system in xv6 uses a block size of 512 bytes. This block size is a
fundamental unit of data storage in the file system. Disk blocks are typically
read from or written to in multiples of this block size.
*/
#define BSIZE 512

/*
The superblock is a special disk block in a file system that holds metadata
about the file system itself. It describes the overall layout and organization
of the file system.
*/
struct superblock {
    /*
    Total size of the filesystem image, in terms of blocks. This total size
    includes all types of blocks used in the filesystem. It includes inode
    blocks (which store file metadata), log blocks (which are used for crash
    recovery), data blocks (which store the actual file data), as well as other
    system-related blocks such as the superblock itself and the block bitmap
    (which keeps track of free and used blocks). Essentially, size is the
    overall total number of blocks in the filesystem.

    In the context of a file system, a block is the smallest unit of data that a
    file system can read or write. The block size is typically a multiple of the
    sector size, which is the smallest unit of data that the hardware (usually a
    hard drive or SSD) can read or write. A block may contain a portion of a
    file's data or a filesystem metadata structure like an inode or directory
    entry.

    When a file is read from or written to disk, it's done in block-sized units.
    If a file isn't large enough to fill an entire block, the rest of the block
    may be filled with zeroes (this is known as internal fragmentation). If a
    file is larger than one block, it will span multiple blocks.

    Block sizes vary depending on the filesystem and the specific hardware, but
    common sizes are 512 bytes, 1 kilobyte (KB), 4 KB, and 8 KB. The block size
    is chosen when the filesystem is created and usually can't be changed
    without reformatting the filesystem.

    In the xv6 file system, for example, a block is 512 bytes.
    */
    uint size;
    /*
    Total number of data blocks in the filesystem. Data blocks are the blocks
    that actually store file data. These are the blocks that are read from or
    written to when you open, modify, or save a file.
    */
    uint nblocks;
    /*
    Total number of inodes in the file system. An inode (index node) is a data
    structure that represents a file or a directory. It contains information
    about a file or directory such as its size, permissions, timestamps, and
    the locations of the data blocks that store the file's data.
    */
    uint ninodes;
    /*
    Total number of log blocks in the file system. These are used by the file
    system's log to provide crash recovery capabilities.
    */
    uint nlog;
    /*
    Block number of the first log block. This tells the system where the log
    section begins in the file system.

    Log blocks are used to implement a feature in file systems known as
    journaling or logging. The purpose of this feature is to enhance the
    reliability and integrity of the file system, particularly in scenarios
    where the system might crash or lose power suddenly.

    In a logging or journaling file system, before any change is made to the
    file system, the details of the change are first written to a special area
    on the disk called the log (or journal). These changes may include
    operations like creating, deleting, moving, or modifying files or
    directories.

    Once the log entry is written, the file system can then apply the change to
    the main part of the file system. If the system crashes after the log entry
    is written but before the change is applied, when the system restarts, it
    can look at the log to see what was supposed to happen and complete the
    operation. This makes the file system more robust against corruption from
    crashes or power loss.

    In the case of xv6, the log is used to implement a simplified version of
    this concept. It doesn't support full journaling, but it does use the log to
    help recover from certain types of crashes. The log blocks are the disk
    blocks dedicated to storing this log.
    */
    uint logstart;
    /*
    Block number of the first inode block. This tells the system where the inode
    section begins.
    */
    uint inodestart;
    /*
    Block number of the first free map block. This free map, also known as a
    block bitmap, keeps track of which blocks are free or in use. A 0 bit
    indicates a free block, while a 1 bit indicates a block in use. This field
    tells the system where the free map section begins.
    */
    uint bmapstart;
};

/*
represents the number of direct block pointers stored in the addrs array of the
struct dinode.

By defining NDIRECT as 12, it means that the addrs array can store addresses of
12 data blocks directly. These 12 direct block pointers provide direct access to
the first 12 data blocks associated with a file. The 13th data block and beyond
are accessed indirectly through the indirect block pointer, which is stored in
the last element of the addrs array.
*/
#define NDIRECT 12
/*
represents the number of block pointers that can be stored in a single indirect
block. An indirect block contains block pointers that point to additional blocks
that store file data. By using an indirect block, more file blocks can be
addressed indirectly through the indirect block pointers.
*/
#define NINDIRECT (BSIZE / sizeof(uint))
/*
defines the maximum number of file blocks that can be directly addressed by an
inode in the file system.
*/
#define MAXFILE (NDIRECT + NINDIRECT)

/*
represents the on-disk format of an inode in the file system.
*/
struct dinode {
    /*
    A short value indicating the type of the file. This can be one of the file
    types defined in the stat.h header file.
    */
    short type;
    /*
    major and minor: short values representing the major and minor device
    numbers associated with the file. These fields are only used for device
    files (T_DEV type).
    */
    short major;
    /*
    major and minor: short values representing the major and minor device
    numbers associated with the file. These fields are only used for device
    files (T_DEV type).
    */
    short minor;
    /*
    A short value representing the number of directory entries that reference
    this inode. This is used to keep track of the inode's link count.
    */
    short nlink;
    /*
    This field stores the size of the file in bytes. It represents the current
    size of the associated file.
    */
    uint size;
    /*
    An array of uint values representing the data block addresses (data block
    number) associated with the file. NDIRECT defines the number of direct block
    pointers that can be stored directly in the addrs array, and NDIRECT + 1
    represents the total number of block pointers including the indirect block
    pointer. The direct block pointers store the addresses of the file's data
    blocks, while the indirect block pointer points to a block that contains
    additional block addresses.

    Direct Block Pointers:

    Direct block pointers are the block pointers that directly store the
    addresses of data blocks that contain file data. Each element of the addrs
    array can store the address of a data block, allowing direct access to the
    file's content.

    Indirect Block Pointer:

    The indirect block pointer is an additional block pointer that points to a
    block called the indirect block. The indirect block contains an array of
    block addresses, similar to the addrs array in the struct dinode. By using
    the indirect block pointer, the file system can indirectly access more data
    blocks beyond the capacity of the direct block pointers.
    */
    uint addrs[NDIRECT + 1];
};

// Inodes per block.
#define INODE_PER_BLOCK (BSIZE / sizeof(struct dinode))

/*
 used to calculate the disk block number (sector) that contains the inode with
 the given inode number i. The macro takes two parameters:

    i: The inode number.
    sb: The superblock structure that contains information about the file
 system.
*/
#define IBLOCK(i, sb) ((i) / INODE_PER_BLOCK + sb.inodestart)

/*
bits per block
*/
#define BPB (BSIZE * 8)

/*
Calculates the block number of the block bitmap containing the target block (b)
based on the file system's superblock (sb).
*/
#define BBLOCK(b, sb) (b / BPB + sb.bmapstart)

/*
Typically represents the maximum length of a file name in the file system.
*/
#define DIRSIZ 14

/*
represents a directory entry in the file system. It defines the format and
layout of a directory entry, which consists of an inode number (inum) and a
name (name) associated with that inode.
*/
struct dirent {
    /*
    The inode number associated with the directory entry. It specifies the inode
    that corresponds to the file or subdirectory represented by this entry.

    If the directory entry has an associated inode equal to 0, it means the
    entry is unused or deleted.
    */
    ushort inum;
    /*
    name of the file or subdirectory represented by this entry.
    */
    char name[DIRSIZ];
};

#endif