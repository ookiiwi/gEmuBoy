#include "gb.h"
#include "cpudef.h"
#include "cpu/decode.h"

#include <stdlib.h>
#include <stdio.h>

#include <SDL.h>

int main(int argc, char **argv) {
    int isrunning = 1;
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

    while(isrunning) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isrunning = 0;
            }
        }

        if (IR==0x40) {printf("BREAKPOINT\n");}\
        GB_cpu_run(gb);
    }

    GB_gameboy_destroy(gb);

    return EXIT_SUCCESS;
}