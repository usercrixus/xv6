struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct rtcdate;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// bio.c
void binit(void);
struct buf* bread(uint, uint);
void brelse(struct buf*);
void bwrite(struct buf*);

// console.c
void consoleinit(void);
void cprintf(char*, ...);
void consoleintr(int (*)(void));
void panic(char*) __attribute__((noreturn));

// file.c

struct file* filealloc(void);
void fileclose(struct file*);
struct file* filedup(struct file*);
void fileinit(void);
int fileread(struct file*, char*, int n);
int filestat(struct file*, struct stat*);
int filewrite(struct file*, char*, int n);

// fs.c

void readsb(int dev, struct superblock* sb);
int dirlink(struct inode*, char*, uint);
struct inode* dirlookup(struct inode*, char*, uint*);
struct inode* ialloc(uint, short);
struct inode* idup(struct inode*);
void iinit(int dev);
void ilock(struct inode*);
void iput(struct inode*);
void iunlock(struct inode*);
void iunlockput(struct inode*);
void iupdate(struct inode*);
int namecmp(const char*, const char*);
struct inode* namei(char*);
struct inode* nameiparent(char*, char*);
int readi(struct inode*, char*, uint, uint);
void stati(struct inode*, struct stat*);
int writei(struct inode*, char*, uint, uint);

// ide.c
void ideinit(void);
void ideintr(void);
void iderw(struct buf*);

// ioapic.c
void ioapicenable(int irq, int cpu);
extern uchar ioapicid;
void ioapicinit(void);

// kalloc.c

char* kalloc(void);
void kfree(char*);
void kinit1(void*, void*);
void kinit2(void*, void*);

// kbd.c

void kbdintr(void);

// log.c

void initlog(int dev);
void log_write(struct buf*);
void begin_op();
void end_op();

// mp.c
extern int ismp;
void mpinit(void);

// picirq.c
void picenable(int);
void picinit(void);

// pipe.c
int pipealloc(struct file*, struct file*);
void pipeclose(struct pipe*, int);
int piperead(struct pipe*, char*, int);
int pipewrite(struct pipe*, char*, int);

// PAGEBREAK: 16

int cpuid(void);
int growproc(int);
struct cpu* mycpu(void);
struct proc* myproc();
void pinit(void);
void procdump(void);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void setproc(struct proc*);
void userinit(void);
void wakeup(void*);
void yield(void);

/*
responsible for performing a context switch between two sets of registers.

The swtch function takes two arguments: the address of the old context structure
and the address of the new context structure.
*/
void swtch(struct context** old, struct context* new);

// spinlock.c

void acquire(struct spinlock*);
void getcallerpcs(void*, uint*);
int holding(struct spinlock*);
void initlock(struct spinlock*, char*);
void release(struct spinlock*);
void pushcli(void);
void popcli(void);

// sleeplock.c
void acquiresleep(struct sleeplock*);
void releasesleep(struct sleeplock*);
int holdingsleep(struct sleeplock*);
void initsleeplock(struct sleeplock*, char*);

// syscall.c

int argint(int, int*);
int argptr(int, char**);
int argstr(int, char**);
int fetchint(uint, int*);
int fetchstr(uint, char**);
void syscall(void);

// timer.c
void timerinit(void);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))