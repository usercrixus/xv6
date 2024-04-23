// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "./type/types.h"
#include "./memory/memlayout.h"
#include "defs.h"
#include "./memory/mmu.h"
#include "type/param.h"
#include "systemCall/traps.h"
#include "synchronization/spinlock.h"
#include "synchronization/sleeplock.h"
#include "./fileSystem/fs.h"
#include "./fileSystem/file.h"
#include "./processus/proc.h"
#include "x86.h"
#include "userLand/user.h"
#include "userLand/ulib.h"
#include "drivers/uart.h"
#include "drivers/lapic.h"

/*
On your keyboard the character : "<-""
Usualy used to remove a character.
*/
#define BACKSPACE 0x100
/*
CRTPORT stands for Cathode Ray Tube Port. It is a register in the memory-mapped
I/O address space that is used for interacting with text-mode video hardware in
some computer systems.

In the context of xv6 or any system using CGA (Color Graphics Adapter) text-mode
display, this is used to manipulate what is displayed on the screen. Writing to
the CRT (Cathode Ray Tube) memory area changes the characters and their
attributes (like color) on the screen.

For example, in xv6, the cgaputc function uses CRTPORT to update the position of
the cursor on the screen after writing a character. This way, the operating
system knows where the next character will be written.

So, in summary, CRTPORT is used to communicate with the video hardware to manage
the screen display in text-mode.
*/
#define CRTPORT 0x3d4

/*
allows you to easily obtain the control character value corresponding to a given
character x (example CTRL-D CTRL-C etc)

example ctrl+D : C(D) = D-'@' = 44-40 = 4
*/
#define C(x) ((x) - '@')
/*
Buffer size to edit the console text
*/
#define INPUT_BUF 128

static void consputc(int);

static int panicked = 0;

/*
Contains a spinlock and a flag to control whether locking is enabled for console
operations. The spinlock is used to ensure exclusive access to the console when
locking is enabled.

Synchronize access to the console, ensuring that only one process can write or
read from the console at a time.
*/
static struct {
    /*
    A spinlock used to synchronize access to the console. A spinlock is a type
    of lock that allows multiple threads or processes to wait for the lock to be
    released. Once the lock is acquired by one thread, other threads spinning on
    the lock will wait until it is released before attempting to acquire it.
    */
    struct spinlock lock;
    /*
    An integer flag indicating whether locking is enabled for the console. If
    locking is set to 1, it means that access to the console operations (such as
    reading and writing) should be synchronized using the lock field. If locking
    is set to 0, no locking is performed, and multiple threads can access the
    console concurrently without synchronization.
    */
    int locking;
} cons;

static void printint(int xx, int base, int sign) {
    static char digits[] = "0123456789abcdef";
    char buf[16];
    int i;
    uint x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i++] = '-';

    while (--i >= 0)
        consputc(buf[i]);
}
// PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void cprintf(char* fmt, ...) {
    int i, c, locking;
    uint* argp;
    char* s;

    locking = cons.locking;
    if (locking)
        acquire(&cons.lock);

    if (fmt == 0)
        panic("null fmt");

    argp = (uint*)(void*)(&fmt + 1);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            consputc(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c) {
            case 'd':
                printint(*argp++, 10, 1);
                break;
            case 'x':
            case 'p':
                printint(*argp++, 16, 0);
                break;
            case 's':
                if ((s = (char*)*argp++) == 0)
                    s = "(null)";
                for (; *s; s++)
                    consputc(*s);
                break;
            case '%':
                consputc('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                consputc('%');
                consputc(c);
                break;
        }
    }

    if (locking)
        release(&cons.lock);
}

/*
This code defines a function called panic which is used to halt the system and
print an error message when an unrecoverable error is detected.
*/
void panic(char* s) {
    int i;
    uint pcs[10];

    cli();
    cons.locking = 0;
    // use lapiccpunum so that we can call panic from mycpu()
    cprintf("lapicid %d: panic: ", lapicid());
    cprintf(s);
    cprintf("\n");
    getcallerpcs(&s, pcs);
    for (i = 0; i < 10; i++)
        cprintf(" %p", pcs[i]);
    panicked = 1;  // freeze other CPU
    for (;;)
        ;
}

/*
Indicates that the crt pointer is set to the virtual address corresponding to
the physical address 0xb8000. In this case, 0xb8000 represents the start address
of the memory-mapped region used by the CGA (Color Graphics Adapter) display.

The CGA memory is a special region of memory that is directly mapped to the
video display hardware. It is organized as an array of 16-bit values, where each
value represents a character and its associated attributes (such as color) on
the screen. By accessing this memory region through the crt pointer, the
operating system can read from or write to specific locations in the CGA memory
to display characters and control the screen output.
*/
static ushort* crt = (ushort*)P2V(0xb8000);

/*
Return the CGA cursor position
*/
int getCursorPosition() {
    /*
    sends a command to the CRT control register port (CRTPORT) to indicate that
    we want to read the high byte of the cursor position.
    */
    outb(CRTPORT, 14);
    /*
    Reads the high byte of the cursor position by reading from the CRT control
    register data port (CRTPORT + 1) and shifting the value by 8 bits to the
    left. This value is then stored in the pos variable.
    */
    int pos = inb(CRTPORT + 1) << 8;
    /*
    sends another command to the CRT control register port to indicate that we
    want to read the low byte of the cursor position.
    */
    outb(CRTPORT, 15);
    /*
    Reads the low byte of the cursor position by reading from the CRT control
    register data port and performs a bitwise OR operation with the pos
    variable, combining the high and low bytes to form the complete cursor
    position.
    */
    pos |= inb(CRTPORT + 1);

    return pos;
}

/*
Set the CGA cursor position
*/
void setCursorPosition(int pos) {
    /*
    writes the value 14 to the control register port, indicating that the high
    byte of the cursor position will be sent next.
    */
    outb(CRTPORT, 14);
    /*
    writes the high byte of the cursor position (pos >> 8) to the data port of
    the CGA I/O ports.
    */
    outb(CRTPORT + 1, pos >> 8);
    /*
    writes the value 15 to the control register port, indicating that the low
    byte of the cursor position will be sent next.
    */
    outb(CRTPORT, 15);
    /*
    writes the low byte of the cursor position (pos) to the data port of the CGA
    I/O ports.
    */
    outb(CRTPORT + 1, pos);
    /*
    updates the video memory at the new cursor position (crt[pos]) to display a
    space character with the text attribute 0x0200, which represents green on
    black.
    */
    crt[pos] = ' ' | 0x0200;
}

/*
Scroll the CGA screen by 1 line
*/
int scrollCGA(int pos) {
    /*
    move the contents of the video memory (crt) up by one row. It copies the
    data from the second row to the last row, effectively shifting the
    screen contents up.
    */
    memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
    /*
    adjusts the cursor position (pos) by subtracting 80, which represents
    moving up by one row.
    */
    pos -= 80;
    /*
    clear the last row of the video memory, setting it to all zeros. This
    effectively erases the previous content that was shifted up.
    */
    memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));

    return pos;
}

/*
responsible for outputting a character (c) to the CGA (Color Graphics Adapter)
display.

CGA (Color Graphics Adapter) is a standard graphics display system introduced by
IBM for its early PC models. In the context of the xv6 operating system, CGA
refers to the display mode used for text-based output on the screen.

The CGA display mode has a resolution of 80 columns by 25 rows. However, the
last row (row index 24) is typically used for status information, so the usable
portion of the screen is typically 80 columns by 24 rows.

In the code snippet you provided, the cgaputc function is responsible for
updating the CGA display. It handles cursor movement, scrolling, and character
output on the screen.
*/
static void cgaputc(int c) {
    /*
    current cursor position from the CGA I/O ports (CRTPORT).
    */
    int pos = getCursorPosition();

    if (c == '\n')
        /*
        If the character c is a newline ('\n'), it moves the cursor to the
        beginning of the next line.

        pos % 80 calculates the current column position of the cursor by taking
        the remainder of the current position divided by 80 (the number of
        columns in the console display).

        80 - (pos % 80) determines the number of empty spaces remaining until
        the end of the current line.

        Adding this value to pos effectively moves the cursor to the beginning
        of the next line. For example, if the cursor is at column 5 on the
        current line, pos += 80 - 5 will result in the cursor being positioned
        at column 0 of the next line.
        */
        pos += 80 - pos % 80;
    else if (c == BACKSPACE) {
        /*
        If the character c is a backspace (BACKSPACE), it moves the cursor back
        one position, if it's not already at the beginning of the line.
        */
        if (pos > 0)
            --pos;
    } else
        /*
        For any other character, it writes the character to the video memory at
        the current cursor position (crt[pos]). The character is combined with
        the attribute 0x0200, which sets the text color to green on black.
        */
        crt[pos++] = (c & 0xff) | 0x0200;

    /*
    checks if the cursor position is within the valid range of the video memory.
    */
    if (pos < 0 || pos > 25 * 80)
        panic("pos under/overflow");

    /*
    If the cursor is in the last row of the screen, it scrolls up the entire
    screen by moving the contents of the video memory.
    */
    if ((pos / 80) >= 24) {
        pos = scrollCGA(pos);
    }

    setCursorPosition(pos);
}

/*
responsible for outputting a character (c) to both the UART (serial port) and
the CGA (console display).
*/
void consputc(int c) {
    if (panicked) {
        cli();
        while (1) {
            // loop
        }
    }
    /*
    It first sends a backspace character to the UART to move the cursor back one
    position. Then, it sends a space character to erase the previous character
    on the console. Finally, it sends another backspace character to move the
    cursor back again. This effectively simulates the behavior of erasing a
    character on the console.
    */
    if (c == BACKSPACE) {
        uartputc('\b');
        uartputc(' ');
        uartputc('\b');
    } else
        uartputc(c);  // remote console

    cgaputc(c);  // local console
}

/*
defines a structure named input that represents an input buffer for the console.
*/
struct {
    /*
    An array of characters that serves as the actual input buffer. It can hold
    INPUT_BUF number of characters.
    */
    char buf[INPUT_BUF];
    /*
    the read index, indicating the next position to read from in the buf array.
    */
    uint r;
    /*
    the write index, indicating the next position to write to in the buf array.
    */
    uint w;
    /*
    edit index, used for editing operations on the input buffer.
    */
    uint e;
} input;

/*
an interrupt handler for console input in xv6. It processes input characters
received from the console and performs various actions based on the input
*/
void consoleintr(int (*getc)(void)) {
    int c, doprocdump = 0;

    acquire(&cons.lock);
    while ((c = getc()) >= 0) {
        switch (c) {
            case C('P'):  // Process listing.
                // invoke later, after lock release
                doprocdump = 1;
                break;
            case C('U'):  // Kill line.  erasing the current line.
                while (input.e != input.w &&
                       input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
                    input.e--;
                    consputc(BACKSPACE);
                }
                break;
            case C('H'):
            case '\x7f':  // Backspace, delete a char "<-"
                if (input.e != input.w) {
                    input.e--;
                    consputc(BACKSPACE);
                }
                break;
            default:
                if (c != 0 && input.e - input.r < INPUT_BUF) {
                    c = (c == '\r') ? '\n' : c;
                    input.buf[input.e++ % INPUT_BUF] = c;
                    consputc(c);
                    if (c == '\n' || c == C('D') ||
                        input.e == input.r + INPUT_BUF) {
                        input.w = input.e;
                        wakeup(&input.r);
                    }
                }
                break;
        }
    }
    release(&cons.lock);
    if (doprocdump) {
        procdump();  // now call procdump()
    }
}

/*
responsible for reading input from the console (keyboard) into a buffer.

The function receives an inode pointer ip, a destination buffer pointer dst, and
the number of bytes to read n.
*/
int consoleread(struct inode* ip, char* dst, int n) {
    /*
    unlocks the inode to allow other processes to access it concurrently.

    The consoleread function is typically called with the corresponding inode
    locked. This is done to ensure exclusive access to the inode and prevent any
    concurrent modifications or conflicts while reading from the console.
    */
    iunlock(ip);
    /*
    initializes a variable target with the original number of bytes to read.
    */
    uint target = n;
    /*
    acquires the console lock to ensure exclusive access to the console device
    while reading.
    */
    acquire(&cons.lock);
    while (n > 0) {
        /*
        responsible for waiting until there is input available to read from the
        console. It uses a loop with a condition input.r == input.w to check if
        the input buffer is empty. If it is empty, it means there are no
        characters available to read.
        */
        while (input.r == input.w) {
            /*
            checks if the process executing consoleread has been killed. If the
            process has been killed, it releases the console lock, locks the
            inode, and returns -1 to indicate an error.
            */
            if (myproc()->killed) {
                release(&cons.lock);
                ilock(ip);
                return -1;
            }
            sleep(&input.r, &cons.lock);
        }
        /*
        reads a character from the input buffer at the current read index
        (input.r). It increments the read index afterwards using input.r++. The
        modulus operator % ensures that the read index wraps around within the
        size of the input buffer (INPUT_BUF) if it exceeds the buffer's size.
        */
        int c = input.buf[input.r++ % INPUT_BUF];
        if (c == C('D')) {  // EOF
            /*
            Checks if the number of characters read (n) is less than the target
            number (target) specified by the caller. This check ensures that if
            there are more characters left in the buffer than the requested read
            size, the Ctrl+D character is not consumed and is left in the buffer
            for the next read operation.
            */
            if (n < target) {
                /*
                This ensures that the Ctrl+D character is not consumed and will
                be available in the buffer for the next read operation.
                */
                input.r--;
            }
            /*
            exits the loop and terminates the reading process.
            */
            break;
        }
        /*
        copying the input character c to the destination buffer dst and managing
        the loop counter n
        */
        *dst++ = c;
        --n;
        /*
        checks if the input character c is a newline character ('\n'). If it is,
        it means that a newline has been encountered, and the loop is terminated
        using the break statement. This allows the reading process to stop after
        encountering a newline character, typically used to read input
        line-by-line.
        */
        if (c == '\n')
            break;
    }
    /*
    Release console lock and acquire inode lock
    */
    release(&cons.lock);
    ilock(ip);
    /*
    The target variable represents the initial requested count of characters to
    be read, and n represents the remaining count after processing the input.
    Subtracting n from target gives the number of characters successfully read
    and stored in the destination buffer.
    */
    return target - n;
}

/*
An implementation of the write operation for the console device. It is
responsible for writing data from a buffer buf to the console.
*/
int consolewrite(struct inode* ip, char* buf, int n) {
    /*
    Releases the lock on the inode ip, allowing other processes to access it
    concurrently. It is common practice to release locks before performing
    potentially blocking operations like acquiring a new lock.

    If one process unlocks the inode and another process writes data to the same
    inode before the first process finishes printing to the screen, it can
    introduce a bug or unexpected behavior.

    Unlocking the inode before performing potentially blocking operations, such
    as writing to the console, is generally a good practice to avoid holding
    locks unnecessarily and allowing other processes to access the resource.
    However, in cases where multiple processes are sharing the same resource,
    additional synchronization mechanisms should be used to ensure proper
    coordination and prevent race conditions.
    */
    iunlock(ip);
    /*
    Acquires the lock on the console, ensuring exclusive access to the console
    device while writing to it.
    */
    acquire(&cons.lock);
    /*
    iterates over each character in the buffer and calls the consputc function
    to output the character to the console. The & 0xff operation ensures that
    only the lowest 8 bits of the character are used.
    */
    for (int i = 0; i < n; i++)
        consputc(buf[i] & 0xff);

    /*
    releases the lock on the console, allowing other processes to access it.
    */
    release(&cons.lock);
    /*
    re-acquires the lock on the inode ip, ensuring exclusive access to it again.
    */
    ilock(ip);
    /*
    returns the number of bytes written (n) as an indication of success.
    */
    return n;
}

/*
sets up the necessary components and configurations for the console,
allowing the system to interact with the user through input and output
operations
*/
void consoleinit(void) {
    /*
    Initializes a lock (cons.lock), the console spinlock.
    */
    initlock(&cons.lock, "console");
    /*
    Sets the write and read functions for the console device in the device
    switch table (devsw). This allows other parts of the kernel to interact with
    the console using the defined read and write functions.
    */
    devsw[CONSOLE].write = consolewrite;
    devsw[CONSOLE].read = consoleread;
    cons.locking = 1;
    /*
    enables the keyboard interrupt (IRQ_KBD) in the I/O APIC (Advanced
    Programmable Interrupt Controller). The I/O APIC is responsible for routing
    interrupts from devices to the appropriate CPUs in the system. By enabling
    the keyboard interrupt, the system can handle keyboard input events and
    generate interrupts when keys are pressed or released. The 0 parameter
    passed to ioapicenable specifies the CPU that should handle the interrupt
    (in this case, it is set to 0, indicating the first CPU).
    */
    ioapicenable(IRQ_KBD, 0);
}
