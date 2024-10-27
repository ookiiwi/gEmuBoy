#ifndef TESTBOY_TEST_EXEC_H_
#define TESTBOY_TEST_EXEC_H_

#include "custom_io.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#define TEST_PRGM_PATH GB_MAIN_PRGM_DIR "gemuboy -l"

#define TIMEOUT_LOOP(timeout)                       \
    struct timespec start, end;                     \
    int elapsed = 0;                                \
    clock_gettime(CLOCK_MONOTONIC, &start);         \
    while ( (clock_gettime(CLOCK_MONOTONIC, &end) == 0) && (elapsed = (end.tv_sec - start.tv_sec) ) < timeout)

/**
 * Execute a test and get its output
 */
static inline int test_exec(const char *test_path, const char *success_seq, const char *failure_seq, int timeout) {
    FILE* fp;
    int pid, fd;
    long cmd_len = strlen(TEST_PRGM_PATH) + strlen(test_path) + 2;
    char cmd[cmd_len];
    char output[1024] = { 0 };

    snprintf(cmd, cmd_len, "%s %s", TEST_PRGM_PATH, test_path);

    fp = popen2(cmd, "r", &pid, &fd);

    if (!fp) {
        fprintf(stderr, "Cannot run command (%s) `%s`\n", strerror(errno), cmd);
        return 0;
    }

    fcntl(fd, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    int i = 0;
    char c;
    int res = 2;

    TIMEOUT_LOOP(timeout) {
        if (read(fd, &c, 1) > 0) output[i++] = c;

        if (i >= strlen(success_seq) && strcmp(output, success_seq) == 0) {
            res = 1;
            break;
        } else if (i >= strlen(failure_seq) && strcmp(output, failure_seq) == 0) {
            res = 0;
            break;
        }

        // Check if test program did not end prematuraly
        if (waitpid(pid, NULL, WNOHANG) == pid) {
            res = 3;
            break;
        } 
    }

    kill(pid, 2);
    pclose2(fp, pid);

    return res;
}

#endif
