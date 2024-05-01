#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int parentfd;
int childfd;

int is_root;

int child_pid;

int sieve;

int create_child(int next_sieve) {
    // 0->child  to parent
    // 1->parent to child 
    int pipeline[2];
    if (pipe(pipeline) == -1) {
        fprintf(2, "Failed when call pipe\n");
        exit(1);
    }

    if ((child_pid = fork()) == -1){
        fprintf(2, "Failed when call fork\n");
        exit(1);
    }
    
    // parent section start
    if (child_pid != 0){
        close(pipeline[0]);
        childfd = pipeline[1];
        return 1;
    }
    // parent section end

    // child section
    is_root = 0;
    child_pid = -1; // reset child_pid to -1
    childfd = -1;
    sieve = next_sieve;
    close(pipeline[1]);
    parentfd = pipeline[0];
    return 0;
}

int main () {
    is_root = 1;
    sieve = 2;
    child_pid = -1;

    printf("prime 2\n");
    for (int i = 2; i <= 35; ++i) {
        if (i % sieve == 0) {
            continue;
        }
        else if (child_pid != -1) {
            write(childfd, (void *)&i, sizeof(int) * 1);
        }
        else {
            if (create_child(i) == 1) {
                printf("prime %d\n", i);
            }
            else
                break;
        }
    }

    if (is_root == 0) {
        while (1) {
            int data;
            read(parentfd, (void *)&data, sizeof(int) * 1);
            if (data == -1) {
                if (child_pid != -1) // if still have child, pass end signal(-1) to child
                    write(childfd, (void *)&data, sizeof(int) * 1);
                exit(0);
            }
            else if (data % sieve == 0)
                continue;
            else if (child_pid != -1)
                write(childfd, (void *)&data, sizeof(int) * 1);
            else {
                if (create_child(data) == 1) {
                    printf("prime %d\n", data);
                }
            }
        }
    }
    else {
        sleep(10);
        int end = -1;
        write(childfd, (void *)&end, sizeof(int) * 1); // send end signal(-1) to child
    }

    exit(0);
}