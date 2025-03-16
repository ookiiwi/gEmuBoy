#include "graphics/ppu.h"
#include "graphics/internal/oambuffer.h"
#include "graphics/internal/pixelfetcher.h"
#include "graphics/internal/pixelfifo.h"
#include "graphics/internal/ppu_defs.h"
#include "graphics/internal/ppu_mem_access.h"
#include "mmu.h"
#include "gb.h"
#include "cpu/interrupt.h"
#include "memmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

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
    DOT_PER_MODE_COUNTER = -1;                                  \
} while(0)

#define PPU_MODE_SWITCHED                           ( gb->ppu->m_ppu_mode_switched + ( gb->ppu->m_ppu_mode_switched = 0) )

#define LX                                          ( gb->ppu->lx                       )
#define SCANLINE_DOT_COUNTER                        ( gb->ppu->scanline_dot_counter     )
#define DOT_PER_MODE_COUNTER                        ( gb->ppu->mode_dot_counters[PPU_MODE] )

#define OAMBUFFER                                   ( &( gb->ppu->oambuffer ) )
#define _fetcher                                    ( &( gb->ppu->pixelfetcher) )

// The first four steps take 2 dots each.
// The fifth is tries to push pixels every dot until it succeeds
#define FETCHER_OBJ_PENALTY_ZONE    (0)
#define FETCHER_GET_TILE_ID         (1)
#define FETCHER_GET_DATA_LOW        (FETCHER_GET_TILE_ID + 2)
#define FETCHER_GET_DATA_HIGH       (FETCHER_GET_DATA_LOW + 2)
#define FETCHER_IDLE                (FETCHER_GET_DATA_HIGH + 2)
#define FETCHER_PUSH                (FETCHER_IDLE + 1)

#define PPU_DOTS_PER_SCANLINE   (456)
#define MAX_LY                  (153)
#define VBLANK_LY_START         (143)

#define NB_RENDERED_PIXELS      ( LX - (SCX % 8) )

// Default values
#define PPU_MODE_SWITCHED_DEFAULT       (1)

#define _INT_TO_ENABLE_DISABLE_STR(n) (n ? "enabled" : "disabled")

void GB_ppu_print_state(GB_gameboy_t *gb) {
    printf(
        "LCDC:\n"                           \
        "\tLCD: %s\n"                       \
        "\tWindow tilemap: $%04X\n"         \
        "\tWindow: %s\n"                    \
        "\tBG & Window tileset: $%04X\n"    \
        "\tBG tilemap: $%04X\n"             \
        "\tOBJ size: %s\n"                  \
        "\tOBJ: %s\n"                       \
        "\tBG & Window: %s\n",
        _INT_TO_ENABLE_DISABLE_STR(LCDC_LCD_EN),
        LCDC_WIN_MAP ? 0x9c00 : 0x9800,
        _INT_TO_ENABLE_DISABLE_STR(LCDC_WIN_EN),
        LCDC_TILE_SEL ? 0x8000 : 0x8800,
        LCDC_BG_MAP ? 0x9c00 : 0x9800,
        LCDC_OBJ_SIZE ? "8x16" : "8x8",
        _INT_TO_ENABLE_DISABLE_STR(LCDC_OBJ_EN),
        _INT_TO_ENABLE_DISABLE_STR(LCDC_BG_EN)

    );

    printf(
        "STAT:\n"                           \
        "\tLYC interrupt: %s\n"             \
        "\tMode 2 interrupt: %s\n"          \
        "\tMode 1 interrupt: %s\n"          \
        "\tMode 0 interrupt: %s\n"          \
        "\tLYC flag: %s\n"                  \
        "\tPPU mode: Mode %d\n",
        _INT_TO_ENABLE_DISABLE_STR(LYC_INT),
        _INT_TO_ENABLE_DISABLE_STR(MODE2_INT),
        _INT_TO_ENABLE_DISABLE_STR(MODE1_INT),
        _INT_TO_ENABLE_DISABLE_STR(MODE0_INT),
        LYC_LY ? "On" : "Off",
        PPU_MODE
    );

    printf(
        "PPU state\n"                       \
        "\tDots: %d\n"                      \
        "\tLY: %d\n"                        \
        "\tLX: %d\n"                        \
        "\tBG fetcher size: %zu\n"           \
        "\tBG fetcher status: %d\n",        \
        DOT_PER_MODE_COUNTER,
        LY,
        LX,
        gb->ppu->pixelfetcher.fifo->size,
        gb->ppu->pixelfetcher.state
    );
}

BYTE GB_ppu_vram_read(GB_gameboy_t *gb, WORD addr) {
    BYTE res;
    vram_read(gb->ppu, addr, &res);
    return res;
}

void GB_ppu_vram_write(GB_gameboy_t *gb, WORD addr, BYTE data) {
    vram_write(gb->ppu, addr, data);

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

GB_ppu_t* GB_ppu_create(int headless) {
    GB_ppu_t *ppu = (GB_ppu_t*)( malloc( sizeof(GB_ppu_t) ) );

    if (!ppu) {
        return NULL;
    }

    ppu->vram = (BYTE*)( calloc( 0x2000+1, sizeof (BYTE) ) );
    if (!ppu->vram) {
        free(ppu);
        return NULL;
    }

    ppu->oam = (BYTE*)( calloc( 161, sizeof (BYTE) ) );
    if (!ppu->oam) {
        free(ppu->vram);
        free(ppu);
        return NULL;
    }

    oambuffer_init(&ppu->oambuffer);
    pixelfetcher_init(&ppu->pixelfetcher);

    ppu->obj_penalty_checked_tile   = -1;
    ppu->obj_fetch_penalty          = 0;
    ppu->lcd                        = headless ? NULL : GB_lcd_create();
    ppu->lx                         = 0;
    ppu->m_ppu_mode_switched        = PPU_MODE_SWITCHED_DEFAULT;
    ppu->mode_dot_counters[0]       = 0;
    ppu->mode_dot_counters[1]       = 0;
    ppu->mode_dot_counters[2]       = 79;
    ppu->mode_dot_counters[3]       = 288;

    if ( (!headless && ppu->lcd == NULL) || oambuffer_err || pixelfetcher_err ) {
        GB_ppu_destroy(ppu);
        return NULL;
    }

    return ppu;
}

void GB_ppu_destroy(GB_ppu_t *ppu) {
    if (ppu == NULL) return;

    oambuffer_free(&ppu->oambuffer);
    pixelfetcher_free(&ppu->pixelfetcher);
    GB_lcd_destroy(ppu->lcd);

    free(ppu->oam);
    free(ppu->vram);
    free(ppu);
}

void ppu_render(GB_gameboy_t *gb) {    
    assert(_fetcher->bg_fifo.size);

    BYTE color_id, bg_priority;
    BYTE palette, color_index;

    pixelfifo_pop(&_fetcher->bg_fifo, &color_id, NULL, &bg_priority);
    palette = BGP;

    if (!LCDC_BG_EN) {
        color_id = 0;
        bg_priority = 0;
    }
    
    // TODO: bg_priority
    if (_fetcher->obj_fifo.size > 0) {
        BYTE cid, pid, bgp;
        pixelfifo_pop(&_fetcher->obj_fifo, &cid, &pid, &bgp);

        if ( LCDC_OBJ_EN && cid && !( bgp && color_id ) ) {
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
}

void ppu_oamsearch(GB_gameboy_t *gb) {
    if ( PPU_MODE_SWITCHED ) {
        oambuffer_clear(OAMBUFFER);

        SET_STAT(2, LY == LYC);
        if (LY == LYC && LYC_INT) {
            REQUEST_INTERRUPT(IF_LCD);
        }

        if ( MODE2_INT ) REQUEST_INTERRUPT(IF_LCD);
    }

    if (LCDC_OBJ_EN && oambuffer_size(OAMBUFFER) < OAMBUFFER_SIZE && OAMBUFFER->cur_oam_addr < GB_OAM_END_ADDR && (DOT_PER_MODE_COUNTER&1)) {
        oam_obj_t obj; 
        oam_get_obj_at(gb->ppu, OAMBUFFER->cur_oam_addr, &obj);

        if (obj.x >= 0 && 
            ( LY + 16 ) >= obj.y &&
            ( LY + 16 ) < ( obj.y + (LCDC_OBJ_SIZE+1) * 8 ) ) {
            oambuffer_push(OAMBUFFER, &obj);
        }

        OAMBUFFER->cur_oam_addr+=4; // Note: Skip 4 bytes meta-data
    }

    if (DOT_PER_MODE_COUNTER >= 79) {
        oambuffer_lock(OAMBUFFER);
        SET_PPU_MODE(PPU_MODE_DRAW);
    }
}

void ppu_draw(GB_gameboy_t *gb) {
    if (PPU_MODE_SWITCHED) {
        pixelfetcher_reset_all(_fetcher);
        gb->ppu->obj_penalty_checked_tile = -1;
    }

    pixelfetcher_fetch(gb);

    // TODO: if not fetching sprite and bg fifo not empty
    if ((!_fetcher->oam_obj || _fetcher->obj_fifo.size) && _fetcher->bg_fifo.size) {
        ppu_render(gb);
    }

    // Finish drawing if 160 have been drawn or 289 dots consumed
    if ( NB_RENDERED_PIXELS >= 160 || DOT_PER_MODE_COUNTER >= 288 ) {
        if (! (DOT_PER_MODE_COUNTER >= 173 && DOT_PER_MODE_COUNTER < 289) ) printf("DRAW TIMING WRONG EXPECTED 171 <= %d < 289 %d\n", DOT_PER_MODE_COUNTER, NB_RENDERED_PIXELS);

        LX = 0;
        SET_PPU_MODE(PPU_MODE_HBLANK);
    }

    if (!_fetcher->draw_window && SHOW_WIN && WY <= LY && WX-7 <= 160 && !_fetcher->oam_obj) {
        pixelfetcher_reset_bg_fifo(_fetcher);
        _fetcher->draw_window = 1;
        _fetcher->window_line_counter++;
    }
}

void ppu_hblank(GB_gameboy_t *gb) {
    if ( PPU_MODE_SWITCHED ) {
        if (MODE0_INT) REQUEST_INTERRUPT(IF_LCD);
        _fetcher->draw_window = 0;
    }

    int tmp = gb->ppu->mode_dot_counters[2] + gb->ppu->mode_dot_counters[3] + gb->ppu->mode_dot_counters[0]; 
    if ( DOT_PER_MODE_COUNTER >= (374-gb->ppu->mode_dot_counters[3]) ) {
        int mode = LY >= VBLANK_LY_START ? PPU_MODE_VBLANK : PPU_MODE_OAMSEARCH;

        if (! (DOT_PER_MODE_COUNTER >= 86 &&  DOT_PER_MODE_COUNTER < 204) ) printf("HBLANK TIMING WRONG EXPECTED 87 <= %d < 204 %d\n", DOT_PER_MODE_COUNTER, gb->ppu->mode_dot_counters[3]);
        if ( tmp != 453 ) printf("SCANLINE WRONG TIMING: %d -- 2: %d, 3: %d, 0: %d\n", tmp, gb->ppu->mode_dot_counters[2] , gb->ppu->mode_dot_counters[3] , gb->ppu->mode_dot_counters[0]);

        LY++;
        SET_PPU_MODE(mode);
    }
}

void ppu_vblank(GB_gameboy_t *gb) {
    // VBLANK start
    if ( PPU_MODE_SWITCHED ) {
        REQUEST_INTERRUPT(IF_VBLANK);

        if (MODE1_INT || MODE2_INT) {
            REQUEST_INTERRUPT(IF_LCD);
        }

        GB_lcd_clear(gb->ppu->lcd);
        GB_lcd_render(gb->ppu->lcd);

        _fetcher->window_line_counter = WINDOW_LINE_COUNTER_DEFAULT;
    }

    // Scanline start
    if ( !DOT_PER_MODE_COUNTER ) {
        SET_STAT(2, LY == LYC);
        if (LY == LYC && LYC_INT) {
            REQUEST_INTERRUPT(IF_LCD);
        }
    } else if ( DOT_PER_MODE_COUNTER >= 455 ) {
        LY = (LY+1)%154;

        DOT_PER_MODE_COUNTER = -1;

        if (!LY) {
            SET_PPU_MODE(PPU_MODE_OAMSEARCH);
        }
    }
}

void GB_ppu_tick(GB_gameboy_t *gb, int cycles) {
    if (!LCDC_LCD_EN || gb == NULL || gb->ppu == NULL) return;

    for ( int i = 0; i < 4; i++) {
        switch (PPU_MODE) {
            case PPU_MODE_HBLANK:       ppu_hblank(gb);     break;  // 87-204 dots
            case PPU_MODE_VBLANK:       ppu_vblank(gb);     break;  // 456 * 10 = 4560 dots
            case PPU_MODE_OAMSEARCH:    ppu_oamsearch(gb);  break;  // 80 dots
            case PPU_MODE_DRAW:         ppu_draw(gb);       break;  // 172-289 dots
        }

        DOT_PER_MODE_COUNTER++;
    }
}
