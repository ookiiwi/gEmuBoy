#include "mmu.h"
#include "gb.h"
#include "graphics/ppu.h"
#include "memmap.h"
#include "cartridge/mbc.h"

#include <stdio.h>
#include <stdlib.h>

enum DMA_STATE {
	DMA_STOP,
	DMA_START_M1,	
	DMA_START_M2,
	DMA_INIT,
	DMA_RUNNING
};

#define DMA_RESTART_CNTDOWN_DEFAULT (3) // Waits 2 M-cycle and restart on the third one
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

#define _START_ADDR(area_name_upper_case)   (GB_ ## area_name_upper_case ## _START_ADDR) 
#define _END_ADDR(area_name_upper_case)     (GB_ ## area_name_upper_case ## _END_ADDR) 

#define ADJUST_ADDR(area_name_upper_case) do {                                                                                                                      \
    int min = _START_ADDR(area_name_upper_case);                                                                                                                    \
    int max = _END_ADDR(area_name_upper_case);                                                                                                                      \
    if ( addr <  min || addr > max ) {                                                                                                                              \
        fprintf(stderr, "OUT OF RANGE $%04X NOT IN RANGE [$%04X-$%04X]\n", addr, min, max);                                                                         \
        addr = max;                                                                                                                                                 \
    }                                                                                                                                                               \
    addr -= min;                                                                                                                                                    \
} while(0)

#define IF_ADDR_IN_RANGE(area_name_upper_case, statement)                                   if (addr >= _START_ADDR(area_name_upper_case) && addr <= _END_ADDR(area_name_upper_case)) { statement }
#define IF_ADDR(a, statement)                                                               if (addr == a) { statement }
#define MEM_write_FUNC_DECL                                                                 void mem_write(GB_gameboy_t *gb, WORD addr, BYTE data)
#define MEM_read_FUNC_DECL                                                                  BYTE mem_read(GB_gameboy_t *gb, WORD addr)
#define MEM_write_DEFAULT_RETURN                                                            return
#define MEM_read_DEFAULT_RETURN                                                             return 0xFF
#define MAKE_MEM_write_RANGE_ACCESS_CALLBACK(area_name_upper_case, prefix, user_data)       IF_ADDR_IN_RANGE ( area_name_upper_case, { prefix##_write(user_data, addr, data); return; } )
#define MAKE_MEM_read_RANGE_ACCESS_CALLBACK(area_name_upper_case, prefix, user_data)        IF_ADDR_IN_RANGE ( area_name_upper_case, return prefix##_read(user_data, addr); )
#define MAKE_MEM_write_ACCESS_CALLBACK(access_addr, prefix, user_data)                      IF_ADDR          ( access_addr, { prefix##_write(user_data, addr, data); return; } )
#define MAKE_MEM_read_ACCESS_CALLBACK(access_addr, prefix, user_data)                       IF_ADDR          ( access_addr, return prefix##_read(user_data, addr); )
#define MAKE_MEM_write_RANGE_ACCESS_ARRAY(area_name_upper_case, array_name)                 IF_ADDR_IN_RANGE ( area_name_upper_case, { ADJUST_ADDR(area_name_upper_case); gb->array_name[addr] = data; return; } ) 
#define MAKE_MEM_read_RANGE_ACCESS_ARRAY(area_name_upper_case, array_name)                  IF_ADDR_IN_RANGE ( area_name_upper_case, { ADJUST_ADDR(area_name_upper_case); return gb->array_name[addr]; } )
#define MAKE_MEM_write_ACCESS_VAR(access_addr, var_name)                                    IF_ADDR          ( access_addr, gb->var_name = data; return; )
#define MAKE_MEM_read_ACCESS_VAR(access_addr, var_name)                                     IF_ADDR          ( access_addr, return gb->var_name; )

#define _GB_io_reg_write(io_regs, addr, data)           ( io_regs[addr&0xFF] = data )
#define _GB_io_reg_read(io_regs, addr)                  ( io_regs[addr&0xFF] )

#define GB_joypad_write(io_regs, addr, data)            _GB_io_reg_write(io_regs, addr, data)
#define GB_joypad_read(io_regs, addr)                   ( io_regs[addr&0xFF] & 0x3F )

#define GB_timer_write(io_regs, addr, data)             _GB_io_reg_write(io_regs, addr, GB_timer_write_check(gb, addr, data))
#define GB_timer_read(io_regs, addr)                    _GB_io_reg_read(io_regs, addr)

#define GB_lcd_write(io_regs, addr, data) do {                                                                                                                      \
        if (addr == DMA_SOURCE_ADDR) {                                                                                                                              \
            if (gb->mmu->dma_state == DMA_STOP) {                                                                                                                   \
                gb->mmu->dma_state = DMA_START_M1;                                                                                                                  \
            } else {                                                                                                                                                \
                gb->mmu->dma_restart_cntdown = DMA_RESTART_CNTDOWN_DEFAULT;                                                                                         \
            }                                                                                                                                                       \
        }                                                                                                                                                           \
        _GB_io_reg_write(io_regs, addr, data);                                                                                                                      \
} while (0)
#define GB_lcd_read(io_regs, addr) _GB_io_reg_read(io_regs, addr)

#define GB_boot_rom_write(io_regs, addr, data) (io_regs[addr&0xFF] |= (data!=0))
#define GB_boot_rom_read(io_regs, addr) _GB_io_reg_read(io_regs, addr) 

#define DECL_MEM_ACCESSOR(access_type)                                                                                                                              \
    MEM_##access_type##_FUNC_DECL {                                                                                                                                 \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(ROM,                     GB_mbc,                 gb->cartridge->mbc)     /* ROM bank     -- 0000-7FFF */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(VRAM,                    GB_ppu_vram,            gb->ppu)                /* VRAM         -- 8000-9FFF */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(EXT_RAM,                 GB_mbc,                 gb->cartridge->mbc)     /* RAM bank     -- A000-BFFF */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_ARRAY   (WRAM,                    wram)                                           /* WRAM         -- C000-DFFF */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(OAM,                     GB_ppu_oam,             gb->ppu)                /* OAM          -- FE00-FE9F */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_ARRAY   (UNUSABLE,                unusable)                                       /* Not Usable   -- FEA0-FEFF */     \
        /* ============= IO REGS ============= */                                                                                                                   \
        MAKE_MEM_##access_type##_ACCESS_CALLBACK      (GB_JOYP_ADDR,            GB_joypad,              gb->io_regs)            /* Joypad       -- FF00      */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(SERIAL,                  _GB_io_reg,             gb->io_regs)            /* Serial       -- FF01-FF02 */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(TIMER_REGS,              GB_timer,               gb->io_regs)            /* Timer        -- FF04-FF07 */     \
        MAKE_MEM_##access_type##_ACCESS_CALLBACK      (GB_IF_ADDR,              _GB_io_reg,             gb->io_regs)            /* Interrupts   -- FF0F      */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(APU,                     _GB_io_reg,             gb->io_regs)            /* Audio        -- FF10-FF26 */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(WAVE_PATTERN_RAM,        _GB_io_reg,             gb->io_regs)            /* Wave pattern -- FF30-FF3F */     \
        MAKE_MEM_##access_type##_RANGE_ACCESS_CALLBACK(LCD_REGS,                GB_lcd,                 gb->io_regs)            /* LCD Control, -- FF40-FF4B */     \
        MAKE_MEM_##access_type##_ACCESS_CALLBACK      (GB_BOOT_ROM_UNMAP_ADDR,  GB_boot_rom,            gb->io_regs)            /* Boot rom     -- FF50      */     \
        /* =================================== */                                                                                                                   \
        MAKE_MEM_##access_type##_RANGE_ACCESS_ARRAY   (HRAM,                    hram)                                           /* HRAM         -- FF80-FFFE */     \
        MAKE_MEM_##access_type##_ACCESS_VAR           (GB_IE_ADDR,              ie)                                             /* IE           -- FFFF      */     \
        /* Adjust address match wram address range in case echo ram is accessed */                                                                                  \
        addr -= 0x2000;                                                                                                                                             \
        MAKE_MEM_##access_type##_RANGE_ACCESS_ARRAY   (WRAM,                    wram)                                           /* Echo wram    -- E000-FDFF */     \
        MEM_##access_type##_DEFAULT_RETURN;                                                                                                                         \
    }

DECL_MEM_ACCESSOR(read)
DECL_MEM_ACCESSOR(write)

BYTE GB_mem_read(GB_gameboy_t *gb, WORD addr) { 
    if (gb->mmu->is_dma_active) {
		// TODO: DMA conflicts

		if (addr >= GB_OAM_START_ADDR && addr <= GB_OAM_END_ADDR) {
			return 0xFF;
        }
	}

    return mem_read(gb, addr); 
}

void GB_mem_write(GB_gameboy_t *gb, WORD addr, BYTE data) {
    if (gb->mmu->is_dma_active) {
		if (addr >= GB_OAM_START_ADDR && addr <= GB_OAM_END_ADDR) {
			return;
        }
	}
    
    // temporary solution until serial transfer is implemented
    if (addr == GB_SC_ADDR && (data & 0x80) == 0x80) {
        printf("%d", gb->io_regs[1]);
        fflush(stdout);
    }

    mem_write(gb, addr, data);
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
	if (gb->mmu->dma_restart_cntdown && gb->mmu->dma_restart_cntdown-- == 1) {
		gb->mmu->dma_state = DMA_INIT;
	}

	switch (gb->mmu->dma_state) { 									
    	case DMA_START_M1: 											
			gb->mmu->dma_state = DMA_START_M2;
			break;
    	case DMA_START_M2: 											
    		gb->mmu->dma_state = DMA_INIT; 						
    		break; 													
		case DMA_INIT:
			gb->mmu->dma_source     = ( mem_read(gb, DMA_SOURCE_ADDR) << 8 );
    		gb->mmu->dma_state 		= DMA_RUNNING; 						
			gb->mmu->is_dma_active 	= 1;
			gb->mmu->dma_offset 	= 0;
			// NOTE: break is omitted on purpose
    	case DMA_RUNNING: {
			gb->mmu->addr_bus = GB_OAM_START_ADDR | gb->mmu->dma_offset;
            gb->mmu->data_bus = mem_read(gb, (gb->mmu->dma_source | gb->mmu->dma_offset));
    		mem_write(gb, gb->mmu->addr_bus, gb->mmu->data_bus);
    		if (++gb->mmu->dma_offset > ( GB_OAM_END_ADDR & 0xFF ) ) { 	
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
