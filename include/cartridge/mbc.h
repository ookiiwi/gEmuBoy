#ifndef GB_MBCS_H_
#define GB_MBCS_H_

#include "type.h"
#include "defs.h"
#include "cartridge/cartridge.h"

#include <stdlib.h>

struct GB_MBC_s {
    WORD bank_number;
    WORD ram_bank_number;
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
MBC_DECL(2);
MBC_DECL(5);


static inline void GB_mbc2_init(GB_cartridge_t *cartridge) {
    cartridge->ram = (BYTE*)malloc( 0x200 * sizeof (BYTE) );
}

#endif
