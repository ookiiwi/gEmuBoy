#ifndef GB_MMU_H_
#define GB_MMU_H_

#include "type.h"
#include "defs.h"

BYTE        GB_mem_read(GB_gameboy_t *gb, WORD addr);
void        GB_mem_write(GB_gameboy_t *gb, WORD addr, BYTE data);

GB_mmu_t*   GB_mmu_create();
void        GB_mmu_destroy(GB_mmu_t *mmu);

void        GB_dma_run(GB_gameboy_t *gb);

#endif