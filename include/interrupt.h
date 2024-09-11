#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include "defs.h"

#define IE (gb->memory[0xFFFF])
#define IF (gb->memory[0xFF0F])
#define INT_DEFAULT (0xE0)

#define IF_VBLANK   (0x01)
#define IF_LCD      (0x02)
#define IF_TIMER    (0x04)
#define IF_SERIAL   (0x08)
#define IF_JOYPAD   (0x10)

#define REQUEST_INTERRUPT(b)    ( IF |= ( INT_DEFAULT | b )         )
#define ENABLE_INTERRUPT(b)     ( IE |= ( INT_DEFAULT | b )         )
#define DISABLE_INTERRUPT(b)    ( IE  = ( IE | INT_DEFAULT ) & ~b   )

void GB_handle_interrupt(GB_gameboy_t *gb);

#endif