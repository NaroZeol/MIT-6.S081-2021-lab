#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *dename2path(char *buf, char *path, char *dename) {
    int path_len = strlen(path);
    int dename_len = strlen(dename);
    char *pbuf = buf;
    memmove(pbuf, path, path_len);
    pbuf += path_len;
    *(pbuf++) = '/';
    memmove(pbuf, dename, dename_len);
    pbuf += dename_len;
    *(pbuf++) = 0;
    return buf;
}

void find (char *path, char *target) {
    char buf[512];
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (st.type != T_DIR) {
        fprintf(2, "find: %s is not a dir", path);
        close(fd);
        return;
    }

    while (read(fd, &de, sizeof(struct dirent)) == sizeof(struct dirent)) {
        if (de.inum == 0)
            continue;
        
        dename2path(buf, path, de.name); 
        /* tranfer de.name to path which stat can use
         * Example:                             (dename2path)
         *      de.name = "hello" path = "./abc" -----------> buf = "./abc/hello"
         */
        if(stat(buf, &st) < 0){
            fprintf(2, "find: cannot stat %s\n", de.name);
            close(fd);
            return;
        }

        if (st.type == T_DIR && strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0) {
            find(buf, target);
        }
        else if (st.type == T_FILE && strcmp(de.name, target) == 0) {
            printf("%s\n", buf);
        }
    }
    close(fd);
}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(2, "Usage: find <directory> <file>\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}