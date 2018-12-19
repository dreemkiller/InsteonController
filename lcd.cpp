#include "mbed.h"
#include "stdio_thread.h"
#include "fsl_lcdc.h"

#include "lcd.h"

#define LCD_PANEL_CLK 9000000
#define LCD_PPL 480
#define LCD_HSW 2
#define LCD_HFP 8
#define LCD_HBP 43
#define LCD_LPP 272
#define LCD_VSW 10
#define LCD_VFP 4
#define LCD_VBP 12
#define LCD_POL_FLAGS kLCDC_InvertVsyncPolarity | kLCDC_InvertHsyncPolarity
#define IMG_HEIGHT 272
#define IMG_WIDTH 480
#define LCD_INPUT_CLK_FREQ CLOCK_GetFreq(kCLOCK_LCD)
#define APP_LCD_IRQHandler LCD_IRQHandler
#define APP_LCD_IRQn LCD_IRQn
#define APP_PIXEL_PER_BYTE 8

extern const char _binary_Floorplan_first_bmp_start;
extern const char _binary_Floorplan_first_bmp_end;
extern const int _binary_Floorplan_first_bmp_size;

extern const char _binary_Floorplan_second_bmp_start;
extern const char _binary_Floorplan_second_bmp_end;
extern const int _binary_Floorplan_second_bmp_size;

/* Frame end flag. */
static volatile bool s_frameEndFlag;

/* Color palette. */
#define YELLOW_NYBBLE 0xe784U
#define WHITE_NYBBLE  0xffffU
#define BLACK_NYBBLE  0x0000U
static const uint32_t palette[] = {(BLACK_NYBBLE <<  16) | WHITE_NYBBLE,
                                   (YELLOW_NYBBLE << 16) | WHITE_NYBBLE};

DigitalOut backlight(P3_31);

void APP_LCD_IRQHandler(void)
{
    uint32_t intStatus = LCDC_GetEnabledInterruptsPendingStatus(APP_LCD);

    LCDC_ClearInterruptsStatus(APP_LCD, intStatus);

    if (intStatus & kLCDC_VerticalCompareInterrupt)
    {
        s_frameEndFlag = true;
    }
    __DSB();
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

uint32_t current_floor = 0; // first floor. Arrays start at 0
uint8_t *display_buffer = NULL;
uint32_t display_buffer_size = 0;
uint8_t *floorplan_first = NULL;
uint32_t floorplan_first_size = 0;
uint8_t *floorplan_second = NULL;
uint32_t floorplan_second_size = 0;
#define PIXELS_PER_BYTE 4

uint32_t get_pixel_byte(uint32_t x, uint32_t y) {
    uint32_t y_dim = 480;
    //uint32_t x_dim = 270;
    uint32_t pixel_location = (x * y_dim + y) / PIXELS_PER_BYTE;
    return pixel_location;
}

uint32_t get_pixel_shift(uint32_t x, uint32_t y) {
    uint32_t y_dim = 480;
    uint32_t pixel_shift = (x * y_dim + y) % PIXELS_PER_BYTE;
    pixel_shift *= 2;
    // The following is to adjust for little-endianness within the byte
    if (pixel_shift < 4) {
        pixel_shift += 4;
    } else {
        pixel_shift -= 4;
    }
    return pixel_shift;
}

#define STIPPLE_SKIP 3
void light_region(uint32_t x_min, uint32_t y_min, uint32_t x_max, uint32_t y_max) {
    for (uint32_t x = x_min; x < x_max; x += STIPPLE_SKIP) {
        for (uint32_t y = y_min; y < y_max; y += STIPPLE_SKIP) {
            uint32_t pixel_byte = get_pixel_byte(x, y);
            uint32_t pixel_shift = get_pixel_shift(x, y);
            floorplan_first[pixel_byte] |= (0x2 << pixel_shift);
        }
    }
}

void unlight_region(uint32_t x_min, uint32_t y_min, uint32_t x_max, uint32_t y_max) {
    for (uint32_t x = x_min; x < x_max; x += STIPPLE_SKIP) {
        for (uint32_t y = y_min; y < y_max; y += STIPPLE_SKIP) {
            uint32_t pixel_byte = get_pixel_byte(x, y);
            uint32_t pixel_shift = get_pixel_shift(x, y);
            floorplan_first[pixel_byte] &= ~(0x2 << pixel_shift);
        }
    }    
}

void change_floors(uint32_t new_floor) {
    const uint32_t scroll_in_delay = 3;
    const uint32_t display_columns = 480;
    const uint32_t display_rows = 272;
    const uint32_t scroll_in_columns = 5;
    const uint32_t bits_per_pixel = 2;
    if (new_floor == current_floor) {
        return; // nothing to do
    }
    if (new_floor == 0) {
        for (uint32_t i = 0; i <= display_columns / scroll_in_columns; i++) {
            for (uint32_t row = 0; row < display_rows; row++) {
                uint32_t dest_byte_offset = (row * display_columns) * bits_per_pixel / 8;
                uint32_t source_byte_offset = ((row * display_columns) + (display_columns - scroll_in_columns) - i * scroll_in_columns) * bits_per_pixel / 8;
                uint32_t row_size_in_bytes = (i + 1) * scroll_in_columns * bits_per_pixel / 8;
                if (dest_byte_offset + row_size_in_bytes < display_buffer_size && (source_byte_offset + row_size_in_bytes < floorplan_first_size)) { // This is a dirty hack. I should be ashamed of myself
                    memcpy(&display_buffer[dest_byte_offset], &floorplan_first[source_byte_offset], row_size_in_bytes);
                }
            }
            wait_ms(scroll_in_delay);
        }
        memcpy(display_buffer, floorplan_second, floorplan_second_size);
    } else if (new_floor == 1) {
        for (uint32_t i = 0; i <= display_columns / scroll_in_columns; i++) {
            for (uint32_t row = 0; row < display_rows; row++) {
                uint32_t dest_byte_offset = ((row * display_columns) + (display_columns - scroll_in_columns) - i * scroll_in_columns) * bits_per_pixel / 8;
                uint32_t source_byte_offset = (row * display_columns) * bits_per_pixel / 8;
                uint32_t row_size_in_bytes = (i + 1) * scroll_in_columns * bits_per_pixel / 8;
                if (dest_byte_offset + row_size_in_bytes < display_buffer_size && (source_byte_offset + row_size_in_bytes < floorplan_second_size)) { // This is a dirty hack. I should be ashamed of myself
                    memcpy(&display_buffer[dest_byte_offset], &floorplan_second[source_byte_offset], row_size_in_bytes);
                }
            }
            wait_ms(scroll_in_delay);
        }
        memcpy(display_buffer, floorplan_second, floorplan_second_size);
    } else {
        safe_printf("Unknown floor:%d\n", new_floor);
        assert(0);
    }
    current_floor = new_floor;
    return;
}

typedef enum {
    ONEBPP_TO_TWOBPP,
} BitmapConversion;

int convert_bitmap(uint8_t *source, uint32_t source_size, uint8_t *dest, uint32_t dest_size, BitmapConversion conversion) {
    switch(conversion) {
        case (ONEBPP_TO_TWOBPP):
            if (dest_size != 2 * source_size) {
                safe_printf("Dest size not correct\n");
                return 1;
            }
            memset(dest, 0, dest_size);
            for (uint32_t i = 0; i < source_size; i++) {
                uint8_t source_byte = source[i];
                uint16_t word = 0;
                for (uint32_t pixel = 0; pixel < 8; pixel++) {
                    uint8_t pixel_value = (source_byte >> pixel) & 0x01;
                    
                    word |= (pixel_value << (2 * pixel));
                }
                // The following two assignments are "out of order" because if little-endianness within the byte
                dest[i * 2 + 1] = (uint8_t) (word & 0xff);
                dest[i * 2] = (uint8_t) ((word >> 8) & 0xff);
            }
            break;
        default:
            safe_printf("Unsupported conversion:%d\n", conversion);
            return 1;
    }
    return 0;
}
status_t APP_LCDC_Init(void)
{
    uint32_t bmp_size = (uint32_t) (&_binary_Floorplan_first_bmp_end - &_binary_Floorplan_first_bmp_start);

    display_buffer_size = 272 * 480 * 2 / 8;
    display_buffer = (uint8_t *) malloc( display_buffer_size );
    if (display_buffer == NULL) {
        safe_printf("Failed to allocate display buffer\n");
        assert(0);
    }
    memset(display_buffer, 0, display_buffer_size);

    floorplan_first_size = bmp_size * 2;
    floorplan_first = (uint8_t *) malloc( floorplan_first_size );
    if (floorplan_first == NULL) {
        safe_printf("Failed to allocate floor plan copy\n");
        assert(0);
    }

    int convert_result = convert_bitmap((uint8_t *)&_binary_Floorplan_first_bmp_start, bmp_size, floorplan_first, floorplan_first_size, ONEBPP_TO_TWOBPP);
    if (convert_result) {
        safe_printf("Failed to convert bitmap:%d\n", convert_result);
        assert(0);
    }

    uint32_t second_bmp_size = (uint32_t) (&_binary_Floorplan_second_bmp_end - &_binary_Floorplan_second_bmp_start);
    floorplan_second_size = second_bmp_size * 2;
    floorplan_second = (uint8_t *) malloc( floorplan_second_size );
    if (floorplan_second == NULL) {
        safe_printf("Failed to allocate space for second floorplan\n");
        assert(0);
    }

    convert_result = convert_bitmap((uint8_t *)&_binary_Floorplan_second_bmp_start, second_bmp_size, floorplan_second, floorplan_second_size, ONEBPP_TO_TWOBPP);
    if (convert_result) {
        safe_printf("Failed to convert second floor bitmap:%d\n", convert_result);
        assert(0);
    }

    current_floor = 0;
    memcpy( display_buffer, floorplan_first, floorplan_first_size );
    /* Initialize the display. */
    lcdc_config_t lcdConfig;

    LCDC_GetDefaultConfig(&lcdConfig);

    lcdConfig.panelClock_Hz = LCD_PANEL_CLK;
    lcdConfig.ppl = LCD_PPL;
    lcdConfig.hsw = LCD_HSW;
    lcdConfig.hfp = LCD_HFP;
    lcdConfig.hbp = LCD_HBP;
    lcdConfig.lpp = LCD_LPP;
    lcdConfig.vsw = LCD_VSW;
    lcdConfig.vfp = LCD_VFP;
    lcdConfig.vbp = LCD_VBP;
    lcdConfig.polarityFlags = LCD_POL_FLAGS;
    lcdConfig.upperPanelAddr = (uint32_t)(display_buffer);
    lcdConfig.bpp = kLCDC_2BPP;
    lcdConfig.display = kLCDC_DisplayTFT;
    lcdConfig.swapRedBlue = true;
    lcdConfig.dataFormat = kLCDC_WinCeMode;

    LCDC_Init(APP_LCD, &lcdConfig, LCD_INPUT_CLK_FREQ);

    LCDC_SetPalette(APP_LCD, palette, ARRAY_SIZE(palette));

    /* Trigger interrupt at start of every vertical back porch. */
    LCDC_SetVerticalInterruptMode(APP_LCD, kLCDC_StartOfBackPorch);
    LCDC_EnableInterrupts(APP_LCD, kLCDC_VerticalCompareInterrupt);
 
    LCDC_Start(APP_LCD);
    LCDC_PowerUp(APP_LCD);
    backlight = 1;
    return kStatus_Success;
}