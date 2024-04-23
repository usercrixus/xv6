#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "../synchronization/spinlock.h"
#include "../synchronization/sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "../userLand/ulib.h"
#include "../synchronization/spinlock.h"
#include "../userLand/user.h"

/*
The logging system in a file system allows concurrent file system system calls
to be logged together as a transaction. It ensures that all updates from
multiple system calls are either fully applied or not applied at all.
*/
struct logheader {
    /*
    represents the number of blocks being logged. It indicates the count of
    blocks that are part of the log transaction.
    */
    int n;
    /*
    stores the block numbers of the logged blocks. The LOGSIZE constant
    determines the size of the array, specifying the maximum number of blocks
    that can be logged in a transaction.
    */
    int block[LOGSIZE];
};

/*
The struct log and its instance log are used to manage and control the log
system in the file system. They provide the necessary data structures and
variables to track and coordinate log-related operations, including
synchronization, tracking outstanding system calls, and managing the log header.
*/
struct log {
    /*
    represents a spinlock used to synchronize access to the log data structure.
    */
    struct spinlock lock;
    /*
    holds the starting block number of the log area. It specifies the first
    block of the disk dedicated to the log.
    */
    int start;
    /*
    represents the size of the log area in terms of the number of blocks. It
    indicates the total number of blocks reserved for logging changes.
    */
    int size;
    /*
    keeps track of the number of file system system calls that are currently
    executing. It is used to ensure that no commit operation takes place while
    there are outstanding system calls in progress. This helps maintain the
    atomicity and consistency of the file system operations.
    */
    int outstanding;
    /*
    used as a flag to indicate that a commit operation is in progress. When
    committing, other operations that modify the file system are paused until
    the commit completes.

    a committing operation refers to the process of persistently writing the log
    entries to disk and updating the file system metadata to reflect the changes
    made during a transaction.
    */
    int committing;
    /*
    stores the device number associated with the file system. It indicates the
    specific device where the file system is located.
    */
    int dev;
    /*
    represents a structure that holds information about the log header. The log
    header contains metadata related to the log, such as the sequence number and
    checksum, which are used for log recovery and verification.
    */
    struct logheader lh;
};
struct log log;

/*
responsible for copying committed blocks from the log to their designated
locations in the file system.
*/
static void install_trans(void) {
    /*
    Iterates over the blocks recorded in the log header (log.lh) to process each
    committed block.
    */
    for (int tail = 0; tail < log.lh.n; tail++) {
        /*
        A buffer lbuf is created by reading the corresponding log block from the
        disk device. log.start + tail + 1 calculates the block number within the
        log area. The +1 is used to calculate the block number within the log
        area. In the xv6 file system, block 0 is reserved for the log header, so
        the actual log data blocks start from block 1.
        */
        struct buf* lbuf = bread(log.dev, log.start + tail + 1);
        /*
        Another buffer dbuf is created by reading the destination block
        associated with the current log block from the disk device. The
        destination block number is obtained from log.lh.block[tail].
        */
        struct buf* dbuf = bread(log.dev, log.lh.block[tail]);
        /*
        contents of the log block (lbuf->data) are copied to the destination
        block (dbuf->data) using the memmove function. This effectively
        transfers the data from the log block to its original location in the
        file system.
        */
        memmove(dbuf->data, lbuf->data, BSIZE);
        /*
        written back to the disk device, persisting the changes made during the
        log replay process.
        */
        bwrite(dbuf);
        /*
        Released after the necessary operations have been performed.
        */
        brelse(lbuf);
        brelse(dbuf);
    }
}

/*
responsible for reading the log header from disk into the in-memory log header
structure : struct log log;
*/
static void read_head(void) {
    struct buf* buf = bread(log.dev, log.start);
    struct logheader* lh = (struct logheader*)(buf->data);
    /*
    The n field of the in-memory log header (log.lh) is updated with the value
    of n from the on-disk log header. It reflects the number of blocks logged in
    the transaction.
    */
    log.lh.n = lh->n;
    /*
    copies the block numbers from an on-disk log header (lh->block[]) into the
    corresponding block numbers in the in-memory log header (log.lh.block[]).
    */
    for (int i = 0; i < log.lh.n; i++) {
        log.lh.block[i] = lh->block[i];
    }
    brelse(buf);
}

/*
responsible for writing the in-memory log header to the disk. This operation
signifies the true point at which the current transaction commits. Updates the
on-disk log header with the correct count of logged blocks and their block
numbers. This marks the commit point of the current transaction, ensuring that
the changes recorded in the log are persisted on disk and can be used for
recovery in case of system crashes or restarts.
*/
static void write_head(void) {
    /*
    Reads the log header block from the disk into a buffer (buf) using the
    log.dev and log.start parameters
    */
    struct buf* buf = bread(log.dev, log.start);
    struct logheader* hb = (struct logheader*)(buf->data);
    /*
    copies each block number from the in-memory log header (log.lh.block[i]) to
    the corresponding block number in the on-disk log header (hb->block[i]).
    This ensures that the on-disk log header reflects the block numbers of the
    logged blocks in the in-memory log header.
    */
    for (int i = 0; i < log.lh.n; i++) {
        hb->block[i] = log.lh.block[i];
    }
    bwrite(buf);
    brelse(buf);
}

/*
Responsible for recovering the file system by replaying any pending log entries
and bringing the file system to a consistent state. Here's an explanation of the
steps involved:
*/
static void recover_from_log(void) {
    /*
    It retrieves the information about the logged blocks stored in the log
    header. The log header contains the block numbers of the blocks that are
    part of the logged transaction.
    */
    read_head();
    /*
    This function installs the committed blocks from the log to their home
    locations on the disk. It iterates through each logged block, reads the
    corresponding block from the log area, and copies its contents to the
    destination block on the disk. This ensures that the changes made during the
    logged transaction are applied to the file system's persistent storage.
    */
    install_trans();
    /*
    Once the committed blocks are successfully copied from the log to the disk,
    the log.lh.n field is set to 0. This clears the number of logged blocks
    stored in the log header, indicating that there are no pending log entries
    remaining.
    */
    log.lh.n = 0;
    write_head();  // clear the log
}

/*
called at the start of each file system system call in xv6. Its purpose is to
coordinate access to the file system log.
*/
void begin_op(void) {
    acquire(&log.lock);
    while (1) {
        /*
        If log.committing is true, it means a commit operation is in progress,
        so the current thread goes to sleep by calling sleep(&log, &log.lock).
        This allows other threads to proceed with their operations until the
        commit is finished.
        */
        if (log.committing) {
            sleep(&log, &log.lock);
        } else if (log.lh.n + (log.outstanding + 1) * MAXOPBLOCKS > LOGSIZE) {
            /*
            The condition compares the sum of the current log size (log.lh.n)
            and the estimated size of the upcoming operation ((log.outstanding +
            1) * MAXOPBLOCKS) with the maximum log size (LOGSIZE).

            If the condition evaluates to true, it means that executing the
            current operation might exceed the available space in the log. In
            this case, the thread needs to wait until the committing operation
            is complete before proceeding, to prevent the log from becoming full
            and potentially causing data corruption.
            */
            sleep(&log, &log.lock);
        } else {
            /*
            If neither of the above conditions is met, it means the thread can
            proceed with its operation. It increments the log.outstanding
            counter to indicate that there is an ongoing operation.
            */
            log.outstanding += 1;
            release(&log.lock);
            break;
        }
    }
}

/*
copy modified blocks from the cache to the log in the xv6 file system. It is an
essential step in the commit process to ensure that the modified data blocks are
persisted in the log, which acts as a temporary storage for changes before they
are permanently written to disk.
*/
static void write_log(void) {
    for (int tail = 0; tail < log.lh.n; tail++) {
        /*
        The reason for +1 is to reserve the first block in the log region for
        the log header, which contains metadata about the log.
        */
        struct buf* to = bread(log.dev, log.start + tail + 1);
        struct buf* from = bread(log.dev, log.lh.block[tail]);
        memmove(to->data, from->data, BSIZE);
        bwrite(to);
        brelse(from);
        brelse(to);
    }
}

/*
Performe the commit operation in the xv6 file system. It ensures that any
modifications made to the file system's data blocks are persisted to disk.
*/
static void commit() {
    if (log.lh.n > 0) {
        write_log();      // Write modified blocks from cache to log
        write_head();     // Write header to disk -- the real commit
        install_trans();  // Now install writes to home locations
        log.lh.n = 0;
        write_head();  // Erase the transaction from the log
    }
}

/*
called at the end of each file system system call. Its purpose is to handle the
finalization and potential commit of the outstanding file system operation.
*/
void end_op(void) {
    if (log.committing)
        panic("log.committing");

    acquire(&log.lock);

    --log.outstanding;
    if (log.outstanding == 0) {
        log.committing = 1;
    } else {
        /*
        begin_op() may be waiting for log space, and decrementing
        log.outstanding has decreased the amount of reserved space.
        */
        wakeup(&log);
    }
    release(&log.lock);

    if (log.committing) {
        commit();
        acquire(&log.lock);
        log.committing = 0;
        wakeup(&log);
        release(&log.lock);
    }
}

/*
used to record a modified block in the log for later disk write during a
transaction in a file system.

b represents the buffer that contains the modified block of data that needs to
be recorded in the log.

a transaction refers to a sequence of operations that need to be performed
atomically. It represents a logical unit of work where a set of modifications or
updates are made to the file system's data structures. These modifications could
include creating or deleting files, modifying file metadata, or changing data
blocks.

The purpose of performing operations within a transaction is to ensure
consistency and integrity of the file system. By grouping related operations
together as a transaction, it guarantees that either all the operations within
the transaction are successfully completed and applied to the file system, or
none of them are applied.
*/
void log_write(struct buf* b) {
    int i;
    /*
    checks if the number of logged blocks (log.lh.n) has exceeded the maximum
    log size (LOGSIZE) or the available space in the log (log.size - 1). If the
    condition is true, it raises a panic to indicate that the transaction is too
    large to fit in the log.
    */
    if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
        panic("too big a transaction");
    if (log.outstanding < 1)
        panic("log_write outside of trans");

    acquire(&log.lock);
    /*
    iterates over the logged blocks in the log header (log.lh) to check if the
    current block being logged (b->blockno) is already present in the log. This
    step is known as log absorption, where duplicate blocks are not logged again
    to conserve space.
    */
    for (i = 0; i < log.lh.n; i++) {
        if (log.lh.block[i] == b->blockno)  // log absorbtion
            break;
    }

    if (i == log.lh.n) {
        /*
        If the current block is not found in the log header, it is added to the
        log by storing its block number (b->blockno) in the log header's block
        array (log.lh.block).
        */
        log.lh.block[i] = b->blockno;
        /*
        If the current block was added to the log header (i.e., it is not a
        duplicate), the number of logged blocks (log.lh.n) is incremented.
        */
        log.lh.n++;
    }
    /*
    The block's flags are updated by setting the B_DIRTY flag. This flag
    prevents the block from being evicted from the cache before it is written to
    the disk. It ensures that the modified block remains in the cache until the
    transaction is committed and the changes are written to disk.
    */
    b->flags |= B_DIRTY;  // prevent eviction
    release(&log.lock);
}

/*
responsible for initializing the log system for a given device (dev).

In a file system, the log system is a mechanism used to ensure the consistency
and durability of data on disk. It is designed to handle crash recovery,
maintaining the integrity of the file system even in the event of system
failures or power outages.

The log system works by recording changes to the file system in a log, which is
a sequential record of modifications. Instead of directly modifying the file
system's metadata structures (such as inodes and data blocks), the changes are
first written to the log in a specific format. These logged changes are then
later applied to the file system's structures during recovery.
*/
void initlog(int dev) {
    /*
    This condition checks if the size of the logheader structure is greater than
    or equal to the block size (BSIZE). The logheader structure is a data
    structure used to store information about the log in the file system. If the
    size of logheader is larger than or equal to the block size, it indicates
    that the log header is too big to fit within a single block, which is not
    supported. In such a case, the system panics with an error message.
    */
    if (sizeof(struct logheader) >= BSIZE)
        panic("initlog: too big logheader");

    struct superblock sb;
    initlock(&log.lock, "log");
    readsb(dev, &sb);
    log.start = sb.logstart;
    log.size = sb.nlog;
    log.dev = dev;
    recover_from_log();
}