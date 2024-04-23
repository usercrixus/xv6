/*
All function usable in userland exept system call
*/

#include ".././type/types.h"
#include "../fileSystem/stat.h"

/*
Retrieves information about the file named n and stores it in the stat structure
pointed to by st. Opens the file, calls fstat to fill the stat structure, then
closes the file. Returns 0 on success, or -1 on error.
*/
int stat(const char*, struct stat*);
/*
Copies the string t to s, including the terminating null byte (\0).
Returns the destination string s.
*/
char* strcpy(char*, const char*);
/*
a memory utility function used to copy a block of memory from one location to
another. It handles overlapping memory regions correctly, ensuring that the copy
is performed safely and accurately.
*/
void* memmove(void*, const void*, uint);
/*
Locates the first occurrence of the character c in the string s.
Returns a pointer to the matched character or NULL if the character is not
found.
*/
char* strchr(const char*, char c);
/*
Compares the two strings p and q lexicographically.
Returns an integer less than, equal to, or greater than zero if p is found,
respectively, to be less than, to match, or be greater than q.
*/
int strcmp(const char*, const char*);
/*
Prints formatted output to a file descriptor.
*/
void printf(int, const char*, ...);
/*
Reads a line from standard input (stdin) into the buffer buf, but not more than
max - 1 characters, and null-terminates the result. Stops reading upon
encountering a newline or end-of-file. Returns buf.
*/
char* gets(char*, int max);
/*
Calculates the length of the string s, excluding the terminating null byte (\0).
Returns the number of characters in the string s.
*/
uint strlen(const char*);
/*
Fills the first n bytes of the memory area pointed to by dst with the constant
byte c. Returns a pointer to the memory area dst.
*/
void* memset(void*, int, uint);
/*
Converts the initial portion of the string s to an integer.
Returns the converted integer.
*/
int atoi(const char*);
/*
Copies up to n-1 characters from string t to string s, ensuring that s is always
null-terminated.
*/
char* safestrcpy(char*, const char*, int);
/*
represents the implementation of the memcmp function, which is a standard C
library function used to compare the values of two memory regions.

takes three arguments: v1 and v2, which are pointers to the memory regions to
compare, and n, which specifies the number of bytes to compare.
*/
int memcmp(const void*, const void*, uint);
/*
Compares the first n character of the two strings p and q lexicographically.
Returns an integer less than, equal to, or greater than zero if p is found,
respectively, to be less than, to match, or be greater than q.
*/
int strncmp(const char*, const char*, uint);
/*
Copies the first n character of the string t to s, including the terminating
null byte (\0). Returns the destination string s.
*/
char* strncpy(char*, const char*, int);
/*
copies n characters from memory area src to memory area dest.
*/
void* memcpy(void* dst, const void* src, uint n);