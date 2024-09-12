#include "timer.h"
#include "cpu.h"
#include "cpudef.h"
#include "gb.h"
#include "interrupt.h"
#include "memmap.h"

#define DIV     (gb->memory[GB_DIV_ADDR])
#define TIMA    (gb->memory[GB_TIMA_ADDR])
#define TAC     (gb->memory[GB_TAC_ADDR])
#define TMA     (gb->memory[GB_TMA_ADDR])

#define TAC_FREQ                (TAC & 3)
#define TAC_ENABLE              (TAC >> 2)

#define SYSCLK                  ( gb->cpu->m_timer->m_sysclk            )
#define LAST_TIMA_INC_VAL       ( gb->cpu->m_timer->last_tima_inc_val   )
#define TIMA_STATE              ( gb->cpu->m_timer->m_tima_state        )

static const int TAC_MULTIPLEXER[] = { 512, 8, 32, 128 };

void GB_update_timer(GB_gameboy_t *gb) {
    if (TIMA_STATE) {
        TIMA = TMA;
        REQUEST_INTERRUPT(IF_TIMER);
        TIMA_STATE = 0;
    }

    for (int i = 0; i < 4; i++) {
        int tima_inc_val;
        
        SYSCLK++;
        tima_inc_val = TAC_ENABLE && ( TAC_MULTIPLEXER[TAC_FREQ] & SYSCLK );

        // Falling edge detection
        if ( !TIMA_STATE && LAST_TIMA_INC_VAL && !tima_inc_val ) {
            if (TIMA == 0xFF) {
                TIMA_STATE = 1;
            }

            TIMA++;
        }

        LAST_TIMA_INC_VAL = tima_inc_val;
    }

    DIV = SYSCLK >> 8;
}
