#include "win_utils.h"
#include <stdlib.h>

void GB_window_destroy(GB_window_t *context);

GB_window_t* GB_window_create(const char *window_id, unsigned window_width, unsigned window_height, unsigned renderer_width, unsigned renderer_height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return NULL;
    }
    
    GB_window_t *context = (GB_window_t*)( malloc( sizeof(GB_window_t) ) );

    if (context == NULL) {
        return NULL;
    }

    context->pixels             = NULL;
    context->window             = NULL;
    context->texture            = NULL;
    context->renderer           = NULL;
    context->window_width       = window_width;
    context->window_height      = window_height;
    context->renderer_width     = renderer_width;
    context->renderer_height    = renderer_height;

    context->pixels = (Uint32*)( malloc(renderer_height * renderer_height * sizeof(Uint32)) );
    if (context->pixels == NULL) {
        free(context);
        return NULL;
    }

    SDL_WindowFlags winflags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    context->window = SDL_CreateWindow(window_id, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, winflags);
    if (context->window == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        GB_window_destroy(context);

        return NULL;
    }

    context->renderer = SDL_CreateRenderer(context->window, -1, SDL_RENDERER_ACCELERATED);
    if (context->renderer == NULL) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        GB_window_destroy(context);

        return NULL;
    }

    context->texture = SDL_CreateTexture(context->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, renderer_width, renderer_height);
    if (context->texture == NULL) {
        printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
        GB_window_destroy(context);

        return NULL;
    }

    return context;
}

void GB_window_destroy(GB_window_t *context) {
    if (context == NULL) return;

    SDL_DestroyTexture(context->texture);
    SDL_DestroyRenderer(context->renderer);
    SDL_DestroyWindow(context->window);
    if (context->pixels) free(context->pixels);

    free(context);
}

void GB_window_update_texture(GB_window_t *context) {
    Uint32 *pixels;
    int     pitch;

    SDL_LockTexture(context->texture, NULL, (void**)&pixels, &pitch);
    memcpy((void*)pixels, (void*)context->pixels, context->renderer_width * context->renderer_height * sizeof(Uint32));
    SDL_UnlockTexture(context->texture);
}

void GB_window_render(GB_window_t *context) {
    SDL_RenderClear(context->renderer);
    SDL_RenderCopy(context->renderer, context->texture, NULL, NULL);
    SDL_RenderPresent(context->renderer);
}

