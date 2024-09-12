#ifndef TIMER_H_
#define TIMER_H_

#include "defs.h"
#include "gbtypes.h"

#include <stdlib.h>

typedef struct {
    WORD m_sysclk;
    int last_tima_inc_val;
    int m_tima_state;
} GB_timer_t;

static inline GB_timer_t *GB_timer_create() {
    GB_timer_t *timer = (GB_timer_t*)( malloc(sizeof(GB_timer_t)) );
    timer->m_sysclk             = 0;
    timer->last_tima_inc_val    = 0;

    return timer;
}

void GB_update_timer(GB_gameboy_t *gb);

static inline void GB_timer_destroy(GB_timer_t *timer) {
    if (timer == NULL) return;

    free(timer);
}

#define TIMER_CHECK_WRITE(gb, addr, data) do {                      \
    /* Checks if TIMA is written during the M-cycle delay */        \
    if (addr == 0xFF05) {                                           \
        gb->cpu->m_timer->m_tima_state = 0;                         \
    } else if (addr == 0xFF04) { /* DIV */                          \
        data = 0;                                                   \
        gb->cpu->m_timer->m_sysclk = 0;                             \
    }                                                               \
} while(0)

#endif