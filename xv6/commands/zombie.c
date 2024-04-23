// Create a zombie process that
// must be reparented at exit.

#include "../type/types.h"
#include "../fileSystem/stat.h"
#include "../userLand/user.h"
#include "../userLand/printf.h"
#include "../userLand/ulib.h"
#include "../synchronization/spinlock.h"

int main(void) {
    struct spinlock lk;
    uint chan = 5;
    if (fork() > 0)
        sleep(&chan, &lk);  // Let child exit before parent.
    exit();
}
