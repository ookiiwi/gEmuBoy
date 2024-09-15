#ifndef MMU_H_
#define MMU_H_

#include "defs.h"
#include "gb.h"
#include "gbtypes.h"

GB_mmu_t* 	GB_mmu_create();
void 		GB_mmu_destroy(GB_mmu_t *mmu);
BYTE 		GB_mem_read(GB_gameboy_t *gb, int addr);
void 		GB_mem_write(GB_gameboy_t *gb, int addr, BYTE data);

void 		GB_dma_run(GB_gameboy_t *gb);

#endif
