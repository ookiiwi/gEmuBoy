#ifndef GB_PPU_H_
#define GB_PPU_H_

#include "type.h"

typedef struct {
    BYTE    *vram;
} GB_ppu_t;

GB_ppu_t*   GB_ppu_create();
void        GB_ppu_destroy(GB_ppu_t *ppu);

BYTE        GB_ppu_vram_read(GB_ppu_t *ppu, WORD addr);
void        GB_ppu_vram_write(GB_ppu_t *ppu, WORD addr, BYTE data);

#endif