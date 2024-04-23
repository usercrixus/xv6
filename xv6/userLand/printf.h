/*
Writes the C string pointed by fmt to the given fd. If format includes format
specifiers (subsequences beginning with %), the additional arguments following
format are formatted and inserted in the resulting string replacing their
respective specifiers.

Only understands %d, %x, %p, %s.
*/
void printf(int fd, const char* fmt, ...);