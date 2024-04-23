/*
defines the system call interfaces. system calls work with usys.s file
*/

#include "../type/types.h"
#include "../fileSystem/stat.h"
#include "../synchronization/spinlock.h"

/*
Creates a new process by duplicating the calling process.
*/
int fork(void);
/*
Terminates the calling process.
*/
void exit(void);
/*
Waits for a child process to change state (e.g., to terminate).
*/
int wait(void);
/*
Creates a pipe for inter-process communication.
*/
int pipe(int*);
/*
Writes data to a file descriptor.
*/
int write(int, const void*, int);
/*
Reads data from a file descriptor.
*/
int read(int, void*, int);
/*
Closes a file descriptor.
*/
int close(int);
/*
Sends a signal to a process or a group of processes.
*/
int kill(int);
/*
Replaces the current process image with a new process image.
*/
int exec(char*, char**);
/*
Opens a file descriptor for a file.
*/
int open(const char*, int);
/*
Creates a filesystem node (file, device special file, or named pipe).
*/
int mknod(const char*, short, short);
/*
Deletes a name from the filesystem.
*/
int unlink(const char*);
/*
Gets file status information.
*/
int fstat(int fd, struct stat*);
/*
Creates a new link (hard link) to an existing file.
*/
int link(const char*, const char*);
/*
Creates a new directory.
*/
int mkdir(const char*);
/*
Changes the current working directory.
*/
int chdir(const char*);
/*
Duplicates a file descriptor.
*/
int dup(int);
/*
Returns the process ID of the calling process.
*/
int getpid(void);
/*
Increases the program's data space (heap).
*/
char* sbrk(int);
/*
Suspends the process for a specified number of seconds.
*/
void sleep(void* chan, struct spinlock* lk);
/*
Returns the time since the system was booted.
*/
int uptime(void);