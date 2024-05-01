#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main (int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "Usage: sleep <time>\n");
        exit(1);
    }
    int t = atoi(argv[1]);

    if (t < 1) {
        fprintf (2, "time should not be negative\n");
        exit(1);
    }
    sleep(t);
    exit(0);
    return 0;
}