#include "cartridge/mbc.h"
#include "cartridge/cartridge.h"

#include <stdlib.h>
#include <stdio.h>

#define BANK_MASK       ( ( 2 << cartridge->header->rom_size ) - 1 )
#define BANK1_NUMBER    ( ( cartridge->mbc->bank_number & 0x1F ) & BANK_MASK )
#define BANK2_NUMBER    ( ( cartridge->mbc->bank_number & 0x60 ) & BANK_MASK )
#define BANK_NUMBER     ( cartridge->mbc->bank_number & BANK_MASK )
#define RAMBANK_NUMBER  ( BANK2_NUMBER >> 5  )

#define RAM_ENABLED     ( cartridge->mbc->ram_enabled )
#define BANKING_MODE    ( cartridge->mbc->banking_mode )

#define CHECK_BOUNDERIES(a, b, msg, rv) do {                                                                \
    if (phys_addr < a || phys_addr >= b) {                                                                  \
        fprintf(stderr, "OUT OF RANGE %s BANK%d ($%04X => $%llX)\n", msg, BANK_NUMBER, addr, phys_addr);    \
        return rv;                                                                                          \
    }                                                                                                       \
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

    //CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 READ ROM BANK0", 0xFF);
    return cartridge->data[addr];
}

void GB_mbc0_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    if (addr < 0x8000) {
        //CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 WRITE ROM BANK0", );
        cartridge->data[addr] = data;
    } else if (addr > 0x9FFF && addr < 0xC000 && RAM_SIZE(cartridge->header)) {
        addr -= 0x2000;

        //CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 WRITE RAM BANK0", );
        cartridge->data[addr] = data;
    }
}

BYTE GB_mbc1_read(GB_cartridge_t *cartridge, WORD addr) {
    /**
     * ┌────────────────────────────┬─────────────────────────────────────────┐
     * │                            │              ROM address bits           │
     * │     Accessed address       │    Bank number    │ Address within bank │
     * ├────────────────────────────┼───────────────────┼─────────────────────┤
     * │                            │  20─19  │  18─14  │       13─0          │
     * ├────────────────────────────┼─────────┼─────────┼─────────────────────┤
     * │ 0x0000─0x3FFF, MODE = 0b0  │  0b00   │ 0b00000 │     A<13:0>         │
     * ├────────────────────────────┼─────────┼─────────┼─────────────────────┤
     * │ 0x0000─0x3FFF, MODE = 0b1  │  BANK2  │ 0b00000 │     A<13:0>         │
     * ├────────────────────────────┼─────────┼─────────┼─────────────────────┤
     * │        0x4000─0x7FFF       │  BANK2  │ BANK1   │     A<13:0>         │
     * └────────────────────────────┴───────────────────┴─────────────────────┘
     */

    /**
     * ┌────────────────────────────┬─────────────────────────────────────────┐
     * │                            │              ROM address bits           │
     * │     Accessed address       │    Bank number    │ Address within bank │
     * ├────────────────────────────┼───────────────────┼─────────────────────┤
     * │                            │       14-13       │       12─0          │
     * ├────────────────────────────┼───────────────────┼─────────────────────┤
     * │ 0x0000─0x3FFF, MODE = 0b0  │        0b00       │     A<12:0>         │
     * ├────────────────────────────┼───────────────────┼─────────────────────┤
     * │ 0x0000─0x3FFF, MODE = 0b1  │       BANK2       │     A<12:0>         │
     * └────────────────────────────┴───────────────────┴─────────────────────┘
     */
    
    uint64_t phys_addr = addr;

    if (addr < 0x4000) {                                                // ROM Bank 00
        if (BANKING_MODE) { // MODE1 uses Bank2 only
            phys_addr = (BANK2_NUMBER << 14) | (addr & 0x3FFF);
        }

        CHECK_BOUNDERIES(0, cartridge->data_size, "READ ROM", 0xFF);

        return cartridge->data[phys_addr];
    } else if (addr < 0x8000) {                                         // ROM Bank 01-7F
        phys_addr = (BANK_NUMBER << 14) | ( addr & 0x3FFF );

        CHECK_BOUNDERIES(0, cartridge->data_size, "READ ROM", 0xFF);
        //printf("MBC1 READ (BANK%d) %04X => %llX\n", BANK_NUMBER, addr, phys_addr);

        return cartridge->data[phys_addr];
    } else if (addr > 0x9FFF && addr < 0xC000) {         // RAM Bank 00-03
        if (!RAM_ENABLED || !RAM_SIZE(cartridge->header)) {
            fprintf(stderr, "NO RAM\n");
            return 0xFF;
        }
        
        if (BANKING_MODE && RAM_SIZE(cartridge->header) > 8*1024) {
            phys_addr = (BANK2_NUMBER << 8) | (addr & 0x1FFF);
        }

        CHECK_BOUNDERIES(0, cartridge->data_size, "READ RAM BANK", 0xFF);

        return cartridge->data[phys_addr];
    }

    return 0xFF;
}

void GB_mbc1_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    uint64_t phys_addr = addr;

    if (addr < 0x1FFF) {                                                    // RAM Enable
        cartridge->mbc->ram_enabled = data == 0x0A;
    } else if (addr < 0x4000) {                                             // ROM Bank Number
        data &= 0x1F;
        cartridge->mbc->bank_number = BANK2_NUMBER | (data ? data : 1);
        //printf("BANK1 %X (%X)\n", BANK_NUMBER, data);
    } else if (addr < 0x6000) {                                             // RAM Bank Number
        data = (data & 0x3) << 5;
        cartridge->mbc->bank_number = data | BANK1_NUMBER;
    } else if (addr < 0x8000) {
        cartridge->mbc->banking_mode = (data != 0);
    }

    if (addr > 0x9FFF && addr < 0xC000) {         // RAM Bank 00-03
        if (!RAM_ENABLED || !RAM_SIZE(cartridge->header)) {
            return;
        }
        
        if (BANKING_MODE && RAM_SIZE(cartridge->header) > 8*1024) {
            phys_addr = (BANK2_NUMBER << 8) | (addr & 0x1FFF);
        }

        CHECK_BOUNDERIES(0, cartridge->data_size, "MBC1 WRITE RAM BANK", );

        cartridge->data[phys_addr] = data;
    }
}