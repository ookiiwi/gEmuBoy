#include "debugger.h"
#include "gb.h"
#include "mmu.h"
#include "win_utils.h"

#include <SDL.h>
#include <time.h>

#define VRAMVIEWER_VIEWPORT_HEIGHT ( 8*8*3 )
#define VRAMVIEWER_VIEWPORT_WIDTH  ( 8*16 )
#define VRAMVIEWER_WINDOW_HEIGHT   ( VRAMVIEWER_VIEWPORT_HEIGHT * 2 )
#define VRAMVIEWER_WINDOW_WIDTH    ( VRAMVIEWER_VIEWPORT_WIDTH  * 2 )

#define MAPVIEWER_VIEWPORT_HEIGHT ( 8 * 32)
#define MAPVIEWER_VIEWPORT_WIDTH  ( 8 * 32 )
#define MAPVIEWER_WINDOW_HEIGHT   ( MAPVIEWER_VIEWPORT_HEIGHT * 2 )
#define MAPVIEWER_WINDOW_WIDTH    ( MAPVIEWER_VIEWPORT_WIDTH  * 2 )

typedef struct {
    GB_window_t *vram_viewer;
    GB_window_t *map_viewer;
} GB_ppu_debugger_t;

struct GB_debugger_s {
    GB_gameboy_t        *gb;
    GB_ppu_debugger_t   *ppu_debugger;
};

void GB_ppu_debugger_destroy(GB_ppu_debugger_t *debugger);

GB_ppu_debugger_t* GB_ppu_debugger_create() {
    GB_ppu_debugger_t *debugger = (GB_ppu_debugger_t*)( malloc( sizeof(GB_ppu_debugger_t) ) );

    if (debugger == NULL) {
        return NULL;
    }

    debugger->vram_viewer = GB_window_create( "VRAM Viewer", 
                                                  VRAMVIEWER_WINDOW_WIDTH, 
                                                  VRAMVIEWER_WINDOW_HEIGHT, 
                                                  VRAMVIEWER_VIEWPORT_WIDTH, 
                                                  VRAMVIEWER_VIEWPORT_HEIGHT );

    if (debugger->vram_viewer == NULL) {
        return NULL;
    }

    debugger->map_viewer  = GB_window_create( "Map Viewer", 
                                                  MAPVIEWER_WINDOW_WIDTH, 
                                                  MAPVIEWER_WINDOW_HEIGHT, 
                                                  MAPVIEWER_VIEWPORT_WIDTH, 
                                                  MAPVIEWER_VIEWPORT_HEIGHT );

    if (debugger->map_viewer == NULL) {
        return NULL;
    }

    return debugger;
}

void GB_ppu_debugger_destroy(GB_ppu_debugger_t *debugger) {
    if (debugger == NULL) return;

    GB_window_destroy(debugger->vram_viewer); 
    GB_window_destroy(debugger->map_viewer); 

    SDL_Quit();

    free(debugger);
}

static const int gb_colors[] = { 255, 211, 169, 0 };
void ppu_debugger_get_pixels(GB_debugger_t *debugger) {
    for (int y = 0; y < VRAMVIEWER_VIEWPORT_HEIGHT; y++) {
        for (int x = 0; x < VRAMVIEWER_VIEWPORT_WIDTH/8; x++) {
            int addr    = 0x8000 + x*16 + y*2;
            int low     = debugger->gb->memory[addr];
            int high    = debugger->gb->memory[addr+1];

            for (int k = 0; k < 8; k++) {
                int data = ( ( high & 0x80 ) >> 6 ) | ( ( low & 0x80 ) >> 7 );
                int color = gb_colors[data];
                int pixelpos = x*8+k+y*VRAMVIEWER_VIEWPORT_WIDTH;
                debugger->ppu_debugger->vram_viewer->pixels[pixelpos] = ( (color << 24)|(color << 16)|(color << 8)|0x000000FF );
                high <<= 1;
                low  <<= 1;
            }
        }
    }
}

void ppu_debugger_get_tile(GB_debugger_t *debugger, int addr) {
    int x = ( ( addr & 0x00FF ) / 0x010 ) * 8;
    int y = ( ( addr & 0x1FFF ) / 0x100 ) * 8;

    for (int ly = 0; ly < 8; ly++) {
        int low  = debugger->gb->memory[addr++];
        int high = debugger->gb->memory[addr++];

        for (int lx = 0; lx < 8; lx++) {
            int data = ( ( high & 0x80 ) >> 6 ) | ( ( low & 0x80 ) >> 7 );
            int color = gb_colors[data];

            int pixelpos = ( x+lx ) + ( y+ly ) * VRAMVIEWER_VIEWPORT_WIDTH;
            debugger->ppu_debugger->vram_viewer->pixels[pixelpos] = ( (color << 24)|(color << 16)|(color << 8)|0x000000FF );

            high <<= 1;
            low  <<= 1;
        }
    }
}

void ppu_debugger_get_map(GB_debugger_t *debugger) {
    int map_start = 0x9C00;
    int map_end = map_start + 0x3FF;
    int data_base = 0x8000;
    for ( int map = map_start; map <= map_end; map++) {
        int tile_id = GB_mem_read(debugger->gb, map);
        if (data_base == 0x9000) tile_id = (SIGNED_BYTE)tile_id;
        int addr = data_base + tile_id * 16;

        int x = ( ( map - map_start ) % 0x20 ) * 8;
        int y = ( ( map - map_start ) / 0x20 ) * 8;

        for (int ly = 0; ly < 8; ly++) {
            int low  = debugger->gb->memory[addr++];
            int high = debugger->gb->memory[addr++];

            for (int lx = 0; lx < 8; lx++) {
                int data = ( ( high & 0x80 ) >> 6 ) | ( ( low & 0x80 ) >> 7 );
                int color = gb_colors[data];

                int pixelpos = ( x+lx ) + ( y+ly ) * MAPVIEWER_VIEWPORT_WIDTH;
                debugger->ppu_debugger->map_viewer->pixels[pixelpos] = ( (color << 24)|(color << 16)|(color << 8)|0x000000FF );

                high <<= 1;
                low  <<= 1;
            }
        }
    }
}

void GB_ppu_debugger_run(GB_debugger_t *debugger) {
    Uint32 *pixels;
    int     pitch;

    if (debugger == NULL) return;
    if ( ( debugger->gb->memory[0xFF41] & 3 ) != 1 ) return;

    for (int i = 0; i < 0x9800; i+=16) {
        ppu_debugger_get_tile(debugger, i);
    }

    ppu_debugger_get_map(debugger);

    GB_window_update_texture(debugger->ppu_debugger->vram_viewer);
    GB_window_update_texture(debugger->ppu_debugger->map_viewer);

    GB_window_render(debugger->ppu_debugger->vram_viewer);
    GB_window_render(debugger->ppu_debugger->map_viewer);
}

GB_debugger_t* GB_debugger_create(GB_gameboy_t *gb) {
    GB_debugger_t *debugger = (GB_debugger_t*)( malloc(sizeof(GB_debugger_t)) );

    if (debugger == NULL) return NULL;

    debugger->gb            = gb;
#ifndef NO_PPU
    debugger->ppu_debugger  = GB_ppu_debugger_create();

    if (debugger->ppu_debugger == NULL) return NULL;
#endif
    return debugger;
}

void GB_debugger_destroy(GB_debugger_t *debugger) {
    if (debugger == NULL) return;

#ifndef NO_PPU
    GB_ppu_debugger_destroy(debugger->ppu_debugger);
#endif
    free(debugger);
}

void GB_debugger_run(GB_debugger_t *debugger) {
    if (debugger == NULL) return;
#ifndef NO_PPU
    GB_ppu_debugger_run(debugger);
#endif
}

void GB_debugger_show(GB_debugger_t *debugger) {
    SDL_ShowWindow(debugger->ppu_debugger->vram_viewer->window);
    SDL_ShowWindow(debugger->ppu_debugger->map_viewer->window);
}

void GB_debugger_hide(GB_debugger_t *debugger) {
    SDL_HideWindow(debugger->ppu_debugger->vram_viewer->window);
    SDL_HideWindow(debugger->ppu_debugger->map_viewer->window);
}
