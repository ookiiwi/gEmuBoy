#include "gb.h"
#include "load.h"
#include "decode.h"
#include "log.h"
#include "mmu.h"
#include "timer.h"
#include "memmap.h"
#include "lcd.h"

#include <SDL.h>
#include <stdio.h>

#define GB_MSEC_PER_FRAME   (1000/60)
#define GB_CYCLES_PER_FRAME (4194304/60)

struct GB_clock_s {
    Uint64  last_tick;
    Uint64  last_tick_cycle_cnt;
    Uint64  avg_cycles;
    int     avg_nb;    
};

GB_clock_t* GB_clock_create() {
    GB_clock_t *clock = (GB_clock_t*)( malloc(sizeof(GB_clock_t)) );
    if (clock == NULL) {
        return NULL;
    }

    clock->last_tick            = 0;
    clock->last_tick_cycle_cnt  = 0;

    return clock;
}

void GB_clock_destroy(GB_clock_t *clk) {
    if (clk) free(clk);
}

GB_gameboy_t* GB_create(const char *src_rom_path) {
    GB_gameboy_t *gb = (GB_gameboy_t*)( malloc( sizeof(GB_gameboy_t) ) );
    gb->cpu     = NULL;
    gb->ppu     = NULL;
    gb->memory  = NULL;
    gb->rom     = NULL;
	gb->mmu 	= NULL;
    gb->clock   = NULL;

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

    gb->clock   = GB_clock_create();
    if (gb->clock == NULL) {
        return NULL;
    }

	gb->mmu = GB_mmu_create();
	if (gb->mmu == NULL) {
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

    GB_clock_destroy(gb->clock);
	GB_mmu_destroy(gb->mmu);
#ifndef NO_PPU
    ppu_destroy(gb->ppu);
#endif
    cpu_destroy(gb->cpu);

    if (gb->rom)    free(gb->rom);
    if (gb->memory) free(gb->memory);
    
    free(gb);
}

// DEBUG ONLY
int GB_clock_avg_cycles(GB_gameboy_t *gb) {
    return gb->clock->avg_cycles/gb->clock->avg_nb;
}

int GB_clock_pulse(GB_gameboy_t *gb) {
    Uint64 tick     = SDL_GetTicks64();
    Uint64 delta    = tick - gb->clock->last_tick;
    Uint64 cycles   = gb->cpu->m_cycle_counter - gb->clock->last_tick_cycle_cnt;

    if (cycles < GB_CYCLES_PER_FRAME && delta < GB_MSEC_PER_FRAME) {
        return 1;
    }

    if (delta >= GB_MSEC_PER_FRAME) {
        gb->clock->last_tick = tick;
        gb->clock->last_tick_cycle_cnt = gb->cpu->m_cycle_counter;

        gb->clock->avg_cycles += cycles;
        gb->clock->avg_nb++;
    }

    return 0;
}

void GB_run(GB_gameboy_t *gb) {
    while (GB_clock_pulse(gb)) {
        if (gb->cpu->m_is_halted) {
            GB_handle_interrupt(gb);
            INC_CYCLE();
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
}
