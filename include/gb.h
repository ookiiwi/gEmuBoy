#ifndef GB_H_
#define GB_H_

#include "defs.h"
#include "cpu.h"

struct GB_gameboy_s {
    GB_cpu_t *cpu;

    /* Memory components */
    //BYTE *rom_0;            /* 0000-3FFF: 16KiB ROM bank 00         */
    //BYTE *rom_1;            /* 4000-7FFF: 16KiB ROM bank 01-NN      */
    //BYTE *vram;             /* 8000-9FFF: 8KiB Video RAM            */
    //BYTE *extram;           /* A000-BFFF: 8KiB External RAM         */
    //BYTE *wram_0;           /* C000-CFFF: 4 KiB Work RAM            */
    //BYTE *wram_1;           /* D000-DFFF: 4 KiB Work RAM            */
    //                        /* E000-FDFF: 4 KiB Work RAM            */
    //BYTE *oam;              /* FE00-FE9F: Object attribute memory   */
    //BYTE *forbidden;        /* FEA0-FEFF: Not usable                */
    //BYTE *io;               /* FF00-FF7F: I/O Registers             */
    //BYTE *hram;             /* FF80-FFFE: High RAM (0 Page)         */
    //BYTE *hram;             /* FF80-FFFE: I/O Registers             */
    //BYTE ie;                /* FFFF-FFFF: Interrupt Enable register */
    BYTE *memory;
    BYTE *rom;
    size_t              rom_size;

    WORD                m_div;                      // DIV timer
};

GB_gameboy_t*   GB_create(const char *src_rom_path);
void            GB_destroy(GB_gameboy_t *gb);

#endif