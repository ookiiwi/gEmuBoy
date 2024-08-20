#ifndef CPU_UTILS_H_
#define CPU_UTILS_H_

#include "cpudef.h"

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
#define _READ_MEMORY(addr, _inc_cycle_dummy)                                                                                        \
    (addr == 0xFF44 ? 0x90 : (addr <= 0x7FFF ? context->rom[addr] : context->memory[addr]))
#else
// TODO: Read BOOT ROM when accessing 0h-100h if still mapped
#define _READ_MEMORY(addr, _inc_cycle_dummy)        (addr <= 0x7FFF ? context->rom[addr] : context->memory[addr])
#endif

/* !!! VRAM and OAM inaccessible during PPU read and writes are ignored !!! 0xFF is returned */
#define READ_MEMORY(addr)           _READ_MEMORY(addr, context->m_cycle_counter++)
#define WRITE_MEMORY(addr, data) do {                                                                                               \
    int _data = data;                                                                                                               \
    switch(addr) {                                                                                                                  \
        case 0xFF04: _data = 0; /* DIV */       break;                                                                              \
        case 0xFF46: /* TODO: DMA */            break;                                                                              \
    }                                                                                                                               \
    context->memory[addr] = _data;                                                                                                  \
    context->m_cycle_counter++;                                                                                                     \
} while(0)

#endif