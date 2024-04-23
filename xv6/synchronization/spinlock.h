#ifndef SPINLOCK_H
#define SPINLOCK_H

/*
Mutual exclusion lock. (MUTEX)
*/
struct spinlock {
    /*
    uint locked: This is an unsigned integer that is used to keep track of
    whether the lock is currently held or not. If it is set to 0, the lock is
    not held, and if it is set to 1, the lock is held.
    */
    uint locked;

    // For debugging:

    /*
    char *name: This is a pointer to a character array that holds the name of
    the lock. It is used for debugging purposes to help identify which lock is
    causing problems in case of a deadlock.
    */
    char* name;
    /*
    struct cpu *cpu: This is a pointer to a struct of type cpu that holds
    information about the CPU that is holding the lock. This field is also used
    for debugging purposes.
    */
    struct cpu* cpu;
    /*
    A program counter (PC) is a register that stores the memory address of the
    next instruction to be executed in a program. uint pcs[10]: This is an array
    of 10 unsigned integers that holds the program counters (PCs) of the
    instructions that locked the lock. If a new program counter is to be added
    when pcs is already full, the oldest program counter (pcs[0]) would be
    discarded, and all of the remaining program counters would be shifted down
    one position in the array. This field is also used for debugging purposes,
    example : Let's say Thread A acquires the spinlock and adds its PC to the
    pcs array. Later, Thread B tries to acquire the same lock but finds that it
    is already held by Thread A. Thread B can then examine the pcs array of the
    lock to see where Thread A is currently executing. so that if there is a
    problem with the lock (e.g. it is held for too long), the kernel can print a
    diagnostic message showing the call stack that led to the lock being held.
    */
    uint pcs[10];
};

#endif