#ifndef CARTRIDGE_H_
#define CARTRIDGE_H_

#include "type.h"
#include "defs.h"

#include <stddef.h> // size_t

typedef struct GB_cartridge_s GB_cartridge_t; 
typedef BYTE (*cartridge_read_callback)(GB_cartridge_t *, WORD);
typedef void (*cartridge_write_callback)(GB_cartridge_t*, WORD, BYTE);

enum GB_CARTRIDGE_ERR {
	GB_CARTRIDGE_ERR_FAIL_OPEN,
	GB_CARTRIDGE_ERR_FILE_TOO_LARGE,
};

typedef struct {
	const char 		title[16]; 				// 0134-0143 (or 0142 if not 0143 is $80 or $C0)
	const char 		new_licensee_code[2]; 	// 0144-0145
	BYTE 			sgb_flag; 				// 0146
	BYTE 			cartridge_type; 		// 0147
	BYTE 			rom_type; 		        // 0148
	BYTE 			ram_type; 	            // 0149 -- 0 if cartridge_type does not include ram in its name
	BYTE 			dst_code; 				// 014A
	BYTE      		old_licensee_code; 		// 014B -- $33 indicates that new licensee code should be used
	BYTE 			rom_version; 			// 014C
	BYTE 			header_checksum; 		// 014D
	WORD 			global_checksum; 		// 014E-014F
} GB_header_t;

struct GB_cartridge_s {
	GB_header_t 				*header;
	GB_mbc_t					*mbc;
};

void 			GB_print_header(GB_header_t *header);

GB_cartridge_t* GB_cartridge_create(const char *path);
void 			GB_cartridge_destroy(GB_cartridge_t *cartridge);

#endif
