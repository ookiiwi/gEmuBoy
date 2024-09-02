#include "log.h"

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
#include <stdio.h>
#include <string.h>

#include "cpudef.h"
#include "gb.h"
#include "mmu.h"

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

void log_cpu_state(GB_gameboy_t *gb) {
    fprintf(gb->cpu->debug_info.p,
            "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
             A, F, B, C, D, E, H, L, SP, PC, GB_mem_read(gb, PC), GB_mem_read(gb, PC+1), GB_mem_read(gb, PC+2), GB_mem_read(gb, PC+3));
}

#endif