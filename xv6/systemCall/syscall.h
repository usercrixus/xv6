/*
syscall.c contains the mechanisms for fetching arguments passed to system calls,
a mapping of system calls to their handling functions, and the primary function
(syscall) that orchestrates the process of handling a system call made by a user
program.
*/

/*
Responsible for handling system calls made by processes.
*/
void syscall(void);
/*
Fetch the nth word-sized system call argument as a string pointer. Check that
the pointer is valid and the string is nul-terminated. (There is no shared
writable memory, so the string can't change between this check and being used by
the kernel.)
*/
int argstr(int n, char** pp);
/*
fetches the nth word-sized system call argument as a pointer to a block of
memory of a specified size. It also performs checks to ensure that the pointer
lies within the process's address space.

n : the position of the argument
pp : a pointer to a pointer to char where the fetched pointer will be stored
returns an integer value 0 or -1
*/
int argptr(int n, char** pp);
/*
responsible for fetching the nth 32-bit system call argument.

n : the position of the argument
ip : a pointer to an integer where the fetched argument will be stored
returns an integer value.
*/
int argint(int n, int* ip);
/*
Fetch the nul-terminated string at addr from the current process. Doesn't
actually copy the string - just sets *pp to point at it. Returns length of
string, not including nul.
*/
int fetchstr(uint addr, char** pp);
/*
addr : the address from which to fetch the integer value
ip : a pointer to an integer where the fetched value will be stored
returns an integer value 0 or -1
*/
int fetchint(uint addr, int* ip);