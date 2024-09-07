#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include "defs.h"

typedef struct GB_debugger_s GB_debugger_t;

GB_debugger_t*  GB_debugger_create(GB_gameboy_t *gb);
void            GB_debugger_destroy(GB_debugger_t *debugger);
void            GB_debugger_run(GB_debugger_t *debugger);
void            GB_debugger_show(GB_debugger_t *debugger);
void            GB_debugger_hide(GB_debugger_t *debugger);

#endif