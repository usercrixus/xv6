/*
Buffer valid is a bit set to know if the buffer is up to date or if it need to
be update
*/
#define B_VALID 0x2
/*
Flag indicates that the buffer's data has been modified and needs to be written
back to the disk.
*/
#define B_DIRTY 0x4

/*
used to represent a disk buffer.

A disk buffer, also known as a disk cache or cache buffer, is a cache memory
that bridges the main memory (RAM) and the storage device, usually a hard disk
drive or solid-state drive. The primary purpose of a disk buffer is to hold data
that is being written to or read from the disk.

When reading data from the disk, data is typically read into the disk buffer
before it's provided to the process that requested it. This is because reading
data from memory is much faster than reading data from a disk. By reading more
data than is immediately necessary, the system can increase the chance that
subsequent read requests can be fulfilled from the buffer (which is fast) rather
than needing to go to the disk (which is slower). This technique is known as
read-ahead.

Similarly, when writing data to the disk, data is first written to the disk
buffer and then written to the disk in the background. This allows the writing
process to continue without having to wait for the slower disk write to
complete. This is a form of write-behind caching.
*/
struct buf {
    /*
    An integer value representing various flags associated with the buffer.
    These flags can be used to track the state of the buffer, such as whether it
    is dirty (needs to be written to disk), whether it is valid (contains valid
    data), etc.
    */
    int flags;
    /*
    An unsigned integer representing the device number associated with the
    buffer. It identifies the disk device to which the buffer belongs.
    */
    uint dev;
    /*
    An unsigned integer indicating the block number on the disk that the buffer
    represents. It specifies the location of the data in the disk.
    */
    uint blockno;
    /*
    An instance of the sleeplock structure, which is used to provide
    synchronization and mutual exclusion when accessing the buffer.
    */
    struct sleeplock lock;
    /*
    An unsigned integer representing the reference count of the buffer. It
    indicates the number of active references to the buffer. The reference count
    is a measure of how many processes are currently using the buffer.
    */
    uint refcnt;
    /*
    prev and next: Pointers to buf structures that allow for linking the buffers
    together to form a linked list. These pointers are used for managing the
    buffer cache, such as maintaining an LRU (Least Recently Used) list.

    This linked list is commonly known as the buffer cache or the buffer pool.

    The buffer cache is a cache-like structure used to cache disk blocks in
    memory. It holds a collection of buffers that correspond to disk blocks. The
    linked list of buffers allows efficient management of these buffers and
    facilitates operations such as finding an available buffer, evicting a
    buffer, or maintaining the order of buffers based on their usage.

    Here's how the linked list of buffers is typically used:

    When a buffer is read from disk or allocated for a new write operation, it
    is added to the front of the linked list. This ensures that the most
    recently used buffers are at the beginning of the list. As buffers are
    accessed or modified, they may move to the front of the list to maintain
    their "recently used" status. When a new buffer is needed, the operating
    system can quickly find an available buffer by traversing the linked list
    and selecting the first buffer that is not currently in use. When the buffer
    cache is full and a new buffer needs to be allocated, the least recently
    used buffer (typically at the end of the list) can be evicted from the
    cache.

    In summary, the linked list of buffers allows efficient management of the
    buffer cache by providing a way to organize and track the usage of buffers,
    ensuring fast access and appropriate buffer replacement strategies.
    */
    struct buf* prev;
    /*
    prev and next: Pointers to buf structures that allow for linking the buffers
    together to form a linked list. These pointers are used for managing the
    buffer cache, such as maintaining an LRU (Least Recently Used) list.
    */
    struct buf* next;
    /*
    Disk Queue
    A pointer to the next buffer in the disk queue. It is used for scheduling
    disk I/O operations and managing the order in which buffers are written to
    or read from the disk.

    A disk queue, also known as an I/O queue, is a data structure used to manage
    pending disk I/O operations. It is typically implemented as a queue or a
    linked list.

    When a process or the operating system initiates a disk I/O operation (such
    as reading from or writing to a disk block), the request is added to the
    disk queue. The disk queue keeps track of the pending I/O operations in the
    order they were requested. The disk controller then processes the requests
    in the disk queue one by one, performing the necessary read or write
    operations on the disk.

    The disk queue serves several purposes:

    Ordering: The disk queue ensures that the disk I/O operations are processed
    in the order they were requested. This is important to maintain data
    integrity and consistency, especially in scenarios where multiple processes
    or threads are performing concurrent disk I/O operations.

    Scheduling: The disk queue allows for the implementation of various
    scheduling algorithms for disk access. Different scheduling algorithms
    prioritize different requests based on factors such as fairness, throughput,
    latency, or seek time optimization.

    Efficiency: By organizing the I/O operations in a queue, the disk controller
    can perform sequential access to disk blocks, minimizing the time spent on
    seeking between different disk locations. This can improve overall disk I/O
    performance.

    The disk queue is typically maintained by the operating system or the disk
    driver as part of the disk subsystem. It provides a mechanism for
    coordinating and managing disk I/O operations, ensuring efficient
    utilization of the disk resources and maintaining the integrity of the
    requested operations.
    */
    struct buf* qnext;
    /*
    An array of unsigned characters (uchar) representing the actual data stored
    in the buffer. The size of the array is defined by BSIZE, which is the block
    size of the file system.
    */
    uchar data[BSIZE];
};