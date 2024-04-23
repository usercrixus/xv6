/*
All function usable in userland who are dependant of the Xv6 architecture and
system call (function defined in user.h)
*/

#include "../type/types.h"
#include "../fileSystem/stat.h"
#include "../type/fcntl.h"
#include "ulib.h"
#include "user.h"

int stat(const char* n, struct stat* st) {
    int fd;
    int r;

    fd = open(n, O_RDONLY);
    if (fd < 0)
        return -1;
    r = fstat(fd, st);
    close(fd);
    return r;
}

char* gets(char* buf, int max) {
    int i, cc;
    char c;

    for (i = 0; i + 1 < max;) {
        cc = read(0, &c, 1);
        if (cc < 1)
            break;
        buf[i++] = c;
        if (c == '\n' || c == '\r')
            break;
    }
    buf[i] = '\0';
    return buf;
}