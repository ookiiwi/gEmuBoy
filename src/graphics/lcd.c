#include "lcd.h"
#include "gbtypes.h"
#include "win_utils.h"

#include <SDL.h>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#define VIEWPORT_HEIGHT (144)
#define VIEWPORT_WIDTH  (160)
#define WINDOW_HEIGHT   ( VIEWPORT_HEIGHT * 3 )
#define WINDOW_WIDTH    ( VIEWPORT_WIDTH  * 3 )

struct GB_LCD_s {
    GB_window_t *context;
};

GB_LCD_t* GB_lcd_create() {
    GB_LCD_t *lcd = (GB_LCD_t*)( malloc( sizeof(GB_LCD_t) ) );

    if (lcd == NULL) {
        return NULL;
    }

    lcd->context = GB_window_create( "GemuBoy", 
                                     WINDOW_WIDTH, 
                                     WINDOW_HEIGHT, 
                                     VIEWPORT_WIDTH, 
                                     VIEWPORT_HEIGHT );

    return lcd;
}

void GB_lcd_destroy(GB_LCD_t *lcd) {
    if (lcd == NULL) return;

    GB_window_destroy(lcd->context);

    SDL_Quit();

    free(lcd);
}

static const int gb_colors[] = { 255, 211, 169, 0 };

void GB_lcd_set_pixel(GB_LCD_t *lcd, int x, int y, int color_id) {
    int color = gb_colors[color_id];
    int index = y * VIEWPORT_WIDTH + x;
    lcd->context->pixels[index] = ( (color << 24)|(color << 16)|(color << 8)|0x000000FF );
}

void GB_lcd_clear(GB_LCD_t *lcd) {
    SDL_SetRenderDrawColor(lcd->context->renderer, 139, 172, 15, 255);
    SDL_RenderClear(lcd->context->renderer);
}

void update_texture(GB_LCD_t *lcd) {
    Uint32 *pixels;
    int     pitch;

    SDL_LockTexture(lcd->context->texture, NULL, (void**)&pixels, &pitch);
    
    memcpy((void*)pixels, (void*)lcd->context->pixels, VIEWPORT_HEIGHT * VIEWPORT_WIDTH * sizeof(Uint32));

    SDL_UnlockTexture(lcd->context->texture);
}

void GB_lcd_render(GB_LCD_t *lcd) {
    update_texture(lcd);
    SDL_RenderCopy(lcd->context->renderer, lcd->context->texture, NULL, NULL);
    SDL_RenderPresent(lcd->context->renderer);
}
