/*
All function usable in userland who are not dependant of the Xv6 architecture
and system call (function defined in user.h)
*/

#include ".././type/types.h"
#include "ulib.h"
#include "../x86.h"

char* strcpy(char* s, const char* t) {
    char* os;

    os = s;
    while (*t != 0) {
        *s++ = *t++;
    }
    *s = *t;
    return os;
}

char* strncpy(char* s, const char* t, int n) {
    char* os;

    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)
        ;
    while (n-- > 0)
        *s++ = 0;
    return os;
}

int strcmp(const char* p, const char* q) {
    while (*p && *p == *q)
        p++, q++;
    return (uchar)*p - (uchar)*q;
}

int strncmp(const char* p, const char* q, uint n) {
    while (n > 0 && *p && *p == *q)
        n--, p++, q++;
    if (n == 0)
        return 0;
    return (uchar)*p - (uchar)*q;
}

uint strlen(const char* s) {
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}

void* memset(void* dst, int c, uint n) {
    stosb(dst, c, n);
    return dst;
}

void* memcpy(void* dst, const void* src, uint n) {
    return memmove(dst, src, n);
}

void* memmove(void* dst, const void* src, uint n) {
    const char* s;
    char* d;

    s = src;
    d = dst;
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0)
            *--d = *--s;
    } else
        while (n-- > 0)
            *d++ = *s++;

    return dst;
}

int memcmp(const void* v1, const void* v2, uint n) {
    const uchar *s1, *s2;

    s1 = v1;
    s2 = v2;
    while (n-- > 0) {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }

    return 0;
}

char* strchr(const char* s, char c) {
    for (; *s; s++)
        if (*s == c)
            return (char*)s;
    return 0;
}

int atoi(const char* s) {
    int n;

    n = 0;
    while ('0' <= *s && *s <= '9')
        n = n * 10 + *s++ - '0';
    return n;
}

char* safestrcpy(char* s, const char* t, int n) {
    char* os;

    os = s;
    if (n <= 0)
        return os;
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
}