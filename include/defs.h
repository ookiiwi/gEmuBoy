#ifndef DEFS_H_
#define DEFS_H_

typedef struct GB_gameboy_s GB_gameboy_t;

#define GET_MACRO(_0, _1, NAME, ...) NAME
#define _INC_CYCLE0() do {                                              \
    (gb->cpu)->m_cycle_counter++;                                       \
    ppu_tick(gb);                                                       \
    GB_update_timer(gb);                                                \
} while(0)
#define _INC_CYCLE1(step)     for (int i = 0; i < step; i++) _INC_CYCLE0()
#define INC_CYCLE(...)    GET_MACRO(_0, ##__VA_ARGS__, _INC_CYCLE1, _INC_CYCLE0)(__VA_ARGS__)

#define FETCH_CYCLE() do {                                                                                                          \
    LOG_CPU_STATE();                                                                                                                \
    GB_handle_interrupt(gb);                                                                                                        \
    PREV_IR = IR;                                                                                                                   \
    IR = READ_MEMORY(PC); PC++;                                                                                                     \
    if (gb->cpu->m_halt_bug) {  /* Either adjust RST's pushed address or else read byte twice */                                    \
        PC--;                                                                                                                       \
        gb->cpu->m_halt_bug = 0;                                                                                                    \
    }                                                                                                                               \
} while(0)

#endif