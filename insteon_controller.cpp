/*
 * The Clear BSD License
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*  Standard C Included Files */
#include "mbed.h"
#include <stdio.h>
#include "stdio_thread.h"
#include <string.h>
#include "fsl_lcdc.h"
#include "fsl_ft5406.h"
#include "fsl_sctimer.h"
#include "fsl_gpio.h"
#include "board.h"
#include "pin_mux.h"
#include "floorplan_image.h"
#include "http_client.h"
#include "floorplan_regions.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_LCD LCD
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
#define EXAMPLE_I2C_MASTER_BASE (I2C2_BASE)
#define I2C_MASTER_CLOCK_FREQUENCY (12000000)
#define APP_PIXEL_PER_BYTE 8

#define EXAMPLE_I2C_MASTER ((I2C_Type *)EXAMPLE_I2C_MASTER_BASE)
#define I2C_MASTER_SLAVE_ADDR_7BIT 0x7EU
#define I2C_BAUDRATE 100000U

#define MAX_DOUBLECLICK_DELAY 0.2f

#define SCREENSAVER_WAIT_TIME 120.0f

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

#if (defined(__CC_ARM) || defined(__GNUC__))
__attribute__((aligned(8)))
#elif defined(__ICCARM__)
#pragma data_alignment = 8
#else
#error Toolchain not support.
#endif

/* Frame end flag. */
static volatile bool s_frameEndFlag;

/* Color palette. */
//static const uint32_t palette[] = {0x0000001F};
static const uint32_t palette[] = {0x0000ffff};

/* 32x32 pixel cursor image. */
#if (defined(__CC_ARM) || defined(__GNUC__))
__attribute__((aligned(4)))
#elif defined(__ICCARM__)
#pragma data_alignment = 4
#else
#error Toolchain not support.
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
 static void BOARD_InitPWM(void)
{
    sctimer_config_t config;
    sctimer_pwm_signal_param_t pwmParam;
    uint32_t event;

    CLOCK_AttachClk(kMCLK_to_SCT_CLK);

    CLOCK_SetClkDiv(kCLOCK_DivSctClk, 2, true);

    SCTIMER_GetDefaultConfig(&config);

    SCTIMER_Init(SCT0, &config);

    pwmParam.output = kSCTIMER_Out_5;
    pwmParam.level = kSCTIMER_HighTrue;
    pwmParam.dutyCyclePercent = 5;

    SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 1000U, CLOCK_GetFreq(kCLOCK_Sct), &event);
}

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

status_t APP_I2C_Init(void)
{
    i2c_master_config_t masterConfig;

    I2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Bps = I2C_BAUDRATE;

    /* Initialize the I2C master peripheral */
    I2C_MasterInit(EXAMPLE_I2C_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);

    return kStatus_Success;
}

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut backlight(P3_31);

uint32_t padding;
extern const char _binary_Floorplan_bmp_start;

status_t APP_LCDC_Init(void)
{
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
    //lcdConfig.upperPanelAddr = (uint32_t)s_frameBufs;
    safe_printf("_binary_Floorplan_bmp_start:%x\n", &_binary_Floorplan_bmp_start + 4);
    lcdConfig.upperPanelAddr = (uint32_t)(&_binary_Floorplan_bmp_start + 4);
    lcdConfig.bpp = kLCDC_1BPP;
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
    led1 = 0;
    backlight = 1;
    return kStatus_Success;
}



const char* checkRegion(RectangularRegion region, uint32_t x, uint32_t y) {
    if (region.XMin < x && x < region.XMax && 
        region.YMin < y && y < region.YMax) {
        return region.Name;
    }
    return NULL;
}

struct EventInfo {
        bool active;
        int x_pos;
        int y_pos;
        float last_event_time;
        Timer click_timer;
        Mutex click_mutex;

    };
struct EventInfo event_info = {false, 0, 0, 0.0f};

void single_click_detect_loop();

void screen_saver_loop();

extern RectangularRegion floorplan_regions[];
extern uint32_t num_floorplan_regions;

Thread InsteonHttpThread;

void turn_off_screensaver();
bool screensaver_on;

int main(void)
{
    led1 = 1;
    led2 = 1;
    led3 = 1;
    printf("starting\n");
    int cursorPosX = 0U;
    int cursorPosY = 0U;
    
    ft5406_handle_t touch_handle;
    touch_event_t touch_event;

    status_t status;

    gpio_pin_config_t pin_config = {
        kGPIO_DigitalOutput, 0,
    };

    /* Board pin, clock, debug console init */
    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* Route Main clock to LCD. */
    CLOCK_AttachClk(kMCLK_to_LCD_CLK);

    /* attach 12 MHz clock to FLEXCOMM2 (I2C master for touch controller) */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

    CLOCK_EnableClock(kCLOCK_Gpio2);

    CLOCK_SetClkDiv(kCLOCK_DivLcdClk, 1, true);

    BOARD_InitPins();
    BOARD_BootClockFROHF48M();
    //BOARD_InitDebugConsole();

    /* Set the back light PWM. */
    BOARD_InitPWM();

    //APP_FillBuffer((void *)(s_frameBufs));

    status = APP_LCDC_Init();
    if (status != kStatus_Success)
    {
        safe_printf("LCD init failed\n");
    }
    assert(status == kStatus_Success);
    led2 = 0;
    status = APP_I2C_Init();
    if (status != kStatus_Success)
    {
        safe_printf("I2C init failed\n");
    }
    assert(status == kStatus_Success);

    GPIO_PinInit(GPIO, 2, 27, &pin_config);
    //GPIO_PinWrite(GPIO, 2, 27, 1);
    GPIO->B[2][27] = 1;
    
    status = FT5406_Init(&touch_handle, EXAMPLE_I2C_MASTER);
    if (status != kStatus_Success)
    {
        safe_printf("Touch panel init failed\n");
        
    }
    assert( status == kStatus_Success);

    safe_printf("Ready to go! joe\n");

    osStatus err = InsteonHttpThread.start(&http_loop);
    if (err) {
        safe_printf("Http Setup thread failed\n");
        assert(0);
    }

    event_info.active = false;
    event_info.x_pos = 0;
    event_info.y_pos = 0;

    event_info.click_timer.start();
    event_info.last_event_time = event_info.click_timer.read();

    Thread single_click_detect_thread;
    err = single_click_detect_thread.start(&single_click_detect_loop);
    if (err) {
        safe_printf("single_click_detect thread failed\n");
        assert(0);
    }

    Thread screensaver_thread;
    err = screensaver_thread.start(&screen_saver_loop);
    if (err) {
        safe_printf("screen saver thread failed to start\n");
        assert(0);
    }
    screensaver_on = false;

    for (;;)
    {
        if (kStatus_Success == FT5406_GetSingleTouch(&touch_handle, &touch_event, &cursorPosX, &cursorPosY))
        {
            if (touch_event == kTouch_Down) {
                safe_printf("X:%d Y:%d\n", cursorPosX, cursorPosY);
                if (screensaver_on) {
                    turn_off_screensaver();
                    continue;
                }
                event_info.click_mutex.lock();
                event_info.x_pos = cursorPosX;
                event_info.y_pos = cursorPosY;
                float current_time = event_info.click_timer.read();
                if (event_info.active && (current_time - event_info.last_event_time) < MAX_DOUBLECLICK_DELAY) {
                    safe_printf("Second click detected\n");
                    event_info.active = false;
                    // We've got a doubleclick event
                    for (size_t i = 0; i < num_floorplan_regions; i++) {
                        const char *regionName = checkRegion(floorplan_regions[i], cursorPosX, cursorPosY);
                        if ( regionName ) {
                            safe_printf("Double Click In %s\n", regionName);       
                            InsteonHttpThread.signal_set(floorplan_regions[i].off_signal);
                            break;
                        }
                    }
                } else {
                    safe_printf("First click detected\n");
                    event_info.click_timer.stop();
                    event_info.click_timer.start();
                    event_info.last_event_time = event_info.click_timer.read();
                    event_info.active = true;
                    event_info.x_pos = cursorPosX;
                    event_info.y_pos = cursorPosY;
                }
                event_info.click_mutex.unlock();
            }
        }
        else
        {
            safe_printf("error reading touch controller\r\n");
        }
        
    }
}

void single_click_detect_loop() {
    while(1) {
        float current_time = event_info.click_timer.read();
        if (event_info.active && ((current_time - event_info.last_event_time) > MAX_DOUBLECLICK_DELAY)) {
            event_info.click_mutex.lock();
            event_info.active = false;
            event_info.last_event_time = current_time;
            for (size_t i = 0; i < num_floorplan_regions; i++) {
                const char *regionName = checkRegion(floorplan_regions[i], event_info.x_pos, event_info.y_pos);
                if ( regionName ) {
                    safe_printf("Single Click In %s\n", regionName);
                    InsteonHttpThread.signal_set(floorplan_regions[i].on_signal);
                    break;
                }
            }
            event_info.click_mutex.unlock();
        }
    }
}

void turn_on_screensaver() {
    screensaver_on = true;
    backlight = 0;
    LCDC_PowerDown(APP_LCD);
    LCDC_Stop(APP_LCD);
}

void turn_off_screensaver() {
    backlight = 1;
    screensaver_on = false;
    LCDC_Start(APP_LCD);
    LCDC_PowerUp(APP_LCD);
}

void screen_saver_loop() {
    while(1) {
        wait(SCREENSAVER_WAIT_TIME);
        float current_time = event_info.click_timer.read();
        if ((current_time - event_info.last_event_time) > SCREENSAVER_WAIT_TIME) {
            safe_printf("Turning on screensaver\n");
            turn_on_screensaver();
        } 
    }
}