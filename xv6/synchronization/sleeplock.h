#include ".././type/types.h"
#include "./spinlock.h"

#ifndef SLEEPLOCK_H
#define SLEEPLOCK_H

/*
Long-term locks for processes

A synchronization primitive used to protect resources or critical sections in a
multi-threaded or multi-process environment.

Blocking vs. Spinning: When a thread tries to acquire a sleeplock and it is
already held by another thread, the thread will block (i.e., put to sleep) until
the lock becomes available. On the other hand, when a thread tries to acquire a
spinlock and it is already held by another thread, the thread will spin in a
loop continuously checking for the lock's availability. Spinning is a
busy-waiting technique where the thread repeatedly checks the lock, consuming
CPU resources.

Resource Utilization: sleeplock is more efficient in terms of resource
utilization because it puts the waiting thread to sleep, allowing the CPU to be
used by other threads or processes. This is suitable when the expected waiting
time is relatively long. In contrast, spinlock consumes CPU resources as it
continuously spins in a loop until the lock is available. It is more suitable
for scenarios where the expected waiting time is short, and spinning is expected
to be more efficient than putting the thread to sleep.

Priority Inversion: sleeplock is less prone to priority inversion issues.
Priority inversion occurs when a low-priority thread holding a lock prevents a
higher-priority thread from acquiring the lock. With sleeplock, the blocked
thread is put to sleep, allowing higher-priority threads to execute. In
contrast, spinlock can suffer from priority inversion because the spinning
thread continues to hold the CPU, potentially blocking higher-priority threads
from executing.

Implementation Complexity: Implementing sleeplock is generally more complex
than spinlock. sleeplock requires additional mechanisms, such as a sleeping
queue, to handle the blocking and waking up of threads. On the other hand,
spinlock is relatively simpler as it relies on spinning in a loop until the lock
becomes available.

In summary, sleeplock is suitable when the expected waiting time is long, and
efficient resource utilization is desired. It avoids wasting CPU resources and
prevents priority inversion. spinlock is suitable when the expected waiting time
is short, and the overhead of putting threads to sleep and waking them up is
considered excessive. It is simpler but can consume CPU resources and is more
prone to priority inversion. The choice between sleeplock and spinlock depends
on the specific requirements and characteristics of the multi-threaded or
multi-process system.
*/
struct sleeplock {
    /*
    indicates whether the lock is currently held or not. If locked is non-zero,
    it means the lock is held. If it is zero, it implies the lock is available
    and can be acquired.
    */
    uint locked;
    /*
    A spin lock that protects the sleep lock itself. It ensures exclusive access
    to the sleep lock structure and guarantees that only one thread or process
    can modify or access the sleep lock at a time.
    */
    struct spinlock lk;

    // For debugging:
    /*
    holds the name or identifier of the sleep lock. It is typically used for
    debugging or informational purposes to help identify the specific lock being
    referred to.
    */
    char* name;
    /*
    process ID (PID) of the process that currently holds the sleep lock. It is
    used for debugging or tracking purposes to identify which process has
    acquired the lock.
    */
    int pid;
};

#endif