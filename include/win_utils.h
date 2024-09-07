#ifndef SDL_UTILS_H_
#define SDL_UTILS_H_

#include <SDL.h>

typedef struct {
    Uint32          *pixels;
    
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    SDL_Texture     *texture;
    
    unsigned        window_width;
    unsigned        window_height;
    unsigned        renderer_width;
    unsigned        renderer_height;
} GB_window_t;

GB_window_t*    GB_window_create(const char *window_id, unsigned window_width, unsigned window_height, unsigned renderer_width, unsigned renderer_height);
void            GB_window_destroy(GB_window_t *context);
void            GB_window_update_texture(GB_window_t *context);
void            GB_window_render(GB_window_t *context);

#endif