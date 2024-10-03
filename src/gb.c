#include "gb.h"
#include "cpudef.h"
#include "mmu.h"

#include <stdlib.h>

#define WRAM_SIZE       (0x2000)
#define UNUSABLE_SIZE   (0x006F)
#define IO_REGS_SIZE    (0x0080)
#define HRAM_SIZE       (0x007F)

#define CHECK_ALLOC(var) if (!var) { GB_gameboy_destroy(gb); return NULL;}
#define ALLOC_BYTE_ARRAY(elt_cnt) ( (BYTE*)( malloc( sizeof (BYTE) * elt_cnt ) ) )

void gameboy_init(GB_gameboy_t *gb);

GB_gameboy_t*   GB_gameboy_create(const char *rom_path) {
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

    gb->ppu = GB_ppu_create();
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
}

void gameboy_init(GB_gameboy_t *gb) {
    PC = 0x100;
}
