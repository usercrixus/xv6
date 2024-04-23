#ifndef file_h
#define file_h

#include "../type/param.h"
#include "../type/types.h"
#include "../synchronization/sleeplock.h"
#include "./fs.h"

#define CONSOLE 1
/*
Represents a file descriptor in an operating system. It contains several fields
that store information about the file and its associated resources
*/
struct file {
    /*
    specifies the type of file represented by the file descriptor.
    */
    enum {
        /*
        the file descriptor is not associated with any file.
        */
        FD_NONE,
        /*
        the file descriptor represents a pipe, which is a communication
        mechanism between processes.
        */
        FD_PIPE,
        /*
        the file descriptor represents a regular file associated with an inode.
        */
        FD_INODE
    } type;
    /*
    Represents the reference count of the file descriptor. It is used to keep
    track of how many file descriptors refer to the same file. The reference
    count is incremented when a new file descriptor is created that refers to
    the same file, and decremented when a file descriptor is closed or no longer
    references the file. When the reference count reaches zero, the file and its
    associated resources can be safely deallocated.
    */
    int ref;
    /*
    Indicate whether the file descriptor is readable, if it is set to a non-zero
    value, it means the file can be read from.
    */
    char readable;
    /*
    Indicate whether the file descriptor is writable, if is set to a non-zero
    value, it means the file can be write to.
    */
    char writable;
    /*
    Pointer to a struct pipe if the file descriptor represents a pipe. A pipe is
    a special type of file used for inter-process communication.
    */
    struct pipe* pipe;
    /*
    Pointer to a struct inode if the file descriptor represents a regular file
    associated with an inode. An inode is a data structure that stores metadata
    about a file (e.g., permissions, ownership, timestamps).
    */
    struct inode* ip;
    /*
    Stores the byte offset from the beginning of the file. When reading or
    writing to the file, this offset is updated to keep track of the next read
    or write operation's position.
    */
    uint off;
};

/*
In-memory copy of an inode
It contains various metadata and data necessary for managing and accessing files
in the file system. An inode, short for "index node," is a data structure used
in many file systems to store metadata about a file or directory. Each file or
directory in a file system is associated with an inode, which contains important
information about the file's attributes and data blocks.
*/
struct inode {
    /*
    device number on which the inode resides. The device number uniquely
    identifies the storage device (e.g., disk partition) to which the inode
    belongs.
    */
    uint dev;
    /*
    Inode number, which is a unique identifier for the inode within the file
    system. It allows the file system to locate and manage the specific inode.
    */
    uint inum;
    /*
    Reference count of the inode. It keeps track of the number of references to
    the inode. When a reference is made to an inode (e.g., opening a file), the
    reference count is incremented. When a reference is removed (e.g., closing a
    file), the reference count is decremented. The inode can be safely
    deallocated when the reference count reaches zero.
    */
    int ref;
    /*
    Represents a sleep lock that protects the inode's metadata and data from
    concurrent access. The sleep lock ensures mutual exclusion, allowing only
    one thread at a time to access or modify the inode.
    */
    struct sleeplock lock;
    /*
    Indicates whether the inode's contents have been read from the disk and are
    valid in memory. If valid is non-zero, it means the inode has been read and
    is valid; otherwise, it needs to be read from the disk.
    */
    int valid;
    /*
    Stores the type of the file associated with the inode. It can have values
    such as regular file, directory, device file, etc. The specific values are
    typically defined using enumeration or constants.
    */
    short type;
    /*
    Stores the major number associated with the device file. The major number
    is used to identify the specific device driver that controls the device. In
    Unix-like systems, the major number is typically assigned by the operating
    system and corresponds to a specific driver responsible for managing a
    particular type of device. For example, a major number of 1 might be
    associated with disk devices, while a major number of 2 might be associated
    with network devices.
     */
    short major;
    /*
    Stores the minor number associated with the device file. The minor number,
    together with the major number, helps specify the specific device instance.
    In other words, it distinguishes between multiple devices of the same type
    that are managed by the same driver. The minor number is typically assigned
    by the device driver when the device is detected or configured. For example,
    if you have multiple disk drives in your system, each drive might be
    assigned a unique minor number to differentiate them.
    */
    short minor;
    /*
    represents the number of hard links to the inode.

    A hard link is an additional name (or link) within the file system that
    points to the same underlying file. In other words, multiple hard links can
    exist for a single file, giving it multiple names or paths. All hard links
    refer to the same inode, meaning they represent the same file content and
    attributes.

    short nlink stores the number of hard links that exist for the
    file associated with the inode. Each time a new hard link is created to the
    file, the nlink field is incremented, and when a hard link is removed, the
    nlink field is decremented. This field helps keep track of the number of
    hard links to the inode.

    By tracking the nlink count, the file system can properly manage the
    allocation and deallocation of inodes and associated file resources,
    ensuring that they are only released when they are no longer in use.
    */
    short nlink;
    /*
    This field stores the size of the file in bytes. It represents the current
    size of the associated file.
    */
    uint size;
    /*
    Array that holds the disk block addresses of the file's data blocks. The
    NDIRECT constant represents the number of direct block addresses that can be
    stored in the addrs array. Additionally, there is an extra block address to
    store indirect block addresses that allow for larger file sizes.
    */
    uint addrs[NDIRECT + 1];
};

/*
Represents the device switch table. It maps the major device number to the
device functions.

A device switch table, also known as a device dispatch table or device function
table, is a data structure used in an operating system to manage device drivers.
It maps the operations that can be performed on a device to their corresponding
implementation functions.

Each entry in the device switch table represents a specific device and contains
function pointers to the device-specific implementations of operations such as
read, write, open, close, etc. The table is typically indexed by the major
device number, which identifies the type of device.

The devsw structure has two function pointer members: read and write. These
function pointers point to device-specific implementations of read and write
operations for a particular device.
*/
struct devsw {
    /*
    The read function pointer takes a pointer to an inode structure, a character
    buffer to store the read data, and the size of the buffer. It performs the
    necessary operations to read data from the device and returns the number of
    bytes read or an error code.
    */
    int (*read)(struct inode*, char*, int);
    /*
    the write function pointer takes a pointer to an inode structure, a
    character buffer containing the data to be written, and the size of the
    data. It performs the necessary operations to write the data to the device
    and returns the number of bytes written or an error code.
    */
    int (*write)(struct inode*, char*, int);
};

/*
typically contains function pointers that point to device-specific
implementations of read, write, open, close, and other device operations. It
allows the operating system to interact with different devices using a common
interface, abstracting away the specific details of each device.

By using an array of structures, each element in the devsw array corresponds to
a specific device, and the array as a whole provides a unified interface for
managing and accessing different devices in the system.
*/
extern struct devsw devsw[];

#endif