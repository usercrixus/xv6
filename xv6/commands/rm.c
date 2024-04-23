#include "../type/types.h"
#include "../fileSystem/stat.h"
#include "../userLand/user.h"
#include "../userLand/printf.h"
#include "../userLand/ulib.h"

int main(int argc, char* argv[]) {
    int i;

    if (argc < 2) {
        printf(2, "Usage: rm files...\n");
        exit();
    }

    for (i = 1; i < argc; i++) {
        if (unlink(argv[i]) < 0) {
            printf(2, "rm: %s failed to delete\n", argv[i]);
            break;
        }
    }

    exit();
}
