#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "../memory/memlayout.h"
#include "../memory/mmu.h"
#include "../x86.h"
#include "proc.h"
#include "../synchronization/spinlock.h"
#include "../userLand/user.h"
#include "../userLand/ulib.h"
#include "../drivers/lapic.h"
#include "../memory/vm.h"

/*
ptable is a structure that represents the process table in an operating system.
The process table is a data structure used by the operating system to keep track
of all the processes currently running or waiting to be executed.
*/
struct {
    /*
    a spinlock used to provide synchronization when accessing or modifying the
    process table. The spinlock ensures that only one thread can acquire the
    lock at a time, preventing concurrent access and maintaining consistency in
    the process table.
    */
    struct spinlock lock;
    /*
    array of NPROC elements representing the individual process entries in the
    process table. Each element of the proc array corresponds to a specific
    process and contains information and state related to that process, such as
    process ID, process state, CPU context, and other relevant data.
    */
    struct proc proc[NPROC];
} ptable;

/*
Used to store a pointer to the initial process created during system
initialization. It allows easy access to the initial process from various parts
of the xv6 operating system.
*/
static struct proc* initproc;
/*
integer that keeps track of the next available process ID (pid) to be assigned
to a newly created process. It is initialized with a value of 1, indicating that
the first process that will be created will have a pid of 1.
*/
int nextpid = 1;
extern void forkret(void);
/*
Handles trap returns. A trap return refers to the process of returning control
from a trap or interrupt handler back to the interrupted code. trapret function
is defined in the trapasm.S. trapret function is responsible for restoring the
saved registers and executing the iret instruction, which is used to return from
the trap or interrupt handling routine and resume the execution of the
interrupted code.
*/
extern void trapret(void);

static void wakeup1(void* chan);

/*
responsible for initializing the process table lock (ptable.lock)

By initializing the process table lock, the pinit() function prepares the lock
for synchronization and concurrent access by multiple processes or threads. It
ensures that proper synchronization mechanisms are in place when manipulating
the process table data structure, preventing data corruption and ensuring
consistency in a multi-threaded environment.
*/
void pinit(void) {
    initlock(&ptable.lock, "ptable");
}

/*
Returns the ID of the current CPU.
Must be called with interrupts disabled
*/
int cpuid() {
    return mycpu() - cpus;
}

/*
used to retrieve a pointer to the struct cpu corresponding to the current CPU
Must be called with interrupts disabled to avoid the caller being rescheduled
between reading lapicid and running through the loop.
*/
struct cpu* mycpu(void) {
    int apicid;
    int i;
    /*
    checks if interrupts are enabled by examining the Interrupt Flag (IF) in
    the flags register (eflags). If interrupts are enabled (readeflags() &
    FL_IF evaluates to true), it indicates a programming error, and the kernel
    panics with the message "mycpu called with interrupts enabled." This check
    ensures that the function is always called with interrupts disabled to
    avoid potential issues related to interrupt handling during the retrieval
    of the current CPU.
    */
    if (readeflags() & FL_IF)
        panic("mycpu called with interrupts enabled\n");

    /*
    retrieves the Local Advanced Programmable Interrupt Controller (APIC) ID of
    the current CPU by calling the lapicid() function. The APIC ID uniquely
    identifies each CPU in the system.
    */
    apicid = lapicid();
    /*
    APIC IDs are not guaranteed to be contiguous. Maybe we should have
    a reverse map, or reserve a register to store &cpus[i].
    Iterates through the array of struct cpu called cpus to find the entry that
    matches the retrieved APIC ID. The loop compares the APIC ID of each struct
    cpu with the retrieved APIC ID until a match is found.
    */
    for (i = 0; i < ncpu; ++i) {
        if (cpus[i].apicid == apicid)
            /*
            returns a pointer to the corresponding struct cpu entry.
            */
            return &cpus[i];
    }
    panic("unknown apicid\n");
}

/*
Used to retrieve a pointer to the struct proc representing the currently running
process for the calling CPU.

Disable interrupts so that we are not rescheduled while reading proc from the
cpu structure
*/
struct proc* myproc(void) {
    pushcli();
    struct cpu* c = mycpu();
    struct proc* p = c->proc;
    popcli();
    return p;
}

/*
Responsible for allocating a new process structure (struct proc) and
initializing its fields
*/
static struct proc* allocproc(void) {
    struct proc* p;
    char* sp;

    acquire(&ptable.lock);
    /*
    Iterate over the process table (ptable.proc) to find an unused process slot.
    This is done by checking the state field of each process.
    */
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == UNUSED) {
            p->state = EMBRYO;
            p->pid = nextpid++;

            release(&ptable.lock);

            /*
            Allocate a kernel stack for the process using the kalloc() function.
            If the allocation fails (returns 0), set the state of the process to
            UNUSED and return 0
            */
            if ((p->kstack = kalloc()) == 0) {
                p->state = UNUSED;
                return 0;
            }
            /*
            Calculate the stack pointer (sp) to the top of the kernel stack.
            */
            sp = p->kstack + KSTACKSIZE;

            /*
            Reserve space for the trap frame on the kernel stack by subtracting
            its size from the stack pointer. The trap frame (p->tf) is used to
            save and restore the CPU state during traps and context switches.
            */
            sp -= sizeof *p->tf;
            p->tf = (struct trapframe*)sp;

            /*
            sp represents the current stack pointer, and it is decremented by 4
            bytes to make room for the return address of the trapret function.
            The value of trapret is stored at the location pointed to by sp.
            This return address is used to return to the point in the code where
            a trap occurred when the process is scheduled to run again.
            */
            sp -= 4;
            *(uint*)sp = (uint)trapret;

            /*
            Reserve space and link the context struct with the stack

            initializes the memory pointed to by p->context with zeros. It sets
            all the fields of the struct context to zero, ensuring a clean and
            predictable initial state for the context.

            sets the eip field of the struct context to the address of the
            forkret function.
            */
            sp -= sizeof *p->context;
            p->context = (struct context*)sp;
            memset(p->context, 0, sizeof *p->context);
            p->context->eip = (uint)forkret;

            return p;
        }
    /*
    If no unused slot is found, release the process table lock and return 0 to
    indicate failure in allocating a process.
    */
    release(&ptable.lock);
    return 0;
}

/*
responsible for setting up the first user process
*/
void userinit(void) {
    /*
    refer to the beginning address and size of the initial user program's code
    initcode.S
    */
    extern char _binary_initcode_start[], _binary_initcode_size[];

    /*
    allocates a new process using allocproc() and assigns it to the p variable.
    The allocproc() function allocates a struct proc to represent the new
    process.
    */
    struct proc* p = allocproc();
    /*
    Sets initproc (a global variable) to point to the newly created process,
    indicating that this process is the initial user process.
    */
    initproc = p;
    /*
    sets up the page directory for the process by calling setupkvm(). The
    setupkvm() function allocates and initializes a new kernel page table for
    the process. If the allocation fails (returns 0), the kernel panics with an
    "out of memory" error.
    */
    if ((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");

    /*
    initializes the user address space by calling inituvm() with the process's
    page directory, the starting address of the initial user program's code, and
    the size of the code. The inituvm() function maps the code pages into the
    process's address space.
    */
    inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
    /*
    sets the process's size (sz) to PGSIZE, which is the size of a page.
    */
    p->sz = PGSIZE;
    /*
    initializes the process's trap frame (tf) by zeroing it out with memset().
    */
    memset(p->tf, 0, sizeof(*p->tf));
    /*
    sets the appropriate segment registers in the trap frame (cs, ds, es, ss) to
    specify the user code and data segments.
    */
    p->tf->trapframeHardware.cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->trapframeSystem.ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->trapframeSystem.es = p->tf->trapframeSystem.ds;
    p->tf->trapframeHardware.ss = p->tf->trapframeSystem.ds;
    /*
    sets the eflags in the trap frame to enable interrupts (FL_IF flag).
    */
    p->tf->trapframeHardware.eflags = FL_IF;
    /*
    sets the stack pointer (esp) in the trap frame to PGSIZE, indicating the top
    of the user stack.
    */
    p->tf->trapframeHardware.esp = PGSIZE;
    /*
    It sets the instruction pointer (eip) in the trap frame to 0, which is the
    beginning of the initcode.S program.
    */
    p->tf->trapframeHardware.eip = 0;

    /*
    copies the string "initcode" into the name field of the process structure p.
    It sets the name of the initial process to "initcode".
    */
    safestrcpy(p->name, "initcode", sizeof(p->name));
    /*
    initializes the current working directory (cwd) of the process by calling
    the namei function with the argument "/". The namei function converts the
    given path name ("/") into a corresponding struct inode representing the
    directory.
    */
    p->cwd = namei("/");

    acquire(&ptable.lock);
    p->state = RUNNABLE;
    release(&ptable.lock);
}

/*
Grow current process's memory by n bytes.
Return 0 on success, -1 on failure.
*/
int growproc(int n) {
    struct proc* curproc = myproc();

    uint sz = curproc->sz;
    if (n > 0) {
        if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if (n < 0) {
        if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    curproc->sz = sz;
    switchuvm(curproc);
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void) {
    int i, pid;
    struct proc* np;
    struct proc* curproc = myproc();

    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }

    // Copy process state from proc.
    if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    np->sz = curproc->sz;
    np->parent = curproc;
    *np->tf = *curproc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->trapframeSystem.eax = 0;

    for (i = 0; i < NOFILE; i++)
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    np->state = RUNNABLE;

    release(&ptable.lock);

    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void) {
    struct proc* curproc = myproc();
    struct proc* p;
    int fd;

    if (curproc == initproc)
        panic("init exiting");

    // Close all open files.
    for (fd = 0; fd < NOFILE; fd++) {
        if (curproc->ofile[fd]) {
            fileclose(curproc->ofile[fd]);
            curproc->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(curproc->cwd);
    end_op();
    curproc->cwd = 0;

    acquire(&ptable.lock);

    // Parent might be sleeping in wait().
    wakeup1(curproc->parent);

    // Pass abandoned children to init.
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->parent == curproc) {
            p->parent = initproc;
            if (p->state == ZOMBIE)
                wakeup1(initproc);
        }
    }

    // Jump into the scheduler, never to return.
    curproc->state = ZOMBIE;
    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void) {
    struct proc* p;
    int havekids, pid;
    struct proc* curproc = myproc();

    acquire(&ptable.lock);
    for (;;) {
        // Scan through table looking for exited children.
        havekids = 0;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->parent != curproc)
                continue;
            havekids = 1;
            if (p->state == ZOMBIE) {
                // Found one.
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                freevm(p->pgdir);
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->state = UNUSED;
                release(&ptable.lock);
                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if (!havekids || curproc->killed) {
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curproc, &ptable.lock);  // DOC: wait-sleep
    }
}

// PAGEBREAK: 42
//  Per-CPU process scheduler.
//  Each CPU calls scheduler() after setting itself up.
//  Scheduler never returns.  It loops, doing:
//   - choose a process to run
//   - swtch to start running that process
//   - eventually that process transfers control
//       via swtch back to the scheduler.
void scheduler(void) {
    struct proc* p;
    struct cpu* c = mycpu();
    c->proc = 0;

    for (;;) {
        // Enable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->state != RUNNABLE)
                continue;

            // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
        }
        release(&ptable.lock);
    }
}

/*
responsible for entering the scheduler, which determines the next process to run
on the CPU.
*/
void sched(void) {
    int intena;
    struct proc* p = myproc();

    /*
    checks some sanity conditions to ensure that the function is called in the
    correct state. It verifies that the caller is holding the ptable.lock, which
    is the lock for the process table
    */
    if (!holding(&ptable.lock))
        panic("sched ptable.lock");
    /*
    checks that the current CPU has only one lock (mycpu()->ncli == 1),
    */
    if (mycpu()->ncli != 1)
        panic("sched locks");
    /*
    check that the process calling sched is not already in the running state
    */
    if (p->state == RUNNING)
        panic("sched running");
    /*
    And check that interrupts are disabled (readeflags() & FL_IF checks if the
    Interrupt Flag is set).
    */
    if (readeflags() & FL_IF)
        panic("sched interruptible");
    /*
    saves the current state of the interrupt enable flag (intena) for the
    current CPU and retrieves the current process (p).
    */
    intena = mycpu()->intena;
    /*
    The swtch function performs a context switch to switch to the next process
    to run. The scheduler function is responsible for selecting the next process
    based on a specific scheduling algorithm.
    */
    swtch(&p->context, mycpu()->scheduler);
    /*
    Restores the interrupt enable flag (intena) for the current CPU.

    During a context switch, the current process's execution context (registers,
    program counter, etc.) is saved, and the execution context of another
    process is loaded to allow it to resume execution. This involves switching
    the values of various registers, including the EFLAGS register, which
    contains the IF flag.
    */
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void) {
    acquire(&ptable.lock);  // DOC: yieldlock
    myproc()->state = RUNNABLE;
    sched();
    release(&ptable.lock);
}

/*
Responsible for initializing the file system and log system when a newly created
child process is scheduled to run for the first time. In fact, only the first
process, the root process, respect the conditions to initialize the file system
mechanism.
*/
void forkret(void) {
    static int first = 1;
    // Still holding ptable.lock from scheduler.
    release(&ptable.lock);
    /*
    This conditional block is executed only when the forkret function is called
    for the first time. executed only once during the entire execution of the
    operating system
    */
    if (first) {
        /*
        Some initialization functions must be run in the context of a regular
        process (e.g., they call sleep), and thus cannot be run from main().
        */
        first = 0;
        iinit(ROOTDEV);
        initlog(ROOTDEV);
    }
}

/*
Used to put the calling process to sleep until it is awakened by another process
or an event. It atomically releases the specified lock (lk) and then sleeps on
the specified channel (chan). When awakened, it reacquires the lock before
returning.
*/
void sleep(void* chan, struct spinlock* lk) {
    struct proc* p = myproc();
    /*
    Checks if the calling process (myproc()) exists
    */
    if (p == 0)
        panic("sleep");
    /*
    Check if the lock (lk) is valid

    While it's possible to imagine scenarios where a process might want to sleep
    without holding a lock, it's generally considered good practice to have a
    well-defined locking strategy and ensure proper synchronization before
    entering a sleep state. Requiring a lock for sleep helps enforce this
    practice and provides a clear synchronization mechanism for managing shared
    resources or critical sections.
    */
    if (lk == 0)
        panic("sleep without lk");

    /*
    If the lock is not the global process table lock (ptable.lock), it acquires
    the global process table lock and releases the specified lock (lk). This is
    done to ensure consistency and avoid missing any wake-up signals while
    changing the process state.
    */
    if (lk != &ptable.lock) {
        acquire(&ptable.lock);
        /*
        The sleep function is typically used when a process wants to voluntarily
        yield the CPU and wait for some condition to be satisfied before
        resuming execution. Before going to sleep, it's necessary to release any
        locks held by the process to avoid holding locks indefinitely while
        waiting. Releasing the lk lock ensures that other threads or processes
        can acquire the lock and continue their execution, potentially making
        progress and satisfying the conditions required for the sleeping process
        to wake up.
        */
        release(lk);
    }
    // Go to sleep.
    p->chan = chan;
    p->state = SLEEPING;

    /*
    change the active process
    */
    sched();

    /*
    sets the chan field of the process (p) to 0, indicating that the process is
    no longer waiting on a particular channel (represented by chan). It is a
    cleanup step to clear the channel association before resuming execution.
    */
    p->chan = 0;

    /*
    This conditional block checks if the lock (lk) that was released before
    entering sleep is the original lock (&ptable.lock) or a different lock. If
    it's a different lock, it proceeds to reacquire the original lock.
    */
    if (lk != &ptable.lock) {
        release(&ptable.lock);
        acquire(lk);
    }
}

// PAGEBREAK!
//  Wake up all processes sleeping on chan.
//  The ptable lock must be held.
static void wakeup1(void* chan) {
    struct proc* p;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == SLEEPING && p->chan == chan)
            p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void* chan) {
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid) {
    struct proc* p;

    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid) {
            p->killed = 1;
            // Wake process from sleep if necessary.
            if (p->state == SLEEPING)
                p->state = RUNNABLE;
            release(&ptable.lock);
            return 0;
        }
    }
    release(&ptable.lock);
    return -1;
}

// PAGEBREAK: 36
//  Print a process listing to console.  For debugging.
//  Runs when user types ^P on console.
//  No lock to avoid wedging a stuck machine further.
void procdump(void) {
    static char* states[] = {
        [UNUSED] "unused",   [EMBRYO] "embryo",  [SLEEPING] "sleep ",
        [RUNNABLE] "runble", [RUNNING] "run   ", [ZOMBIE] "zombie"};
    int i;
    struct proc* p;
    char* state;
    uint pc[10];

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        cprintf("%d %s %s", p->pid, state, p->name);
        if (p->state == SLEEPING) {
            getcallerpcs((uint*)p->context->ebp + 2, pc);
            for (i = 0; i < 10 && pc[i] != 0; i++)
                cprintf(" %p", pc[i]);
        }
        cprintf("\n");
    }
}
