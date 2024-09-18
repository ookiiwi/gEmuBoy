#include "cpu.h"
#include "timer.h"
#include <stdlib.h>

GB_cpu_t* cpu_create() {
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
    cpu->m_IME                  = 0;
    cpu->m_ei_delay             = 0;
    cpu->m_is_halted            = 0;
    cpu->m_halt_bug             = 0;
    cpu->m_is_stopped           = 0;
    cpu->m_cycle_counter        = 0;
    cpu->m_timer                = GB_timer_create();

    return cpu;                                                                         
}

void cpu_destroy(GB_cpu_t *cpu) {
    if (cpu == NULL) return; 
    GB_timer_destroy(cpu->m_timer);

    free(cpu);
}
