#include "graphics/ppu.h"
#include "mmu.h"
#include "cpudef.h"
#include "gb.h"
#include "cpu/interrupt.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

/* LCD Status Register */
#define LY                          ( gb->io_regs[0x44] )
#define LYC                         ( GB_mem_read(gb, 0xFF45) )
#define STAT                        ( gb->io_regs[0x41] )
#define LYC_INT                     ( (STAT >> 6) & 1 )
#define MODE2_INT                   ( (STAT >> 5) & 1 )
#define MODE1_INT                   ( (STAT >> 4) & 1 )
#define MODE0_INT                   ( (STAT >> 3) & 1 )
#define LYC_LY                      ( (STAT >> 2) & 1 )     /* READ ONLY */
#define PPU_MODE                    ( STAT & 3 )            /* READ ONLY */

#define PPU_MODE_HBLANK     (0)
#define PPU_MODE_VBLANK     (1)
#define PPU_MODE_OAMSEARCH  (2)
#define PPU_MODE_DRAW       (3)

/* Background Coordinates */
#define SCY                         ( GB_mem_read(gb, 0xFF42) )
#define SCX                         ( GB_mem_read(gb, 0xFF43) )

/* Window Coordinates */
#define WY                          ( GB_mem_read(gb, 0xFF4A) )
#define WX                          ( GB_mem_read(gb, 0xFF4B) )
#define SHOW_WIN                    ( LCDC_BG_EN && LCDC_WIN_EN && (WX - 7) <= LX && WY <= LY )

/* LCD Monochrome Palettes */
#define BGP                         ( GB_mem_read(gb, 0xFF47) )
#define OBP0                        ( GB_mem_read(gb, 0xFF48) )
#define OBP1                        ( GB_mem_read(gb, 0xFF49) )

#define SET_REGISTER_BIT(addr, value, pos, bit)     ( GB_mem_write(gb, addr, ( ( value & ( ~(1 << pos) ) ) ) | (1 << pos) ) )
#define SET_LCDC(pos, bit)                          ( SET_REGISTER_BIT(0xFF40, LCDC, pos, bit) )
#define SET_STAT(pos, bit)                          ( SET_REGISTER_BIT(0xFF41, STAT, pos, bit) )
#define SET_PPU_MODE(mode) do {                                 \
    GB_mem_write(gb, 0xFF41, ( STAT & 0xFC ) | (mode & 3) );    \
    gb->ppu->m_ppu_mode_switched = 1;                           \
} while(0)

#define PPU_MODE_SWITCHED                           ( gb->ppu->m_ppu_mode_switched + ( gb->ppu->m_ppu_mode_switched = 0) )

#define LX                                          ( gb->ppu->lx                       )
#define PENDING_CYCLES                              ( gb->ppu->pending_cycles           )
#define SCANLINE_DOT_COUNTER                        ( gb->ppu->scanline_dot_counter     )

#define OAMBUFFER                                   ( gb->ppu->oam_buffer               )

#define BG_FETCHER                                  ( gb->ppu->bg_fetcher               )
#define OBJ_FETCHER                                 ( gb->ppu->obj_fetcher              )

#define FETCHER_GET_TILE_ID     (0)
#define FETCHER_GET_DATA_LOW    (1)
#define FETCHER_GET_DATA_HIGH   (2)
#define FETCHER_IDLE            (3)
#define FETCHER_PUSH            (4)

#define GET_VIEWPORT_BOTTOM()                       ( ( SCY + 143 ) % 256 )
#define GET_VIEWPORT_RIGHT()                        ( ( SCX + 159 ) % 256 )

#define PPU_DOTS_PER_SCANLINE   (456)
#define MAX_LY                  (153)
#define VBLANK_LY_START         (143)

#define NB_RENDERED_PIXELS      ( LX - (SCX % 8) )

// Default values
#define WINDOW_LINE_COUNTER_DEFAULT     (-1)
#define SPRITE_TALL_LY_START_DEFAULT    (-1)
#define OAM_START_ADDR                  (0xFE00)
#define PPU_MODE_SWITCHED_DEFAULT       (1)

typedef struct PixelFIFO_Cell PixelFIFO_Cell;

struct PixelFIFO_Cell {
    BYTE color_id;
    BYTE palette_id;
    BYTE bg_priority;
};

typedef struct {
    PixelFIFO_Cell  buffer[16];
    int start;
    int size;
} PixelFIFO;

struct OAMBuffer {
    BYTE buffer[10];
    int buf_size;
    int cur_oam_addr;
};

struct PixelFetcher {
    int             x;
    int             y;
    int             status;
    BYTE            tile_id;
    BYTE            tile_data_high;
    BYTE            tile_data_low;
    PixelFIFO      *fifo;

    int             draw_window;
    int             window_line_counter;

    int             sprite_addr;
    int             sprite_tall_ly_start;
    int             last_sprite_x_end;
};

PixelFIFO* pixelfifo_create() {
    PixelFIFO *fifo = (PixelFIFO*)( malloc( sizeof(PixelFIFO) ) );
    fifo->start = 0;
    fifo->size  = 0;

    return fifo;
}

void pixelfifo_push(PixelFIFO *fifo, BYTE color_id, BYTE palette_id, BYTE bg_priority) { 
    if ( fifo->size >= 16 ) return;

    PixelFIFO_Cell cell;
    cell.color_id          = color_id;
    cell.palette_id        = palette_id;
    cell.bg_priority       = bg_priority;

    fifo->buffer[ ( fifo->start + fifo->size ) % 16 ] = cell;
    fifo->size++;
}

void pixelfifo_overlap(PixelFIFO *fifo, BYTE color_id, BYTE palette_id, BYTE bg_priority, unsigned offset) {
    if ( fifo->size >= 16 || offset >= fifo->size ) return;

    PixelFIFO_Cell cell;
    cell.color_id          = color_id;
    cell.palette_id        = palette_id;
    cell.bg_priority       = bg_priority;

    int index = ( fifo->start + fifo->size - offset ) % 16;

    // Replace current cell if transparent
    if (fifo->buffer[index].color_id == 0) {
        fifo->buffer[index] = cell;
    }
}

/// Merge [high] and [low] to form a row of 8 pixels and push them into the fifo
void pixelfifo_push_row(PixelFIFO *fifo, BYTE high, BYTE low, BYTE palette_id, BYTE bg_priority, int flip_x, unsigned overlap_offset) {
    for ( int i = 0; i < 8; i++) {
        int data;
        
        if (flip_x) {
            data = ( ( high & 1 ) << 1 ) | ( ( low & 1 ) );
            high >>= 1;
            low  >>= 1;
        } else {
            data = ( ( high & 0x80 ) >> 6 ) | ( ( low & 0x80 ) >> 7 );
            high <<= 1;
            low  <<= 1;
        }

        if (overlap_offset) {  
            pixelfifo_overlap( fifo, data, palette_id, bg_priority, overlap_offset-- );
        } else {
            pixelfifo_push( fifo, data, palette_id, bg_priority );
        }
    }
}

int pixelfifo_pop(PixelFIFO *fifo, BYTE *color_id, BYTE *palette_id, BYTE *bg_priority) {
    if (!fifo->size) return -1;

    if (color_id)       *color_id       = fifo->buffer[fifo->start].color_id;
    if (palette_id)     *palette_id     = fifo->buffer[fifo->start].palette_id;
    if (bg_priority)    *bg_priority    = fifo->buffer[fifo->start].bg_priority;
     
    fifo->start = ( fifo->start + 1 ) % 16;
    fifo->size--;

    return 0;
}

void pixelfifo_clear(PixelFIFO *fifo) {
    if (fifo == NULL) return;

    fifo->start = 0;
    fifo->size = 0;
}

void pixelfifo_destroy(PixelFIFO *fifo) {
    pixelfifo_clear(fifo);

    free(fifo);
}

/// Check whether a fifo is less than 8 pixels
static inline int pixelfifo_empty(PixelFIFO *fifo) { return fifo->size <= 8; }

OAMBuffer* oambuffer_create() {
    OAMBuffer *buffer               = (OAMBuffer*)( malloc( sizeof(OAMBuffer) ) );
    buffer->buf_size                = 0;
    buffer->cur_oam_addr            = OAM_START_ADDR;

    return buffer;
}

void oambuffer_destroy(OAMBuffer *buffer) {
    if (buffer) free(buffer);
}

#define OAMBUFFER_CLEAR() do {                                  \
    memset((void*)OAMBUFFER->buffer, 0, 10);                    \
    OAMBUFFER->cur_oam_addr = OAM_START_ADDR;                   \
    OAMBUFFER->buf_size = 0;                                    \
} while(0)

PixelFetcher* pixelfetcher_create() {
    PixelFetcher *fetcher           = (PixelFetcher*)( malloc( sizeof(PixelFetcher) ) );
    fetcher->status                 = 0;
    fetcher->x                      = 0;
    fetcher->y                      = 0;
    fetcher->fifo                   = pixelfifo_create();
    fetcher->draw_window            = 0;
    fetcher->window_line_counter    = WINDOW_LINE_COUNTER_DEFAULT;
    fetcher->sprite_addr            = 0;
    fetcher->last_sprite_x_end      = 0;
    fetcher->sprite_tall_ly_start   = SPRITE_TALL_LY_START_DEFAULT;

    return fetcher;
}

void pixelfetcher_destroy(PixelFetcher *fetcher) {
    if (fetcher == NULL) return;

    pixelfifo_destroy(fetcher->fifo); 
    free(fetcher);
}

BYTE GB_ppu_vram_read(GB_ppu_t *ppu, WORD addr) {
    if (addr < 0x8000 || addr >= 0xA000) {
        fprintf(stderr, "VRAM READ OUT OF RANGE\n");
        return 0xFF;
    }

    addr = ( addr - 0x8000 ) & 0x1FFF;
    return ppu->vram[addr];
}

void GB_ppu_vram_write(GB_ppu_t *ppu, WORD addr, BYTE data) {
    if (addr < 0x8000 || addr >= 0xA000) {
        fprintf(stderr, "VRAM WRITE OUT OF RANGE\n");
        return;
    }

    addr = ( addr - 0x8000 ) & 0x1FFF;
    ppu->vram[addr] = data;
}

BYTE GB_ppu_oam_read(GB_ppu_t *ppu, WORD addr) {
    if (addr < 0xFE00 || addr > 0xFE9F) {
        fprintf(stderr, "OAM READ OUT OF RANGE\n");
        return 0xFF;
    }

    addr -= 0xFE00;
    return ppu->oam[addr];
}

void GB_ppu_oam_write(GB_ppu_t *ppu, WORD addr, BYTE data) {
    if (addr < 0xFE00 || addr > 0xFE9F) {
        fprintf(stderr, "OAM WRITE OUT OF RANGE\n");
    }

    addr -= 0xFE00;
    ppu->oam[addr] = data;
}

GB_ppu_t* GB_ppu_create() {
    GB_ppu_t *ppu = (GB_ppu_t*)( malloc( sizeof(GB_ppu_t) ) );

    ppu->vram = (BYTE*)( malloc( sizeof (BYTE) * 0x2000 ) );
    if (!ppu->vram) {
        free(ppu);
        return NULL;
    }

    ppu->oam = (BYTE*)( malloc( sizeof (BYTE) * 160 ) );
    if (!ppu->oam) {
        free(ppu);
        return NULL;
    }

    ppu->oam_buffer                 = oambuffer_create();
    ppu->bg_fetcher                 = pixelfetcher_create();
    ppu->obj_fetcher                = pixelfetcher_create();
    ppu->lcd                        = GB_lcd_create();
    ppu->lx                         = 0;
    ppu->pending_cycles             = 0;
    ppu->scanline_dot_counter       = 0;
    ppu->m_ppu_mode_switched        = PPU_MODE_SWITCHED_DEFAULT;

    return ppu;
}

void GB_ppu_destroy(GB_ppu_t *ppu) {
    if (ppu == NULL) return;

    oambuffer_destroy(ppu->oam_buffer);
    pixelfetcher_destroy(ppu->bg_fetcher);
    pixelfetcher_destroy(ppu->obj_fetcher);
    GB_lcd_destroy(ppu->lcd);
    free(ppu);
}

void pixelfetcher_get_tile_id(GB_gameboy_t *gb) {
    unsigned tilemap;
    unsigned x, y;
    unsigned offset;

    if (!LCDC_BG_EN) {
        return;
    }

    if (SHOW_WIN) {
        tilemap =  LCDC_WIN_MAP ? 0x9C00 : 0x9800;
        x       = BG_FETCHER->x & 0x1F;
        y       = BG_FETCHER->window_line_counter;
    } else {
        tilemap = LCDC_BG_MAP ? 0x9C00 : 0x9800;
        x       = ( ( SCX / 8 ) + BG_FETCHER->x ) & 0x1F;
        y       = ( LY + SCY ) & 0xFF; 
    }

    offset  = ( x + 32 * ( y / 8 ) );

    BG_FETCHER->tile_id = GB_mem_read(gb, tilemap+offset);
    BG_FETCHER->x++;
    BG_FETCHER->y = y;
}

BYTE pixelfetcher_get_tile_data(GB_gameboy_t *gb, int high) {
    int addr = 0x8000;
    int tile_id = BG_FETCHER->tile_id;
    int offset  = SHOW_WIN ? BG_FETCHER->window_line_counter : LY + SCY;

    if (!LCDC_BG_EN) {
        return 0x00;
    }

    high = high ? 1 : 0;

    if ( !LCDC_TILE_SEL ) {
        addr = 0x9000;
        tile_id = (SIGNED_BYTE)tile_id;
    }

    return GB_mem_read(gb, addr + tile_id*16 + high + 2 * (offset%8) );
}

void pixelfetcher_push(GB_gameboy_t *gb) {
    if (pixelfifo_empty(BG_FETCHER->fifo)) {
        pixelfifo_push_row( BG_FETCHER->fifo, 
                            BG_FETCHER->tile_data_high, 
                            BG_FETCHER->tile_data_low,
                            0, 0, 0, 0 );
        BG_FETCHER->status=0;
    }
}

#define PIXEL_FETCHER_RESET(fetcher) do {                                           \
    pixelfifo_clear(fetcher->fifo);                                                 \
    fetcher->x                      = 0;                                            \
    fetcher->y                      = 0;                                            \
    fetcher->status                 = 0;                                            \
    fetcher->tile_id                = 0;                                            \
    fetcher->tile_data_low          = 0;                                            \
    fetcher->tile_data_high         = 0;                                            \
    fetcher->sprite_addr            = 0;                                            \
    fetcher->last_sprite_x_end      = 0;                                            \
} while(0)

void bg_fetch(GB_gameboy_t *gb) {
    // Advance one step every 2-dot
    if (BG_FETCHER->status < 3) {
        if (PENDING_CYCLES < 2) return;
        PENDING_CYCLES -= 2;
    }

    switch (BG_FETCHER->status++) {
        case FETCHER_GET_TILE_ID: 
            pixelfetcher_get_tile_id(gb);
            break;
        case FETCHER_GET_DATA_LOW:
            BG_FETCHER->tile_data_low = pixelfetcher_get_tile_data(gb, 0);
            break;
        case FETCHER_GET_DATA_HIGH:
            BG_FETCHER->tile_data_high = pixelfetcher_get_tile_data(gb, 1);
            break;
        default: /* Try push */
            pixelfetcher_push(gb);
            break;
    }
}

void sprite_fetch(GB_gameboy_t *gb) {
    if (!LCDC_OBJ_EN) return;

    // Here we check for the object to fetch
    int i = 0;
    while (!OBJ_FETCHER->sprite_addr && i < OAMBUFFER->buf_size) {
        WORD addr = 0xFE00 | OAMBUFFER->buffer[i];
        if ( addr <= 0xFE9F && ( GB_mem_read(gb, addr+1) <= ( LX + 8 ) ) ) {
            OBJ_FETCHER->sprite_addr = addr;
            OAMBUFFER->buffer[i] = 0xFF;
            break;
        }

        i++;
    }

    if (!OBJ_FETCHER->sprite_addr || PENDING_CYCLES++ < 2) return;
    PENDING_CYCLES-=2;

    BYTE attr   = GB_mem_read(gb, OBJ_FETCHER->sprite_addr+3);
    BYTE flip_y = ( attr >> 6 ) & 1;
    int offset  = ( LY + SCY ) % 8;

    // If flip vertically, we fetch in reverse
    if (flip_y) {
        offset = 7 - offset;
    }
    offset *= 2;

    switch (OBJ_FETCHER->status++) {
        case FETCHER_GET_TILE_ID:
            OBJ_FETCHER->tile_id = GB_mem_read(gb, OBJ_FETCHER->sprite_addr+2);

            if (LCDC_OBJ_SIZE) {
                // Check if fetch new 8x16 sprite
                if (OBJ_FETCHER->sprite_tall_ly_start < 0) {
                    OBJ_FETCHER->sprite_tall_ly_start = LY;
                }
                
                int line_diff = LY - OBJ_FETCHER->sprite_tall_ly_start;
                int cur_tile = line_diff / 8; // 0 for top tile and 1 for bottom tile

                if (flip_y) cur_tile = !cur_tile;
                OBJ_FETCHER->tile_id = ( OBJ_FETCHER->tile_id & 0xFE ) | cur_tile;

                // 8x16 sprite's end
                if (line_diff > 15) {
                    OBJ_FETCHER->sprite_tall_ly_start = -1;
                }
            }
            break;
        case FETCHER_GET_DATA_LOW:
            OBJ_FETCHER->tile_data_low  = GB_mem_read(gb, 0x8000 + OBJ_FETCHER->tile_id*16     + offset );
            break;
        case FETCHER_GET_DATA_HIGH:
            OBJ_FETCHER->tile_data_high = GB_mem_read(gb, 0x8000 + OBJ_FETCHER->tile_id*16 + 1 + offset );
            break;
        default: {
            BYTE palette    = ( attr >> 4 ) & 1;
            BYTE priority   = ( attr >> 7 ) & 1;
            BYTE flip_x     = ( attr >> 5 ) & 1;
            BYTE data_high  = OBJ_FETCHER->tile_data_high;
            BYTE data_low   = OBJ_FETCHER->tile_data_low;

            int overlap_offset = 0;
            if (LX < OBJ_FETCHER->last_sprite_x_end) {
                overlap_offset = OBJ_FETCHER->last_sprite_x_end - LX;
            }

            pixelfifo_push_row( OBJ_FETCHER->fifo, 
                                data_high, 
                                data_low,
                                palette,
                                priority,
                                flip_x,
                                overlap_offset );

            OBJ_FETCHER->sprite_addr        = 0;
            OBJ_FETCHER->status             = 0;
            OBJ_FETCHER->last_sprite_x_end  = LX+8;
        }
            break;
    }
}

void ppu_render(GB_gameboy_t *gb) {
    if (pixelfifo_empty(BG_FETCHER->fifo)) return;

    BYTE color_id, bg_priority;
    BYTE palette, color_index;

    pixelfifo_pop(BG_FETCHER->fifo, &color_id, NULL, &bg_priority);
    palette = BGP;
    
    if (OBJ_FETCHER->fifo->size > 0) {
        BYTE cid, pid, bgp;
        pixelfifo_pop(OBJ_FETCHER->fifo, &cid, &pid, &bgp);

        if ( cid && !( bgp && color_id ) ) {
            palette = GB_mem_read(gb, 0xFF48+pid);
            color_id = cid;
        }
    }

    // A color value being stored in 2 bits,
    // we just need to shift the palette byte 
    // by 2 times the color id resulting in a
    // maximum of 6 bits shifted
    color_index = ( palette >> (color_id * 2) ) & 3;

    // BG Scrolling penality
    if ( LX >= (SCX % 8) ) {
        GB_lcd_set_pixel(gb->ppu->lcd, NB_RENDERED_PIXELS, LY, color_index);
    }

    LX++;

    if (!BG_FETCHER->draw_window && SHOW_WIN) {
        PIXEL_FETCHER_RESET(BG_FETCHER);
        BG_FETCHER->draw_window = 1;
    } 
}

void ppu_oamsearch(GB_gameboy_t *gb) {
    if ( PPU_MODE_SWITCHED ) {
        OAMBUFFER_CLEAR();

        SET_STAT(2, LY == LYC);
        if (LY == LYC && LYC_INT) {
            REQUEST_INTERRUPT(IF_LCD);
        }

        if ( MODE2_INT ) REQUEST_INTERRUPT(IF_LCD);
    }

    if (LCDC_OBJ_EN && OAMBUFFER->buf_size < 10 && PENDING_CYCLES++ >= 2) {
        BYTE y = GB_mem_read(gb, OAMBUFFER->cur_oam_addr);
        BYTE x = GB_mem_read(gb, OAMBUFFER->cur_oam_addr+1);

        if (x > 0 && 
            ( LY + 16 ) >= y &&
            ( LY + 16 ) < ( y + (LCDC_OBJ_SIZE+1) * 8 ) ) {
            OAMBUFFER->buffer[OAMBUFFER->buf_size++] = OAMBUFFER->cur_oam_addr & 0xFF;
        }

        OAMBUFFER->cur_oam_addr+=4; // Note: Skip 4 bytes meta-data
        PENDING_CYCLES-=2;
    }

    if (++SCANLINE_DOT_COUNTER >= 80) {
        SET_PPU_MODE(PPU_MODE_DRAW);

        if (WY <= LY && WX-7 <= 160) {
            BG_FETCHER->window_line_counter++;
        }
    }
}

void ppu_draw(GB_gameboy_t *gb) {
    sprite_fetch(gb);

    if (!OBJ_FETCHER->sprite_addr) {
        bg_fetch(gb);
        ppu_render(gb);
    }

    SCANLINE_DOT_COUNTER++;

    // Finish drawing if 160 have been drawn or 289 dots consumed
    if ( NB_RENDERED_PIXELS >= 160 || SCANLINE_DOT_COUNTER >= 289 ) {
        PIXEL_FETCHER_RESET(BG_FETCHER);
        PIXEL_FETCHER_RESET(OBJ_FETCHER);
        SET_PPU_MODE(PPU_MODE_HBLANK);
    }
}

void ppu_hblank(GB_gameboy_t *gb) {
    if ( PPU_MODE_SWITCHED ) {
        if (MODE0_INT) REQUEST_INTERRUPT(IF_LCD);
        BG_FETCHER->draw_window = 0;
    }

    if ( ++SCANLINE_DOT_COUNTER >= PPU_DOTS_PER_SCANLINE ) {
        int mode = LY >= VBLANK_LY_START ? PPU_MODE_VBLANK : PPU_MODE_OAMSEARCH;

        SCANLINE_DOT_COUNTER = 0;
        LX = 0;
        LY++;

        SET_PPU_MODE(mode);
    }
}

void ppu_vblank(GB_gameboy_t *gb) {

    // VBLANK start
    if ( PPU_MODE_SWITCHED ) {
        REQUEST_INTERRUPT(IF_VBLANK);

        if (MODE1_INT) {
            REQUEST_INTERRUPT(IF_LCD);
        }

        GB_lcd_clear(gb->ppu->lcd);
        GB_lcd_render(gb->ppu->lcd);

        BG_FETCHER->window_line_counter = WINDOW_LINE_COUNTER_DEFAULT;
    }

    // Scanline start
    if ( !SCANLINE_DOT_COUNTER ) {
        SET_STAT(2, LY == LYC);
        if (LY == LYC && LYC_INT) {
            REQUEST_INTERRUPT(IF_LCD);
        }
    }

    // Scanline end
    if (SCANLINE_DOT_COUNTER >= PPU_DOTS_PER_SCANLINE) {
        LX = 0;                                         
        LY++;                                           
        SCANLINE_DOT_COUNTER = 0;                       
    }                                                   

    // VBLANK end
    if (LY > MAX_LY) {
        LX = 0;
        LY = 0;
        SCANLINE_DOT_COUNTER = 0;
        SET_PPU_MODE(PPU_MODE_OAMSEARCH);
    }

    SCANLINE_DOT_COUNTER++;
}

void GB_ppu_tick(GB_gameboy_t *gb, int cycles) {
    if (!LCDC_LCD_EN || gb == NULL || gb->ppu == NULL) return;
    int dot_cnt, mode;

    for ( int i = 0; i < 4; i++) {
        dot_cnt = SCANLINE_DOT_COUNTER;
        mode = PPU_MODE;
        PENDING_CYCLES++;
        switch (PPU_MODE) {
            case PPU_MODE_HBLANK:       ppu_hblank(gb);     break;  // 87-204 dots
            case PPU_MODE_VBLANK:       ppu_vblank(gb);     break;  // 456 * 10 = 4560 dots
            case PPU_MODE_OAMSEARCH:    ppu_oamsearch(gb);  break;  // 80 dots
            case PPU_MODE_DRAW:         ppu_draw(gb);       break;  // 172-289 dots
        }

        if (dot_cnt == SCANLINE_DOT_COUNTER) {
            fprintf(stderr, "DOT ERROR: %d\n", mode);
        }
    }
}
