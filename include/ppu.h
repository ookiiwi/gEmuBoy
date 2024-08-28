#ifndef GRAPHIC_H_
#define GRAPHIC_H_

#include "defs.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 *  One pixel of this FIFO is represented by a color ID and a color palette ID 
 */

typedef struct OAMBuffer OAMBuffer;
typedef struct PixelFetcher PixelFetcher;

typedef struct {
    OAMBuffer       *oam_buffer;
    PixelFetcher    *pixel_fetcher;
    int             lx;                         /* Current scanline X coordinate */
    int             pending_cycles;
    int             scanline_dot_counter;
} GB_ppu_t;

GB_ppu_t* ppu_create();
void ppu_destroy(GB_ppu_t *ppu);

void ppu_tick(GB_gameboy_t *gb);

void ppu_load_sample(GB_gameboy_t *gb);

#endif