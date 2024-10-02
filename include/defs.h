#ifndef DEFS_H_
#define DEFS_H_

typedef struct GB_gameboy_s GB_gameboy_t;

#define INC_CYCLE() do {                                              															    \
    gb->cpu->t_cycle_counter+=4;                                        															\
    GB_ppu_tick(gb, 4);                                                                                                             \
    GB_timer_update(gb);                                               															    \
} while(0)

#define FETCH_CYCLE() do {                                                                                                          \
    PREV_IR = IR;                                                                                                                   \
    INC_CYCLE();                                                                                                                    \
    IR = GB_mem_read(gb, PC++);                                                                                                     \
    GB_interrupt_handle(gb);                                                                                                        \
} while(0)

#endif
