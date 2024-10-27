#include "custom_io.h"
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

FILE* popen2(const char *cmd, const char *mode, int *pid, int *file_descriptor) {
    FILE    *fp;
    int     fd[2];
    int     op;

    if (*mode == 'r') {
        op = 0;
    } else if (*mode == 'w') {
        op = 1;
    } else {
        errno = EINVAL;
        return NULL;
    }
    
    if (pipe(fd))                   { return NULL;  }
    if ( ( *pid = fork() ) == -1 )  { goto fail;    }

    if (*pid == 0) {
        // child only uses opposite end of its parent
        close(fd[op]);
        dup2(fd[!op], !op);

        setpgid(*pid, *pid); // kills all children processes if negative PID
        execl("/bin/sh", "/bin/sh", "-c", cmd, (char*)NULL);

        goto fail; // execl only returns on error
    } else {
        // we close the opposite end as it is used by child
        close(fd[!op]);
    }

    if ( ( fp = fdopen(fd[op], op ? "w" : "r") ) == NULL ) { goto fail; }
    *file_descriptor = fd[op];
    
    return fp;

fail:
    close(fd[!op]);

    return NULL;
}

int pclose2(FILE *fp, int pid) {
    int stat;

    fclose(fp);
    while(waitpid(pid, &stat, 0) == -1) {
        if (errno != EINTR) {
            stat = -1;
            break;
        }
    }

    return stat;
}
