#include "graphics/internal/pixelfetcher.h"
#include "gb.h" 
#include "graphics/internal/oambuffer.h"
#include "graphics/internal/pixelfifo.h"
#include "graphics/internal/ppu_defs.h"
#include "graphics/internal/ppu_mem_access.h"

#include <assert.h>

#define _fetcher    ( & ( gb->ppu->pixelfetcher ) )
#define _oambuffer  ( & ( gb->ppu->oambuffer ) )

#define is_fetching_objects()  (_fetcher->fetching_obj != 0 )

static WORD compute_bg_tile_id_addr(GB_gameboy_t *gb) {
    unsigned tilemap;
    unsigned x, y;
    unsigned offset;

    if (SHOW_WIN) {
        tilemap =  LCDC_WIN_MAP ? 0x9C00 : 0x9800;
        x       = _fetcher->x & 0x1F;
        y       = _fetcher->window_line_counter;
    } else {
        tilemap = LCDC_BG_MAP ? 0x9C00 : 0x9800;
        x       = ( ( SCX / 8 ) + _fetcher->x ) & 0x1F;
        y       = ( rLY + SCY ) & 0xFF; 
    }


    _fetcher->x++;

    offset  = ( x + 32 * ( y / 8 ) );
    return tilemap+offset;
}

static inline WORD compute_tile_data_addr(GB_gameboy_t *gb) {
    int addr = 0x8000;
    int tile_id = _fetcher->tile_id;
    int offset  = (rLY + SCY) % 8;

    if (is_fetching_objects()) {
        if (_fetcher->oam_obj->flip_y) {
            offset = 7 - offset;
        }
    } else {
        if (SHOW_WIN) {
            offset = _fetcher->window_line_counter%8;
        }

        if ( !LCDC_TILE_SEL) {
            addr = 0x9000;
            tile_id = (SIGNED_BYTE)tile_id;
        }
    }
    
    return addr + tile_id*16 + 2 * offset;
}

static inline void adjust_obj_tile_id(GB_gameboy_t *gb) {
    if (LCDC_OBJ_SIZE) {
        // Check if fetch new 8x16 sprite
        if (_fetcher->sprite_tall_ly_start < 0) {
            _fetcher->sprite_tall_ly_start = rLY;
        }
        
        int line_diff = rLY - _fetcher->sprite_tall_ly_start;
        int cur_tile = line_diff / 8; // 0 for top tile and 1 for bottom tile

        if (_fetcher->oam_obj->flip_y) cur_tile = !cur_tile;
        _fetcher->tile_id = ( _fetcher->tile_id & 0xFE ) | cur_tile;

        // 8x16 sprite's end
        if (line_diff > 15) {
            _fetcher->sprite_tall_ly_start = -1;
        }
    }

}

static inline void push_tile_row(GB_gameboy_t *gb) {
    oam_obj_t obj = (oam_obj_t){ 0 };;
    int overlap_offset = 0;
    WORD data; 
    BYTE data_low; 
    BYTE data_high;

    data = ( (_fetcher->tile_data_low << 8) | _fetcher->tile_data_high );

    if (is_fetching_objects()) {
        obj = *(_fetcher->oam_obj);
        int objx_start = obj.x-8;
        
        if (obj.x<8) {
            data <<= (8-obj.x);  // (8-x)*2 ?
            objx_start = 0;
        }

        if (objx_start < _fetcher->last_sprite_x_end) {
            overlap_offset = _fetcher->last_sprite_x_end - objx_start;
        }
        
        _fetcher->last_sprite_x_end = obj.x;
    }

    data_low  = data >> 8;
    data_high = data & 0xFF ;

    /* Try push */
    pixelfifo_push_row(
        _fetcher->fifo, 
        data_high,
        data_low, 
        obj.palette,
        obj.priority,
        obj.flip_x,
        overlap_offset
    ); 

    _fetcher->state = 0;

    if (is_fetching_objects()) {
        _fetcher->fifo = &_fetcher->bg_fifo;
        _fetcher->fetching_obj = 0;
        _fetcher->oam_obj = NULL;
    }
}

static void pixelfetcher_step(GB_gameboy_t *gb) {
    switch (_fetcher->state++) {
        case PF_TILE_ID_T1: 
            // compute address
            if (!is_fetching_objects()) {
                _fetcher->tile_id_addr = compute_bg_tile_id_addr(gb);
            }
            break;

        case PF_TILE_ID_T2:
            // read tile id
            if (is_fetching_objects()) {
                _fetcher->tile_id = _fetcher->oam_obj->tile_idx;
                adjust_obj_tile_id(gb);
            } else {
                vram_read(gb->ppu, _fetcher->tile_id_addr, &_fetcher->tile_id);
            }

            break;

        case PF_TILE_DATA_LO_T1:
            _fetcher->tile_data_addr = compute_tile_data_addr(gb);
            break;

        case PF_TILE_DATA_LO_T2:
            vram_read(gb->ppu, _fetcher->tile_data_addr, &_fetcher->tile_data_low);
            break;

        case PF_TILE_DATA_HI_T1:
            _fetcher->tile_data_addr = compute_tile_data_addr(gb)+1;
            break;
        
        case PF_TILE_DATA_HI_T2:
            vram_read(gb->ppu, _fetcher->tile_data_addr, &_fetcher->tile_data_high);
            break;

        case PF_IDLE:
            break;

        default: 
        case PF_PUSH:
            if ( is_fetching_objects() || _fetcher->bg_fifo.size <= 0 ) {
                push_tile_row(gb); 
            }

            break;
    }
}

static void check_for_obj_to_fetch(GB_gameboy_t *gb) {
    // We don't need to check for obj if we are already fetching one 
    if ( _fetcher->oam_obj ) return;

    oam_obj_t *obj = NULL;
    oambuffer_peek(_oambuffer, obj);

    if (obj && (obj->x-8) <= rLX) {
        _fetcher->oam_obj = obj;
        oambuffer_pop(_oambuffer);
    }
}

void pixelfetcher_fetch(GB_gameboy_t *gb) {
    // B01
    // B01S *

    if (_fetcher->dummy_fetch_cnt_down) {
        _fetcher->dummy_fetch_cnt_down--;
        return;
    }
    
    pixelfetcher_step(gb);
    check_for_obj_to_fetch(gb);

    if (_fetcher->bg_fifo.size && !is_fetching_objects() && _fetcher->oam_obj) {
        if (_fetcher->state > PF_PUSH) {
            push_tile_row(gb); // state reset to 0 here
        }

        if (_fetcher->state == PF_TILE_ID_T1) {
            _fetcher->fetching_obj = 1;
            _fetcher->fifo = &_fetcher->obj_fifo;

            assert(_fetcher->oam_obj != NULL);
            assert(is_fetching_objects());

            pixelfetcher_step(gb); // first sprite fetch overlaps bg fetch
        }
    }
}
