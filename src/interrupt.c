#include "interrupt.h"
#include "gb.h"
#include "cpudef.h"
#include "ppu.h"
#include "mmu.h"

#define IE (gb->memory[GB_IE_ADDR])
#define IF (gb->memory[GB_IF_ADDR])

static const int INTERRUPT_VECTOR[] = {
    0x40,
    0x48,
    0x50,
    0x58,
    0x60
};

#define PUSH_PC() do {                          \
    --SP; GB_mem_write(gb, SP, PCh);            \
    --SP; GB_mem_write(gb, SP, PCl);            \
    INC_CYCLE(2);                               \
} while(0)

#define PROCESS_INTERRUPTS() do {               \
    for (int i = 0; i < 5; i++) {               \
        if (intreq & (1 << i)) {                \
            PC = INTERRUPT_VECTOR[i];           \
            IF &= ~(1 << i);                    \
            break;                              \
        }                                       \
    }                                           \
    INC_CYCLE();                                \
} while(0);

// TODO: pass halt bug test rom 
void halt_bug_handler(GB_gameboy_t *gb, int intreq) {
    int next_ir = GB_mem_read(gb, PC+1);

    // HALT precedeed by EI
    if (PREV_IR == 0xFB) {
        PUSH_PC();              // 2 M-cycles
        PROCESS_INTERRUPTS();   // 1 M-cycle
        return;
    }

    // RST and byte read twice cases handled
    // in FETCH_CYCLE by checking m_halt_bug
    
    gb->cpu->m_is_halted = 0;
    gb->cpu->m_halt_bug  = 1;
}

void GB_handle_interrupt(GB_gameboy_t *gb) {
    int intreq = IE & IF & 0x1F;

    if (!intreq) return;

    // Wait 2 M-cycles
    INC_CYCLE(2);

    if ( !_IME && IR == 0x76 /* HALT */ ) {
        halt_bug_handler(gb, intreq);
    } else if (_IME) {
        PUSH_PC();                  // 2 M-cycles
        PROCESS_INTERRUPTS();       // 1 M-cycle
        gb->cpu->m_is_halted = 0;
    } else {
        gb->cpu->m_is_halted = 0;
    }

}