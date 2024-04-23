/*
A Universal Asynchronous Receiver-Transmitter (UART /ˈjuːɑːrt/) is a computer
hardware device for asynchronous serial communication in which the data format
and transmission speeds are configurable. It sends data bits one by one, from
the least significant to the most significant, framed by start and stop bits so
that precise timing is handled by the communication channel. The electric
signaling levels are handled by a driver circuit external to the UART.

It was one of the earliest computer communication devices, used to attach
teletypewriters for an operator console. It was also an early hardware system
for the Internet.

A UART is usually an individual (or part of an) integrated circuit (IC) used for
serial communications over a computer or peripheral device serial port. One or
more UART peripherals are commonly integrated in microcontroller chips.
Specialised UARTs are used for automobiles, smart cards and SIMs.

A related device, the universal synchronous and asynchronous
receiver-transmitter (USART) also supports synchronous operation.

Starting in the 2000s, most IBM PC compatible computers removed their external
RS-232 COM ports and used USB ports that can send data faster.
*/

#include "../type/types.h"
#include "../defs.h"
#include "../type/param.h"
#include "../systemCall/traps.h"
#include "../synchronization/spinlock.h"
#include "../synchronization/sleeplock.h"
#include "../fileSystem/fs.h"
#include "../fileSystem/file.h"
#include "../memory/mmu.h"
#include "../processus/proc.h"
#include "../x86.h"
#include "uart.h"
#include "lapic.h"

/*
represents the base I/O port address of the first serial port (COM1) on
x86-based systems. The serial port is commonly used for communication with
external devices, such as terminals, modems, or other serial devices.
*/
#define COM1 0x3f8

/*
serves as a flag indicating whether there is a UART (serial port) present in the
system.
*/
static int uart;

/*
Responsible for initializing the UART (Universal Asynchronous
Receiver-Transmitter) device

the purpose of uart in xv6 :

# -serial mon:stdio: Redirects the serial port to the standard input/output of
the QEMU # that mean, the UART port of qemu is linked to our terminal
qemu: fs.img xv6.img
        $(QEMU) -serial mon:stdio $(QEMUOPTS)
*/
void uartinit(void) {
    /*
    Turns off the FIFO (First-In-First-Out) feature of the UART. The FIFO is a
    buffer that stores incoming and outgoing data. By turning it off, the UART
    operates in a non-buffered mode.
    */
    outb(COM1 + 2, 0);

    // 9600 baud, 8 data bits, 1 stop bit, parity off.
    /*
    unlocking the divisor latch register (DLAB) of the UART (Universal
    Asynchronous Receiver-Transmitter) to allow the configuration of the baud
    rate.
    */
    outb(COM1 + 3, 0x80);
    /*
    The expression 115200 / 9600 calculates the divisor value based on the
    desired baud rate. The UART's internal clock frequency is typically fixed
    (e.g., 115200 Hz), and by dividing it with the desired baud rate (e.g., 9600
    baud), the appropriate divisor value is obtained. This divisor value is then
    written to the I/O port COM1 + 0, configuring the UART to operate at the
    specified baud rate.

    In the context of UART communication, a baud rate determines the number of
    bits per second that can be transmitted/received. It specifies the speed at
    which the UART communicates and is typically expressed in bits per second
    (bps).
    */
    outb(COM1 + 0, 115200 / 9600);
    /*
    This instruction writes a value of 0 to the UART's Interrupt Enable Register
    (IER). By setting it to 0, interrupts are disabled for the UART. This means
    that the UART will not generate interrupt signals when specific events
    occur, such as data received or transmission completed.
    */
    outb(COM1 + 1, 0);
    /*
    This instruction writes a value of 0x03 to the Line Control Register (LCR)
    of the UART. By setting it to 0x03, it configures the UART for 8 data bits,
    no parity, and 1 stop bit. These settings specify the format of the data
    being transmitted and received, indicating that each data frame consists of
    8 bits, with no parity bit for error detection, and 1 stop bit to mark the
    end of a frame.
    */
    outb(COM1 + 3, 0x03);
    /*
    This instruction writes a value of 0 to the Modem Control Register (MCR) of
    the UART. The specific purpose of this instruction may depend on the UART
    implementation and its usage in the system. It might control features such
    as modem control signals, loopback mode, or other hardware-specific
    functionalities.
    */
    outb(COM1 + 4, 0);
    /*
    This instruction writes a value of 0x01 to the IER register again, but with
    a different value compared to the first outb instruction. By setting it to
    0x01, it enables receive interrupts for the UART. This means that the UART
    will generate interrupt signals when data is received and ready to be read.
    */
    outb(COM1 + 1, 0x01);

    /*
    the status of the serial port is checked to determine if a serial port is
    present or not.

    If the value read from the UART status register is 0xFF, it indicates that
    there is no serial port detected at the specified COM1 port address. In this
    case, the code returns, indicating that the initialization process cannot
    proceed further.

    On the other hand, if the value is not 0xFF, it implies that a serial port
    is present and operational. The code sets the uart variable to 1 to indicate
    that the UART has been successfully initialized.
    */
    if (inb(COM1 + 5) == 0xFF)
        return;
    uart = 1;

    /*
    These operations are commonly performed during the initialization of the
    serial port (UART). They acknowledge any existing interrupts, ensure that
    the interrupt line is enabled, and configure the system to handle interrupts
    generated by the serial port.

    Reads a byte from the specified I/O port address. In this case, it reads a
    byte from the COM1 (serial port 1) + 2 port. This operation is used to
    acknowledge any pre-existing interrupt conditions on the serial port.
    */
    inb(COM1 + 2);
    /*
    reads a byte from the COM1 + 0 port. It is typically used to perform a
    similar acknowledgement or to clear any other interrupt conditions.
    */
    inb(COM1 + 0);
    /*
    enables the interrupt for COM1 in the I/O APIC (Advanced Programmable
    Interrupt Controller). It specifies the IRQ_COM1 as the interrupt number and
    0 as the CPU number, indicating that the interrupt should be handled by the
    first CPU.
    */
    ioapicenable(IRQ_COM1, 0);

    /*
    this loop is sending each character of the string "xv6...\n" to the UART
    (serial port) for output.
    */
    char* p = "xv6...\n";
    while (*p) {
        uartputc(*p);
        p++;
    }
}

void uartputc(int c) {
    if (!uart)
        return;
    /*
    The function enters a loop to wait for the UART transmitter to become ready.
    It checks the status register of the UART (COM1 + 5) by using the inb
    function to read its value. The 0x20 mask is applied to check the Transmit
    Holding Register Empty (THRE) bit, which indicates if the transmitter is
    ready to accept new data.

    The loop continues as long as the transmitter is not ready (!(inb(COM1 + 5)
    & 0x20)) and the maximum loop count (i) is not exceeded. The purpose of this
    loop is to wait for the UART to become ready to transmit the character. The
    microdelay function is called to introduce a small delay between each
    iteration of the loop.
    */
    for (int i = 0; i < 128 && !(inb(COM1 + 5) & 0x20); i++)
        microdelay(10);
    /*
    Once the UART transmitter is ready, the character c is sent to the UART by
    writing it to the data register of the UART (COM1 + 0) using the outb
    function.
    */
    outb(COM1 + 0, c);
}

/*
read a character from the UART
*/
static int uartgetc(void) {
    if (!uart)
        return -1;
    /*
    reads from the UART line status register. The & 0x01 checks the least
    significant bit, which indicates if there is data available to read. If
    there's no data (!(inb(COM1 + 5) & 0x01)), it returns -1.
    */
    if (!(inb(COM1 + 5) & 0x01))
        return -1;
    /*
    If data is available, inb(COM1 + 0) reads a byte from the receiver buffer
    (the UART data port), returning the character received.
    */
    return inb(COM1 + 0);
}

void uartintr(void) {
    consoleintr(uartgetc);
}
