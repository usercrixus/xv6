#include "../type/types.h"
#include "defs.h"
#include "../type/param.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "fs.h"
#include "../synchronization/spinlock.h"
#include "../synchronization/sleeplock.h"
#include "file.h"
#include "../userLand/user.h"

/*
Determines the maximum amount of data that can be stored in the pipe.
*/
#define PIPESIZE 512

/*
Represents a pipe, which is a communication channel between processes in an
operating system. It allows one process to write data into the pipe, while
another process can read the same data from the pipe.
*/
struct pipe {
    /*
    spin lock used to synchronize access to the pipe structure. It ensures that
    concurrent operations on the pipe are properly serialized to prevent race
    conditions.
    */
    struct spinlock lock;
    /*
    Represents the actual data buffer of the pipe. It has a fixed size defined
    by PIPESIZE, which determines the maximum amount of data that can be stored
    in the pipe.
    */
    char data[PIPESIZE];
    /*
    Keeps track of the number of bytes that have been read from the pipe. It is
    incremented whenever data is read from the pipe, allowing the reader to keep
    track of its progress.
    */
    uint nread;
    /*
    This field tracks the number of bytes that have been written into the pipe.
    It is incremented when data is written, enabling the writer to keep track of
    the amount of data written.
    */
    uint nwrite;
    /*
    Indicates whether the read end of the pipe is still open. If readopen is
    non-zero, it means the pipe is still open for reading. If it is zero, it
    implies that the read end has been closed.
    */
    int readopen;  // read fd is still open
    /*
    This flag denotes whether the write end of the pipe is still open. If
    writeopen is non-zero, it means the pipe is still open for writing. If it is
    zero, it signifies that the write end has been closed.
    */
    int writeopen;  // write fd is still open
};

/*
allocate and initializing a pipe, and assigning file descriptors to the read and
write ends of the pipe.

struct file** f0: It declares an argument named f0 of type "pointer to pointer
to struct file". This argument is used to pass the address of a pointer to a
struct file object. The function will modify the value of *f0 to point to a
valid struct file object.

struct file** f1: It declares an argument named f1 of type "pointer to pointer
to struct file". This argument is used to pass the address of another pointer to
a struct file object. The function will modify the value of *f1 to point to a
valid struct file object.
*/
int pipealloc(struct file* f0, struct file* f1) {
    struct pipe* p = 0;

    if ((f0 = filealloc()) == 0 || (f1 = filealloc()) == 0 ||
        (p = (struct pipe*)kalloc()) == 0) {
        if (p)
            kfree((char*)p);
        if (f0)
            fileclose(f0);
        if (f1)
            fileclose(f1);
        return -1;
    }

    p->readopen = 1;
    p->writeopen = 1;
    p->nwrite = 0;
    p->nread = 0;
    initlock(&p->lock, "pipe");
    f0->type = FD_PIPE;
    f0->readable = 1;
    f0->writable = 0;
    f0->pipe = p;
    f1->type = FD_PIPE;
    f1->readable = 0;
    f1->writable = 1;
    f1->pipe = p;
    return 0;
}

/*
close a pipe

struct pipe* p: It declares an argument named p of type "pointer to struct
pipe". This argument is a pointer to a struct pipe object, which represents a
pipe.

int writable: It declares an argument named writable of type int. This argument
specifies whether the pipe is writable or not. If writable is non-zero, it
indicates that the pipe is writable; otherwise, it indicates that the pipe is
not writable.
*/
void pipeclose(struct pipe* p, int writable) {
    acquire(&p->lock);
    if (writable) {
        p->writeopen = 0;
    } else {
        p->readopen = 0;
    }
    if (p->readopen == 0 && p->writeopen == 0) {
        release(&p->lock);
        kfree((char*)p);
    } else
        release(&p->lock);
}

/*
writz data into a pipe

struct pipe* p: It declares an argument named p of type "pointer to struct
pipe". This argument is a pointer to a struct pipe object, which represents a
pipe.

char* addr: It declares an argument named addr of type "pointer to char". This
argument is a pointer to the data that needs to be written to the pipe.

int n: It declares an argument named n of type int. This argument specifies the
number of bytes to be written to the pipe.
*/
int pipewrite(struct pipe* p, char* addr, int n) {
    acquire(&p->lock);
    for (int i = 0; i < n; i++) {
        /*
        The loop checks if the pipe is full by comparing p->nwrite (the current
        write index) with p->nread (the current read index) plus the maximum
        size of the pipe (PIPESIZE). If the pipe is full, it means there is no
        available space to write data at the moment.
        */
        while (p->nwrite == p->nread + PIPESIZE) {
            /*
            If the pipe is full, the code enters a while loop, waiting for space
            to become available in the pipe. It checks if the read end of the
            pipe is closed (p->readopen == 0) or if the current process
            (myproc()) has been killed (myproc()->killed). If either condition
            is true, it releases the lock, indicating an error (-1), and
            returns.
            */
            if (p->readopen == 0 || myproc()->killed) {
                release(&p->lock);
                return -1;
            }
            /*
            ensures that any processes waiting on the nread condition are
            awakened and have an opportunity to proceed. In synergy with sleep
            and piperead it is like a ping-pong effect.
            */
            wakeup(&p->nread);
            /*
            puts the current process to sleep until it is awakened by a
            corresponding wakeup call on the nwrite condition. Generaly, when a
            piperead has been performed
            */
            sleep(&p->nwrite, &p->lock);
        }
        /*
        Write a single byte of data from the addr buffer into the circular
        buffer data of the pipe p
        */
        p->data[p->nwrite++ % PIPESIZE] = addr[i];
    }
    /*
    ensures that any processes waiting on the nread condition are
    awakened and have an opportunity to proceed. In synergy with sleep
    and piperead it is like a ping-pong effect.
    */
    wakeup(&p->nread);
    release(&p->lock);
    return n;
}

/*
reade data from a pipe

struct pipe* p: It declares an argument named p of type "pointer to struct
pipe". This argument is a pointer to a struct pipe object, which represents a
pipe. The pipe holds the data that will be read by the piperead function.

char* addr: It declares an argument named addr of type "pointer to char". This
argument is a pointer to the buffer where the read data will be stored. The
piperead function will copy the data read from the pipe into this buffer.

int n: It declares an argument named n of type int. This argument specifies the
number of bytes to read from the pipe. The piperead function will attempt to
read n bytes from the pipe into the buffer specified by addr.
*/
int piperead(struct pipe* p, char* addr, int n) {
    acquire(&p->lock);
    /*
    checks if the pipe is empty ; if their is nothing to read.
    */
    while (p->nread == p->nwrite && p->writeopen) {
        if (myproc()->killed) {
            release(&p->lock);
            return -1;
        }
        sleep(&p->nread, &p->lock);
    }
    /*
    copy pipe data to addr buffer
    */
    int i = 0;
    while (i < n) {
        if (p->nread == p->nwrite)
            break;
        addr[i] = p->data[p->nread++ % PIPESIZE];
        i++;
    }
    /*
    wake up the process who is likely to write on the pipe.
    */
    wakeup(&p->nwrite);
    release(&p->lock);
    return i;
}

/*
how to obtain a pipe in XV6 ?

    int pipe_fds[2];
    char buffer[256];

    if (pipe(pipe_fds) < 0) {
        printf("Error creating pipe\n");
        return 1;
    }

    int read_end_fd = open("pipe", O_RDONLY);   // Open read end of the pipe
    int write_end_fd = open("pipe", O_WRONLY);  // Open write end of the pipe

    printf("Read end FD: %d\n", read_end_fd);
    printf("Write end FD: %d\n", write_end_fd);

    // Now the process can use the file descriptors to read from and write to
    // the pipe
    // ..

    close(read_end_fd);
    close(write_end_fd);

*/
