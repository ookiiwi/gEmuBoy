#ifndef GB_OAMBUFFER_H_
#define GB_OAMBUFFER_H_

#include "graphics/internal/fifo.h"
#include "memmap.h"
#include "type.h" /* BYTE */

#include <assert.h>
#include <stdio.h>  /* fprintf, stderr */

#define OAMBUFFER_SIZE (10)

static int oambuffer_err = 0;

typedef struct {
    BYTE y;
    BYTE x;
    BYTE tile_idx;
    BYTE priority    : 1;
    BYTE flip_y      : 1;
    BYTE flip_x      : 1;
    BYTE palette     : 1;
} oam_obj_t;

FIFO_DEFINE_STRUCT(_oambuffer_t, oam_obj_t, OAMBUFFER_SIZE);

typedef struct {
    _oambuffer_t buffer;
    int cur_oam_addr;
    int locked;
} oambuffer_t;

#define oam_get_obj_at(ppu, addr, res) do {                                             \
    BYTE attributes = GB_ppu_oam_read(ppu, addr+3);                                     \
    (res)->y        = GB_ppu_oam_read(ppu, addr);                                       \
    (res)->x        = GB_ppu_oam_read(ppu, addr+1);                                     \
    (res)->tile_idx = GB_ppu_oam_read(ppu, addr+2);                                     \
    (res)->palette  = (attributes >>= 4) & 1;                                           \
    (res)->flip_x   = (attributes >>= 1) & 1;                                           \
    (res)->flip_y   = (attributes >>= 1) & 1;                                           \
    (res)->priority = (attributes >>= 1) & 1;                                           \
} while (0)

static inline void oambuffer_push(oambuffer_t *buf, oam_obj_t *obj) {
    if (buf->locked) {
        fprintf(stderr, "OAM BUFFER LOCKED\n");
        return;
    }

    fifo_push(&buf->buffer, *obj);
}

static inline oam_obj_t oambuffer_pop(oambuffer_t *buf) {
    _oambuffer_t *fifo = &(buf->buffer);
    oam_obj_t *obj = NULL;

    fifo_peek(fifo, obj);
    fifo_pop(fifo);

    // TODO: check errors

    return *obj;
}

#define oambuffer_peek(buf, res) do {                                                   \
    _oambuffer_t *fifo = &(buf->buffer);                                                \
    fifo_peek(fifo, res);                                                               \
} while (0)

#define oambuffer_size(buf) ( buf->buffer.size )

static inline void oambuffer_clear(oambuffer_t *buf) {
    oam_obj_t *obj = NULL;

    buf->cur_oam_addr = GB_OAM_BEG_ADDR;
    buf->locked = 0;

    while (buf->buffer.size) {
       oambuffer_pop(buf); 
    } 

    buf->buffer.start = 0;
}

#define oambuffer_init(buf) do {                                                        \
    if ((buf)) {                                                                        \
        fifo_init(&(buf)->buffer, OAMBUFFER_SIZE);                                      \
        oambuffer_clear((buf));                                                         \
    }                                                                                   \
} while (0)

#define oambuffer_free(buf) do {                                                        \
    oambuffer_clear(buf);                                                               \
    fifo_free(&(buf)->buffer);                                                          \
} while (0)

#define _oambuf_sort(fifo) do {                                                         \
    size_t i = 1, j;                                                                    \
    oam_obj_t tmp;                                                                      \
    while (i < (fifo).size) {                                                           \
        j = i;                                                                          \
        while (j > 0 && (fifo).arr[j-1].x > (fifo).arr[j].x) {                          \
            tmp = (fifo).arr[j];                                                        \
            (fifo).arr[j] = (fifo).arr[j-1];                                            \
            (fifo).arr[j-1] = tmp;                                                      \
            j--;                                                                        \
        }                                                                               \
        i++;                                                                            \
    }                                                                                   \
} while (0)

#define oambuffer_lock(oambuf) do {                                                     \
    assert((oambuf)->buffer.start == 0);                                                \
    (oambuf)->locked = 1;                                                               \
    _oambuf_sort((oambuf)->buffer);                                                     \
} while (0)

#endif
