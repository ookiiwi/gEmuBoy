#ifndef GB_CPU_H_
#define GB_CPU_H_

#include "cpu/timer.h"
#include "type.h"
#include "defs.h"

typedef union
{
    WORD w;

    struct {
        BYTE l, h;
    } b;
} CPU_Reg;

typedef struct {
    WORD                ir;                         // Instruction register
    WORD                prev_ir;

    CPU_Reg             af;                         // Register AF (Accumulator + Flags)
    CPU_Reg             bc;                         // Register BC (B + C)
    CPU_Reg             de;                         // Register DE (D + E)
    CPU_Reg             hl;                         // Register HL (H + L)

    CPU_Reg             sp;                         // Stack pointer
    CPU_Reg             pc;                         // Program counter
    CPU_Reg             wz;                         // Memory pointer

    int                 IME;                        // Interrup master enable [write only]
    int                 ei_delay;

    int                 is_halted;
    int                 halt_bug;
    int                 is_stopped;
    uint64_t            t_cycle_counter;
    GB_timer_t         *timer;
} GB_cpu_t;

GB_cpu_t*   GB_cpu_create();
void        GB_cpu_destroy(GB_cpu_t *cpu);
void        GB_cpu_run(GB_gameboy_t *gb);

#endif