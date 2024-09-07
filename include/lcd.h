#ifndef RENDERER_H_
#define RENDERER_H_

typedef struct GB_LCD_s GB_LCD_t;

GB_LCD_t*   GB_lcd_create();
void        GB_lcd_destroy(GB_LCD_t *lcd);
void        GB_lcd_set_pixel(GB_LCD_t *lcd, int x, int y, int color_id);
void        GB_lcd_clear(GB_LCD_t *lcd);
void        GB_lcd_render(GB_LCD_t *lcd);

#endif