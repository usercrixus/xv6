// Mutual exclusion spin locks.

#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "../x86.h"
#include "../memory/memlayout.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "spinlock.h"

/*
Used to initialize a spinlock
*/
void initlock(struct spinlock* lk, char* name) {
    lk->name = name;
    /*
    Sets the locked field of the spinlock structure to 0, indicating that the
    lock is initially not held.
    */
    lk->locked = 0;
    /*
    When the lock is not held, the cpu field is usually set to 0 to indicate
    that no CPU currently holds the lock.
    */
    lk->cpu = 0;
}

/*
Acquire the lock.
Loops (spins) until the lock is acquired.
Holding a lock for a long time may cause other CPUs to waste time spinning to
acquire it.
*/
void acquire(struct spinlock* lk) {
    /*
    Disabling interrupts is necessary to prevent potential deadlocks when
    acquiring a lock. If an interrupt occurs while a lock is being acquired, it
    can lead to a deadlock situation.
    */
    pushcli();  // disable interrupts to avoid deadlock.

    /*
    checks if the lock is already held by the current CPU thread. If the lock is
    held, it indicates a programming error, and the kernel panics with the
    message "acquire".
    */
    if (holding(lk))
        panic("acquire");

    /*
    Using the xchg instruction in a loop is a common and efficient way to
    implement spin locks, especially on x86 processors.
    xchg instruction is atomic. In the context of x86 processors, the xchg
    instruction performs an atomic exchange operation. It atomically swaps the
    values of the specified operand (a register or memory location) with the
    value in the destination operand (register or memory).
    When used to implement synchronization primitives like locks or semaphores,
    the xchg instruction ensures that the critical section protected by the lock
    is accessed exclusively by a single thread or processor. This atomicity
    guarantees that no other thread or processor can observe an intermediate or
    inconsistent state during the exchange operation.
    */
    while (xchg(&lk->locked, 1) != 0) {
        // wait
    }

    /*
    Intrinsic function provided by the GNU C library
    (GCC) to create a full memory barrier. It acts as a synchronization point
    that ensures all memory operations issued before the barrier are completed
    before any subsequent memory operations.
    A memory barrier, also known as a memory fence, is a CPU instruction or
    compiler directive that prevents reordering of memory operations across the
    barrier. It enforces strict ordering of memory accesses to maintain
    consistency and synchronization between different threads or processors.
    */
    __atomic_thread_fence(__ATOMIC_SEQ_CST);

    // Record info about lock acquisition for debugging :
    /*
    stores the CPU structure corresponding to the current CPU thread
    */
    lk->cpu = mycpu();
    getcallerpcs(&lk, lk->pcs);
}

/*
Responsible for releasing a spinlock.
*/
void release(struct spinlock* lk) {
    /*
    Checks if the current CPU holds the lock (holding(lk)). If it doesn't hold
    the lock, it indicates an error and triggers a panic.
    */
    if (!holding(lk))
        panic("release");
    /*
    Sets the lock owner (lk->pcs[0]) and CPU ID (lk->cpu) to 0, indicating that
    the lock is no longer owned by any CPU.
    */
    lk->pcs[0] = 0;
    lk->cpu = 0;

    /*
    A memory barrier that prevents the compiler and the processor from
    reordering memory operations across this point. It ensures that all the
    stores in the critical section (before the lock release) are visible to
    other cores before the lock is released.
    */
    __sync_synchronize();
    /*
    Lock is released by setting lk->locked to 0. This is done using an assembly
    instruction (movl $0, %0). This operation is not atomic in this
    implementation, so using a C assignment is not sufficient.
    */
    asm volatile("movl $0, %0" : "+m"(lk->locked) :);
    /*
    Called to decrement the count of the number of times the CPU disabled
    interrupts (cli instruction) using pushcli(). This ensures that interrupt
    handling is properly restored after releasing the lock.
    */
    popcli();
}

/*
Record the current call stack in pcs[] by following the %ebp chain.
record the call stack of the current execution context. It follows the %ebp
(extended base pointer) chain to traverse the stack frames and extract the
return addresses of the calling functions. It is the address where the control
flow will resume after the called function completes its execution.

By storing the return addresses in the pcs array, the getcallerpcs function
captures the sequence of return addresses from the stack frames. This allows you
to examine the call stack and see the order in which functions were called and
the subsequent return points.

The function takes two parameters: v (a pointer to the base of the current stack
frame) and pcs (an array to store the return addresses).
*/
void getcallerpcs(void* v, uint pcs[]) {
    /*
    Sets the ebp variable to the address of the previous %ebp value. By
    subtracting 2 from v, it accounts for the ebp and %eip values stored on the
    stack before the current stack frame.
    */
    uint* ebp = (uint*)v - 2;
    /*
    The ebp register is used to point to the base or beginning of the current
    stack frame. It is typically used by compilers to access function
    parameters, local variables, and saved registers within a stack frame.

    In a function call, when a new stack frame is created, the previous value of
    ebp is pushed onto the stack as part of the function prologue. This allows
    the current function to access the variables and parameters of the calling
    function.

    example of a stack :

    void function1(int arg1, int arg2) {
    function2(3, 4);
    }

    void function2(int arg3, int arg4) {
        function3(5, 6);
    }

    Higher memory addresses
    ---------------------
    | arg6            |
    ---------------------
    | arg5            |
    ---------------------
    | Return address 3 | <- Points to the next instruction after function3()
    ---------------------
    | Saved %ebp 3    | <- Points to the previous %ebp value (for function2)
    ---------------------
    | arg4            |
    ---------------------
    | arg3            |
    ---------------------
    | Return address 2 | <- Points to the next instruction after function2()
    ---------------------
    | Saved %ebp 2    | <- Points to the previous %ebp value (for function1)
    ---------------------
    | arg2            |
    ---------------------
    | arg1            |
    ---------------------
    | Return address 1 | <- Points to the next instruction after function1()
    ---------------------
    Lower memory addresses
    */
    int i = 0;  // index variable
    while (i < 10) {
        if (ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
            break;
        pcs[i] = ebp[1];      // saved %eip
        ebp = (uint*)ebp[0];  // saved %ebp
        i++;
    }
    /*
    fills the remaining elements of the pcs array with 0 to indicate that there
    are no more valid return addresses.
    */
    while (i < 10) {
        pcs[i] = 0;
        i++;
    }
}

/*
Checks if the current CPU holds the lock
*/
int holding(struct spinlock* lock) {
    int r;
    pushcli();
    r = lock->locked && lock->cpu == mycpu();
    popcli();
    return r;
}

/*
pushcli is part of a mechanism designed to safely disable interrupts while
executing critical sections of code. It's particularly important in a
multi-processor environment where different processors might be running
different parts of the kernel code concurrently.

The function ensures that interrupts are only re-enabled when the number of
popcli calls matches the number of pushcli calls. This prevents premature
enabling of interrupts, which could lead to race conditions or inconsistent
states within the kernel.
*/
void pushcli(void) {
    int eflags;

    eflags = readeflags();
    cli();
    if (mycpu()->ncli == 0)
        mycpu()->intena = eflags & FL_IF;
    mycpu()->ncli += 1;
}
/*
popcli is part of a mechanism designed to safely disable interrupts while
executing critical sections of code. It's particularly important in a
multi-processor environment where different processors might be running
different parts of the kernel code concurrently.

The function ensures that interrupts are only re-enabled when the number of
popcli calls matches the number of pushcli calls. This prevents premature
enabling of interrupts, which could lead to race conditions or inconsistent
states within the kernel.
*/
void popcli(void) {
    if (readeflags() & FL_IF)
        panic("popcli - interruptible");
    if (--mycpu()->ncli < 0)
        panic("popcli");
    if (mycpu()->ncli == 0 && mycpu()->intena)
        sti();
}
