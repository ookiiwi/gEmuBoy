#ifndef GRAPHIC_H_
#define GRAPHIC_H_

#include "defs.h"
#include "lcd.h"

typedef struct OAMBuffer OAMBuffer;
typedef struct PixelFetcher PixelFetcher;

typedef struct {
    OAMBuffer       *oam_buffer;
    PixelFetcher    *pixel_fetcher;
    int             lx;                         /* Current scanline X coordinate */
    int             pending_cycles;
    int             scanline_dot_counter;
    int             m_ppu_mode_switched;

    GB_LCD_t        *lcd;
} GB_ppu_t;

GB_ppu_t* ppu_create();
void ppu_destroy(GB_ppu_t *ppu);

void ppu_tick(GB_gameboy_t *gb);

void ppu_load_sample(GB_gameboy_t *gb);

#endif