#ifndef LOG_H_
#define LOG_H_

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
#include <stdio.h>
#include <string.h>
#include "defs.h"

#define DEBUG_LOG_FILE DEBUG_LOG_DIR "%02d.log"

typedef struct {
    FILE *p;
} CPU_Debug_Info;

void log_cpu_state(GB_gameboy_t *gb);
int extract_test_number(const char *path);

#define LOG_CPU_STATE() log_cpu_state(gb)                                                                                          

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
    gb->cpu->debug_info.p = NULL;                                                                                       \
    gb->cpu->debug_info.p = fopen(filename, "w");                                                                       \
    if (gb->cpu->debug_info.p == NULL) { printf("Cannot open\n"); }                                                     \
    free(filename);                                                                                                     \
} while(0)

#define CPU_GAMEBOY_DOCTOR_CLEANUP() do {                                                                               \
    if (gb->cpu->debug_info.p != NULL) fclose(gb->cpu->debug_info.p);                                                   \
} while(0)

#else
#define LOG_CPU_STATE() do {                                                                                            \
} while(0)
#endif

#endif