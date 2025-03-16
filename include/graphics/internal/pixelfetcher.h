#ifndef GB_PIXEL_FETCHER_H_
#define GB_PIXEL_FETCHER_H_

#include "defs.h" /* GB_gameboy_t */
#include "graphics/internal/oambuffer.h"
#include "graphics/internal/pixelfifo.h"
#include "graphics/internal/ppu_defs.h"
#include "type.h" /* BYTE */

static int pixelfetcher_err = 0;

typedef enum {
    PF_TILE_ID_T1,
    PF_TILE_ID_T2,
    PF_TILE_DATA_LO_T1,
    PF_TILE_DATA_LO_T2,
    PF_TILE_DATA_HI_T1,
    PF_TILE_DATA_HI_T2,
    PF_IDLE,
    PF_PUSH
} pf_state;

typedef struct {
    int             dummy_fetch_cnt_down;
    int             fetching_obj;
    int             x;
    int             y;
    WORD            tile_id_addr;
    WORD            tile_data_addr;
    BYTE            tile_id;
    BYTE            tile_data_high;
    BYTE            tile_data_low;
    pf_state        state;
    pixelfifo_t     bg_fifo;
    pixelfifo_t     obj_fifo;
    pixelfifo_t     *fifo; // current fifo
    oam_obj_t       *oam_obj;

    int             draw_window;
    int             window_line_counter;

    int             sprite_tall_ly_start;
    int             last_sprite_x_end;
} pixelfetcher_t;

#define pixelfetcher_reset_bg_fifo(fetcher) do {                                    \
    pixelfifo_clear(&(fetcher)->bg_fifo);                                           \
    (fetcher)->fetching_obj             = 0;                                        \
    (fetcher)->x                        = 0;                                        \
    (fetcher)->y                        = 0;                                        \
    (fetcher)->state                    = PF_TILE_ID_T1;                            \
    (fetcher)->tile_id                  = 0;                                        \
    (fetcher)->tile_data_low            = 0;                                        \
    (fetcher)->tile_data_high           = 0;                                        \
} while (0)

#define pixelfetcher_reset_all(fetcher) do {                                        \
    pixelfetcher_reset_bg_fifo(fetcher);                                            \
    pixelfifo_clear(&(fetcher)->obj_fifo);                                          \
    (fetcher)->fifo                     = &(fetcher)->bg_fifo;                      \
    (fetcher)->dummy_fetch_cnt_down     = PF_PUSH;                                  \
    (fetcher)->last_sprite_x_end        = 0;                                        \
    (fetcher)->oam_obj                  = NULL;                                     \
} while(0)

#define pixelfetcher_init(fetcher) do {                                             \
    pixelfifo_init(&(fetcher)->bg_fifo);                                            \
    pixelfifo_init(&(fetcher)->obj_fifo);                                           \
    (fetcher)->draw_window              = 0;                                        \
    (fetcher)->window_line_counter      = WINDOW_LINE_COUNTER_DEFAULT;              \
    (fetcher)->sprite_tall_ly_start     = SPRITE_TALL_LY_START_DEFAULT;             \
    pixelfetcher_reset_all(fetcher);                                                \
} while (0)

#define pixelfetcher_free(fetcher) do {                                             \
    pixelfifo_free(&(fetcher)->bg_fifo);                                            \
    pixelfifo_free(&(fetcher)->obj_fifo);                                           \
    (fetcher)->fifo = NULL;                                                         \
} while (0)

void pixelfetcher_fetch(GB_gameboy_t *gb);

#endif
