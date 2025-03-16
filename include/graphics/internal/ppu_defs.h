#ifndef GB_PPU_DEFS_H_
#define GB_PPU_DEFS_H_

#include "mmu.h" /* GB_mem_read */

/* LCD Control Register */
#define LCDC                        ( GB_mem_read(gb, 0xFF40) )
#define LCDC_LCD_EN                 ( LCDC >> 7 )
#define LCDC_WIN_MAP                ( (LCDC >> 6) & 1 )
#define LCDC_WIN_EN                 ( (LCDC >> 5) & 1 )
#define LCDC_TILE_SEL               ( (LCDC >> 4) & 1 )
#define LCDC_BG_MAP                 ( (LCDC >> 3) & 1 )
#define LCDC_OBJ_SIZE               ( (LCDC >> 2) & 1 )
#define LCDC_OBJ_EN                 ( (LCDC >> 1) & 1 )
#define LCDC_BG_EN                  ( LCDC & 1 )            /* BG and WIN enable */

#define rLY                         ( gb->io_regs[0x44]&0xFF )
#define rLX                         ( gb->ppu->lx&0xFF )

/* Background Coordinates */
#define SCY                         ( GB_mem_read(gb, 0xFF42) )
#define SCX                         ( GB_mem_read(gb, 0xFF43) )

/* Window Coordinates */
#define WY                          ( GB_mem_read(gb, 0xFF4A) )
#define WX                          ( GB_mem_read(gb, 0xFF4B) )
#define SHOW_WIN                    ( LCDC_WIN_EN && (WX - 7) <= rLX && WY <= rLY )

#define WINDOW_LINE_COUNTER_DEFAULT     (-1)
#define SPRITE_TALL_LY_START_DEFAULT    (-1)

#endif
