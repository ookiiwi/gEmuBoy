#include "mmu.h"
#include "defs.h"
#include "timer.h"
#include "memmap.h"
#include "cpudef.h"

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

GB_mmu_t* GB_mmu_create() {
	GB_mmu_t *mmu = (GB_mmu_t*)( malloc(sizeof(GB_mmu_t)) );

	if (mmu == NULL) {
		return NULL;
	}

	mmu->dma_state 	= DMA_STOP;
	mmu->dma_offset = 0;
	mmu->dma_restart_cntdown = 0;

	return mmu;
}

void GB_mmu_destroy(GB_mmu_t *mmu) {
	if (mmu) free(mmu);
}

#define MEMREAD(gb, addr, res) do { 									\
    if (addr <= 0x3FFF) {           /* Bank 00 */ 						\
        if (addr < 0x100 && !GB_BOOT_ROM_UNMAPPED) { 					\
            res = BOOTROM[addr]; 										\
        } else { 														\
            res = gb->rom[addr]; 										\
        } 																\
    } else if (addr <= 0x7FFF) {    /* Bank 01-NN */ 					\
        res = gb->rom[addr]; 											\
    } else { 															\
		res = gb->memory[addr]; 										\
	} 																	\
} while (0)

BYTE GB_mem_read(GB_gameboy_t *gb, int addr) {
	if (gb->mmu->is_dma_active) {
		// TODO: DMA conflicts

		if (addr >= OAM_START_ADDR && addr <= OAM_END_ADDR) {
			return 0xFF;
		}
	}


    BYTE res;
	MEMREAD(gb, addr, res);

    return res;
}

void GB_mem_write(GB_gameboy_t *gb, int addr, BYTE data) {
    if (addr == GB_LY_ADDR) { // LY
        return;
    } else if (addr == DMA_SOURCE_ADDR) {
		if (gb->mmu->dma_state == DMA_STOP) {
			gb->mmu->dma_state = DMA_START_M1;
		} else {
			gb->mmu->dma_restart_cntdown = DMA_RESTART_CNTDOWN_DEFAULT;
		}
    } else if (gb->mmu->dma_state == DMA_RUNNING) {
		return;
	}

    TIMER_CHECK_WRITE(gb, addr, data);

    gb->memory[addr] = data;
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
			gb->mmu->dma_source = ( gb->memory[DMA_SOURCE_ADDR] << 8 );
    		gb->mmu->dma_state 		= DMA_RUNNING; 						
			gb->mmu->is_dma_active 	= 1;
			gb->mmu->dma_offset 	= 0;
			// NOTE: break is omitted on purpose
    	case DMA_RUNNING: {
			gb->mmu->addr_bus = OAM_START_ADDR | gb->mmu->dma_offset;
			MEMREAD(gb, (gb->mmu->dma_source | gb->mmu->dma_offset), gb->mmu->data_bus);
    		gb->memory[gb->mmu->addr_bus] = gb->mmu->data_bus;
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

