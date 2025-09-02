// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
//
// The implementation uses two state flags internally:
// * B_VALID: the buffer data has been read from the disk.
// * B_DIRTY: the buffer data has been modified
//     and needs to be written to disk.

#include "../type/types.h"
#include "defs.h"
#include "../type/param.h"
#include "../synchronization/spinlock.h"
#include "../synchronization/sleeplock.h"
#include "fs.h"
#include "buf.h"

/*
Used to manage a cache of disk buffers. Represents a cache of disk buffers
*/
struct {
    /*
    used as a lock for synchronizing access to the buffer cache.
    */
    struct spinlock lock;
    /*
    array of buf structures, where buf represents a disk buffer
    */
    struct buf buf[NBUF];
    /*
    A member of type buf and represents a linked list of all buffers in the
    cache. The list is maintained using the prev and next pointers in each
    buffer. The head structure acts as a sentinel node and points to the most
    recently used buffer in the cache.
    */
    struct buf* head;
} bcache;

/*
responsible for initializing the buffer cache in xv6. The buffer cache is a
cache of disk blocks in memory, used to speed up disk I/O operations.
*/
void binit(void) {
    /*
    Initializes a spinlock called bcache.lock, which is used to protect
    concurrent access to the buffer cache.
    */
    initlock(&bcache.lock, "bcache");
    /*
    Used to initialize each buffer in the bcache buffer array and link them into
    the linked list.

    By performing these steps for each buffer in the loop, all the buffers in
    the bcache.buf array are initialized and linked together into a linked list.
    The head buffer's next pointer is updated to point to the first buffer in
    the array, and the prev pointers of the first buffer and the head buffer are
    set accordingly to establish the proper links.
    */
    bcache.head = &bcache.buf[0];
    for (int i = 1; i < NBUF; i++) {
        struct buf* b = &bcache.buf[i];
        b->next = bcache.head;
        bcache.head->prev = b;
        bcache.head = b;
        /*
        The sleep lock associated with the current buffer b is initialized. This
        prepares the buffer's sleep lock for synchronization purposes.
        */
        initsleeplock(&b->lock, "buffer");
    }
    bcache.buf[0].next = bcache.head;
    bcache.head->prev = &bcache.buf[0];
}

/*
designed to fetch a block from the buffer cache

provide a mechanism for efficient disk I/O by maintaining a cache of disk blocks
in memory. By reusing buffers that hold recently used blocks, the system can
avoid time-consuming disk operations. This is important because disk I/O
operations are much slower than memory operations. The function ensures that if
a requested block is already in memory, the system uses that instead of fetching
the block from the disk. If the block is not in memory, the function reuses an
unused buffer to fetch the block from the disk.
*/
static struct buf* bget(uint dev, uint blockno) {
    acquire(&bcache.lock);
    /*
    checks if the requested block is already present in the cache.

    This is a for loop that goes through the linked list of buffers in the
    buffer cache (bcache). It starts at the first buffer (bcache.head.next) and
    continues until it reaches the head of the list again (b != &bcache.head).
    */
    struct buf* b = bcache.head;
    do {
        /*
        checks each buffer (b) to see if it's the buffer for the requested
        block. It does this by comparing the device number (b->dev) and the
        block number (b->blockno) of the buffer with the requested device and
        block numbers (dev and blockno).
        */
        if (b->dev == dev && b->blockno == blockno) {
            /*
            If it finds the buffer for the requested block, it increments the
            reference count of the buffer. This is to indicate that one more
            process is now using this buffer.
            */
            b->refcnt++;
            /*
            acquires a lock on the found buffer. This prevents other processes
            from using this buffer while the current process is using it.
            */
            acquiresleep(&b->lock);
            /*
            releases the lock on the buffer cache. This allows other processes
            to access the buffer cache while the current process is working with
            the found buffer.
            */
            release(&bcache.lock);

            return b;
        }
        b = b->next;
    } while (b != bcache.head);

    // Not cached; recycle an unused buffer.
    // Even if refcnt==0, B_DIRTY indicates a buffer is in use
    // because log.c has modified it but not yet committed it.
    b = bcache.head;
    do {
        if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->flags = 0;
            b->refcnt = 1;
            release(&bcache.lock);
            acquiresleep(&b->lock);
            return b;
        }
        b = b->next;
    } while (b != bcache.head);

    /*
    better solution is to create a sleep lock. panic is a fast and easy
    solution, who is enough for educationnal purpose. most time, one processor
    will use 2 or 3 buffer. If we have 5 processor for example, we will use 5*2
    or 5*3 buffer (10 or 15) and we have access to NBUF buffers (30 buffers).
    */
    panic("bget: no buffers");
}

/*
return a locked buffer that contains the data of a specific disk block. It first
checks if the block is already in the cache (by calling bget()), and if it is
not, it reads the block from the disk (by calling iderw()). This way, bread()
helps to minimize disk I/O by caching disk blocks in memory.
*/
struct buf* bread(uint dev, uint blockno) {
    struct buf* b = bget(dev, blockno);
    /*
    checks if the buffer is valid. b->flags & B_VALID performs a bitwise AND
    operation between the flags field of the buffer and B_VALID. If the result
    is zero, it means that the buffer is not valid (i.e., it does not contain
    valid data), and it needs to be filled with data from disk. The iderw()
    function is called to read the block from the disk into the buffer or write
    the buffer to the disk if needed.
    */
    if ((b->flags & B_VALID) == 0) {
        iderw(b);
    }
    return b;
}

/*
responsible for writing the contents of a buffer b back to the disk.
*/
void bwrite(struct buf* b) {
    /*
    checks if the buffer b is currently locked by the caller. The holdingsleep
    function is used to determine if the lock is held by the current thread. If
    the lock is not held, it means that the caller does not have the necessary
    lock on the buffer, and it results in a panic.
    */
    if (!holdingsleep(&b->lock))
        panic("bwrite");
    /*
    The B_DIRTY flag is set in the flags field of the buffer b. This flag
    indicates that the buffer's contents have been modified and need to be
    written back to the disk.
    */
    b->flags |= B_DIRTY;
    /*
    IT LOOK TO NOT WORK CORRECTLY... Not call the good function.

    Performs the actual reading or writing of the buffer's contents to the disk.
    In this case, since the B_DIRTY flag is set, it indicates a write operation,
    and the buffer's contents will be written to the disk.
    */
    iderw(b);
}

/*
responsible for releasing a locked buffer b and moving it to the head of a "most
recently used" (MRU) list.
*/
void brelse(struct buf* b) {
    /*
    checks whether the current CPU is holding the lock for the buffer b. If it
    is not, the system panics and throws an error because brelse() is supposed
    to be called when you're done with a buffer that you had locked.
    */
    if (!holdingsleep(&b->lock))
        panic("brelse");

    /*
    This line releases the lock on the buffer b. This means that other processes
    are now able to acquire this lock.
    */
    releasesleep(&b->lock);
    /*
    acquire the lock on the entire buffer cache. This is necessary because we're
    going to modify the lin
    */
    acquire(&bcache.lock);
    /*
    the reference count for the buffer b.
    */
    b->refcnt--;
    /*
    If b->refcnt is 0, that means the buffer is not in use by any process.
    */
    if (b->refcnt == 0 && b != bcache.head) {
        /*
        remove b from the list
        */
        b->next->prev = b->prev;
        b->prev->next = b->next;
        /*
        set b next & prev
        */
        b->next = bcache.head;
        b->prev = bcache.head->prev;
        /*
        insert b in the list
        */
        bcache.head->prev->next = b;
        bcache.head->prev = b;
        bcache.head = b;
        /*
        After these steps, b will be at the head of the MRU list, and the rest
        of the list will remain in its original order. If the same buffer b is
        needed again soon, it can be found quickly because it's now at the front
        of the list.
        */
    }

    release(&bcache.lock);
}
