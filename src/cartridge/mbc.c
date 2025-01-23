#include "cartridge/mbc.h"
#include "cartridge/cartridge.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define ROM_BANK_MASK       (   cartridge->mbc->rom_bank_count - 1 )
#define RAM_BANK_MASK       ( ( cartridge->mbc->ram_bank_count - 1) & 0xF )
#define ROM_BANK_NUMBER     ( cartridge->mbc->rom_bank_number & ROM_BANK_MASK )
#define RAM_BANK_NUMBER     ( cartridge->mbc->ram_bank_number & RAM_BANK_MASK )

#define RAM_ENABLED     ( cartridge->mbc->ram_bank_count && cartridge->mbc->ram_enabled )
#define BANKING_MODE    ( cartridge->mbc->banking_mode )

#define CHECK_BOUNDERIES(a, b, msg, rv) do {                                                                    \
    if (phys_addr < a || phys_addr >= b) {                                                                      \
        fprintf(stderr, "OUT OF RANGE %s BANK%d ($%04X => $%llX)\n", msg, ROM_BANK_NUMBER, addr, (uint64_t)phys_addr);    \
        return rv;                                                                                              \
    }                                                                                                           \
} while(0)

GB_mbc_t* GB_mbc_create(GB_header_t *header) {
    GB_mbc_t *mbc;

    if (header->rom_type > 8) {
        fprintf(stderr, "Unsupported ROM type: $%02X\n", header->rom_type); 
        return NULL;
    }

    mbc = (GB_mbc_t*)( malloc( sizeof (GB_mbc_t) ) );
    if (!mbc) {
        return NULL;
    }

    mbc->rom_bank_number    = 1;
    mbc->ram_bank_number    = 0;
    mbc->banking_mode       = 0;
    mbc->ram_enabled        = 0;
    mbc->rom_bank_count     = (2 << header->rom_type);

    // TODO: mbc2 has 1 ram bank
    switch (header->ram_type) {
        case 0: mbc->ram_bank_count = 0;  break;
        case 2: mbc->ram_bank_count = 1;  break;
        case 3: mbc->ram_bank_count = 4;  break;
        case 4: mbc->ram_bank_count = 16; break;
        case 5: mbc->ram_bank_count = 8;  break;
        default:
            fprintf(stderr, "Unknown or unsupported RAM type: $%02X\n", header->ram_type);
            GB_mbc_destroy(mbc);
            return NULL;
    }


    // TMP SOLUTION
    if (header->cartridge_type == 5 || header->cartridge_type == 6) {
        mbc->ram_bank_count = 1;
    }

    return mbc;
}

void GB_mbc_destroy(GB_mbc_t *mbc) {
    if (mbc) free(mbc);
}

BYTE GB_mbc0_read(GB_cartridge_t *cartridge, WORD addr) {
    if (addr > 0x9FFF && addr < 0xC000) {
        if (!cartridge->ram_size) return 0x00;
        addr -= 0x2000;
    }

    //CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 READ ROM BANK0", 0xFF);
    return cartridge->rom[addr];
}

void GB_mbc0_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    if (addr < 0x8000) {
        //CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 WRITE ROM BANK0", );
        cartridge->rom[addr] = data;
    } else if (addr > 0x9FFF && addr < 0xC000 && cartridge->ram_size) {
        addr -= 0x2000;

        //CHECK_BOUNDERIES(0, cartridge->data_size, "MBC0 WRITE RAM BANK0", );
        cartridge->rom[addr] = data;
    }
}

BYTE GB_mbc1_read(GB_cartridge_t *cartridge, WORD addr) {
    /**
     * ┌────────────────────────────┬─────────────────────────────────────────┐
     * │                            │              ROM address bits           │
     * │     Accessed address       │    Bank number    │ Address within bank │
     * ├────────────────────────────┼─────────┬─────────┼─────────────────────┤
     * │                            │  20─19  │  18─14  │       13─0          │
     * ├────────────────────────────┼─────────┼─────────┼─────────────────────┤
     * │ 0x0000─0x3FFF, MODE = 0b0  │  0b00   │ 0b00000 │     A<13:0>         │
     * ├────────────────────────────┼─────────┼─────────┼─────────────────────┤
     * │ 0x0000─0x3FFF, MODE = 0b1  │  BANK2  │ 0b00000 │     A<13:0>         │
     * ├────────────────────────────┼─────────┼─────────┼─────────────────────┤
     * │        0x4000─0x7FFF       │  BANK2  │ BANK1   │     A<13:0>         │
     * └────────────────────────────┴─────────┴─────────┴─────────────────────┘
     */

    /**
     * ┌────────────────────────────┬─────────────────────────────────────────┐
     * │                            │              RAM address bits           │
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
        phys_addr += 0x4000 * (((cartridge->mbc->ram_bank_number << 5) & BANKING_MODE) & ROM_BANK_MASK);
        CHECK_BOUNDERIES(0, cartridge->rom_size, "READ ROM", 0xFF);

        return cartridge->rom[phys_addr];
    } else if (addr < 0x8000) {                                         // ROM Bank 01-7F
        phys_addr = 0x4000 * (((cartridge->mbc->ram_bank_number << 5) | ROM_BANK_NUMBER) & ROM_BANK_MASK) + (addr - 0x4000);

        CHECK_BOUNDERIES(0, cartridge->rom_size, "READ ROM", 0xFF);

        return cartridge->rom[phys_addr];
    } else if (addr > 0x9FFF && addr < 0xC000 && RAM_ENABLED) {         // RAM Bank 00-03
        phys_addr &= 0x1fff; // Use gameboy address bits 0-12

        if (BANKING_MODE && cartridge->ram_size > 8*1024) {
            phys_addr |= (RAM_BANK_NUMBER << 13);
        }

        CHECK_BOUNDERIES(0, cartridge->ram_size, "READ RAM BANK", 0xFF);

        return cartridge->ram[phys_addr];
    }

    return 0xFF;
}

void GB_mbc1_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    uint64_t phys_addr = addr;

    if (addr < 0x2000) {                                                    // RAM Enable
        cartridge->mbc->ram_enabled = ( (data&0xF) == 0xA );
    } else if (addr < 0x4000) {                                             // ROM Bank Number
        data &= 0x1F;
        cartridge->mbc->rom_bank_number = (data ? data : 1);
    } else if (addr < 0x6000) {                                             // RAM Bank Number
        cartridge->mbc->ram_bank_number = (data & 3); 
    } else if (addr < 0x8000) {
        cartridge->mbc->banking_mode = (data&1) ? ~((int)0) : 0;
    }

    if (addr > 0x9FFF && addr < 0xC000 && RAM_ENABLED) {                                   // RAM Bank 00-03
        phys_addr &= 0x1fff;
        
        if (BANKING_MODE && cartridge->ram_size > 8*1024) {
            phys_addr |= (RAM_BANK_NUMBER << 13);
        }

        CHECK_BOUNDERIES(0, cartridge->ram_size, "MBC1 WRITE RAM BANK", );

        cartridge->ram[phys_addr] = data;
    }
}

BYTE GB_mbc2_read(GB_cartridge_t *cartridge, WORD addr) {
    if (addr < 0x4000) {
        return cartridge->rom[addr];
    } else if (addr < 0x8000) {
        return cartridge->rom[0x4000 * ROM_BANK_NUMBER + (addr - 0x4000)];
    } else if (addr > 0x9FFF && addr < 0xC000 && RAM_ENABLED) {
        return cartridge->ram[addr&0x1ff];
    }

    return 0xFF;
}

void GB_mbc2_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    if (addr < 0x4000) {
        if ( addr&0x100 ) { // bit 8 set = ROM bank select
            cartridge->mbc->rom_bank_number = (data&0xF) ? (data&0xF) : 1;
        } else {
            cartridge->mbc->ram_enabled = ( (data&0xF) == 0xA );
        }
    }

    else if (addr >= 0xA000 && addr < 0xC000 && RAM_ENABLED) {
        cartridge->ram[addr&0x1ff] = 0xF0 | (data&0xF);
    }
}

BYTE GB_mbc5_read(GB_cartridge_t *cartridge, WORD addr) {
    if (addr < 0x4000) {
        return cartridge->rom[addr];
    } else if (addr < 0x8000) {
        return cartridge->rom[0x4000 * ROM_BANK_NUMBER + (addr - 0x4000)];
    } else if (addr > 0x9FFF && addr < 0xC000 && RAM_ENABLED) {
        return cartridge->ram[0x2000 * cartridge->mbc->ram_bank_number + (addr - 0xA000)];
    }

    return 0xFF;
}

void GB_mbc5_write(GB_cartridge_t *cartridge, WORD addr, BYTE data) {
    if (addr < 0x2000) {
        cartridge->mbc->ram_enabled = ( (data&0xF) == 0xA );
    } else if (addr < 0x3000) {
        cartridge->mbc->rom_bank_number = (ROM_BANK_NUMBER & 0x100) | data;
    } else if (addr < 0x4000) {
        cartridge->mbc->rom_bank_number = ((data & 1) << 8) | (ROM_BANK_NUMBER & 0x0FF);
    } else if (addr < 0x6000) {
        cartridge->mbc->ram_bank_number = ((data & 0x0F) & RAM_BANK_MASK);
    }

    else if (addr >= 0xA000 && addr < 0xC000 && RAM_ENABLED) {
        cartridge->ram[0x2000 * cartridge->mbc->ram_bank_number + (addr - 0xA000)] = data;
    }
}
