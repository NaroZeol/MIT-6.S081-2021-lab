#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

char xargs[MAXARG][512];

int main (int argc, char *argv[]) {
    int arg_index = 1;
    if (argc < 2) {
        fprintf(2, "Usage: xargs <command> <args...>\n");
        exit(1);
    }

    char *pxargs[MAXARG + 1]; 
    for (int i = 0; i != MAXARG; ++i) {
        pxargs[i] = xargs[i];
    }

    // copy command to xargs[0]
    int xarg_index = 0;
    memcpy(pxargs[xarg_index], argv[arg_index], strlen(argv[arg_index]));
    pxargs[xarg_index][strlen(argv[arg_index])] = '\0';
    arg_index += 1;
    xarg_index += 1;

    // copy argv to xargs
    for (int i = arg_index; i < argc; ++i) {
        int len = strlen(argv[i]);
        memcpy(pxargs[xarg_index], argv[i], len);
        pxargs[xarg_index][len] = '\0';
        xarg_index += 1;
    }

    char line[512];
    while (1) {
        char ch;
        int exit_flag = 0;
        int line_index = 0;
        int read_arg_index = xarg_index;

        // read line
        while (1) {
            if (read(0, (void *)&ch, sizeof(char) * 1) == 0) {
                exit_flag = 1;
                break;
            }
            if (ch == '\n') {
                break;
            }
            line[line_index++] = ch;
        }
        line[line_index++] = '\0';

        if (exit_flag == 1)
            break;

        // copy line to addtional args
        int len = strlen(line);
        memcpy(pxargs[read_arg_index], line, len);
        pxargs[read_arg_index][len] = '\0';
        read_arg_index += 1;

        if (exit_flag == 1)
            break;

        pxargs[read_arg_index++] = 0;

        // for (int i = 0; i != MAXARG; ++i) {
        //    if (pxargs[i] == NULL)
        //        break;
        //    else
        //        printf("%s\n", pxargs[i]);
        //}

        int pid;
        pid = fork();
        if (pid < 0) {
            fprintf(2, "Failed when fork\n");
            exit(1);
        }

        // child
        if (pid == 0) {
            if (exec(pxargs[0], pxargs) < 0) {
                fprintf(2, "Failed when execv\n");
                exit(1);
            }
        }
        
        // parent, wait all child
        wait(0);
    }
    exit(0);
}
