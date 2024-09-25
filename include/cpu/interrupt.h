#ifndef GB_INTERRUPT_H_
#define GB_INTERRUPT_H_

#include "defs.h"

#define IF_VBLANK   (0x01)
#define IF_LCD      (0x02)
#define IF_TIMER    (0x04)
#define IF_SERIAL   (0x08)
#define IF_JOYPAD   (0x10)

#define INT_DEFAULT (0xE0)

#define REQUEST_INTERRUPT(b)    ( gb->io_regs[0xF] |= b )
#define ENABLE_INTERRUPT(b)     ( gb->ie |= b           )
#define DISABLE_INTERRUPT(b)    ( gb->ie &= ~b          )

// This call is supposed to happen after next instruction fetching
void GB_interrupt_handle(GB_gameboy_t *gb);

#endif