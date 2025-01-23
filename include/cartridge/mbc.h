#ifndef GB_MBCS_H_
#define GB_MBCS_H_

#include "defs.h"
#include "cartridge/cartridge.h"

#include <stdio.h>

GB_mbc_t*   GB_mbc_create(GB_header_t *header, FILE *rom_fp, long file_size);
void        GB_mbc_destroy(GB_mbc_t *mbc);

BYTE        GB_mbc_read(GB_mbc_t *mbc, WORD addr);
void        GB_mbc_write(GB_mbc_t *mbc, WORD addr, BYTE data);

#endif
