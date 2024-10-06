#include "cartridge/mbc.h"
#include "cartridge/cartridge.h"

#include <stdlib.h>
#include <stdio.h>

#define BANK_MASK       ( 1 << cartridge->header->rom_size )
#define BANK1_NUMBER    ( ( cartridge->mbc->bank_number & 0x1F ) & BANK_MASK )
#define BANK2_NUMBER    ( ( cartridge->mbc->bank_number & 0x60 ) & BANK_MASK )
#define BANK_NUMBER     ( cartridge->mbc->bank_number  )
#define RAMBANK_NUMBER  ( BANK2_NUMBER >> 5  )

#define RAM_ENABLED     ( cartridge->mbc->ram_enabled )
#define BANKING_MODE    ( cartridge->mbc->banking_mode )

#define CHECK_BOUNDERIES(a, b, msg, rv) do {                                \
    if (addr < a || addr >= b) {                                            \
        fprintf(stderr, "OUT OF RANGE %s ($%04X)\n", msg, addr);            \
        return rv;                                                          \
    }                                                                       \
} while(0)

GB_MBC_t* GB_MBC_create() {
    GB_MBC_t *mbc = (GB_MBC_t*)( malloc( sizeof (GB_MBC_t) ) );

    if (mbc) {
        mbc->bank_number    = 1;
        mbc->banking_mode   = 0;
    }

    return mbc;
}

void GB_MBC_destroy(GB_MBC_t *mbc) {
    if (mbc) free(mbc);
}

BYTE GB_mbc0_read(GB_cartridge_t *cartridge, WORD addr) {
    if (addr > 0x9FFF && addr < 0xC000) {
        if (!RAM_SIZE(cartridge->header)) return 0x00;
        addr -= 0x2000;
    }

    CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 READ ROM BANK0", 0xFF);
    return cartridge->data[addr];
}

void GB_mbc0_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    if (addr < 0x8000) {
        CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 WRITE ROM BANK0", );
        cartridge->data[addr] = data;
    } else if (addr > 0x9FFF && addr < 0xC000 && RAM_SIZE(cartridge->header)) {
        addr -= 0x2000;

        CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 WRITE RAM BANK0", );
        cartridge->data[addr] = data;
    }
}

BYTE GB_mbc1_read(GB_cartridge_t *cartridge, WORD addr) {
    if (addr < 0x4000) {                                                // ROM Bank 00
        if (BANKING_MODE) { // MODE1 uses Bank2 only
            addr = (BANK2_NUMBER << 14) | (addr & 0x3FFF);
        }

        CHECK_BOUNDERIES(0, cartridge->data_size, "READ ROM BANK0", 0xFF);

        return cartridge->data[addr];
    } else if (addr < 0x8000) {                                         // ROM Bank 01-7F
        addr = (BANK_NUMBER << 14) | ( addr & 0x3FFF );

        CHECK_BOUNDERIES(0, cartridge->data_size, "READ ROM BANK N", 0xFF);

        return cartridge->data[addr];
    } else if (addr > 0x9FFF && addr < 0xC000) {         // RAM Bank 00-03
        if (!RAM_ENABLED || !RAM_SIZE(cartridge->header)) {
            fprintf(stderr, "NO RAM\n");
            return 0xFF;
        }
        
        if (BANKING_MODE && RAM_SIZE(cartridge->header) > 8*1024) {
            addr = (BANK2_NUMBER << 8) | (addr & 0x1FFF);
        }

        CHECK_BOUNDERIES(0, cartridge->data_size, "READ RAM BANK", 0xFF);

        return cartridge->data[addr];
    }

    return 0xFF;
}

void GB_mbc1_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    if (addr < 0x1FFF) {                                                    // RAM Enable
        cartridge->mbc->ram_enabled = data == 0x0A;
    } else if (addr < 0x4000) {                                             // ROM Bank Number
        data &= 0x1F;
        cartridge->mbc->bank_number = BANK2_NUMBER | (data ? data : 1);
    } else if (addr < 0x6000) {                                             // RAM Bank Number
        data = (data &0x3) << 5;
        cartridge->mbc->bank_number = data | BANK1_NUMBER;
    } else if (addr < 0x8000) {
        cartridge->mbc->banking_mode = (data != 0);
    }
}