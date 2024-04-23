/*
Responsible for initializing the UART (Universal Asynchronous
Receiver-Transmitter) device, which is used for serial communication in the xv6
operating system. Serial communication involves sending and receiving data one
bit at a time over a serial port.
*/
void uartinit(void);
/*
Interrupt handler for UART
*/
void uartintr(void);
/*
responsible for transmitting a character (c) to the UART (serial port) for
output.
*/
void uartputc(int);