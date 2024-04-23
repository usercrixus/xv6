/*
Creates a new process. The new process is a copy of the calling process,
referred to as the child process. This system call is typically used for
creating a new process in UNIX-like operating systems.
*/
int sys_fork(void);
/*
Terminates the calling process. When a process calls exit, it ends its execution
and releases resources back to the system. This system call is crucial for
process lifecycle management.
*/
int sys_exit(void);
/*
Waits for a child process to finish execution. The calling process (usually the
parent) is blocked until one of its child processes exits or a signal is
received. This system call is essential for synchronization between parent and
child processes.
*/
int sys_wait(void);
/*
Sends a signal to a process or a group of processes, typically to terminate the
process. The kill system call is used for process control, allowing one process
to control the termination or behavior of another process.
*/
int sys_kill(void);
/*
Returns the process ID (PID) of the calling process. This is often used by
processes to inquire about their own identity in the system.
*/
int sys_getpid(void);
/*
Increases the program's data space by a specified amount. This system call is
commonly used for dynamic memory allocation, where a process needs to increase
its heap size.
*/
int sys_sbrk(void);
/*
Suspends the execution of the calling process for a specified duration. This
allows a process to pause its execution, which can be useful for timing and
synchronization purposes.
*/
int sys_sleep(void);
/*
Returns the amount of time since the system was booted. This can be used for
monitoring, benchmarking, or timing the execution of programs.
*/
int sys_uptime(void);