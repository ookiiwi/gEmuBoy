#ifndef GB_PIXEL_FIFO_H_
#define GB_PIXEL_FIFO_H_

#include "graphics/internal/fifo.h"
#include "type.h"

#include <stdlib.h> /* malloc */

#define PIXELFIFO_CAPACITY (16)

typedef struct {
    BYTE color_id;
    BYTE palette_id;
    BYTE bg_priority;
} pixelfifo_cell_t;

FIFO_DEFINE_STRUCT(pixelfifo_t, pixelfifo_cell_t, PIXELFIFO_CAPACITY);

#define pixelfifo_init(fifo) do { fifo_init(fifo, PIXELFIFO_CAPACITY); } while (0)

#define pixelfifo_clear(fifo) do {                                          \
    while ((fifo)->size) fifo_pop(fifo);                                    \
} while (0)

#define pixelfifo_free(fifo) do {                                           \
    pixelfifo_clear(fifo);                                                  \
    fifo_free(fifo);                                                        \
} while (0)

static inline void pixelfifo_push(pixelfifo_t *fifo, BYTE color_id, BYTE palette_id, BYTE bg_priority) { 
    pixelfifo_cell_t cell;
    cell.color_id      = color_id;
    cell.palette_id    = palette_id;
    cell.bg_priority   = bg_priority;

    fifo_push(fifo, cell);
}

static inline void pixelfifo_overlap(pixelfifo_t *fifo, BYTE color_id, BYTE palette_id, BYTE bg_priority, unsigned offset) {
    if ( fifo->size >= 16 || offset >= fifo->size ) return;

    pixelfifo_cell_t cell;
    cell.color_id      = color_id;
    cell.palette_id    = palette_id;
    cell.bg_priority   = bg_priority;

    int index = ( fifo->start + fifo->size - offset ) % PIXELFIFO_CAPACITY;
    pixelfifo_cell_t *tmp = fifo->arr + index;

    // Replace current cell if transparent
    if (!tmp || tmp->color_id == 0) {
        fifo->arr[index] = cell;
    }
}

/// Merge [high] and [low] to form a row of 8 pixels and push them into the fifo
static inline void pixelfifo_push_row(pixelfifo_t *fifo, BYTE high, BYTE low, BYTE palette_id, BYTE bg_priority, int flip_x, unsigned overlap_offset) {
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

static inline int pixelfifo_pop(pixelfifo_t *fifo, BYTE *color_id, BYTE *palette_id, BYTE *bg_priority) {
    pixelfifo_cell_t *cell = NULL;
    fifo_peek(fifo, cell);

    if (!cell) return -1;
    
    if (color_id)       *color_id       = cell->color_id;
    if (palette_id)     *palette_id     = cell->palette_id;
    if (bg_priority)    *bg_priority    = cell->bg_priority;
     
    fifo_pop(fifo);

    return 0;
}

#endif
