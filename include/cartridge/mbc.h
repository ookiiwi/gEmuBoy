#ifndef GB_MBCS_H_
#define GB_MBCS_H_

#include "type.h"
#include "defs.h"
#include "cartridge/cartridge.h"

#define MBC_DECL(nb)                                                            \
    BYTE GB_mbc##nb##_read(GB_cartridge_t *cartridge, WORD addr);               \
    void GB_mbc##nb##_write(GB_cartridge_t *cartridge, WORD addr, BYTE data);

MBC_DECL(0);

#endif