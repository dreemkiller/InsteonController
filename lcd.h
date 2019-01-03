#ifndef __LCD_H__
#define __LCD_H__

#define APP_LCD LCD

typedef int32_t status_t;

status_t APP_LCDC_Init(void);

extern Mutex lcd_update_mutex;

void light_region(uint32_t x_min, uint32_t y_min, uint32_t x_max, uint32_t y_max);
void unlight_region(uint32_t x_min, uint32_t y_min, uint32_t x_max, uint32_t y_max);
void change_floors(uint32_t new_floor);
#endif // __LCD_H__