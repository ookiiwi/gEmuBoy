#include "cpu/cpu.h"
#include "cpu/timer.h"
#include "cpu/decode.h"

#include <stdlib.h>
#include <stdio.h>

GB_cpu_t* GB_cpu_create() {
    GB_cpu_t *cpu               = (GB_cpu_t*)( malloc( sizeof(GB_cpu_t) ) );
    cpu->ir                     = 0;
    cpu->prev_ir                = 0;
    cpu->af.w                   = 0;
    cpu->bc.w                   = 0;
    cpu->de.w                   = 0;
    cpu->hl.w                   = 0;
    cpu->sp.w                   = 0;
    cpu->pc.w                   = 0;
    cpu->wz.w                   = 0;
    cpu->IME                    = 0;
    cpu->ei_delay               = 0;
    cpu->is_halted              = 0;
    cpu->halt_bug               = 0;
    cpu->is_stopped             = 0;
    cpu->t_cycle_counter        = 0;
    cpu->timer                  = GB_timer_create();

    return cpu;                                                                         
}

void GB_cpu_destroy(GB_cpu_t *cpu) {
    if (cpu == NULL) return; 
    GB_timer_destroy(cpu->timer);

    free(cpu);
}

void GB_cpu_run(GB_gameboy_t *gb) {
    if (!gb->cpu->is_halted) {
        DECODE();
        FETCH_CYCLE();
    } else {
        INC_CYCLE();
        GB_interrupt_handle(gb);
    }

    if (gb->cpu->ei_delay) {
        gb->cpu->ei_delay = 0;
        _IME = 1;
    }
}

