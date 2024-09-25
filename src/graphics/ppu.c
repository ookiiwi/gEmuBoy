#include "graphics/ppu.h"

#include <stdlib.h>
#include <stdio.h>

GB_ppu_t* GB_ppu_create() {
    GB_ppu_t *ppu = (GB_ppu_t*)( malloc( sizeof (GB_ppu_t) ) );
    if (!ppu) { return NULL; }

    ppu->vram = (BYTE*)( malloc( sizeof (BYTE) *  0x2000 ) );
    if (!ppu->vram) {
        free(ppu);
        return NULL;
    }

    return ppu;
}

void GB_ppu_destroy(GB_ppu_t *ppu) {
    if (!ppu) return;

    if (ppu->vram) free(ppu->vram);
    free(ppu);
}

BYTE GB_ppu_vram_read(GB_ppu_t *ppu, WORD addr) {
    addr = ( addr - 0x8000 ) & 0x1FFF;
    return ppu->vram[addr];
}

void GB_ppu_vram_write(GB_ppu_t *ppu, WORD addr, BYTE data) {
    addr = ( addr - 0x8000 ) & 0x1FFF;
    ppu->vram[addr] = data;
}