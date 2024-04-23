#define NPROC 64         // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
/*
maximum number of CPUs supported in the system.
*/
#define NCPU 8
#define NOFILE 16  // open files per process
#define NFILE 100  // open files per system
/*
Maximum number of active i-nodes
*/
#define NINODE 50
/*
determines the maximum number of devices that can be supported by the operating
system.
*/
#define NDEV 10  // maximum major device number
/*
ROOTDEV 1 typically defines the device number for the storage device (such as a
disk) that contains the root file system of the operating system.

The root file system is crucial because it contains all the files necessary for
booting the system and launching the essential system processes. This includes
the OS kernel itself, device drivers, configuration files, system libraries,
user utilities and shell programs, etc.

In a Unix-like operating system, the root file system is always mounted at
startup and it is mounted at the root of the directory structure, represented by
"/". It is from this root file system that the entire directory structure of the
system sprouts.

So in this context, ROOTDEV would refer to the device that contains the OS
*/
#define ROOTDEV 1
#define MAXARG 32  // max exec arguments
/*
MAXOPBLOCKS is a constant that represents the maximum number of blocks per
operation.

In the context of disk I/O operations, a "block per operation" refers to the
number of disk blocks read from or written to the disk in a single operation.

A disk block is a fixed-size unit of data storage on a disk. It represents a
contiguous set of sectors, which are the smallest individually addressable units
on a disk. The size of a disk block can vary depending on the file system and
disk configuration, but it is typically a few kilobytes in size.

When performing disk I/O operations, it is often more efficient to read or write
data in larger chunks rather than accessing individual sectors. This is where
the concept of "block per operation" comes into play. Instead of reading or
writing one sector at a time, a disk block is read or written in a single
operation. This allows for better utilization of disk and memory resources and
can improve overall I/O performance.
*/
#define MAXOPBLOCKS 10
/*
specifies the maximum number of data blocks that can be present in the on-disk
log of the file system.
*/
#define LOGSIZE (MAXOPBLOCKS * 3)
/*
represents the size of the disk block cache.

The value of NBUF is determined by multiplying MAXOPBLOCKS by 3. MAXOPBLOCKS is
likely another defined constant that represents the maximum number of blocks per
operation.
*/
#define NBUF (MAXOPBLOCKS * 3)
/*
represents the total number of disk blocks in the file system
*/
#define FSSIZE 1000
