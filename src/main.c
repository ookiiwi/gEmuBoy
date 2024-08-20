#include "cpu.h"
#include "instr.h"
#include "decode.h"
#include "load.h"

#define DUMMY_UPDATE_TIMER() do {} while(0)

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "FORMAT ERROR: gEmuBoy <SRC_ROM_PATH>");
        exit(EXIT_FAILURE);
    }

    CPU         _context;
    CPU         *context        = &_context;
    const char  *src_filename   = argv[1];

    // init
    CPU_INIT(context, src_filename);

    int i = 0;
    while(i++ < 0x900000) {
        DECODE();

        // fetch
        FETCH_CYCLE();

after_fetch_cycle:
        DUMMY_UPDATE_TIMER();
        /* TODO: UPDATE_TIMERS(); */
    }

    CPU_FREE(context);

    return 0;
}