#ifndef MMU_H_
#define MMU_H_

#include "gb.h"
#include "gbtypes.h"

BYTE GB_mem_read(GB_gameboy_t *gb, int addr);
void GB_mem_write(GB_gameboy_t *gb, int addr, BYTE data);

#endif