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

BYTE GB_mem_read(GB_gameboy_t *gb, WORD addr) {
    if (addr < VRAM_START_ADDR) { // ROM bank
        return gb->cartridge->read_callback(gb->cartridge, addr);
    } 
    
    else if (addr < EXT_RAM_START_ADDR) {   // VRAM
        return GB_ppu_vram_read(gb->ppu, addr);
    } 
    
    else if (addr < WRAM_START_ADDR) {      // RAM bank
        return 0xFF;
    } 
    
    else if (addr < ECHO_RAM_START_ADDR) {  // WRAM
        addr -= WRAM_START_ADDR;
        return gb->wram[addr];
    } 
    
    else if (addr < OAM_START_ADDR) {       // Echo wram C000-DDFF
        addr &= 0x1DFF; // TOODL
        return gb->wram[addr];
    } 
    
    else if (addr < UNUSABLE_START_ADDR) {  // OAM
        return 0x00;
    } 
    
    else if (addr < IO_REGS_START_ADDR) {   // Not Usable cf. https://gbdev.io/pandocs/Memory_Map.html#fea0feff-range
        return 0x00;
    } 
    
    else if (addr < HRAM_START_ADDR) {      // IO REGS
        addr -= IO_REGS_START_ADDR;
        return gb->io_regs[addr];
    } 
    
    else if (addr < IE_START_ADDR) {        // HRAM
        addr -= HRAM_START_ADDR;
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
    }
    
    else if (addr < ECHO_RAM_START_ADDR) {  // WRAM
        addr -= WRAM_START_ADDR;
        gb->wram[addr] = data;
    } 
    
    else if (addr < OAM_START_ADDR) {       // Echo wram C000-DDFF
        addr -= 0xC000; // WRONG
        gb->wram[addr] = data;
    } 

    else if (addr < UNUSABLE_START_ADDR) {  // OAM

    }
    
    else if (addr < IO_REGS_START_ADDR) {   // Not Usable cf. https://gbdev.io/pandocs/Memory_Map.html#fea0feff-range
    } 
    
    else if (addr < HRAM_START_ADDR) {      // IO REGS
        addr -= IO_REGS_START_ADDR;
        gb->io_regs[addr] = data;
    } 
    
    else if (addr < IE_START_ADDR) {        // HRAM
        addr -= HRAM_START_ADDR;
        gb->hram[addr] = data;
    } 
    
    else { // IE
        gb->ie = data;
    }
}
