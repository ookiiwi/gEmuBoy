#include "mmu.h"
#include "timer.h"
#include "memmap.h"

BYTE GB_mem_read(GB_gameboy_t *gb, int addr) {
    BYTE res = gb->memory[addr];

    if (addr <= 0x3FFF) {           /* Bank 00 */
        if (addr < 0x100 && !GB_BOOT_ROM_UNMAPPED) {
            res = BOOTROM[addr];
        } else {
            res = gb->rom[addr];
        }
    } else if (addr <= 0x7FFF) {    /* Bank 01-NN */
        res = gb->rom[addr];
    }

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
    else if (addr == GB_LY_ADDR) {
        res = 0x90;
    }
#endif

    return res;
}

void GB_mem_write(GB_gameboy_t *gb, int addr, BYTE data) {
    if (addr == GB_LY_ADDR) { // LY
        return;
    } else if (addr == 0xFF46) {
        printf("DMA\n");
    }

    TIMER_CHECK_WRITE(gb, addr, data);

    gb->memory[addr] = data;
}