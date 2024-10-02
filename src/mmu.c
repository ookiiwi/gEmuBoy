#include "mmu.h"
#include "gb.h"
#include "graphics/ppu.h"
#include "memmap.h"

#include <stdio.h>

#define VRAM_START_ADDR     (0x8000)
#define EXT_RAM_START_ADDR  (0xA000)
#define WRAM_START_ADDR     (0xC000)
#define ECHO_RAM_START_ADDR (0xE000)
#define OAM_START_ADDR      (0xFE00)
#define UNUSABLE_START_ADDR (0xFEA0)
#define IO_REGS_START_ADDR  (0xFF00)
#define HRAM_START_ADDR     (0xFF80)
#define IE_START_ADDR       (0xFFFF)

#define ADJUST_ADDR(min, max) do {                                                          \
    if ( addr < min || addr >= max) {                                                       \
        fprintf(stderr, "OUT OF RANGE $%04X NOT IN RANGE [$%04X-$%04X]\n", addr, min, max); \
        addr = max-1;                                                                       \
    }                                                                                       \
    addr -= min;                                                                            \
} while(0)

BYTE GB_mem_read(GB_gameboy_t *gb, WORD addr) {
    if (addr < VRAM_START_ADDR) {           // ROM bank     -- 0000-7FFF
        return gb->cartridge->read_callback(gb->cartridge, addr);
    } 
    
    else if (addr < EXT_RAM_START_ADDR) {   // VRAM         -- 8000-9FFF
        return GB_ppu_vram_read(gb->ppu, addr);
    } 
    
    else if (addr < WRAM_START_ADDR) {      // RAM bank     -- A000-BFFF
        return gb->cartridge->read_callback(gb->cartridge, addr);
    } 
    
    else if (addr < ECHO_RAM_START_ADDR) {  // WRAM         -- C000-DFFF
        ADJUST_ADDR(WRAM_START_ADDR, ECHO_RAM_START_ADDR);
        return gb->wram[addr];
    } 
    
    else if (addr < OAM_START_ADDR) {       // Echo wram    -- E000-FDFF
        ADJUST_ADDR(ECHO_RAM_START_ADDR, OAM_START_ADDR);
        return gb->wram[addr];
    } 
    
    else if (addr < UNUSABLE_START_ADDR) {  // OAM          -- FE00-FE9F
        return GB_ppu_oam_read(gb->ppu, addr);
    } 
    
    else if (addr < IO_REGS_START_ADDR) {   // Not Usable   -- FEA0-FEFF cf. https://gbdev.io/pandocs/Memory_Map.html#fea0feff-range
        ADJUST_ADDR(UNUSABLE_START_ADDR, IO_REGS_START_ADDR);
        return gb->unusable[addr];
    } 
    
    else if (addr < HRAM_START_ADDR) {      // IO REGS      -- FF00-FF7F
        ADJUST_ADDR(IO_REGS_START_ADDR, HRAM_START_ADDR);
        return gb->io_regs[addr];
    } 
    
    else if (addr < IE_START_ADDR) {        // HRAM         -- FF80-FFFE
        ADJUST_ADDR(HRAM_START_ADDR, IE_START_ADDR);
        return gb->hram[addr];
    } 

    // IE    
    return gb->ie;
}

void GB_mem_write(GB_gameboy_t *gb, WORD addr, BYTE data) {
    if (addr == GB_SC_ADDR && data == 0x81) {
        printf("%c", (char)gb->io_regs[1]);
    }

    if (addr < VRAM_START_ADDR) {           // ROM bank
        gb->cartridge->write_callback(gb->cartridge, addr, data);
    } 
    
    else if (addr < EXT_RAM_START_ADDR) {   // VRAM
        GB_ppu_vram_write(gb->ppu, addr, data);
    } 
    
    else if (addr < WRAM_START_ADDR) {      // RAM bank
        gb->cartridge->write_callback(gb->cartridge, addr, data);
    }
    
    else if (addr < ECHO_RAM_START_ADDR) {  // WRAM
        ADJUST_ADDR(WRAM_START_ADDR, ECHO_RAM_START_ADDR);
        gb->wram[addr] = data;
    } 
    
    else if (addr < OAM_START_ADDR) {       // Echo wram C000-DDFF
        ADJUST_ADDR(ECHO_RAM_START_ADDR, OAM_START_ADDR);
        gb->wram[addr] = data;
    } 

    else if (addr < UNUSABLE_START_ADDR) {  // OAM
        GB_ppu_oam_write(gb->ppu, addr, data);
    }
    
    else if (addr < IO_REGS_START_ADDR) {   // Not Usable cf. https://gbdev.io/pandocs/Memory_Map.html#fea0feff-range
        ADJUST_ADDR(UNUSABLE_START_ADDR, IO_REGS_START_ADDR);
        gb->unusable[addr] = data;
    } 
    
    else if (addr < HRAM_START_ADDR) {      // IO REGS
        ADJUST_ADDR(IO_REGS_START_ADDR, HRAM_START_ADDR);
        gb->io_regs[addr] = data;
    } 
    
    else if (addr < IE_START_ADDR) {        // HRAM
        ADJUST_ADDR(HRAM_START_ADDR, IE_START_ADDR);
        gb->hram[addr] = data;
    } 
    
    else {                                  // IE
        gb->ie = data;
    }
}
