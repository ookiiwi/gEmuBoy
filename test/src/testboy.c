#include "custom_io.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define GEKKIO_SUCCESS_SEQ "358132134"
#define GEKKIO_FAILURE_SEQ "666666666666"

#define TEST_PRGM_PATH GB_MAIN_PRGM_DIR "gemuboy -l"

#define TEST_SUCCESS (0)
#define TEST_RUN_FAILURE (1)
#define TEST_FAILURE (2)
#define TEST_ERROR (3)

#define TIMEOUT_LOOP(timeout)                       \
    struct timespec start, end;                     \
    int elapsed = 0;                                \
    clock_gettime(CLOCK_MONOTONIC, &start);         \
    while ( (clock_gettime(CLOCK_MONOTONIC, &end) == 0) && (elapsed = (end.tv_sec - start.tv_sec) ) < timeout)

static FILE *fp = NULL;
static int pid;
static char *rom_path = NULL;
static char *success_seq = NULL;
static char *failure_seq = NULL;
static long timeout = 60;

void print_usage() {
    fprintf(stderr, 
        "USAGE: testboy <ROM_PATH> <TEST_SUITE_ID> \n\n" \
        "1\tGEKKIO" \
        "\n"
    ); 
}

void int_handler() {
    fprintf(stderr, "PROGRAM INNTERRUPTED\n");
    if (fp) {
        kill(2, pid);
        pclose2(fp, pid);
    }
}

int exec_test() {
    int res = TEST_FAILURE;
    int fd, status;
    long cmd_len = strlen(TEST_PRGM_PATH) + strlen(rom_path) + 2;
    char cmd[cmd_len];
    char output[1024] = { 0 };

    snprintf(cmd, cmd_len, "%s %s", TEST_PRGM_PATH, rom_path);

    fp = popen2(cmd, "r", &pid, &fd);
    if (!fp) {
        fprintf(stderr, "Cannot run command (%s) `%s`\n", strerror(errno), cmd);
        return TEST_RUN_FAILURE;
    }

    fcntl(fd, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    int i = 0;
    char c;
    TIMEOUT_LOOP(timeout) {
        if (read(fd, &c, 1) > 0) output[i++] = c;

        if (i >= strlen(success_seq) && strcmp(output, success_seq) == 0) {
            res = TEST_SUCCESS;
            break;
        } else if (i >= strlen(failure_seq) && strcmp(output, failure_seq) == 0) {
            res = TEST_FAILURE;
            break;
        }
    }

    kill(pid, 2);
    pclose2(fp, pid);
    fp = NULL;

    return res;
}

int main(int argc, char **argv) {
    int test_suite_id = 0;

   if (argc < 3) {
        print_usage();    
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);
    signal(SIGSEGV, int_handler);

    rom_path = argv[1];
    test_suite_id = (int)strtol(argv[2], NULL, 10);
    if (argc > 3) {
        timeout = (int)strtol(argv[3], NULL, 10);
    }

    switch (test_suite_id) {
        case 1:
            success_seq = GEKKIO_SUCCESS_SEQ;
            failure_seq = GEKKIO_FAILURE_SEQ;
            break;
        default:
            fprintf(stderr, "INVALID TEST SUITE ID\n");
            print_usage();
            exit(EXIT_FAILURE);
    }

    return exec_test();
}
