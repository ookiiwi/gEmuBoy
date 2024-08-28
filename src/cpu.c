#include "cpu.h"
#include <stdlib.h>

GB_cpu_t* cpu_create() {
    GB_cpu_t *cpu                    = (GB_cpu_t*)( malloc( sizeof(GB_cpu_t) ) );
    cpu->ir.w                   = 0;                                             
    cpu->af.w                   = 0;                                             
    cpu->bc.w                   = 0;                                             
    cpu->de.w                   = 0;                                             
    cpu->hl.w                   = 0;                                             
    cpu->sp.w                   = 0;                                             
    cpu->pc.w                   = 0;                                             
    cpu->wz.w                   = 0;                                             
    cpu->m_IME                  = 0;                                             
    cpu->m_cycle_counter        = 0;                                             

    return cpu;                                                                         
}

void cpu_destroy(GB_cpu_t *cpu) {
    if (cpu != NULL) free(cpu);
}