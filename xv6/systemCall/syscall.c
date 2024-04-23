#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "../memory/memlayout.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "../x86.h"
#include "syscallGrid.h"
#include "syscall.h"
#include "sysfile.h"
#include "sysproc.h"

int fetchint(uint addr, int* ip) {
    // struct proc* curproc = myproc();

    /*
    checks whether the specified address addr is within the valid address range
    of the current process. It ensures that the address is not outside the
    process's allocated memory space (curproc->sz). If the address is invalid,
    indicating that it falls outside the allocated memory range or too close to
    the boundary, the function returns -1 to indicate an error. but... the code
    provided is false and bug prone

    if (addr >= curproc->sz || addr + 4 > curproc->sz)
        return -1;
    */
    *ip = *(int*)(addr);
    return 0;
}

int fetchstr(uint addr, char** pp) {
    char *s, *ep;
    struct proc* curproc = myproc();

    if (addr >= curproc->sz)
        return -1;
    *pp = (char*)addr;
    ep = (char*)curproc->sz;
    for (s = *pp; s < ep; s++) {
        if (*s == 0)
            return s - *pp;
    }
    return -1;
}

int argint(int n, int* ip) {
    /*
    (myproc()->tf->esp): Retrieves the current stack pointer from the
    process control block (myproc()) of the executing process. tf stands for
    "trapframe" and contains information about the current state of the
    process.
    + 4: Adds an offset of 4 bytes to skip the return address on the stack.
    The return address is typically pushed onto the stack by the calling code.
    + 4*n: Adds an additional offset based on the position of the argument
    (n). Each argument is assumed to be 32 bits (4 bytes) in size, so
    multiplying n by 4 calculates the correct offset for the desired argument.
    */
    return fetchint((myproc()->tf->trapframeHardware.esp) + 4 + 4 * n, ip);
}

int argptr(int n, char** pp) {
    int i;

    if (argint(n, &i) < 0)
        return -1;

    *pp = (char*)i;
    return 0;
}

int argstr(int n, char** pp) {
    int addr;
    if (argint(n, &addr) < 0)
        return -1;
    return fetchstr(addr, pp);
}

/*
The array syscalls[] is used to map system call numbers (like SYS_fork,
SYS_exit, etc.) to their respective function implementations. When a system call
is invoked, the kernel uses this array to find and execute the correct function.
This is a part of the kernel's syscall dispatch mechanism, which is essential
for handling system calls made by user programs.

In C, you can use designated initializers to initialize elements of an array at
specific indices. The SYS_XXX values are constant expressions (likely defined
using #define or as enum constants) representing unique indices for each system
call. For example, SYS_fork, SYS_exit, SYS_read, etc., are integer constants
that specify the position in the syscalls array where the corresponding function
pointer should be placed. The line [SYS_fork] sys_fork means that the function
pointer sys_fork is being placed at the index specified by SYS_fork
*/
static int (*syscalls[])(void) = {
    [SYS_fork] sys_fork,   [SYS_exit] sys_exit,     [SYS_wait] sys_wait,
    [SYS_pipe] sys_pipe,   [SYS_read] sys_read,     [SYS_kill] sys_kill,
    [SYS_exec] sys_exec,   [SYS_fstat] sys_fstat,   [SYS_chdir] sys_chdir,
    [SYS_dup] sys_dup,     [SYS_getpid] sys_getpid, [SYS_sbrk] sys_sbrk,
    [SYS_sleep] sys_sleep, [SYS_uptime] sys_uptime, [SYS_open] sys_open,
    [SYS_write] sys_write, [SYS_mknod] sys_mknod,   [SYS_unlink] sys_unlink,
    [SYS_link] sys_link,   [SYS_mkdir] sys_mkdir,   [SYS_close] sys_close,
};

void syscall(void) {
    struct proc* curproc = myproc();
    /*
    retrieves the system call number from the eax register (stored in the trap
    frame tf of the current process's proc structure)
    */
    int num = curproc->tf->trapframeSystem.eax;
    if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
        curproc->tf->trapframeSystem.eax = syscalls[num]();
    } else {
        cprintf("%d %s: unknown sys call %d\n", curproc->pid, curproc->name,
                num);
        /*
        Sets the eax register in the trap frame to -1, indicating an error.
        */
        curproc->tf->trapframeSystem.eax = -1;
    }
}
