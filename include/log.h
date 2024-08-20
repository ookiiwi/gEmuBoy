#ifndef LOG_H_
#define LOG_H_

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
#include <stdio.h>
#include <string.h>

#include "cpudef.h"
#include "cpu_utils.h"

#define DEBUG_LOG_FILE DEBUG_LOG_DIR "%02d.log"

typedef struct {
    FILE *p;
} CPU_Debug_Info;

int extract_test_number(const char *path) {
    char *src = strdup(path);
    char *delim = "/";
    char *token = strtok(src, delim);
    char *s;
    char str_num[3] = { 0 }; 
    while(token) {
        s = token;
        token = strtok(NULL, delim);
    }

    strncpy(str_num, s, 2);
    free(src);

    const long i = strtol(str_num, (char **)NULL, 10);

    return i;
}

#define CPU_GAMEBOY_DOCTOR_SETUP(src_rom_path) do {                                                                     \
    AF = 0x01B0;                                                                                                        \
    BC = 0x0013;                                                                                                        \
    DE = 0x00D8;                                                                                                        \
    HL = 0x014D;                                                                                                        \
    SP = 0xFFFE;                                                                                                        \
    PC = 0x0100;                                                                                                        \
    int test_number = extract_test_number(src_rom_path);                                                                \
    char *filename = (char*)(calloc(strlen(DEBUG_LOG_FILE)-1, sizeof(char)));                                           \
    snprintf(filename, strlen(filename)-1, DEBUG_LOG_FILE, test_number);                                                \
    context->debug_info.p = NULL;                                                                                       \
    context->debug_info.p = fopen(filename, "w");                                                                       \
    if (context->debug_info.p == NULL) { printf("Cannot open\n"); }                                                     \
    free(filename);                                                                                                     \
} while(0)

#define CPU_GAMEBOY_DOCTOR_CLEANUP() do {                                                                               \
    if (context->debug_info.p != NULL) fclose(context->debug_info.p);                                                                     \
} while(0)

#define LOG_CPU_STATE() do {                                                                                            \
    fprintf(context->debug_info.p,                                                                                      \
            "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",      \
             A, F, B, C, D, E, H, L, SP, PC, READ_MEMORY(PC), READ_MEMORY(PC+1), READ_MEMORY(PC+2), READ_MEMORY(PC+3)); \
} while(0)

#else
#define LOG_CPU_STATE() do {                                                                                            \
    printf("LOG NO DEBUG");                                                                                             \
} while(0)
#endif

#endif