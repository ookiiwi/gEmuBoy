#ifndef TESTBOY_CUSTOM_IO
#define TESTBOY_CUSTOM_IO

#include <stdio.h>

FILE*   popen2(const char *cmd, const char *mode, int *pid, int *fd);
int     pclose2(FILE *fp, int pid);

#endif