#include "mmu.h"
#include "gb.h"
#include "graphics/ppu.h"
#include "memmap.h"

#include <stdio.h>
#include <stdlib.h>

#define VRAM_START_ADDR     (0x8000)
#define EXT_RAM_START_ADDR  (0xA000)
#define WRAM_START_ADDR     (0xC000)
#define ECHO_RAM_START_ADDR (0xE000)
#define OAM_START_ADDR      (0xFE00)
#define UNUSABLE_START_ADDR (0xFEA0)
#define IO_REGS_START_ADDR  (0xFF00)
#define HRAM_START_ADDR     (0xFF80)
#define IE_START_ADDR       (0xFFFF)

#define ADJUST_ADDR(min, max) do {                                                          \
    if ( addr < min || addr >= max) {                                                       \
        fprintf(stderr, "OUT OF RANGE $%04X NOT IN RANGE [$%04X-$%04X]\n", addr, min, max); \
        addr = max-1;                                                                       \
    }                                                                                       \
    addr -= min;                                                                            \
} while(0)

enum DMA_STATE {
	DMA_STOP,
	DMA_START_M1,	
	DMA_START_M2,
	DMA_INIT,
	DMA_RUNNING
};

#define DMA_RESTART_CNTDOWN_DEFAULT (3) // Waits 2 M-cycle and restart on the third one

#define OAM_START_ADDR 	(0xFE00)
#define OAM_END_ADDR 	(0xFE9F)
#define DMA_SOURCE_ADDR (0xFF46)

struct GB_mmu_s {
	int dma_state;
	int dma_offset;
	int is_dma_active; // Whether dma is running regardless of its state
	int dma_restart_cntdown;
	WORD dma_source;

	WORD addr_bus; // Used for DMA conflict
	BYTE data_bus;
};


BYTE mem_read(GB_gameboy_t *gb, WORD addr) {
    if (addr < VRAM_START_ADDR) {           // ROM bank     -- 0000-7FFF
        return gb->cartridge->read_callback(gb->cartridge, addr);
    } 
    
    else if (addr < EXT_RAM_START_ADDR) {   // VRAM         -- 8000-9FFF
        return GB_ppu_vram_read(gb->ppu, addr);
    } 
    
    else if (addr < WRAM_START_ADDR) {      // RAM bank     -- A000-BFFF
        return gb->cartridge->read_callback(gb->cartridge, addr);
    } 
    
    else if (addr < ECHO_RAM_START_ADDR) {  // WRAM         -- C000-DFFF
        ADJUST_ADDR(WRAM_START_ADDR, ECHO_RAM_START_ADDR);
        return gb->wram[addr];
    } 
    
    else if (addr < OAM_START_ADDR) {       // Echo wram    -- E000-FDFF
        ADJUST_ADDR(ECHO_RAM_START_ADDR, OAM_START_ADDR);
        return gb->wram[addr];
    } 
    
    else if (addr < UNUSABLE_START_ADDR) {  // OAM          -- FE00-FE9F
        return GB_ppu_oam_read(gb->ppu, addr);
    } 
    
    else if (addr < IO_REGS_START_ADDR) {   // Not Usable   -- FEA0-FEFF cf. https://gbdev.io/pandocs/Memory_Map.html#fea0feff-range
        ADJUST_ADDR(UNUSABLE_START_ADDR, IO_REGS_START_ADDR);
        return gb->unusable[addr];
    } 
    
    else if (addr < HRAM_START_ADDR) {      // IO REGS      -- FF00-FF7F
        ADJUST_ADDR(IO_REGS_START_ADDR, HRAM_START_ADDR);
        return gb->io_regs[addr];
    } 
    
    else if (addr < IE_START_ADDR) {        // HRAM         -- FF80-FFFE
        ADJUST_ADDR(HRAM_START_ADDR, IE_START_ADDR);
        return gb->hram[addr];
    } 

    // IE    
    return gb->ie;
}

BYTE GB_mem_read(GB_gameboy_t *gb, WORD addr) { 
    if (gb->mmu->is_dma_active) {
		// TODO: DMA conflicts

		if (addr >= OAM_START_ADDR && addr <= OAM_END_ADDR) {
			return 0xFF;
		}
	}

    return mem_read(gb, addr); 
}

void GB_mem_write(GB_gameboy_t *gb, WORD addr, BYTE data) {
    // temporary solution until serial transfer is implemented
    if (addr == GB_SC_ADDR && (data & 0x80) == 0x80) {
        printf("%d", gb->io_regs[1]);
        fflush(stdout);
    }

    if (addr < VRAM_START_ADDR) {           // ROM bank
        gb->cartridge->write_callback(gb->cartridge, addr, data);
    } 
    
    else if (addr < EXT_RAM_START_ADDR) {   // VRAM
        GB_ppu_vram_write(gb->ppu, addr, data);
    } 
    
    else if (addr < WRAM_START_ADDR) {      // RAM bank
        gb->cartridge->write_callback(gb->cartridge, addr, data);
    }
    
    else if (addr < ECHO_RAM_START_ADDR) {  // WRAM
        ADJUST_ADDR(WRAM_START_ADDR, ECHO_RAM_START_ADDR);
        gb->wram[addr] = data;
    } 
    
    else if (addr < OAM_START_ADDR) {       // Echo wram C000-DDFF
        ADJUST_ADDR(ECHO_RAM_START_ADDR, OAM_START_ADDR);
        gb->wram[addr] = data;
    } 

    else if (addr < UNUSABLE_START_ADDR) {  // OAM
        GB_ppu_oam_write(gb->ppu, addr, data);
    }
    
    else if (addr < IO_REGS_START_ADDR) {   // Not Usable cf. https://gbdev.io/pandocs/Memory_Map.html#fea0feff-range
        ADJUST_ADDR(UNUSABLE_START_ADDR, IO_REGS_START_ADDR);
        gb->unusable[addr] = data;
    } 
    
    else if (addr < HRAM_START_ADDR) {      // IO REGS
        if (addr == 0xFF44) { return; }
        
        if (addr == DMA_SOURCE_ADDR) {
            if (gb->mmu->dma_state == DMA_STOP) {
                gb->mmu->dma_state = DMA_START_M1;
            } else {
                gb->mmu->dma_restart_cntdown = DMA_RESTART_CNTDOWN_DEFAULT;
            }
        }

        ADJUST_ADDR(IO_REGS_START_ADDR, HRAM_START_ADDR);
        gb->io_regs[addr] = GB_timer_write_check(gb, 0xFF00 | addr, data);
    } 
    
    else if (addr < IE_START_ADDR) {        // HRAM
        ADJUST_ADDR(HRAM_START_ADDR, IE_START_ADDR);
        gb->hram[addr] = data;
    } 
    
    else {                                  // IE
        gb->ie = data;
    }
}

GB_mmu_t* GB_mmu_create() {
	GB_mmu_t *mmu = (GB_mmu_t*)( malloc( sizeof (GB_mmu_t) ) );

	if (mmu == NULL) {
		return NULL;
	}

	mmu->dma_state 	= DMA_STOP;
	mmu->dma_offset = 0;
	mmu->dma_restart_cntdown = 0;
    mmu->is_dma_active = 0;

	return mmu;
}

void GB_mmu_destroy(GB_mmu_t *mmu) {
	if (mmu) free(mmu);
}

void GB_dma_run(GB_gameboy_t *gb) {
	if (gb->mmu->dma_restart_cntdown-- == 1) {
		gb->mmu->dma_state = DMA_INIT;
	}

	// TODO: Pass oam_dma_restart.gb
	switch (gb->mmu->dma_state) { 									
    	case DMA_START_M1: 											
			gb->mmu->dma_state = DMA_START_M2;
			break;
    	case DMA_START_M2: 											
    		gb->mmu->dma_state = DMA_INIT; 						
    		break; 													
		case DMA_INIT:
			gb->mmu->dma_source = ( GB_mem_read(gb, DMA_SOURCE_ADDR) << 8 );
    		gb->mmu->dma_state 		= DMA_RUNNING; 						
			gb->mmu->is_dma_active 	= 1;
			gb->mmu->dma_offset 	= 0;
			// NOTE: break is omitted on purpose
    	case DMA_RUNNING: {
			gb->mmu->addr_bus = OAM_START_ADDR | gb->mmu->dma_offset;
			//MEMREAD(gb, (gb->mmu->dma_source | gb->mmu->dma_offset), gb->mmu->data_bus);
            gb->mmu->data_bus = GB_mem_read(gb, (gb->mmu->dma_source | gb->mmu->dma_offset));
    		GB_mem_write(gb, gb->mmu->addr_bus, gb->mmu->data_bus);
    		if (++gb->mmu->dma_offset > ( OAM_END_ADDR & 0xFF ) ) { 	
    			gb->mmu->dma_state 		= DMA_STOP; 						
				gb->mmu->dma_offset 	= 0;
				gb->mmu->is_dma_active 	= 0;
    		} 			
		}
    		break;												
    	default: 													
    		break; 													
    } 																
}
