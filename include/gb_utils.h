#ifndef GB_UTILS_H_
#define GB_UTILS_H_

#include "gb.h"
#include "cpu/interrupt.h"
#include "cpu/timer.h"
#include "graphics/ppu.h"

#define INC_CYCLE() do {                                              															    \
    gb->cpu->t_cycle_counter+=4;                                        															\
    GB_dma_run(gb);                                                                                                                 \
    GB_ppu_tick(gb, 4);                                                                                                             \
    GB_timer_update(gb);                                               															    \
} while(0)

#define FETCH_CYCLE() do {                                                                                                          \
    BYTE ir = IR, prev_ir = PREV_IR;                                                                                                \
    PREV_IR = IR;                                                                                                                   \
    INC_CYCLE();                                                                                                                    \
    IR = GB_mem_read(gb, PC++);                                                                                                     \
    GB_interrupt_handle(gb, ir, prev_ir);                                                                                           \
} while(0)

#endif
