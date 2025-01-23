#include "cartridge/cartridge.h"
#include "cartridge/mbc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE_IN_BYTES (0x0150 - 0x0134)

#define ROM_SIZE(header) ( (32 * 1024UL) * ( 1 << header->rom_type ) )

GB_header_t* GB_header_create(FILE *fp, long fsize) {
	GB_header_t *header;

	if (fsize < 0x150) {
		return NULL;
	}

	header = (GB_header_t*)( malloc( sizeof(GB_header_t) ) );
	if (header == NULL) {
		return NULL;
	}
   
	fseek(fp, 0x134, SEEK_SET);

	int rv = fread((void*)header, HEADER_SIZE_IN_BYTES, 1, fp);

	if (rv != 1) {
		fprintf(stderr, "FAIL READ BUFFER\n");

		free(header);
		return NULL;
	}

	if (fsize != ROM_SIZE(header)) {
		fprintf(stderr, "ROM SIZE(%lu) doesn't match file size(%lu)\n", ROM_SIZE(header), fsize);
		free(header);
		return NULL;
	}

	return header;
}

void GB_header_destroy(GB_header_t *header) {
	if (header) free(header);
}

void GB_print_header(GB_header_t *header) { 
	printf( "HEADER\n" 						\
			"\tTITLE: %s\n" 				\
			"\tNEW LICENSEE CODE: %s\n" 	\
			"\tCARTRIDGE TYPE: $%02X\n" 	\
			"\tROM SIZE: $%02X\n" 	\
			"\tRAM SIZE: $%02X\n"		\
			"\tDESTINATION CODE: $%02X\n" 	\
			"\tOLD LICENSEE CODE: $%02X\n" 	\
			"\tROM VERSION: $%02X\n" 		\
			"\tHEADER CHECKSUM: $%02X\n" 	\
			"\tGLOBAL CHECKSUM: $%04X\n", 	\
			header->title,
			header->new_licensee_code, 
			header->cartridge_type,
			header->rom_type,
			header->ram_type,
			header->dst_code,
			header->old_licensee_code,
			header->rom_version,
			header->header_checksum,
			header->global_checksum );
}

GB_cartridge_t* GB_cartridge_create(const char *path) {
	GB_cartridge_t *cartridge;
    FILE *fp;
	long fsize;

	cartridge = (GB_cartridge_t*)( malloc( sizeof (GB_cartridge_t) ) );
	if (!cartridge) { return NULL; }

	cartridge->header 	= NULL;
	cartridge->mbc 		= NULL;
	
	fp = fopen(path, "rb");
    if (!fp) { 
		GB_cartridge_destroy(cartridge);
		return NULL; 
	}

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

	cartridge->header = GB_header_create(fp, fsize);

	if (!cartridge->header) {
        fclose(fp);
		GB_cartridge_destroy(cartridge);
		return NULL;
	}

	cartridge->mbc = GB_mbc_create(cartridge->header, fp, fsize);
	if (!cartridge->mbc) {
        fclose(fp);
		GB_cartridge_destroy(cartridge);
		return NULL;
	}
    
    fclose(fp);

    return cartridge;
}

void GB_cartridge_destroy(GB_cartridge_t *cartridge) {
	if (!cartridge) return;
    GB_mbc_destroy(cartridge->mbc);
	GB_header_destroy(cartridge->header);

	free(cartridge);
}
