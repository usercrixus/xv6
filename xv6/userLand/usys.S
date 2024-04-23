/*
The usys.S file in xv6 is an assembly file that provides the user-space stubs for system calls. 
These stubs are the pieces of code that user programs call when they want to make a system call. 
The purpose of these stubs is to set up the necessary registers and execute the appropriate 
instructions to transition control to the kernel for handling the system call.
*/

#include "../systemCall/syscallGrid.h"
#include "../systemCall/traps.h"

/*
The SYSCALL macro is a preprocessor directive that generates the assembly code needed 
for each system call stub.
*/
#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; /* This line moves the system call number (defined in syscall.h) into the %eax register. The ## operator in the macro concatenation appends the system call name to SYS_, forming the correct identifier. */ \ 
    int $T_SYSCALL; /* Triggers a software interrupt. $T_SYSCALL is the interrupt number used by xv6 to handle system calls (in traps.h)*/ \ 
    ret /* returns control to the calling function once the system call is complete. */

/*
System Call Stubs: For each system call, the SYSCALL macro is used to generate a stub.
For example, SYSCALL(fork) creates a stub for the fork system call. When a user program calls fork(), 
it's actually calling this stub.
*/
SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
