#include "cartridge/cartridge.h"
#include "cartridge/mbc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE_IN_BYTES (0x0150 - 0x0134)

static const ssize_t RAM_SIZES[] = { 0, 0, 8, 32, 128, 64 };

#define SET_MBC_CALLBACKS(n) 							\
	cartridge->read_callback 	= GB_mbc##n##_read;		\
	cartridge->write_callback 	= GB_mbc##n##_write;

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
			"\tROM SIZE: $%02X (%lu)\n" 	\
			"\tRAM SIZE: $%02X (%lu)\n"		\
			"\tDESTINATION CODE: $%02X\n" 	\
			"\tOLD LICENSEE CODE: $%02X\n" 	\
			"\tROM VERSION: $%02X\n" 		\
			"\tHEADER CHECKSUM: $%02X\n" 	\
			"\tGLOBAL CHECKSUM: $%04X\n", 	\
			header->title,
			header->new_licensee_code, 
			header->cartridge_type,
			header->rom_size, ROM_SIZE(header),
			header->ram_size, RAM_SIZE(header),
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

	cartridge->rom 		= NULL;
	cartridge->ram 		= NULL;
	cartridge->rom_size = 0;
	cartridge->ram_size = 0;
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

	cartridge->rom_size = fsize;
	cartridge->rom = (BYTE*)( malloc( sizeof (BYTE) * fsize + 1 ) );
	if (!cartridge->rom) {
    	fclose(fp);
		free(cartridge);
		return NULL;
	}

    int rv = fread((void*)(cartridge->rom), sizeof(BYTE), fsize, fp);
	if (rv != fsize) {
		fclose(fp);
		free(cartridge);
		return NULL;
	}

	cartridge->header = GB_header_create(fp, fsize);
    fclose(fp);

	if (!cartridge->header) {
		GB_cartridge_destroy(cartridge);
		return NULL;
	}

	cartridge->ram_size = RAM_SIZE(cartridge->header);

	if (cartridge->ram_size) {
		cartridge->ram = (BYTE*)( malloc( sizeof (BYTE) * cartridge->ram_size + 1 ) );
		if (!cartridge->ram) {
			GB_cartridge_destroy(cartridge);
			return NULL;
		}
	}

	cartridge->mbc = GB_MBC_create();
	if (!cartridge->mbc) {
		GB_cartridge_destroy(cartridge);
		return NULL;
	}

	switch(cartridge->header->cartridge_type) {
		case 0: SET_MBC_CALLBACKS(0); break;
		case 1: SET_MBC_CALLBACKS(1); break;
		case 2: SET_MBC_CALLBACKS(1); break;
		case 3: SET_MBC_CALLBACKS(1); break;
        case 0x19: SET_MBC_CALLBACKS(5); break;
        case 0x1A: SET_MBC_CALLBACKS(5); break;
        case 0x1B: SET_MBC_CALLBACKS(5); break;
        case 0x1C: SET_MBC_CALLBACKS(5); break;
        case 0x1D: SET_MBC_CALLBACKS(5); break;
        case 0x1E: SET_MBC_CALLBACKS(5); break;
		default: 
			fprintf(stderr, "UNSUPPORTED CARTRIDGE %d\n", cartridge->header->cartridge_type);
			GB_cartridge_destroy(cartridge);
			return NULL;
	}

    return cartridge;
}

void GB_cartridge_destroy(GB_cartridge_t *cartridge) {
	if (!cartridge) return;
	if (cartridge->ram) free(cartridge->ram);
	if (cartridge->rom) free(cartridge->rom);
	GB_MBC_destroy(cartridge->mbc);
	GB_header_destroy(cartridge->header);

	free(cartridge);
}
