#include "cartridge/mbc.h"
#include "cartridge/cartridge.h"

BYTE GB_mbc0_read(GB_cartridge_t *cartridge, WORD addr) {
    return cartridge->data[addr];
}

void GB_mbc0_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    cartridge->data[addr] = data;
}