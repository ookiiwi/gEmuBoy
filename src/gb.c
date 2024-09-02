#include "gb.h"
#include "load.h"
#include "decode.h"
#include "log.h"
#include "timer.h"

GB_gameboy_t* GB_create(const char *src_rom_path) {
    GB_gameboy_t *gb = (GB_gameboy_t*)( malloc( sizeof(GB_gameboy_t) ) );
    gb->cpu     = NULL;
    gb->ppu     = NULL;
    gb->memory  = NULL;
    gb->rom     = NULL;

    if (gb == NULL) {
        return NULL;
    }

    gb->cpu     = cpu_create();
#ifndef NO_PPU
    gb->ppu     = ppu_create();
#endif
    gb->memory  = (BYTE*)( calloc( 0xFFFF, sizeof(BYTE) ) );

    if (gb->memory == NULL) {
        return NULL;
    }

    int rv = loadrom(src_rom_path, &gb->rom, &gb->rom_size);     

    if (rv != gb->rom_size) {                                               
        char *errmsg = "FAILED READING PROVIDED ROM";                            
        switch(rv) {                                                             
            case LOADROM_FAIL_OPEN:     errmsg = "FAILED OPENING ROM";           
            case LOADROM_FAIL_ALLOC:    errmsg = "FAILED ALLOCATING ROM MEMORY"; 
        }                                                                        
        fprintf(stderr, "%s: %s\n", errmsg, src_rom_path);

        GB_destroy(gb);
        return NULL;                                                   
    }

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
    CPU_GAMEBOY_DOCTOR_SETUP(src_rom_path);
#endif  

    return gb;
}

void GB_destroy(GB_gameboy_t *gb) {
#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
    CPU_GAMEBOY_DOCTOR_CLEANUP();
#endif

    if (gb == NULL) return;

#ifndef NO_PPU
    ppu_destroy(gb->ppu);
#endif
    cpu_destroy(gb->cpu);

    if (gb->rom)    free(gb->rom);
    if (gb->memory) free(gb->memory);
    
    free(gb);
}

void GB_run(GB_gameboy_t *gb) {
    if (gb->cpu->m_is_halted) {
        GB_handle_interrupt(gb);
        GB_update_timer(gb);
        return;
    }

    DECODE();

    // fetch
    FETCH_CYCLE();

    // ei_delay is assigned 2 by EI instr
    // This allows IME to be set only after one instruction has passed
    if (gb->cpu->m_ei_delay) {
        _IME |= (gb->cpu->m_ei_delay--) & 1;
    }
}
