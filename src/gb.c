#include "gb.h"
#include "cpudef.h"
#include "mmu.h"

#include <stdlib.h>

#define WRAM_SIZE       (0x2000)
#define UNUSABLE_SIZE   (0x006F)
#define IO_REGS_SIZE    (0x0080)
#define HRAM_SIZE       (0x007F)

#define CHECK_ALLOC(var) if (!var) { GB_gameboy_destroy(gb); return NULL;}
#define ALLOC_BYTE_ARRAY(elt_cnt) ( (BYTE*)( malloc( sizeof (BYTE) * elt_cnt + 1 ) ) )

void gameboy_init(GB_gameboy_t *gb);

GB_gameboy_t*   GB_gameboy_create(const char *rom_path, int headless) {
    GB_gameboy_t *gb = (GB_gameboy_t*)( malloc( sizeof (GB_gameboy_t) ) );
    CHECK_ALLOC(gb);

    gb->cartridge   = NULL;
    gb->cpu         = NULL;
    gb->ppu         = NULL;
    gb->mmu         = NULL;
    gb->wram        = NULL;
    gb->unusable    = NULL;
    gb->io_regs     = NULL;
    gb->hram        = NULL;
    gb->ie          = 0;

    gb->cartridge = GB_cartridge_create(rom_path);
    CHECK_ALLOC(gb->cartridge);

    gb->cpu = GB_cpu_create();
    CHECK_ALLOC(gb->cpu);

    gb->ppu = GB_ppu_create(headless);
    CHECK_ALLOC(gb->ppu);

    gb->mmu = GB_mmu_create();
    CHECK_ALLOC(gb->mmu);

    gb->wram = ALLOC_BYTE_ARRAY(WRAM_SIZE);
    CHECK_ALLOC(gb->wram); 

    gb->unusable = ALLOC_BYTE_ARRAY(UNUSABLE_SIZE);
    CHECK_ALLOC(gb->unusable);

    gb->io_regs = ALLOC_BYTE_ARRAY(IO_REGS_SIZE);
    CHECK_ALLOC(gb->io_regs);

    gb->hram = ALLOC_BYTE_ARRAY(HRAM_SIZE);
    CHECK_ALLOC(gb->hram);

    gameboy_init(gb);

    // Temp fix
    gb->io_regs[0x44] = 0x91;
    gb->io_regs[0x40] = 0x91;

    return gb;
}

void GB_gameboy_destroy(GB_gameboy_t *gb) {
    if (!gb) return;

    if (gb->hram)       free(gb->hram);
    if (gb->io_regs)    free(gb->io_regs);
    if (gb->unusable)   free(gb->unusable);
    if (gb->wram)       free(gb->wram);
    GB_mmu_destroy(gb->mmu);
    GB_ppu_destroy(gb->ppu);
    GB_cpu_destroy(gb->cpu);
    GB_cartridge_destroy(gb->cartridge);

    free(gb);
}

const static int DMG_INIT[] = {

    /* HWIO */
    0xCF, 0x00, 0x7E, 0xFF, 0xAD, 0x00, 0x00, 0xF8, // 0xFF00
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, // 0xFF08
    0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, // 0xFF10
    0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF, // 0xFF18
    0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, // 0xFF20
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF28
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF30
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF38
    0x91, 0x80, 0x00, 0x00, 0x0A, 0x00, 0xFF, 0xFC, // 0xFF40
    0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF48
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF50
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF58
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF60
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF68
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF70
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // 0xFF78
};

void gameboy_init(GB_gameboy_t *gb) {
  rA = 0x01;
	rF = 0x80 | (gb->cartridge->header->header_checksum ? 0x30 : 0x00); // half-carry and carry are set if header checksum not null
	rB = 0x00;
	rC = 0x13;
	rD = 0x00;
	rE = 0xD8;
	rH = 0x01;
	rL = 0x4D;
	PC = 0x100;
	SP = 0xFFFE;

    
    for (int i = 0; i < 0x80; i++) {
        GB_mem_write(gb, (0xFF00 | i), DMG_INIT[i]);
    }

	GB_mem_write(gb, 0xFFFF, 0x00); //IE
	GB_mem_write(gb, 0xFF50, 1); // DISABLE BOOTROM
}
