#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
/*
used to avoid a clash between two struct types with the same name. In this case,
the stat struct name is being renamed to xv6_stat. The purpose of this renaming
is to prevent conflicts with the stat struct from the host system, which might
be included from the standard library.
*/
#define stat xv6_stat
#include "../type/types.h"
#include "fs.h"
#include "stat.h"
#include "../type/param.h"

#define INODE_NUMBER 200
#define BITMAP_BLOCKS_NUMBER (FSSIZE / (BSIZE * 8) + 1)
#define INODE_BLOCKS_NUMBER (INODE_NUMBER / INODE_PER_BLOCK + 1)
#define METADATA_BLOCKS_NUMBER \
    (2 + LOGSIZE + INODE_BLOCKS_NUMBER + BITMAP_BLOCKS_NUMBER)
#define DATA_BLOCKS_NUMBER (FSSIZE - METADATA_BLOCKS_NUMBER)
#define min(a, b) ((a) < (b) ? (a) : (b))

int fileSystemImageFd;
struct superblock sb;
/*
represents the next available inode number. In the context of the file system,
an inode is a data structure that stores metadata about a file, such as its
type, size, permissions, and disk block addresses.

In this specific case, freeinode is initially set to 1, indicating that the
first available inode number is 1. As new inodes are allocated, the freeinode
value is incremented, ensuring that each allocated inode receives a unique
number.
*/
uint freeinode = 1;
/*
used to keep track of the next available free block in the file system.

In the context of the code snippet you provided, freeblock is initialized to the
value nmeta, which represents the number of METADATA_BLOCKS_NUMBER blocks in the
file system. These METADATA_BLOCKS_NUMBER blocks include the boot block,
superblock, log blocks, inode blocks, and the free bit map. The nmeta value is
calculated based on the size and structure of the file system.
*/
uint freeblock;

/*
used to write a block of data to a specific sector on the file system

The function takes two arguments: sec, which specifies the sector number to
write to, and buf, which is a pointer to the buffer containing the data to be
written.
*/
void wsect(uint sec, void* buf) {
    /*
    The function uses the lseek system call to set the file offset to the
    beginning of the specified sector. It calculates the offset by multiplying
    the sector number (sec) with the block size (BSIZE), which represents the
    size of a disk sector. The third argument of lseek is set to 0, indicating
    that the offset is relative to the beginning of the file.

    The function checks if the lseek call was successful by comparing the
    returned offset with the expected offset (sec * BSIZE). If they don't match,
    it means there was an error in setting the file offset. In that case, it
    prints an error message using perror and exits the program with an exit
    status of 1.
    */
    if (lseek(fileSystemImageFd, sec * BSIZE, 0) != sec * BSIZE) {
        perror("lseek");
        exit(1);
    }
    /*
    If the lseek call is successful, the function proceeds to write the data
    from the buffer (buf) to the file system using the write system call. It
    specifies the file descriptor fileSystemImageFd (which represents the
    file system file descriptor) and the size of a block (BSIZE) as the number
    of bytes to write.
    */
    if (write(fileSystemImageFd, buf, BSIZE) != BSIZE) {
        perror("write");
        exit(1);
    }
}

/*
used to read a sector from a file. It takes two arguments: the sector number sec
and a buffer buf where the sector data will be stored.
*/
void rsect(uint sec, void* buf) {
    if (lseek(fileSystemImageFd, sec * BSIZE, 0) != sec * BSIZE) {
        perror("lseek");
        exit(1);
    }
    if (read(fileSystemImageFd, buf, BSIZE) != BSIZE) {
        perror("read");
        exit(1);
    }
}

/*
used to write an inode (struct dinode) to the disk. It takes two arguments:

inum : inode number
ip : pointer to the inode structure ip that contains the data to be written.
*/
void winode(uint inum, struct dinode* ip) {
    /*
    This buffer will be used to read and write disk sectors.
    */
    char buf[BSIZE];
    /*
    calculates the block number that contains the inode using the IBLOCK macro.
    The IBLOCK macro calculates the block number based on the inode number inum
    and the superblock sb.
    */
    uint bn = IBLOCK(inum, sb);
    rsect(bn, buf);
    /*
    calculates the address of the inode within the buffer buf using pointer
    arithmetic. It adds the offset of the inode within the block (inum %
    INODE_PER_BLOCK) to the base address of the buffer.
    */
    struct dinode* dip = ((struct dinode*)buf) + (inum % INODE_PER_BLOCK);
    *dip = *ip;
    /*
    uses the wsect function to write the modified buffer back to the disk block
    number
    */
    wsect(bn, buf);
}
/*
reads the block containing the specified inode from the disk, locates the inode
within the block, and then copies the inode into the provided memory location.

inum: The inode number of the inode to be read.
ip: A pointer to a struct dinode where the read inode will be stored.
*/
void getinode(uint inum, struct dinode* ip) {
    /*
    declares a temporary buffer buf of size BSIZE (block size).
    */
    char buf[BSIZE];
    struct dinode* dip;

    uint bn = IBLOCK(inum, sb);
    rsect(bn, buf);
    /*
    add the dinode offset of the block
    */
    dip = ((struct dinode*)buf) + (inum % INODE_PER_BLOCK);
    *ip = *dip;
}

/*
allocates a new inode, initializes its properties such as type, link count, and
size, and writes it to the disk.

type : specifies the type of the inode to be allocated (e.g., a file, directory,
device, etc.).
return : the allocated inode number inum is returned, indicating the success of
the allocation process.
*/
uint allocInode(ushort type) {
    /*
    assigns the next available inode number to inum and increments the freeinode
    counter to track the allocation of inodes.
    */
    uint inum = freeinode++;
    /*
    represents the properties of the inode. It holds information such as the
    inode type, number of links, and file size.
    */
    struct dinode din;
    memset(&din, 0, sizeof(din));
    /*
    preset value
    */
    din.type = type;
    din.nlink = 1;
    din.size = 0;
    winode(inum, &din);
    return inum;
}

/*
store the bitmap block in the sector pointed by sb.bmapstart. The
setBlockBitmapAllocStatus function creates a bitmap buffer buf that represents
the allocation status of blocks. Each bit in the buffer corresponds to a block
on the disk, where a value of 1 indicates that the block is allocated and a
value of 0 indicates that the block is free.

used : number of blocks already used.

A bitmap block is a data structure used in file systems to keep track of the
allocation status of blocks on a disk. It is typically represented as an array
of bits, where each bit corresponds to a block on the disk. The value of a bit
in the bitmap indicates whether the corresponding block is allocated (in use) or
free (available for allocation).
*/
void setBlockBitmapAllocStatus(int used) {
    uchar buf[BSIZE];
    /*
    prints a message indicating how many blocks have already been allocated.
    */
    printf("setBlockBitmapAllocStatus: first %d blocks have been allocated\n",
           used);
    /*
    bitmap is on 8 block
    */
    assert(used < BSIZE * 8);
    /*
    loops over the number of used blocks and sets the corresponding bit in the
    buffer to 1. This is done by using bitwise OR (|) and bitwise left shift
    (<<) operations. The i / 8 expression represents the byte index in the
    buffer, and (0x1 << (i % 8)) sets the appropriate bit within that byte.
    */
    for (int i = 0; i < used; i++) {
        buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
    }
    wsect(sb.bmapstart, buf);
}

/*
used to append data to a file represented by the given inode number (inum).

inum: The inode number of the file to which the data is appended.
xp: A pointer to the data that needs to be appended to the file.
n: The size of the data in bytes that needs to be appended.
*/
void appendInode(uint inum, void* xp, int n) {
    /*
    A dinode struct representing the inode of the file.
    */
    struct dinode din;
    getinode(inum, &din);
    /*
    The current offset (in bytes) within the file.
    */
    uint off = din.size;
    /*
    loop that continues until all data (n) has been written:
    */
    while (n > 0) {
        /*
        Calculate the file block number (fbn) based on the current offset and
        the block size (BSIZE).
        */
        uint fbn = off / BSIZE;
        /*
        Use an assertion (assert) to ensure that the file block number is within
        the maximum file size (MAXFILE).
        */
        assert(fbn < MAXFILE);
        /*
        The block number of the current file block.
        */
        uint x;
        /*
        Check if the file block number (fbn) is within the range of direct
        blocks (NDIRECT).
        */
        if (fbn < NDIRECT) {
            /*
            If it is within the direct block range, check if the corresponding
            block is allocated (din.addrs[fbn] != 0).
            */
            if (din.addrs[fbn] == 0) {
                /*
                If the block is not allocated (i.e., din.addrs[fbn] is zero),
                allocate a new block by incrementing the freeblock counter
                (freeblock++) and assign the new block number to din.addrs[fbn].
                */
                din.addrs[fbn] = freeblock++;
            }
            /*
            Retrieve the block number (x) from din.addrs[fbn]. The block
            number is now ready to be used for reading from or writing to the
            disk.
            */
            x = din.addrs[fbn];
        } else {
            /*
            Check if the indirect block is allocated. The indirect block is
            stored in din.addrs[NDIRECT].
            */
            if (din.addrs[NDIRECT] == 0) {
                /*
                If the indirect block is not allocated (i.e., din.addrs[NDIRECT]
                is zero), allocate a new block by incrementing the freeblock
                counter (freeblock++) and assign the new block number to
                din.addrs[NDIRECT].
                */
                din.addrs[NDIRECT] = freeblock++;
            }
            /*
            An array of block pointers for indirect addressing.
            */
            uint indirect[NINDIRECT];
            /*
            Read the contents of the indirect block from the disk into the
            indirect array using the rsect function. The block number to read
            from is obtained by converting din.addrs[NDIRECT].
            */
            rsect(din.addrs[NDIRECT], (char*)indirect);
            /*
            Check if the file block within the indirect blocks range is
            allocated. The block number within the indirect blocks range is
            calculated as indirect[fbn - NDIRECT], where fbn - NDIRECT
            represents the offset from the indirect block.
            */
            if (indirect[fbn - NDIRECT] == 0) {
                /*
                If the block is not allocated (i.e., indirect[fbn - NDIRECT] is
                zero), allocate a new block by incrementing the freeblock
                counter (freeblock++) and assign the new block number to
                indirect[fbn - NDIRECT].
                */
                indirect[fbn - NDIRECT] = freeblock++;
                /*
                Write the updated contents of the indirect block back to the
                disk using the wsect function. The block number to write to is
                obtained by converting din.addrs[NDIRECT].
                */
                wsect(din.addrs[NDIRECT], (char*)indirect);
            }
            /*
            Retrieve the block number (x) from indirect[fbn - NDIRECT]. The
            block number is now ready to be used for reading from or writing to
            the disk.
            */
            x = indirect[fbn - NDIRECT];
        }
        /*
        calculates the number of bytes (n1) to be written in the current data
        block. It takes the minimum value between the remaining number of bytes
        to be written (n) and the available space in the block ((fbn + 1) *
        BSIZE - off).

        The expression (fbn + 1) * BSIZE calculates the end position of the
        current data block. Since fbn represents the current file block number,
        adding 1 and multiplying by BSIZE gives the end position of the block.

        Subtracting off from the end position gives the remaining space within
        the block where the writing operation can continue. It represents the
        number of bytes available from the current offset off until the end of
        the block.

        So, (fbn + 1) * BSIZE - off calculates the remaining space within the
        current data block, indicating how many more bytes can be written before
        moving to the next block.
        */
        uint n1 = min(n, (fbn + 1) * BSIZE - off);
        /*
        A buffer to hold the contents of a disk block.
        */
        char buf[BSIZE];
        /*
        This line reads the data block with the block number x into the buffer
        buf. It retrieves the content of the block from the disk.
        */
        rsect(x, buf);
        /*
        A character pointer to the data to be appended
        */
        char* p = (char*)xp;
        /*
        Copies the data from the source buffer p to the destination buffer buf
        at the appropriate offset. It ensures that the data is written at the
        correct position within the block.
        */
        bcopy(p, buf + off - (fbn * BSIZE), n1);
        /*
        Writes the modified buffer buf back to the disk, updating the content of
        the data block with the new data.
        */
        wsect(x, buf);
        /*
        updates the remaining number of bytes to be written by subtracting the
        number of bytes written in the current data block (n1).
        */
        n -= n1;
        /*
        updates the current offset (off) within the file by adding the number of
        bytes written in the current data block (n1).
        */
        off += n1;
        /*
        updates the source buffer pointer (p) to point to the next chunk of data
        to be written in the file.
        */
        p += n1;
    }
    /*
    Overall, these two lines update the file's inode with the new file size
    */
    din.size = off;
    winode(inum, &din);
}

/*
Initializing and populating a new file system image with the specified files and
directories, setting up the necessary metadata and structures to represent the
file system.
*/
int main(int argc, char* argv[]) {
    /*
    This line performs a compile-time assertion to verify that the size of the
    int type is 4 bytes. If the assertion fails (i.e., the size is not 4 bytes),
    it will display the error message "Integers must be 4 bytes!".
    */
    static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

    /*
    This condition checks if the number of command-line arguments (argc) is less
    than 2. If so, it means the required arguments are missing. It prints the
    usage information to the standard error stream (stderr) and exits the
    program with an exit code of 1.
    */
    if (argc < 2) {
        fprintf(stderr, "Usage: mkfs fs.img files...\n");
        exit(1);
    }

    /*
    This assertion verifies that the block size (BSIZE) is a multiple of the
    size of the struct dinode. If the assertion fails, it indicates a mismatch
    between the block size and the inode structure size.
    */
    assert((BSIZE % sizeof(struct dinode)) == 0);
    /*
    Similarly, this assertion checks if the block size (BSIZE) is a multiple of
    the size of the struct dirent. If it fails, it indicates a mismatch between
    the block size and the directory entry structure size.
    */
    assert((BSIZE % sizeof(struct dirent)) == 0);

    /*
    This line opens the file specified by argv[1] (the first command-line
    argument) with flags O_RDWR (read-write), O_CREAT (create if it doesn't
    exist), and O_TRUNC (truncate to zero length if it exists). It assigns the
    file descriptor to the variable fileSystemImageFd.
    */
    fileSystemImageFd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
    /*
    This condition checks if the file descriptor fileSystemImageFd is
    less than 0, indicating an error in opening the file. If the condition is
    true, it prints the corresponding error message with the file name (argv[1])
    using perror, and exits the program with an exit code of 1.
    */
    if (fileSystemImageFd < 0) {
        perror(argv[1]);
        exit(1);
    }

    sb.size = FSSIZE;
    sb.nblocks = DATA_BLOCKS_NUMBER;
    sb.ninodes = INODE_NUMBER;
    sb.nlog = LOGSIZE;
    sb.logstart = 2;
    sb.inodestart = 2 + LOGSIZE;
    sb.bmapstart = 2 + LOGSIZE + INODE_BLOCKS_NUMBER;

    /*
    Sets the initial value of freeblock to the first free block that can be
    allocated in the file system.
    */
    freeblock = METADATA_BLOCKS_NUMBER;
    /*
    The purpose of this array is to store a block of zeroes, which can be used
    for initializing blocks in the file system. In many file systems, including
    xv6, newly allocated blocks or uninitialized blocks are typically filled
    with zeroes to represent an empty state.
    */
    char zeroes[BSIZE];
    memset(zeroes, 0, sizeof(zeroes));
    /*
    This loop iterates over each block in the file system and calls the wsect
    function to write a block of zeroes to that block.
    */
    for (int i = 0; i < FSSIZE; i++)
        wsect(i, zeroes);

    /*
    Write superblock to the sector 1
    */
    wsect(1, &sb);

    /*
    allocate an inode of type T_DIR, which represents the root directory.
    */
    uint rootino = allocInode(T_DIR);
    /*
    verifies that the allocated inode number (rootino) matches the predefined
    root inode number (ROOTINO). The assert macro is used to check the condition
    and terminate the program if it evaluates to false. In this case, it ensures
    that the allocated inode is indeed the root inode.
    */
    assert(rootino == ROOTINO);

    /*
    What we are doing here is to indicate that for the root directory (ROOTINO),
    . and .. point to itself, the root directory. If you are here "/"", cd ..
    will not change anything.
    */
    struct dirent de;
    memset(&de, 0, sizeof(de));
    de.inum = rootino;
    strcpy(de.name, ".");
    appendInode(rootino, &de, sizeof(de));

    memset(&de, 0, sizeof(de));
    de.inum = rootino;
    strcpy(de.name, "..");
    appendInode(rootino, &de, sizeof(de));

    /*
    This block of code reads files from the host file system and adds them to
    the file system image being created.
    */
    int fd;
    for (int i = 2; i < argc; i++) {
        /*
        if arg contain a slash, aborts the program.
        else, open the file.
        */
        // assert(index(argv[i], '/') == 0);
        if ((fd = open(argv[i], 0)) < 0) {
            perror(argv[i]);
            exit(1);
        }

        /*
        Choose on-disk name: use basename (strip directories),
        then skip leading '_' for user binaries like _rm, _cat, etc.
        */
        const char* ondisk = argv[i];
        const char* slash = strrchr(ondisk, '/');
        if (slash)
            ondisk = slash + 1;
        if (ondisk[0] == '_')
            ++ondisk;
        /*
        allocate an inode for the file using the allocInode function, specifying
        the file type as T_FILE. The resulting inode number is stored in the
        variable inum for further use.
        */
        uint inum = allocInode(T_FILE);

        memset(&de, 0, sizeof(de));
        /*
        set name and inum
        */
        de.inum = inum;
        strncpy(de.name, ondisk, DIRSIZ);
        /*
        appends the directory entry to the root directory.
        */
        appendInode(rootino, &de, sizeof(de));

        /*
        store the number of bytes read from the file.
        */
        int cc;
        char buf[BSIZE];
        while ((cc = read(fd, buf, sizeof(buf))) > 0)
            appendInode(inum, buf, cc);

        close(fd);
    }

    /*
    this block ensures that the size of the root directory is aligned to the
    block size and updates the corresponding inode on the disk. It also
    allocates any remaining free blocks in the file system.
    */
    struct dinode din;
    getinode(rootino, &din);
    uint off = din.size;
    /*
    Round the size 1 sector up.
    Round down the first parenthesis then do the stuff.
    */
    off = ((off + BSIZE - 1) / BSIZE) * BSIZE;
    /*
    correct the size
    */
    din.size = off;
    winode(rootino, &din);

    /*
    set the bitmap
    */
    setBlockBitmapAllocStatus(freeblock);

    exit(0);
}
