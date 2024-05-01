#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main (int argc, char *argv[]) {
    int pipeline[2]; // 0: parent 1: child
    char ch[1];
    if (pipe(pipeline) != 0) {
        fprintf(2, "Create pipe failed");
        exit(1);
    }

    int pid = 0;
    if ((pid = fork()) == 0) {
        close(pipeline[0]);

        read(pipeline[1], ch, 1);
        printf("%d: received ping\n", getpid());
        write(pipeline[1], "c", 1);

        close(pipeline[1]);
        exit(0);
    }

    close(pipeline[1]);

    write(pipeline[0], "c", 1);
    read(pipeline[0], ch, 1);
    printf("%d: received pong\n", getpid());

    close(pipeline[0]);
    exit(0);
} 