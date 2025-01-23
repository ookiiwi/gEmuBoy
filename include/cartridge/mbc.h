#ifndef GB_MBCS_H_
#define GB_MBCS_H_

#include "type.h"
#include "defs.h"
#include "cartridge/cartridge.h"

struct GB_mbc_s {
    WORD rom_bank_number;
    WORD ram_bank_number;
    int ram_enabled;
    int banking_mode;

    int rom_bank_count;
    int ram_bank_count;
};

GB_mbc_t*  GB_mbc_create(GB_header_t *header);
void       GB_mbc_destroy(GB_mbc_t *mbc);

#define MBC_DECL(nb)                                                            \
    BYTE GB_mbc##nb##_read(GB_cartridge_t *cartridge, WORD addr);               \
    void GB_mbc##nb##_write(GB_cartridge_t *cartridge, WORD addr, BYTE data);

MBC_DECL(0);
MBC_DECL(1);
MBC_DECL(2);
MBC_DECL(5);

#endif
