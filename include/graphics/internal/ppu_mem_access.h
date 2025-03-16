#ifndef GB_PPU_MEM_ACCESS_H_
#define GB_PPU_MEM_ACCESS_H_

#include "graphics/ppu.h"
#include "type.h" /* BYTE, WORD */
#include <string.h> /* memcpy */
#include <stdio.h> /* fprintf, stderr */

#define vram_read(ppu, addr, res) do {                                                          \
    if ((addr) < 0x8000 || (addr) >= 0xA000) {                                                  \
        fprintf(stderr, "VRAM READ OUT OF RANGE: $%04X (%s, %d)\n", addr, __FILE__, __LINE__);  \
        *(res) = 0xFF;                                                                          \
    }                                                                                           \
    (addr) = ( (addr) - 0x8000 ) & 0x1FFF;                                                      \
    *(res) = ppu->vram[addr];                                                                   \
} while (0)

static inline void vram_write(GB_ppu_t *ppu, WORD addr, BYTE data) {
    if (addr < 0x8000 || addr >= 0xA000) {
        fprintf(stderr, "VRAM WRITE OUT OF RANGE: $%04X (%s, %d)\n", addr, __FILE__, __LINE__);
        return;
    }

    addr = ( addr - 0x8000 ) & 0x1FFF;
    ppu->vram[addr] = data;
}

#define oam_read_object(ppu, addr, dst) do {                                                    \
    if (addr < 0xFE00 || addr > 0xFE9F) {                                                       \
        fprintf(stderr, "OAM READ OUT OF RANGE: $%04X (%s, %d)\n", addr, __FILE__, __LINE__);   \
        break;                                                                                  \
    }                                                                                           \
    addr -= 0xFE00;                                                                             \
    memcpy((void*)dst, &(ppu->oam[addr]), sizeof *dst);                                         \
} while (0)

#endif
