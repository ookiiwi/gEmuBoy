#ifndef GB_GAMEBOY_H_
#define GB_GAMEBOY_H_

#include "cpu/cpu.h"
#include "cartridge/cartridge.h"
#include "graphics/ppu.h"

#define WRAM_SIZE       (0x2000)
#define IO_REGS_SIZE    (0x0080)
#define HRAM_SIZE       (0x007F)

struct GB_gameboy_s {
    GB_cartridge_t  *cartridge;
    GB_cpu_t        *cpu;
    GB_ppu_t        *ppu;

    // Registers
    BYTE    *wram;      // C000-DFFF
    BYTE    *io_regs;   // FF00-FF7F
    BYTE    *hram;      // FF80-FFFE
    BYTE    ie;         // FFFF
};

GB_gameboy_t*   GB_gameboy_create(const char *rom_path);
void            GB_gameboy_destroy(GB_gameboy_t *gb);

#endif