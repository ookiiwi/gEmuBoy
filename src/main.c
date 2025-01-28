#include "gb.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <SDL.h>

#include "argparse.h"

static volatile int isrunning = 1;

void intHandler(int dummy) {
    isrunning = 0;
}

static const char *const usages[] = {
    "gemuboy <ROM_PATH> [[--] args]",
    NULL,
};

int main(int argc, const char **argv) {
    GB_gameboy_t *gb;

    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    const char *rom_path = NULL;
    int headless = 0;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_BOOLEAN('l', "headless", &headless, "Run without GUI (mainly for test automation)", NULL, 0, 0),
        OPT_END()
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argc = argparse_parse(&argparse, argc, argv);

    if (argc == 0 || ( rom_path = *argv ) == NULL ) {
        argparse_usage(&argparse);
        exit(EXIT_FAILURE);
    }

    gb = GB_gameboy_create(rom_path, headless);
    if (!gb) {
        fprintf(stderr, "CANNOT CREATE GAMEBOY\n");
        return EXIT_FAILURE;
    }

    while(isrunning) {
        SDL_Event event;
        while(!headless && SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isrunning = 0;
                    break;
                default:
                    break;
            }
        }


        // Temporary solution to deal with PollEvent being slow
        for (int i = 0; i < 1000; i++)
            GB_cpu_run(gb);
    }

    GB_gameboy_destroy(gb);

    SDL_Quit();
    return EXIT_SUCCESS;
}
