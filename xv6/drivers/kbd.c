#include "../x86.h"
#include "../defs.h"
#include "./kbd.h"
#include "../type/types.h"

int kbdgetc(void) {
    /*
    keeps track of the current state of the Shift
    */
    static uint shift;
    /*
    This array of pointers points to the different character maps (normalmap,
    shiftmap, ctlmap) used based on the current state of the shift keys.
    */
    static uchar* charcode[4] = {normalmap, shiftmap, ctlmap, ctlmap};
    /*
    the char to be returned
    */
    uint c;

    /*
    Reads the keyboard status from the status port
    */
    uint st = inb(KBSTATP);
    /*
    checks if the keyboard's data-in buffer is empty. If it is, the function
    returns -1, indicating no data to read.
    */
    if ((st & KBS_DIB) == 0)
        return -1;

    uint data = inb(KBDATAP);

    /*
    checks if the high bit (bit 7) of the data byte is set. In keyboard scan
    code terminology, if the high bit of a scan code is set (0x80 is 10000000 in
    binary), it indicates that it's a key release event, not a key press event.
    */
    if (data & 0x80) {
        /*
        remove the release bit
        */
        data = data & 0x7F;
        /*
        Updates the shift state variable. It clears the bits in shift that
        correspond to the released key.
        */
        shift &= ~(shiftcode[data]);
        return 0;
    }
    /*
    shift |= shiftcode[data];: Updates the shift variable with the current state
    of the Shift, Control, or Alt keys.
    */
    shift |= shiftcode[data];
    /*
    Toggles the state of lock keys (Caps Lock, Num Lock, Scroll Lock) on
    keypresses.
    */
    shift ^= togglecode[data];
    /*
    Determines which character map to use based on the current state of the
    Control and Shift keys, and then gets the character corresponding to the
    scan code.
    */
    c = charcode[shift & 3][data];
    /*
    If shift is active the map used is the shifted map, then, the char are
    uppercase. But it shit and CAPSLOCK are active, we want to get a lowercase
    char. These condition handle this behaviour.
    */
    if (shift & CAPSLOCK) {
        if ('a' <= c && c <= 'z')
            c += 'A' - 'a';
        else if ('A' <= c && c <= 'Z')
            c += 'a' - 'A';
    }

    return c;
}

void kbdintr(void) {
    consoleintr(kbdgetc);
}
