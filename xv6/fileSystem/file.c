#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "fs.h"
#include "../synchronization/spinlock.h"
#include "../synchronization/sleeplock.h"
#include "file.h"

struct devsw devsw[NDEV];
/*
represents the file table

The file table is a data structure used by the operating system to manage open
file descriptors. It maintains information about each open file in the system,
such as the file's inode, offset, and other relevant attributes.
*/
struct {
    /*
    spinlock variable called lock. The spinlock is used to provide
    synchronization and mutual exclusion when accessing and modifying the file
    table. It ensures that only one thread or process can access the file table
    at a time, preventing data corruption or race conditions.
    */
    struct spinlock lock;
    /*
    represents an open file in the operating system and contains information
    such as the file descriptor, the inode associated with the file, the file
    position, and other relevant data.
    */
    struct file file[NFILE];
} ftable;

/*
responsible for initializing the file table (ftable).

A file table is a data structure used by an operating system to manage open file
entries. It keeps track of information about each open file in the system,
including its state, associated metadata, and the file descriptor used to
reference it.

In the context of the xv6 operating system, the file table is represented by the
ftable data structure. It consists of an array of struct file objects, where
each entry corresponds to an open file. The file table provides a centralized
location for storing and organizing information about open files, making it
easier for the operating system to manage file-related operations.

The file table allows the operating system to maintain important details about
each open file, such as the file's position within the file data, the underlying
inode associated with the file, and other relevant information. This information
is necessary for performing file operations, such as reading, writing, seeking,
and closing files.

By using a file table, the operating system can efficiently track and manipulate
open files, providing a layer of abstraction for file access and management. It
enables multiple processes or threads to concurrently access files while
ensuring proper synchronization and data consistency through the use of locks or
other synchronization mechanisms.
*/
void fileinit(void) {
    initlock(&ftable.lock, "ftable");
}

/*
Allocate a file structure in the xv6 operating system. The file structure
represents an open file in the system.
*/
struct file* filealloc(void) {
    acquire(&ftable.lock);
    /*
    iterates over the file structures starting from ftable.file (the beginning
    of the file table) and continues until f reaches ftable.file + NFILE (the
    end of the file table).
    */
    for (struct file* f = ftable.file; f < ftable.file + NFILE; f++) {
        /*
        A slot is found if it is set on 0
        */
        if (f->ref == 0) {
            f->ref = 1;
            release(&ftable.lock);
            return f;
        }
    }
    release(&ftable.lock);
    return 0;
}

/*
increment the reference count for a file structure f. It ensures that the file
structure remains valid and accessible by incrementing its reference count.

f : pointer to the struct file who is duplicate.
returns : f
*/
struct file* filedup(struct file* f) {
    acquire(&ftable.lock);
    if (f->ref < 1)
        panic("filedup");
    f->ref++;
    release(&ftable.lock);
    return f;
}

/*
Responsible for closing a file. It decrements the reference count of the file
structure and performs the necessary cleanup operations when the reference count
reaches 0.
*/
void fileclose(struct file* f) {
    acquire(&ftable.lock);
    if (f->ref < 1)
        panic("fileclose");

    if (--f->ref > 0) {
        release(&ftable.lock);
        return;
    }

    /*
    If f->ref == 0
    */
    if (f->type == FD_PIPE)
        pipeclose(f->pipe, f->writable);
    else if (f->type == FD_INODE) {
        begin_op();
        iput(f->ip);
        end_op();
    }

    f->type = FD_NONE;
    release(&ftable.lock);
}

/*
Responsible for obtaining metadata about a file.
takes two arguments: a pointer to a struct file object named f and a pointer to
a struct stat object named st.
*/
int filestat(struct file* f, struct stat* st) {
    if (f->type == FD_INODE) {
        ilock(f->ip);
        stati(f->ip, st);
        iunlock(f->ip);
        return 0;
    }
    return -1;
}

/*
reads data from a file

struct file* f: It declares an argument named f of type "pointer to struct
file". This argument is a pointer to a struct file object that represents the
file being read.

char* addr: It declares an argument named addr of type "pointer to char". This
argument is a pointer to a memory address where the read data will be stored.

int n: It declares an argument named n of type int. This argument specifies the
number of bytes to be read from the file.
*/
int fileread(struct file* f, char* addr, int n) {
    if (f->readable == 0)
        return -1;
    if (f->type == FD_PIPE)
        return piperead(f->pipe, addr, n);
    if (f->type == FD_INODE) {
        ilock(f->ip);
        int r;
        if ((r = readi(f->ip, addr, f->off, n)) > 0)
            f->off += r;
        iunlock(f->ip);
        return r;
    }
    panic("fileread");
}

/*
writes data to a file.

struct file* f: It declares an argument named f of type "pointer to struct
file". This argument is a pointer to a struct file object that represents the
file being written.

char* addr: It declares an argument named addr of type "pointer to char". This
argument is a pointer to a memory address where the data to be written is
stored.

int n: It declares an argument named n of type int. This argument specifies the
number of bytes to be written to the file.

return : number of byte wrinttent or -1
*/
int filewrite(struct file* f, char* addr, int n) {
    if (f->writable == 0)
        return -1;
    if (f->type == FD_PIPE)
        return pipewrite(f->pipe, addr, n);
    if (f->type == FD_INODE) {
        /*
        calculates the maximum number of bytes that can be written at a time,
        based on the maximum log transaction size and other considerations
        */
        int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * 512;
        int i = 0;
        while (i < n) {
            /*
            calculates the number of bytes remaining to be written.
            */
            int n1 = n - i;
            /*
            ensures that the number of bytes to be written (n1) does not exceed
            the maximum allowed size (max) calculated earlier.
            */
            if (n1 > max)
                n1 = max;

            begin_op();
            ilock(f->ip);
            int r;
            if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
                f->off += r;
            iunlock(f->ip);
            end_op();

            if (r < 0)
                break;
            if (r != n1)
                panic("short filewrite");
            /*
            increments the variable i by the value of r, which represents the
            number of bytes successfully written in the current iteration. This
            helps keep track of the overall progress of the write operation.
            */
            i += r;
        }
        /*
        checks if i is equal to n, which represents the total number of bytes
        intended to be written. If they are equal, it means that all the bytes
        were successfully written, and the function returns n, indicating the
        total number of bytes written.

        If i is not equal to n, it means that not all the bytes were written
        successfully, and the function returns -1 to indicate an error or
        incomplete write operation.
        */
        return i == n ? n : -1;
    }
    panic("filewrite");
}
