#ifndef GB_TIMER_H_
#define GB_TIMER_H_

#include "defs.h"
#include "type.h"

typedef struct GB_timer_s GB_timer_t;

GB_timer_t* GB_timer_create();
void        GB_timer_destroy(GB_timer_t *timer);
void        GB_timer_update(GB_gameboy_t *gb);
BYTE        GB_timer_write_check(GB_gameboy_t *gb, WORD addr, BYTE data);

#endif
