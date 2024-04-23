// Sleeping locks

#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "../x86.h"
#include "../memory/memlayout.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "../userLand/user.h"

/*
used to initialize a sleeplock structure in xv6.

Struct sleeplock* lk: This is a pointer to a sleeplock structure. The sleeplock
structure is a data structure used in xv6 to represent a lock that, unlike a
regular lock, will cause a thread to sleep (i.e., temporarily stop executing) if
it cannot immediately acquire the lock. The purpose of this function is to
initialize the sleeplock that lk points to.

char* name: This is a string (in C, represented as a pointer to a char, or
character) that provides a human-readable name for the lock. This name might be
used for debugging purposes, so that if there's an issue with the lock (e.g., a
deadlock situation), the name can be printed out to help identify which lock is
causing the issue. This name is typically a short, static string that describes
the purpose of the lock (e.g., "file table").
*/
void initsleeplock(struct sleeplock* lk, char* name) {
    /*
    initializes the underlying lock (lk->lk) of the sleeplock structure. The
    lock is given the name "sleep lock".
    */
    initlock(&lk->lk, "sleep lock");
    lk->name = name;
    lk->locked = 0;
    lk->pid = 0;
}

/*
Used to acquire a sleep lock (struct sleeplock). A sleep lock is a
synchronization primitive used to protect shared resources in a multi-threaded
or multi-process environment.
*/
void acquiresleep(struct sleeplock* lk) {
    /*
    acquires the underlying spin lock (lk->lk) associated with the sleep lock.
    */
    acquire(&lk->lk);
    /*
    waits until the sleep lock is released by another thread or process
    */
    while (lk->locked) {
        sleep(lk, &lk->lk);
    }
    lk->locked = 1;
    lk->pid = myproc()->pid;
    release(&lk->lk);
}

/*
release a sleep lock (struct sleeplock) that was previously acquired. It allows
other threads or processes to acquire the lock and access the protected
resource.
*/
void releasesleep(struct sleeplock* lk) {
    acquire(&lk->lk);
    lk->locked = 0;
    lk->pid = 0;
    wakeup(lk);
    release(&lk->lk);
}

/*
Used to determine if the calling process holds the sleep lock (lk). It checks
whether the lock is currently locked and if the lock is held by the same process
that is calling the function.
*/
int holdingsleep(struct sleeplock* lk) {
    /*
    Acquires the underlying spin lock (lk->lk) associated with the sleep lock to
    ensure mutual exclusion and synchronization while accessing the sleep lock's
    state.
    */
    acquire(&lk->lk);
    /*
    It checks two conditions:

    lk->locked indicates whether the sleep lock is currently locked.

    (lk->pid == myproc()->pid) compares the process ID (pid) of the sleep lock
    with the process ID of the calling process (myproc()->pid). It checks if the
    sleep lock is held by the same process that is calling the function.
    */
    int r = lk->locked && (lk->pid == myproc()->pid);
    /*
    Releases the underlying spin lock, allowing other processes to acquire it.
    */
    release(&lk->lk);
    /*
    Returns r, which represents whether the calling process holds the sleep lock
    (1 if true, 0 if false).
    */
    return r;
}
