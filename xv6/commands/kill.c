#include "../type/types.h"
#include "../fileSystem/stat.h"
#include "../userLand/user.h"
#include "../userLand/printf.h"
#include "../userLand/ulib.h"

int main(int argc, char** argv) {
    int i;

    if (argc < 2) {
        printf(2, "usage: kill pid...\n");
        exit();
    }
    for (i = 1; i < argc; i++)
        kill(atoi(argv[i]));
    exit();
}
