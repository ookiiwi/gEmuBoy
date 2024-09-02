#include "timer.h"
#include "cpu.h"
#include "cpudef.h"
#include "gb.h"
#include "interrupt.h"

#define TAC_FREQ                (TAC & 3)
#define TAC_ENABLE              (TAC >> 2)

#define SYSCLK                  ( gb->cpu->m_timer->m_sysclk )
#define LAST_TIMA_INC_VAL       ( gb->cpu->m_timer->last_tima_inc_val )
#define TIMA_OVERFLOW_CYCLE     ( gb->cpu->m_timer->m_tima_overflow_cycle )

static const int TAC_MULTIPLEXER[] = { 
    0x100, /* 0b00: bit 9 */
    0x004, /* 0b01: bit 3 */
    0x020, /* 0b10: bit 5 */
    0x040  /* 0b11: bit 7 */
};

void GB_update_timer(GB_gameboy_t *gb) {
    for (int i = 0; i < 4; i++) {
        int tima_inc_val;
        
        SYSCLK++;
        DIV = SYSCLK >> 8;
        tima_inc_val = TAC_ENABLE && (TAC_MULTIPLEXER[TAC_FREQ] & SYSCLK );

        if (TIMA_OVERFLOW_CYCLE) {
            TIMA = TMA;
            REQUEST_INTERRUPT(IF_TIMER);
        }

        // Falling edge detection
        if ( LAST_TIMA_INC_VAL && !tima_inc_val ) {
            if (TIMA == 0xFF) {
                TIMA_OVERFLOW_CYCLE = gb->cpu->m_cycle_counter - 4 + i;
            }

            TIMA++;
        }

        LAST_TIMA_INC_VAL = tima_inc_val;
    }
}