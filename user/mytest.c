#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fs.h"
#include "user/user.h"

#define MAP_FAILED ((char *) -1)

int
main(int argc, char *argv[])
{
    int fd = open("mytest", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("open failed\n");
        exit(1);
    }

    write(fd, "Hello, world!", 13);

    char *p = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) {
        printf("mmap failed\n");
        exit(1);
    }

    printf("mmap succeeded\n");

    if (fork() == 0) {

        *p = 'h';
        printf("child: change the first character to 'h'\n");
        printf("child: %s\n", p);
        exit(0);
    }

    wait(0);

    printf("parent: %s\n", p);
    munmap(p, 4096);
    close(fd);
    exit(0);
}