#include "cpu/interrupt.h"
#include "type.h"
#include "memmap.h"
#include "gb_utils.h"
#include "cpudef.h"
#include "mmu.h"

#define IE ( gb->ie )
#define IF ( gb->io_regs[GB_IF_ADDR & 0xFF] )

#define IRQ ( IE & IF & 0x1F )

static const int INTERRUPT_VECTOR[] = {
    0x40, // VBlank
    0x48, // STAT
    0x50, // Timer
    0x58, // Serial
    0x60  // Joypad
};

#define SERVE_INTERRUPT() do {                                                                                                  \
    WORD pc = PC-1;                 INC_CYCLE();    /* M0: Adjust PC as handling happens after fetch and fetch does PC++  */    \
    SP--;                           INC_CYCLE();    /* M1 */                                                                    \
    GB_mem_write(gb, SP--, (pc>>8));INC_CYCLE();    /* M2 */                                                                    \
    GB_mem_write(gb, SP, (pc&0xFF));INC_CYCLE();    /* M3 */                                                                    \
    int irq = IRQ, irq_index = 0;                                                                                               \
    while( !(irq & 1) ) { irq >>= 1; irq_index++; } /* M3 */                                                                    \
    PC = (8 * irq_index) + 0x40;                    /* M3 */                                                                    \
    IF &= ~(1 << irq_index);                        /* M3 */                                                                    \
    _IME = 0;                                       /* M4 */                                                                    \
    IR = GB_mem_read(gb, PC++);     INC_CYCLE();    /* M4 */                                                                    \
} while(0)


#define HALT_BUG_HANDLER() do {                                                                                                 \
    if (prev_ir == 0xFB) { /* HALT precedeed by EI */                                                                           \
        SERVE_INTERRUPT();                                                                                                      \
    } else { /* Either adjust RST's pushed address or else read byte twice */                                                   \
        PC--;                                                                                                                   \
    }                                                                                                                           \
} while(0)

void GB_interrupt_handle(GB_gameboy_t *gb, BYTE ir, BYTE prev_ir) {
    if (!IRQ) return;

    if ( !_IME && ir == 0x76 /* HALT */ ) {
        HALT_BUG_HANDLER();
    } else if (_IME) {
        SERVE_INTERRUPT();
    }

    gb->cpu->is_halted  = 0;
}
