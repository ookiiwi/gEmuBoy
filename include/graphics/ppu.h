#ifndef GRAPHIC_H_
#define GRAPHIC_H_

#include "defs.h"
#include "type.h"
#include "lcd.h"

typedef struct OAMBuffer OAMBuffer;
typedef struct PixelFetcher PixelFetcher;

typedef struct {
    BYTE            *vram;
    BYTE            *oam;

    OAMBuffer       *oam_buffer;
    PixelFetcher    *bg_fetcher;
    PixelFetcher    *obj_fetcher;
    int             fetch_obj;
    int             lx;                         /* Current scanline X coordinate */
    int             mode_dot_counters[4];
    int             m_ppu_mode_switched;

    GB_LCD_t        *lcd;
} GB_ppu_t;

GB_ppu_t*   GB_ppu_create(int headless);
void        GB_ppu_destroy(GB_ppu_t *ppu);

void        GB_ppu_tick(GB_gameboy_t *gb, int cycles);

BYTE        GB_ppu_vram_read(GB_gameboy_t *gb, WORD addr);
void        GB_ppu_vram_write(GB_gameboy_t *gb, WORD addr, BYTE data);

BYTE        GB_ppu_oam_read(GB_ppu_t *ppu, WORD addr);
void        GB_ppu_oam_write(GB_ppu_t *ppu, WORD addr, BYTE data);

void        GB_ppu_print_state(GB_gameboy_t *gb);

#endif

