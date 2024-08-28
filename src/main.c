#include "gb.h"
#include "instr.h"
#include "decode.h"

#define DUMMY_UPDATE_TIMER() do {} while(0)

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "FORMAT ERROR: gEmuBoy <SRC_ROM_PATH>");
        exit(EXIT_FAILURE);
    }

    GB_gameboy_t *gb;
    const char *src_filename   = argv[1];

    gb = GB_create(src_filename);    

    if (gb == NULL) {
        fprintf(stderr, "ERROR\n");
        exit(EXIT_FAILURE);
    }

    // tmp init
    int i = 0;
    PC = 0x00;  // TMP workaround test ppu
    GB_mem_write(gb, 0xFF41, ( gb->memory[0xFF41] & 0xFC ) | (2 & 3) ); // TMP workaround to set PPU MODE 2
    while(i++ < 0x10000) {
        DECODE();

        // fetch
        FETCH_CYCLE();

after_fetch_cycle:
        DUMMY_UPDATE_TIMER();
        /* TODO: UPDATE_TIMERS(); */
    }

    GB_destroy(gb);

    return 0;
}