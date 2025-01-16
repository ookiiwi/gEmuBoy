#include "cpu/timer.h"
#include "cpu/interrupt.h"
#include "gb.h"
#include "cpudef.h"

#include <stdlib.h>
#include <stdio.h>

#define TIMER                   ( gb->cpu->timer )
#define SYSCLK                  ( TIMER->sysclk )
#define LAST_TIMA_INC_VAL       ( TIMER->last_tima_inc_val   )
#define TIMA_RELOAD_COUNTER     ( TIMER->tima_reload_counter )
#define OLD_TMA                 ( TIMER->old_tma )

#define DIV                     ( gb->io_regs[4] )
#define TIMA                    ( gb->io_regs[5] )
#define TMA                     ( gb->io_regs[6] )
#define TAC                     ( gb->io_regs[7] )

#define TAC_SEL                 ( TAC &  3 )
#define TAC_ENABLE              ( TAC >> 2 )

static const int TAC_MULTIPLEXER[] = { 512, 8, 32, 128 };

struct GB_timer_s {
    WORD sysclk;
    int last_tima_inc_val;
    int tima_reload_counter;
    BYTE old_tma;
    int tmp;
};

GB_timer_t* GB_timer_create() {
    GB_timer_t *timer = (GB_timer_t*)( malloc( sizeof (GB_timer_t) ) );
    
    if (timer != NULL) {
        timer->sysclk = 0;
        timer->last_tima_inc_val = 0;
        timer->tima_reload_counter = 0;
        timer->old_tma = 0;
    }

    return timer;
}

void GB_timer_destroy(GB_timer_t *timer) {
    if (timer) free(timer);
}

void GB_timer_update(GB_gameboy_t *gb) {
    int tima_inc_val;

    for (int i = 0; i < 4; i++) {
        SYSCLK++;
        tima_inc_val = TAC_ENABLE && ( TAC_MULTIPLEXER[TAC_SEL] & SYSCLK );

        if (!TIMA_RELOAD_COUNTER) {
            TIMA = OLD_TMA;
            TIMA_RELOAD_COUNTER = -1;
            OLD_TMA = TMA;
            REQUEST_INTERRUPT(IF_TIMER);
        } else if (TIMA_RELOAD_COUNTER > 0) {
            TIMA_RELOAD_COUNTER--;
        }

        if ( (TIMA_RELOAD_COUNTER < 0) && LAST_TIMA_INC_VAL && !tima_inc_val ) {
            if (TIMA == 0xFF) { TIMA_RELOAD_COUNTER = 3; }
            TIMA++; // On overflow, TIMA equals 0 for 4 T cycles
        }

        LAST_TIMA_INC_VAL = tima_inc_val;
    }

    DIV = SYSCLK>>8;
}

BYTE GB_timer_write_check(GB_gameboy_t *gb, WORD addr, BYTE data) {
    /* Checks if TIMA is written during the M-cycle delay */
    if (addr == 0xFF05) {
        TIMA_RELOAD_COUNTER = -1;
    } else if (addr == 0xFF06) {
        // TODO: pass tma_write_reloading
        OLD_TMA = TMA;
        if (TIMA_RELOAD_COUNTER < 0) {
            OLD_TMA = data;
        }

    } else if (addr == 0xFF04) { /* DIV */
        data = 0;
        SYSCLK = 0;
    }

    return data;
}

