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

extern const char _binary_Floorplan_bmp_start;
extern const char _binary_Floorplan_bmp_end;
extern const int _binary_Floorplan_bmp_size;

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
uint8_t *floorplan_copy = NULL;
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
    return 2 * pixel_shift;
}

void light_region(uint32_t x_min, uint32_t y_min, uint32_t x_max, uint32_t y_max) {
    for (uint32_t x = x_min; x < x_max; x++) {
        for (uint32_t y = y_min; y < y_max; y++) {
            uint32_t pixel_byte = get_pixel_byte(x, y);
            uint32_t pixel_shift = get_pixel_shift(x, y);
            //uint32_t pixel_shift = get_pixel_shift(x, y);
            floorplan_copy[pixel_byte] |= (0x2 << pixel_shift);
        }
    }
}

void unlight_region(uint32_t x_min, uint32_t y_min, uint32_t x_max, uint32_t y_max) {
    for (uint32_t x = x_min; x < x_max; x++) {
        for (uint32_t y = y_min; y < y_max; y++) {
            uint32_t pixel_byte = get_pixel_byte(x, y);
            //uint32_t pixel_shift = get_pixel_shift(x, y);
            floorplan_copy[pixel_byte] &= ~0xaa;
        }
    }    
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
                uint8_t doubled_byte = (source_byte & 0x1) | ((source_byte & 0x2) << 1) | ((source_byte & 0x4) << 2) | ((source_byte & 0x8) << 3);
                dest[i * 2] = doubled_byte;
                doubled_byte = ((source_byte & 0x10) >> 4) | ((source_byte & 0x20) >> 3) | ((source_byte & 0x40) >> 2) | ((source_byte & 0x80) >> 1);
                dest[i * 2 + 1] = doubled_byte;
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
    uint32_t bmp_size = (uint32_t) (&_binary_Floorplan_bmp_end - &_binary_Floorplan_bmp_start);
    floorplan_copy = (uint8_t *) malloc(bmp_size * 2);
    if (floorplan_copy == NULL) {
        safe_printf("Failed to allocate floor plan copy\n");
        assert(0);
    }

    int convert_result = convert_bitmap((uint8_t *)&_binary_Floorplan_bmp_start, bmp_size, floorplan_copy, bmp_size * 2, ONEBPP_TO_TWOBPP);
    if (convert_result) {
        safe_printf("Failed to convert bitmap:%d\n", convert_result);
        assert(0);
    }

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
    safe_printf("First 8 bytes of buffer:");
    for (int i = 0; i < 8; i++) {
        safe_printf("0x%x ", floorplan_copy[i]);
    }
    safe_printf("\n");
    //lcdConfig.upperPanelAddr = (uint32_t)(&_binary_Floorplan_bmp_start + 4);
    lcdConfig.upperPanelAddr = (uint32_t)(floorplan_copy);
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