#include "cartridge/mbc.h"
#include "defs.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define ROM_BANK_MASK       (   mbc->rom_bank_count - 1 )
#define RAM_BANK_MASK       ( ( mbc->ram_bank_count - 1) & 0xF )
#define ROM_BANK_NUMBER     ( mbc->rom_bank_number & ROM_BANK_MASK )
#define RAM_BANK_NUMBER     ( mbc->ram_bank_number & RAM_BANK_MASK )

#define RAM_ENABLED     ( mbc->ram_bank_count && mbc->ram_enabled )
#define BANKING_MODE    ( mbc->banking_mode )

#define _rom            ( mbc->rom )
#define _ram            ( mbc->ram )
#define _rom_size       ( mbc->rom_size )
#define _ram_size       ( mbc->ram_size )

#define CHECK_BOUNDERIES(a, b, msg, rv) do {                                                                    \
    if (phys_addr < a || phys_addr >= b) {                                                                      \
        fprintf(stderr, "OUT OF RANGE %s BANK%d ($%04X => $%llX)\n", msg, ROM_BANK_NUMBER, addr, (uint64_t)phys_addr);    \
        return rv;                                                                                              \
    }                                                                                                           \
} while(0)

typedef BYTE (*mbc_read_callback)(GB_mbc_t *mbc, WORD addr);
typedef void (*mbc_write_callback)(GB_mbc_t *mbc, WORD addr, BYTE data);

struct GB_mbc_s {
    WORD                rom_bank_number;
    WORD                ram_bank_number;
    int                 ram_enabled;
    int                 banking_mode;

    int                 rom_bank_count;
    int                 ram_bank_count;

	BYTE                *rom;
	BYTE                *ram;
	size_t              rom_size;
	size_t              ram_size;

    mbc_read_callback   read_callback;
    mbc_write_callback  write_callback;
};

BYTE GB_mbc0_read(GB_mbc_t *mbc, WORD addr) {
    if (addr > 0x9FFF && addr < 0xC000) {
        if (!_ram_size) return 0x00;
        addr -= 0x2000;
    }

    //CHECK_BOUNDERIES(0, data_size, "MBC0 READ ROM BANK0", 0xFF);
    return _rom[addr];
}

void GB_mbc0_write(GB_mbc_t *mbc, WORD addr, BYTE data) {
    if (addr < 0x8000) {
        //CHECK_BOUNDERIES(0, data_size, "MBC0 WRITE ROM BANK0", );
        _rom[addr] = data;
    } else if (addr > 0x9FFF && addr < 0xC000 && _ram_size) {
        addr -= 0x2000;

        //CHECK_BOUNDERIES(0, data_size, "MBC0 WRITE RAM BANK0", );
        _rom[addr] = data;
    }
}

BYTE GB_mbc1_read(GB_mbc_t *mbc, WORD addr) {
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
        phys_addr += 0x4000 * (((mbc->ram_bank_number << 5) & BANKING_MODE) & ROM_BANK_MASK);
        CHECK_BOUNDERIES(0, _rom_size, "READ ROM", 0xFF);

        return _rom[phys_addr];
    } else if (addr < 0x8000) {                                         // ROM Bank 01-7F
        phys_addr = 0x4000 * (((mbc->ram_bank_number << 5) | ROM_BANK_NUMBER) & ROM_BANK_MASK) + (addr - 0x4000);

        CHECK_BOUNDERIES(0, _rom_size, "READ ROM", 0xFF);

        return _rom[phys_addr];
    } else if (addr > 0x9FFF && addr < 0xC000 && RAM_ENABLED) {         // RAM Bank 00-03
        phys_addr &= 0x1fff; // Use gameboy address bits 0-12

        if (BANKING_MODE && _ram_size > 8*1024) {
            phys_addr |= (RAM_BANK_NUMBER << 13);
        }

        CHECK_BOUNDERIES(0, _ram_size, "READ RAM BANK", 0xFF);

        return _ram[phys_addr];
    }

    return 0xFF;
}

void GB_mbc1_write(GB_mbc_t *mbc, WORD addr, BYTE data) {
    uint64_t phys_addr = addr;

    if (addr < 0x2000) {                                                    // RAM Enable
        mbc->ram_enabled = ( (data&0xF) == 0xA );
    } else if (addr < 0x4000) {                                             // ROM Bank Number
        data &= 0x1F;
        mbc->rom_bank_number = (data ? data : 1);
    } else if (addr < 0x6000) {                                             // RAM Bank Number
        mbc->ram_bank_number = (data & 3); 
    } else if (addr < 0x8000) {
        mbc->banking_mode = (data&1) ? ~((int)0) : 0;
    }

    if (addr > 0x9FFF && addr < 0xC000 && RAM_ENABLED) {                                   // RAM Bank 00-03
        phys_addr &= 0x1fff;
        
        if (BANKING_MODE && _ram_size > 8*1024) {
            phys_addr |= (RAM_BANK_NUMBER << 13);
        }

        CHECK_BOUNDERIES(0, _ram_size, "MBC1 WRITE RAM BANK", );

        _ram[phys_addr] = data;
    }
}

BYTE GB_mbc2_read(GB_mbc_t *mbc, WORD addr) {
    if (addr < 0x4000) {
        return _rom[addr];
    } else if (addr < 0x8000) {
        return _rom[0x4000 * ROM_BANK_NUMBER + (addr - 0x4000)];
    } else if (addr > 0x9FFF && addr < 0xC000 && RAM_ENABLED) {
        return _ram[addr&0x1ff];
    }

    return 0xFF;
}

void GB_mbc2_write(GB_mbc_t *mbc, WORD addr, BYTE data) {
    if (addr < 0x4000) {
        if ( addr&0x100 ) { // bit 8 set = ROM bank select
            mbc->rom_bank_number = (data&0xF) ? (data&0xF) : 1;
        } else {
            mbc->ram_enabled = ( (data&0xF) == 0xA );
        }
    }

    else if (addr >= 0xA000 && addr < 0xC000 && RAM_ENABLED) {
        _ram[addr&0x1ff] = 0xF0 | (data&0xF);
    }
}

BYTE GB_mbc5_read(GB_mbc_t *mbc, WORD addr) {
    if (addr < 0x4000) {
        return _rom[addr];
    } else if (addr < 0x8000) {
        return _rom[0x4000 * ROM_BANK_NUMBER + (addr - 0x4000)];
    } else if (addr > 0x9FFF && addr < 0xC000 && RAM_ENABLED) {
        return _ram[0x2000 * mbc->ram_bank_number + (addr - 0xA000)];
    }

    return 0xFF;
}

void GB_mbc5_write(GB_mbc_t *mbc, WORD addr, BYTE data) {
    if (addr < 0x2000) {
        mbc->ram_enabled = ( (data&0xF) == 0xA );
    } else if (addr < 0x3000) {
        mbc->rom_bank_number = (ROM_BANK_NUMBER & 0x100) | data;
    } else if (addr < 0x4000) {
        mbc->rom_bank_number = ((data & 1) << 8) | (ROM_BANK_NUMBER & 0x0FF);
    } else if (addr < 0x6000) {
        mbc->ram_bank_number = ((data & 0x0F) & RAM_BANK_MASK);
    }

    else if (addr >= 0xA000 && addr < 0xC000 && RAM_ENABLED) {
        _ram[0x2000 * mbc->ram_bank_number + (addr - 0xA000)] = data;
    }
}

BYTE GB_mbc_read(GB_mbc_t *mbc, WORD addr) {
    return mbc->read_callback(mbc, addr);
}

void GB_mbc_write(GB_mbc_t *mbc, WORD addr, BYTE data) {
    mbc->write_callback(mbc, addr, data);
}

/*=================== INIT ===================*/

int load_rom(GB_mbc_t *mbc, FILE *fp, long file_size) {
    rewind(fp);

    mbc->rom_size = file_size; 
	mbc->rom = (BYTE*)( malloc( sizeof (BYTE) * mbc->rom_size + 1 ) );
	if (!mbc->rom) {
		return 1;
	}

    int rv = fread((void*)(mbc->rom), sizeof(BYTE), mbc->rom_size, fp);
	if (rv != mbc->rom_size) {
		return 2;
	}

    return 0;
}

#define FAIL_IF(cond) \
    if (cond) { GB_mbc_destroy(mbc); return NULL; }

#define SET_RAM_BANK_COUNT() do {                                                               \
    switch (header->ram_type) {                                                                 \
        case 0: mbc->ram_bank_count = 0;  break;                                                \
        case 2: mbc->ram_bank_count = 1;  break;                                                \
        case 3: mbc->ram_bank_count = 4;  break;                                                \
        case 4: mbc->ram_bank_count = 16; break;                                                \
        case 5: mbc->ram_bank_count = 8;  break;                                                \
        default:                                                                                \
            fprintf(stderr, "Unknown or unsupported RAM type: $%02X\n", header->ram_type);      \
            FAIL_IF(1);                                                                         \
    }                                                                                           \
} while (0)

#define SET_MBC_CALLBACKS(n) 							                                        \
	mbc->read_callback 	= GB_mbc##n##_read;		                                                \
	mbc->write_callback = GB_mbc##n##_write;

#define SETUP_RW() do {                                                                         \
	switch(header->cartridge_type) {                                                            \
		case 0: SET_MBC_CALLBACKS(0); break;                                                    \
		case 1: SET_MBC_CALLBACKS(1); break;                                                    \
		case 2: SET_MBC_CALLBACKS(1); break;                                                    \
		case 3: SET_MBC_CALLBACKS(1); break;                                                    \
		case 5: SET_MBC_CALLBACKS(2); break;                                                    \
		case 6: SET_MBC_CALLBACKS(2); break;                                                    \
		case 8: SET_MBC_CALLBACKS(0); break;                                                    \
		case 9: SET_MBC_CALLBACKS(0); break;                                                    \
        case 0x19: SET_MBC_CALLBACKS(5); break;                                                 \
        case 0x1A: SET_MBC_CALLBACKS(5); break;                                                 \
        case 0x1B: SET_MBC_CALLBACKS(5); break;                                                 \
        case 0x1C: SET_MBC_CALLBACKS(5); break;                                                 \
        case 0x1D: SET_MBC_CALLBACKS(5); break;                                                 \
        case 0x1E: SET_MBC_CALLBACKS(5); break;                                                 \
		default:                                                                                \
			fprintf(stderr, "UNSUPPORTED CARTRIDGE %d\n", header->cartridge_type);              \
            FAIL_IF(1)                                                                          \
	}                                                                                           \
} while (0)


GB_mbc_t* GB_mbc_create(GB_header_t *header, FILE *rom_fp, long file_size) {
    GB_mbc_t *mbc = NULL;

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
    mbc->rom                = NULL;
    mbc->rom_size           = 0;
    mbc->ram                = NULL;
    mbc->ram_size           = 0;

    FAIL_IF(load_rom(mbc, rom_fp, file_size)) 

    if (header->cartridge_type == 5 || header->cartridge_type == 6) {
        mbc->ram_bank_count = 1;
    } else {
        SET_RAM_BANK_COUNT();
    }

	mbc->ram_size = 8192UL * mbc->ram_bank_count;
	if (mbc->ram_size) {
		mbc->ram = (BYTE*)( malloc( sizeof (BYTE) * mbc->ram_size + 1 ) );
		FAIL_IF(!mbc->ram)
	}

    SETUP_RW();

    return mbc;
}

void GB_mbc_destroy(GB_mbc_t *mbc) {
    if (!mbc) return;

	if (mbc->ram) free(mbc->ram);
	if (mbc->rom) free(mbc->rom);

    mbc->ram = NULL;
    mbc->rom = NULL;
	
    free(mbc);
}


