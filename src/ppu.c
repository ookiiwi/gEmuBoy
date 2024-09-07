#include "ppu.h"
#include "mmu.h"
#include "cpudef.h"
#include "gb.h"
#include "interrupt.h"

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
#define LY                          ( gb->memory[0xFF44] )
#define LYC                         ( GB_mem_read(gb, 0xFF45) )
#define STAT                        ( gb->memory[0xFF41] )
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

#define OAMBUFFER                                   ( gb->ppu->oam_buffer->buffer       )
#define OAMBUFFER_SIZE                              ( gb->ppu->oam_buffer->buf_size     )
#define OAMSEARCH_CUR_ADDR                          ( gb->ppu->oam_buffer->cur_oam_addr )

#define PIXEL_FETCHER                               ( gb->ppu->pixel_fetcher            )
#define BG_FIFO                                     ( PIXEL_FETCHER->bg_fifo            )
#define OBJ_FIFO                                    ( PIXEL_FETCHER->obj_fifo           )

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

typedef struct PixelFIFO_Cell PixelFIFO_Cell;

struct PixelFIFO_Cell {
    BYTE color_id;
    BYTE palette_id;
    BYTE bg_priority;
    PixelFIFO_Cell *next;
};

typedef struct {
    PixelFIFO_Cell *first;
    PixelFIFO_Cell *last;
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
    PixelFIFO      *bg_fifo;
    PixelFIFO      *obj_fifo;
};

PixelFIFO* pixelfifo_create() {
    PixelFIFO *fifo = (PixelFIFO*)( malloc( sizeof(PixelFIFO) ) );
    fifo->first  = NULL;
    fifo->last   = NULL;

    return fifo;
}

void pixelfifo_push(PixelFIFO *fifo, BYTE color_id, BYTE palette_id, BYTE bg_priority) {
    PixelFIFO_Cell *cell    = (PixelFIFO_Cell*)(malloc(sizeof(PixelFIFO_Cell)));
    cell->color_id          = color_id;
    cell->palette_id        = palette_id;
    cell->bg_priority       = bg_priority;

    if (fifo->first != NULL) {
        fifo->last->next = cell;
        fifo->last = cell;
    } else {
        fifo->first = cell;
        fifo->first->next = cell;
        fifo->last = cell;
    }

    fifo->last = cell;
    fifo->size++;
}

/// Merge [high] and [low] to form a row of 8 pixels and push them into the fifo
void pixelfifo_push_row(PixelFIFO *fifo, BYTE high, BYTE low) {
    for ( int i = 0; i < 8; i++) {
        int data = ( ( high & 0x80 ) >> 6 ) | ( ( low & 0x80 ) >> 7 );
        pixelfifo_push( fifo, data, 0, 0 );
        high <<= 1;
        low  <<= 1;
    }
}

int pixelfifo_pop(PixelFIFO *fifo, BYTE *color_id, BYTE *palette_id, BYTE *bg_priority) {
    if (fifo->first == NULL) return -1;

    PixelFIFO_Cell *tmp = fifo->first->next;

    if (tmp) {
        if (color_id)       *color_id       = fifo->first->color_id;
        if (palette_id)     *palette_id     = fifo->first->palette_id;
        if (bg_priority)    *bg_priority    = fifo->first->bg_priority;
    }

    free(fifo->first);
    fifo->first = tmp;
    fifo->size--;

    return 0;
}

void pixelfifo_clear(PixelFIFO *fifo) {
    if (fifo == NULL) return;
    
    while (fifo->first != NULL) {
        pixelfifo_pop(fifo, NULL, NULL, NULL);
    }
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
    buffer->cur_oam_addr            = 0xFE00;

    return buffer;
}

void oambuffer_destroy(OAMBuffer *buffer) {
    if (buffer) free(buffer);
}

PixelFetcher* pixelfetcher_create() {
    PixelFetcher *fetcher           = (PixelFetcher*)( malloc( sizeof(PixelFetcher) ) );
    fetcher->status                 = 0;
    fetcher->x                      = 0;
    fetcher->y                      = 0;
    fetcher->bg_fifo                = pixelfifo_create();
    fetcher->obj_fifo               = pixelfifo_create();

    return fetcher;
}

void pixelfetcher_destroy(PixelFetcher *fetcher) {
    if (fetcher == NULL) return;

    pixelfifo_destroy(fetcher->bg_fifo); 
    pixelfifo_destroy(fetcher->obj_fifo); 
    free(fetcher);
}

GB_ppu_t* ppu_create() {
    GB_ppu_t *ppu                   = (GB_ppu_t*)( malloc( sizeof(GB_ppu_t) ) );

    ppu->oam_buffer                 = oambuffer_create();
    ppu->pixel_fetcher              = pixelfetcher_create();
    ppu->lcd                        = GB_lcd_create();
    ppu->lx                         = 0;
    ppu->pending_cycles             = 0;
    ppu->scanline_dot_counter       = 0;
    ppu->m_ppu_mode_switched        = 1;

    return ppu;
}

void ppu_destroy(GB_ppu_t *ppu) {
    if (ppu == NULL) return;

    oambuffer_destroy(ppu->oam_buffer);
    pixelfetcher_destroy(ppu->pixel_fetcher);
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

    tilemap = LCDC_BG_MAP ? 0x9C00 : 0x9800;
    x       = ( (SCX / 8) + PIXEL_FETCHER->x ) & 0x1F;
    y       = ( LY + SCY ) & 0xFF; 
    offset  = ( x + 32 * ( y / 8 ) );

    PIXEL_FETCHER->tile_id = GB_mem_read(gb, tilemap+offset);
    PIXEL_FETCHER->x++;
    PIXEL_FETCHER->y = y;
}

BYTE pixelfetcher_get_tile_data(GB_gameboy_t *gb, int high) {
    int addr = 0x8000;
    int offset = PIXEL_FETCHER->tile_id;

    if (!LCDC_BG_EN) {
        return 0x00;
    }

    high = high ? 1 : 0;

    if ( !LCDC_TILE_SEL ) {
        addr = 0x9000;
        offset = (SIGNED_BYTE)offset;
    }

    return GB_mem_read(gb, addr + offset*16 + high + 2 * ( ( LY + SCY ) % 8 ) );
}

void pixelfetcher_push(GB_gameboy_t *gb) {
    if (pixelfifo_empty(PIXEL_FETCHER->bg_fifo)) {
        pixelfifo_push_row( PIXEL_FETCHER->bg_fifo, 
                            PIXEL_FETCHER->tile_data_high, 
                            PIXEL_FETCHER->tile_data_low );
        PIXEL_FETCHER->status=0;
    }
}

#define PIXEL_FETCHER_RESET() do {              \
    pixelfifo_clear(PIXEL_FETCHER->bg_fifo);    \
    PIXEL_FETCHER->x = 0;                       \
    PIXEL_FETCHER->status = 0;                  \
} while(0)

void pixel_fetch(GB_gameboy_t *gb) {
    // Advance one step every 2-dot
    if (PENDING_CYCLES < 2) return;
    PENDING_CYCLES -= 2;

    switch (PIXEL_FETCHER->status++) {
        case FETCHER_GET_TILE_ID: 
            pixelfetcher_get_tile_id(gb);
            break;
        case FETCHER_GET_DATA_LOW:
            PIXEL_FETCHER->tile_data_low = pixelfetcher_get_tile_data(gb, 0);
            break;
        case FETCHER_GET_DATA_HIGH:
            PIXEL_FETCHER->tile_data_high = pixelfetcher_get_tile_data(gb, 1);
            break;
        default: /* Try push */
            pixelfetcher_push(gb);
            break;
    }
}

static const char *colors[] = { "█", "▒", "░", " " }; 
void ppu_render(GB_gameboy_t *gb) {
    if (pixelfifo_empty(BG_FIFO)) return;

    BYTE color_id, palette_id, bg_priority;

    pixelfifo_pop(PIXEL_FETCHER->bg_fifo, &color_id, &palette_id, &bg_priority);
    /* TODO: sprite pop */

    if ( LX >= (SCX % 8) ) {
        GB_lcd_set_pixel(gb->ppu->lcd, NB_RENDERED_PIXELS, LY, color_id);
    }

    LX++;
    SCANLINE_DOT_COUNTER++;
}

// Aimed to be called right before exiting mode callback
#define CHECK_SCANLINE_ENDED(if_ended_statement) do {           \
    if (SCANLINE_DOT_COUNTER >= PPU_DOTS_PER_SCANLINE) {        \
        LX = 0;                                                 \
        LY++;                                                   \
        SCANLINE_DOT_COUNTER = 0;                               \
        if_ended_statement;                                     \
    }                                                           \
} while(0)

void ppu_oamsearch(GB_gameboy_t *gb) {
    if ( PPU_MODE_SWITCHED ) {
        SET_STAT(2, LY == LYC);
        if (LY == LYC && LYC_INT) {
            REQUEST_INTERRUPT(IF_LCD);
        }

        if ( MODE2_INT ) REQUEST_INTERRUPT(IF_LCD);
    }

    // SOME CODE ... 

    if (++SCANLINE_DOT_COUNTER >= 80) {
        SET_PPU_MODE(PPU_MODE_DRAW);
    }
}

void ppu_draw(GB_gameboy_t *gb) {
    pixel_fetch(gb);
    ppu_render(gb);

    // Finish drawing if 160 have been drawn or 289 dots consumed
    if ( NB_RENDERED_PIXELS >= 160 || SCANLINE_DOT_COUNTER >= 289 ) {
        PIXEL_FETCHER_RESET();
        SET_PPU_MODE(PPU_MODE_HBLANK);
    }
}

void ppu_hblank(GB_gameboy_t *gb) {
    if ( PPU_MODE_SWITCHED ) {
        if (MODE0_INT) REQUEST_INTERRUPT(IF_LCD);
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
    }

    if ( !SCANLINE_DOT_COUNTER ) {
        SET_STAT(2, LY == LYC);
        if (LY == LYC && LYC_INT) {
            REQUEST_INTERRUPT(IF_LCD);
        }
    }

    CHECK_SCANLINE_ENDED();

    if (LY > MAX_LY) {
        LX = 0;
        LY = 0;
        SCANLINE_DOT_COUNTER = 0;
        SET_PPU_MODE(PPU_MODE_OAMSEARCH);
    }

    SCANLINE_DOT_COUNTER++;
}

void ppu_tick(GB_gameboy_t *gb) {
    if (gb == NULL || gb->ppu == NULL) return;

    for ( int i = 0; i < 4; i++) {
        PENDING_CYCLES++;
        switch (PPU_MODE) {
            case PPU_MODE_HBLANK:       ppu_hblank(gb);     break;  // 87-204 dots
            case PPU_MODE_VBLANK:       ppu_vblank(gb);     break;  // 456 * 10 = 4560 dots
            case PPU_MODE_OAMSEARCH:    ppu_oamsearch(gb);  break;  // 80 dots
            case PPU_MODE_DRAW:         ppu_draw(gb);       break;  // 172-289 dots
        }
    }
}

void ppu_load_sample(GB_gameboy_t *gb) {
    BYTE tiledata[] = { 
        0xFF, 0x00,         // First row of 8-pixels
        0x7E, 0xFF, 
        0x85, 0x81, 
        0x89, 0x83, 
        0x93, 0x85, 
        0xA5, 0x8B, 
        0xC9, 0x97, 
        0x7E, 0xFF 
    };
    BYTE tilemap[1023] = { 0 };

    GB_mem_write(gb, 0xFF40, LCDC | 0x10);

    memcpy((void*)(&gb->memory[0x8000]), (void*)tiledata,  16 * sizeof(BYTE));
    memcpy((void*)(&gb->memory[0x9800]), (void*)tilemap, 1023 * sizeof(BYTE));
}
