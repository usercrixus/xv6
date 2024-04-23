#include "../type/types.h"
#include "../fileSystem/stat.h"
#include "../userLand/user.h"
#include "../userLand/printf.h"
#include "../userLand/ulib.h"

int main(int argc, char* argv[]) {
    int i;

    if (argc < 2) {
        printf(2, "Usage: mkdir files...\n");
        exit();
    }

    for (i = 1; i < argc; i++) {
        if (mkdir(argv[i]) < 0) {
            printf(2, "mkdir: %s failed to create\n", argv[i]);
            break;
        }
    }

    exit();
}
