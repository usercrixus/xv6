// Keyboard header

#define KBSTATP 0x64  // kbd controller status port(I)
#define KBS_DIB 0x01  // kbd data in buffer
#define KBDATAP 0x60  // kbd data port(I)

// undefined key
#define NO 0

// the shift key bit
#define SHIFT (1 << 0)
// the ctrl key bit
#define CTL (1 << 1)
// the alt key bit
#define ALT (1 << 2)

/*
This flag is used to track whether the Caps Lock key has been activated. When
the Caps Lock key is pressed, this flag is toggled, changing the behavior of
certain keypresses, particularly the alphabetic characters.
*/
#define CAPSLOCK (1 << 3)
/*
Keep state of numerical keypad lock
The Num Lock key switches the function of the numeric keypad between entering
numbers and performing secondary functions (like navigation controls).
*/
#define NUMLOCK (1 << 4)
/*
The Scroll Lock key was originally intended to change the way arrow keys work,
but it is now largely obsolete in modern computing.
*/
#define SCROLLLOCK (1 << 5)
/*
E0ESC stands for "Escape for 0xE0 scan code" and is used to mark or identify
extended key sequences in keyboard handling routines.
*/
#define E0ESC (1 << 6)

// Special keycodes :

#define KEY_HOME 0xE0
#define KEY_END 0xE1
#define KEY_UP 0xE2
#define KEY_DN 0xE3
#define KEY_LF 0xE4
#define KEY_RT 0xE5
#define KEY_PGUP 0xE6
#define KEY_PGDN 0xE7
#define KEY_INS 0xE8
#define KEY_DEL 0xE9

/*
The ctrl modifier. when the ctrl key is used, the second key value is equal to
x-'@'. C('A') == Control-A
*/
#define C(x) (x - '@')

/*
Shiftcode array is used to map certain scan codes to their corresponding
modifier key states. Modifier keys are keys like Shift, Control (Ctrl), and Alt,
which modify the behavior of other key presses.
*/
static uchar shiftcode[256] = {
    [0x1D] CTL, [0x2A] SHIFT, [0x36] SHIFT, [0x38] ALT, [0x9D] CTL, [0xB8] ALT};

/*
Togglecode array maps certain scan codes to the states of toggle keys like Caps
Lock, Num Lock, and Scroll Lock. Toggle keys are those whose effect remains
active until the key is pressed again.
*/
static uchar togglecode[256] =
    {[0x3A] CAPSLOCK, [0x45] NUMLOCK, [0x46] SCROLLLOCK};

/*
Part of a keyboard handling routine, typically found in an operating system's
keyboard driver. This array is used to map keyboard scan codes to their
corresponding characters or functions in a standard (non-modified : shift, ctrl)
state.
*/
static uchar normalmap[256] = {NO,
                               0x1B,
                               '1',
                               '2',
                               '3',
                               '4',
                               '5',
                               '6',  // 0x00
                               '7',
                               '8',
                               '9',
                               '0',
                               '-',
                               '=',
                               '\b',
                               '\t',
                               'q',
                               'w',
                               'e',
                               'r',
                               't',
                               'y',
                               'u',
                               'i',  // 0x10
                               'o',
                               'p',
                               '[',
                               ']',
                               '\n',
                               NO,
                               'a',
                               's',
                               'd',
                               'f',
                               'g',
                               'h',
                               'j',
                               'k',
                               'l',
                               ';',  // 0x20
                               '\'',
                               '`',
                               NO,
                               '\\',
                               'z',
                               'x',
                               'c',
                               'v',
                               'b',
                               'n',
                               'm',
                               ',',
                               '.',
                               '/',
                               NO,
                               '*',  // 0x30
                               NO,
                               ' ',
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               NO,
                               '7',  // 0x40
                               '8',
                               '9',
                               '-',
                               '4',
                               '5',
                               '6',
                               '+',
                               '1',
                               '2',
                               '3',
                               '0',
                               '.',
                               NO,
                               NO,
                               NO,
                               NO,           // 0x50
                               [0x9C] '\n',  // KP_Enter
                               [0xB5] '/',   // KP_Div
                               [0xC8] KEY_UP,
                               [0xD0] KEY_DN,
                               [0xC9] KEY_PGUP,
                               [0xD1] KEY_PGDN,
                               [0xCB] KEY_LF,
                               [0xCD] KEY_RT,
                               [0x97] KEY_HOME,
                               [0xCF] KEY_END,
                               [0xD2] KEY_INS,
                               [0xD3] KEY_DEL};

/*
Part of the keyboard handling routine in an operating system's keyboard driver.
The key difference with the normalmap is that shiftmap is used to map keyboard
scan codes to their corresponding characters or functions when the Shift key is
held down.
*/
static uchar shiftmap[256] = {NO,
                              033,
                              '!',
                              '@',
                              '#',
                              '$',
                              '%',
                              '^',  // 0x00
                              '&',
                              '*',
                              '(',
                              ')',
                              '_',
                              '+',
                              '\b',
                              '\t',
                              'Q',
                              'W',
                              'E',
                              'R',
                              'T',
                              'Y',
                              'U',
                              'I',  // 0x10
                              'O',
                              'P',
                              '{',
                              '}',
                              '\n',
                              NO,
                              'A',
                              'S',
                              'D',
                              'F',
                              'G',
                              'H',
                              'J',
                              'K',
                              'L',
                              ':',  // 0x20
                              '"',
                              '~',
                              NO,
                              '|',
                              'Z',
                              'X',
                              'C',
                              'V',
                              'B',
                              'N',
                              'M',
                              '<',
                              '>',
                              '?',
                              NO,
                              '*',  // 0x30
                              NO,
                              ' ',
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              NO,
                              '7',  // 0x40
                              '8',
                              '9',
                              '-',
                              '4',
                              '5',
                              '6',
                              '+',
                              '1',
                              '2',
                              '3',
                              '0',
                              '.',
                              NO,
                              NO,
                              NO,
                              NO,           // 0x50
                              [0x9C] '\n',  // KP_Enter
                              [0xB5] '/',   // KP_Div
                              [0xC8] KEY_UP,
                              [0xD0] KEY_DN,
                              [0xC9] KEY_PGUP,
                              [0xD1] KEY_PGDN,
                              [0xCB] KEY_LF,
                              [0xCD] KEY_RT,
                              [0x97] KEY_HOME,
                              [0xCF] KEY_END,
                              [0xD2] KEY_INS,
                              [0xD3] KEY_DEL};

/*
Part of a keyboard handling routine, used in operating systems for interpreting
keyboard inputs when the Control (Ctrl) key is held down.
*/
static uchar ctlmap[256] = {NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            NO,
                            C('Q'),
                            C('W'),
                            C('E'),
                            C('R'),
                            C('T'),
                            C('Y'),
                            C('U'),
                            C('I'),
                            C('O'),
                            C('P'),
                            NO,
                            NO,
                            '\r',
                            NO,
                            C('A'),
                            C('S'),
                            C('D'),
                            C('F'),
                            C('G'),
                            C('H'),
                            C('J'),
                            C('K'),
                            C('L'),
                            NO,
                            NO,
                            NO,
                            NO,
                            C('\\'),
                            C('Z'),
                            C('X'),
                            C('C'),
                            C('V'),
                            C('B'),
                            C('N'),
                            C('M'),
                            NO,
                            NO,
                            C('/'),
                            NO,
                            NO,
                            [0x9C] '\r',    // KP_Enter
                            [0xB5] C('/'),  // KP_Div
                            [0xC8] KEY_UP,
                            [0xD0] KEY_DN,
                            [0xC9] KEY_PGUP,
                            [0xD1] KEY_PGDN,
                            [0xCB] KEY_LF,
                            [0xCD] KEY_RT,
                            [0x97] KEY_HOME,
                            [0xCF] KEY_END,
                            [0xD2] KEY_INS,
                            [0xD3] KEY_DEL};
