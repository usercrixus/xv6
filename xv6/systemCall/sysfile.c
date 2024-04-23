#ifndef sysfile_c
#define sysfile_c
/*
File-system system calls.
Mostly argument checking, since we don't trust user code
*/

#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "../fileSystem/stat.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "../fileSystem/fs.h"
#include "../synchronization/spinlock.h"
#include "../synchronization/sleeplock.h"
#include "../fileSystem/file.h"
#include "../type/fcntl.h"
#include "../userLand/user.h"
#include "../userLand/ulib.h"
#include "sysfile.h"

/*
extract a file descriptor and its corresponding struct file from the system call
arguments. It is typically used in an operating system or kernel code where
system calls are implemented.

takes three parameters:
n : the position of the argument
pfd : a pointer to an integer where the file descriptor will be stored
pf : a pointer to a pointer to struct file where the file structure will be
stored
returns an integer value 0 or -1.
*/
static int argfd(int n, int* pfd, struct file** pf) {
    int fd;
    struct file* f;

    if (argint(n, &fd) < 0)
        return -1;
    /*
    checks if the file descriptor (fd) is within a valid range (0 to NOFILE) and
    if the corresponding struct file (f) exists in the ofile array of the
    current process (myproc()).
    */
    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == 0)
        return -1;

    /*
    update the values pointed to by pfd and pf with the file descriptor (fd) and
    the file structure pointer (f),
    */
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;

    return 0;
}

/*
allocating a file descriptor for a given file.

f : pointer to a struct file.

returns an integer value, which represents the allocated file descriptor on
success or -1 if no available file descriptor can be allocated.
*/
static int fdalloc(struct file* f) {
    int fd;
    struct proc* curproc = myproc();

    for (fd = 0; fd < NOFILE; fd++) {
        if (curproc->ofile[fd] == 0) {
            curproc->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

/*
Check whether a directory is empty (other than containing the standard '.' and
'..' entries). It is useful in operations like removing a directory, where you
need to ensure the directory is empty before proceeding with the removal
*/
static int isdirempty(struct inode* dp) {
    int off;
    struct dirent de;

    for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
        if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
            panic("isdirempty: readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}

/*
Create a new file system inode (index node) for a file or directory.

A file system inode (index node) is a fundamental concept in many file systems,
including UNIX and UNIX-like systems such as Linux and xv6. An inode is a data
structure that stores important information about a file or directory, excluding
its name or actual data content.
*/
static struct inode* create(char* path, short type, short major, short minor) {
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if ((dp = nameiparent(path, name)) == 0)
        return 0;
    ilock(dp);

    if ((ip = dirlookup(dp, name, 0)) != 0) {
        iunlockput(dp);
        ilock(ip);
        if (type == T_FILE && ip->type == T_FILE)
            return ip;
        iunlockput(ip);
        return 0;
    }

    if ((ip = ialloc(dp->dev, type)) == 0)
        panic("create: ialloc");

    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    iupdate(ip);

    if (type == T_DIR) {  // Create . and .. entries.
        dp->nlink++;      // for ".."
        iupdate(dp);
        // No ip->nlink++ for ".": avoid cyclic ref count.
        if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
            panic("create dots");
    }

    if (dirlink(dp, name, ip->inum) < 0)
        panic("create: dirlink");

    iunlockput(dp);

    return ip;
}

int sys_dup(void) {
    struct file* f;
    int fd;

    if (argfd(0, 0, &f) < 0)
        return -1;
    if ((fd = fdalloc(f)) < 0)
        return -1;

    filedup(f);

    return fd;
}

int sys_read(void) {
    struct file* f;
    char* p;
    int n;

    if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p) < 0)
        return -1;

    return fileread(f, p, n);
}

int sys_write(void) {
    struct file* f;
    int n;
    char* p;

    if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p) < 0)
        return -1;

    return filewrite(f, p, n);
}

int sys_close(void) {
    int fd;
    struct file* f;

    if (argfd(0, &fd, &f) < 0)
        return -1;

    myproc()->ofile[fd] = 0;
    fileclose(f);

    return 0;
}

int sys_fstat(void) {
    struct file* f;
    struct stat* st;

    if (argfd(0, 0, &f) < 0 || argptr(1, (void*)&st) < 0)
        return -1;

    return filestat(f, st);
}

int sys_link(void) {
    char name[DIRSIZ], *new, *old;
    struct inode *dp, *ip;

    if (argstr(0, &old) < 0 || argstr(1, &new) < 0)
        return -1;

    begin_op();
    if ((ip = namei(old)) == 0) {
        end_op();
        return -1;
    }

    ilock(ip);
    if (ip->type == T_DIR) {
        iunlockput(ip);
        end_op();
        return -1;
    }

    ip->nlink++;
    iupdate(ip);
    iunlock(ip);

    if ((dp = nameiparent(new, name)) == 0)
        goto bad;
    ilock(dp);
    if (dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0) {
        iunlockput(dp);
        goto bad;
    }
    iunlockput(dp);
    iput(ip);

    end_op();

    return 0;

bad:
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return -1;
}

int sys_unlink(void) {
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], *path;
    uint off;

    if (argstr(0, &path) < 0)
        return -1;

    begin_op();
    if ((dp = nameiparent(path, name)) == 0) {
        end_op();
        return -1;
    }

    ilock(dp);

    // Cannot unlink "." or "..".
    if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
        goto bad;

    if ((ip = dirlookup(dp, name, &off)) == 0)
        goto bad;
    ilock(ip);

    if (ip->nlink < 1)
        panic("unlink: nlink < 1");
    if (ip->type == T_DIR && !isdirempty(ip)) {
        iunlockput(ip);
        goto bad;
    }

    memset(&de, 0, sizeof(de));
    if (writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
        panic("unlink: writei");
    if (ip->type == T_DIR) {
        dp->nlink--;
        iupdate(dp);
    }
    iunlockput(dp);

    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);

    end_op();

    return 0;

bad:
    iunlockput(dp);
    end_op();
    return -1;
}

int sys_open(void) {
    char* path;
    int fd, omode;
    struct file* f;
    struct inode* ip;

    if (argstr(0, &path) < 0 || argint(1, &omode) < 0)
        return -1;

    begin_op();

    if (omode & O_CREATE) {
        ip = create(path, T_FILE, 0, 0);
        if (ip == 0) {
            end_op();
            return -1;
        }
    } else {
        if ((ip = namei(path)) == 0) {
            end_op();
            return -1;
        }
        ilock(ip);
        if (ip->type == T_DIR && omode != O_RDONLY) {
            iunlockput(ip);
            end_op();
            return -1;
        }
    }

    if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
        if (f)
            fileclose(f);
        iunlockput(ip);
        end_op();
        return -1;
    }
    iunlock(ip);
    end_op();

    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    return fd;
}

int sys_mkdir(void) {
    char* path;
    struct inode* ip;

    begin_op();
    if (argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0) {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

int sys_mknod(void) {
    struct inode* ip;
    char* path;
    int major, minor;

    begin_op();
    if ((argstr(0, &path)) < 0 || argint(1, &major) < 0 ||
        argint(2, &minor) < 0 ||
        (ip = create(path, T_DEV, major, minor)) == 0) {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

int sys_chdir(void) {
    char* path;
    struct inode* ip;
    struct proc* curproc = myproc();

    begin_op();
    if (argstr(0, &path) < 0 || (ip = namei(path)) == 0) {
        end_op();
        return -1;
    }
    ilock(ip);
    if (ip->type != T_DIR) {
        iunlockput(ip);
        end_op();
        return -1;
    }
    iunlock(ip);
    iput(curproc->cwd);
    end_op();
    curproc->cwd = ip;
    return 0;
}

int sys_exec(void) {
    char *path, *argv[MAXARG];
    int i;
    uint uargv, uarg;

    if (argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0) {
        return -1;
    }
    memset(argv, 0, sizeof(argv));
    for (i = 0;; i++) {
        if (i >= NELEM(argv))
            return -1;
        if (fetchint(uargv + 4 * i, (int*)&uarg) < 0)
            return -1;
        if (uarg == 0) {
            argv[i] = 0;
            break;
        }
        if (fetchstr(uarg, &argv[i]) < 0)
            return -1;
    }
    return exec(path, argv);
}

int sys_pipe(void) {
    /*
    store the file descriptors for the read and write ends of the pipe.
    */
    int* fd;
    struct file* rf = 0;
    struct file* wf = 0;
    /*
    file descriptors for the read and write ends of the pipe, respectively.
    */
    int fd0, fd1;

    if (argptr(0, (void*)&fd) < 0)
        return -1;
    if (pipealloc(rf, wf) < 0)
        return -1;
    fd0 = -1;
    if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
        /*
        if fd0 succeeds and fd1 fail
        */
        if (fd0 >= 0)
            myproc()->ofile[fd0] = 0;

        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    fd[0] = fd0;
    fd[1] = fd1;
    return 0;
}

#endif