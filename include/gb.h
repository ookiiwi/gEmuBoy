#ifndef GB_H_
#define GB_H_

#include "defs.h"
#include "cpu.h"
#include "ppu.h"

struct GB_gameboy_s {
    GB_cpu_t *cpu;
    GB_ppu_t *ppu;

    BYTE *memory;
    BYTE *rom;
    size_t              rom_size;

    WORD                m_div;                      // DIV timer
};

GB_gameboy_t*   GB_create(const char *src_rom_path);
void            GB_destroy(GB_gameboy_t *gb);
void            GB_run(GB_gameboy_t *gb);

#endif