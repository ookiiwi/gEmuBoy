#ifndef GB_MBCS_H_
#define GB_MBCS_H_

#include "type.h"
#include "defs.h"
#include "cartridge/cartridge.h"

struct GB_MBC_s {
    BYTE bank_number;
    int ram_enabled;
    int banking_mode;
};

GB_MBC_t*  GB_MBC_create();
void       GB_MBC_destroy(GB_MBC_t *mbc);

#define MBC_DECL(nb)                                                            \
    BYTE GB_mbc##nb##_read(GB_cartridge_t *cartridge, WORD addr);               \
    void GB_mbc##nb##_write(GB_cartridge_t *cartridge, WORD addr, BYTE data);

MBC_DECL(0);
MBC_DECL(1);

#endif