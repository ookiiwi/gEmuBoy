#include "cartridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE_IN_BYTES (0x0150 - 0x0134)

static const ssize_t RAM_SIZES[] = { 0, 0, 8, 32, 128, 64 };

GB_header_t* GB_header_create(const char *path) {
	GB_header_t 	*header;
	FILE 			*fp;
	long 			fsize;

	header = (GB_header_t*)( malloc( sizeof(GB_header_t) ) );
	if (header == NULL) {
		return NULL;
	}

	fp = fopen(path, "rb");
	if (fp == NULL) {
		return NULL;
	}
   
	fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
	fseek(fp, 0x134, SEEK_SET);

	int rv = fread((void*)header, HEADER_SIZE_IN_BYTES, 1, fp);

	fclose(fp);

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
			"\tROM SIZE: $%02X (%lu)\n" 	\
			"\tRAM SIZE: $%02X\n" 			\
			"\tDESTINATION CODE: $%02X\n" 	\
			"\tOLD LICENSEE CODE: $%02X\n" 	\
			"\tROM VERSION: $%02X\n" 		\
			"\tHEADER CHECKSUM: $%02X\n" 	\
			"\tGLOBAL CHECKSUM: $%04X\n", 	\
			header->title,
			header->new_licensee_code, 
			header->cartridge_type,
			header->rom_size, ROM_SIZE(header),
			header->ram_size,
			header->dst_code,
			header->old_licensee_code,
			header->rom_version,
			header->header_checksum,
			header->global_checksum );
}


int GB_cartridge_read_rom(const char *path, void **dst, size_t dst_size) {
    FILE *p = fopen(path, "rb");
    if (p == NULL) {
        return GB_CARTRIDGE_ERR_FAIL_OPEN;
    }

    fseek(p, 0, SEEK_END);
    long fsize = ftell(p);
    fseek(p, 0, SEEK_SET);

	if (fsize > dst_size) {
		return GB_CARTRIDGE_ERR_FILE_TOO_LARGE;
	}


    int rv = fread((void*)(*dst), sizeof(BYTE), fsize, p);
    fclose(p);

    return rv;

}
