#include "cartridge/mbc.h"
#include "cartridge/cartridge.h"

BYTE GB_mbc0_read(GB_cartridge_t *cartridge, WORD addr) {
    if (addr > 0x9FFF && addr < 0xC000) {
        addr -= 0x2000;
    }
    return cartridge->data[addr];
}

void GB_mbc0_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    if (addr < 0x8000) {
        cartridge->data[addr] = data;
    } else if (addr > 0x9FFF && addr < 0xC000) {
        cartridge->data[addr-0x2000] = data;
    }
}