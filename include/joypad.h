#ifndef GB_JOYPAD_H_
#define GB_JOYPAD_H_

#include "memmap.h"
#include "cpu/interrupt.h"

#include <SDL_keyboard.h>
#include <SDL_scancode.h>

typedef struct {
    const Uint8 *kb_state;
} GB_joypad_t;

static inline GB_joypad_t* GB_joypad_create() {
    GB_joypad_t *joypad = (GB_joypad_t*)( malloc( sizeof (GB_joypad_t) ) );

    if (!joypad) {
        return NULL;
    }

    joypad->kb_state = SDL_GetKeyboardState(NULL);

    return joypad;
}

static inline void GB_joypad_destroy(GB_joypad_t *joypad) {
    if (joypad) free(joypad);
}

#define GB_P1                   ( gb->io_regs[GB_JOYP_ADDR&0xFF] )
#define _GB_P1_SELECT_BUTTONS   ( GB_P1 & 0x20 )
#define _GB_P1_SELECT_DPAD      ( GB_P1 & 0x10 )

#define _GB_SET_P1_BIT_TO_X(bit_pos, x)                                                                         \
    ( GB_P1 = (GB_P1 & 0xF0) | ( GB_P1 & ( 0xF & ( ~(1<<bit_pos) ) ) | ( ( ((BYTE)x)&1 ) << bit_pos ) ) )

#define GB_joypad_read(gb, addr) (                                                                              \
    ( 0xC0 | ( GB_P1 & 0x30 ) ) |                               /* Select bits      */                          \
    ( ( ( GB_P1 & 0x30 ) == 0x30 ) ? 0xF : ( GB_P1 & 0xF ) )    /* Buttons          */                          \
)

#define GB_joypad_write(gb, addr, data) do { GB_P1 = ( GB_P1 & (~0x30) ) | ( data & 0x30 );  } while(0)

#define _GB_joypad_set_key(scancode, bit_pos)                                                                   \
    if (gb->joypad->kb_state[scancode]) {                                                                       \
        int prev_state = GB_joypad_read(gb, 0) & 0xF;                                                           \
        _GB_SET_P1_BIT_TO_X(bit_pos, 0);                                                                        \
        if ( prev_state != ( GB_joypad_read(gb, 0) & 0xF ) ) REQUEST_INTERRUPT(IF_JOYPAD);                      \
    } else {                                                                                                    \
        _GB_SET_P1_BIT_TO_X(bit_pos, 1);                                                                        \
    }                                                                                                           \

#define _GB_joypad_update_keys(target_selection_mode, k3, k2, k1, k0) do {                                      \
    if ( !( _GB_P1_SELECT_##target_selection_mode ) ) {                                                         \
        _GB_joypad_set_key(k3, 3);                                                                              \
        _GB_joypad_set_key(k2, 2);                                                                              \
        _GB_joypad_set_key(k1, 1);                                                                              \
        _GB_joypad_set_key(k0, 0);                                                                              \
    }                                                                                                           \
} while (0)

#define GB_joypad_update(gb) do {                                                                               \
    _GB_joypad_update_keys(BUTTONS, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_RETURN, SDL_SCANCODE_Q, SDL_SCANCODE_W);  \
    _GB_joypad_update_keys(DPAD, SDL_SCANCODE_DOWN, SDL_SCANCODE_UP, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);    \
} while (0)

#endif
