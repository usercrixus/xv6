/*
duplicates an existing file descriptor

returns the newly allocated file descriptor or -1 in case of an error.
*/
int sys_dup(void);

/*
Reads data from a file associated with a file descriptor.
*/
int sys_read(void);
/*
writes data to a file associated with a file descriptor.
*/
int sys_write(void);
/*
close a file descriptor.
*/
int sys_close(void);
/*
Retrieve the metadata of a file.
*/
int sys_fstat(void);
/*
Creates a new link (hard link) to an existing file. This system call associates
another filename with a file that already exists in the filesystem.
*/
int sys_link(void);
/*
Removes a file name from the filesystem. If this name was the last link to the
file and no processes have the file open, the file itself is deleted.
*/
int sys_unlink(void);
/*
Opens a file and returns a file descriptor, which can be used for subsequent
read/write operations. It can also create a file if it does not exist.
*/
int sys_open(void);
/*
Creates a new directory in the filesystem with the specified name.
*/
int sys_mkdir(void);
/*
Creates a filesystem node (a file, device special file, or a named pipe). This
is typically used for creating device files in device filesystems.
*/
int sys_mknod(void);
/*
Changes the current working directory of the calling process to the directory
specified.
*/
int sys_chdir(void);
/*
Replaces the current process image with a new process image, effectively running
a new program within the same process.
*/
int sys_exec(void);
/*
Creates a pair of file descriptors pointing to a pipe, which can be used for
inter-process communication. Data written to one file descriptor can be read
from the other.
*/
int sys_pipe(void);