#include "gb.h"
#include "load.h"

GB_gameboy_t* GB_create(const char *src_rom_path) {
    GB_gameboy_t *gb = (GB_gameboy_t*)( malloc( sizeof(GB_gameboy_t) ) );
    gb->cpu     = NULL;
    gb->ppu     = NULL;
    gb->memory  = NULL;
    gb->rom     = NULL;

    if (gb == NULL) {
        return NULL;
    }

    gb->cpu     = cpu_create();
    gb->ppu     = ppu_create();
    gb->memory  = (BYTE*)( calloc( 0xFFFF, sizeof(BYTE) ) );

    if (gb->memory == NULL) {
        return NULL;
    }

    int rv = loadrom(src_rom_path, &gb->rom, &gb->rom_size);     

    if (rv != gb->rom_size) {                                               
        char *errmsg = "FAILED READING PROVIDED ROM";                            
        switch(rv) {                                                             
            case LOADROM_FAIL_OPEN:     errmsg = "FAILED OPENING ROM";           
            case LOADROM_FAIL_ALLOC:    errmsg = "FAILED ALLOCATING ROM MEMORY"; 
        }                                                                        
        fprintf(stderr, "%s: %s\n", errmsg, src_rom_path);

        GB_destroy(gb);
        return NULL;                                                   
    }

    ppu_load_sample(gb);

#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
    CPU_GAMEBOY_DOCTOR_SETUP(src_rom_path);
#endif  

    return gb;
}

void GB_destroy(GB_gameboy_t *gb) {
#ifdef ENABLE_GAMEBOY_DOCTOR_SETUP
    CPU_GAMEBOY_DOCTOR_CLEANUP();
#endif

    if (gb == NULL) return;

    ppu_destroy(gb->ppu);
    cpu_destroy(gb->cpu);

    if (gb->rom)    free(gb->rom);
    if (gb->memory) free(gb->memory);
    
    free(gb);
}