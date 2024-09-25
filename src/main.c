#include "gb.h"
#include "cpudef.h"
#include "cpu/decode.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
    GB_gameboy_t *gb;

    if (argc < 2) {
        fprintf(stderr, "FORMAT ERROR: gEmuBoy <SRC_ROM_PATH>");
        exit(EXIT_FAILURE);
    }

    gb = GB_gameboy_create(argv[1]);
    if (!gb) {
        fprintf(stderr, "CANNOT CREATE GAMEBOY\n");
        return EXIT_FAILURE;
    }

    int i = 0;
    while(i++ < 0x900000) {
        GB_cpu_run(gb);
    }

    GB_gameboy_destroy(gb);

    return EXIT_SUCCESS;
}