#ifndef GB_GAMEBOY_H_
#define GB_GAMEBOY_H_

#include "cpu/cpu.h"
#include "cartridge/cartridge.h"
#include "graphics/ppu.h"
#include "defs.h"

struct GB_gameboy_s {
    GB_cartridge_t  *cartridge;
    GB_cpu_t        *cpu;
    GB_ppu_t        *ppu;
    GB_mmu_t        *mmu;

    // Registers
    BYTE    *wram;      // C000-DFFF
    BYTE    *unusable;  // FEA0-FEFF
    BYTE    *io_regs;   // FF00-FF7F
    BYTE    *hram;      // FF80-FFFE
    BYTE    ie;         // FFFF
};

GB_gameboy_t*   GB_gameboy_create(const char *rom_path, int headless);
void            GB_gameboy_destroy(GB_gameboy_t *gb);

#endif