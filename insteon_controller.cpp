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

#include "lcd.h"
#include "floorplan_regions.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define EXAMPLE_I2C_MASTER_BASE (I2C2_BASE)
#define I2C_MASTER_CLOCK_FREQUENCY (12000000)

#define EXAMPLE_I2C_MASTER ((I2C_Type *)EXAMPLE_I2C_MASTER_BASE)
#define I2C_MASTER_SLAVE_ADDR_7BIT 0x7EU
#define I2C_BAUDRATE 100000U

#define LONG_TOUCH_THRESHOLD 0.25f

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

extern const char _binary_Floorplan_bmp_start;

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

void single_click_detect_loop();

extern RectangularRegion floorplan_regions[];
extern uint32_t num_floorplan_regions;

Thread InsteonHttpThread;

extern bool screensaver_on;

bool screensaver_on;
#define SCREENSAVER_WAIT_TIME 120.0f
extern DigitalOut backlight;
void screensaver_loop();
void turn_off_screensaver();
Timer screensaver_timer;

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
    GPIO->B[2][27] = 1;
    
    status = FT5406_Init(&touch_handle, EXAMPLE_I2C_MASTER);
    if (status != kStatus_Success)
    {
        safe_printf("Touch panel init failed\n");
        // Make LED 1 flash fast forever as feedback for when the console is not connected
        while(1) {
            led1 = 1;
            wait(0.05);
            led1 = 0;
            wait(0.05);
        }
    }
    assert( status == kStatus_Success);

    safe_printf("Ready to go! joe\n");

    osStatus err = InsteonHttpThread.start(&insteon_loop);
    if (err) {
        safe_printf("Http Setup thread failed\n");
        assert(0);
    }

    Thread screensaver_thread;
    screensaver_timer.start();
    err = screensaver_thread.start(&screensaver_loop);
    if (err) {
        safe_printf("screen saver thread failed to start\n");
        assert(0);
    }
    screensaver_on = false;

    Timer touch_timer;
    bool timer_active = false; // in an ideal world, Timer would contain this value
    for (;;)
    {
        if (kStatus_Success == FT5406_GetSingleTouch(&touch_handle, &touch_event, &cursorPosX, &cursorPosY))
        {
            if (touch_event == kTouch_Down) {
                safe_printf("Resetting screensaver_timer\n");
                screensaver_timer.reset();
                if (screensaver_on) {
                    safe_printf("Turning off screensaver\n");
                    turn_off_screensaver();
                } else { // else because we don't want touches when screen saver is on to have UI effects
                    timer_active = true;
                    touch_timer.start();
                }
            } else if (touch_event == kTouch_Up) {
                if (timer_active) {
                    safe_printf("X:%d Y:%d\n", cursorPosX, cursorPosY);
                    float touch_time = touch_timer.read();
                    touch_timer.stop();
                    touch_timer.reset();
                    timer_active = false;
                    safe_printf("touch_time:%f\n", touch_time);
                    if (touch_time < LONG_TOUCH_THRESHOLD) {
                        // short touch - turn on
                        for (size_t i = 0; i < num_floorplan_regions; i++) {
                            const char *regionName = checkRegion(floorplan_regions[i], cursorPosX, cursorPosY);
                            if ( regionName ) {
                                safe_printf("Short Touch In %s\n", regionName);
                                InsteonHttpThread.signal_set(floorplan_regions[i].on_signal);
                                break;
                            }
                        }
                    } else {
                        // long touch - turn off
                        for (size_t i = 0; i < num_floorplan_regions; i++) {
                            const char *regionName = checkRegion(floorplan_regions[i], cursorPosX, cursorPosY);
                            if ( regionName ) {
                                safe_printf("Long Touch In %s\n", regionName);       
                                InsteonHttpThread.signal_set(floorplan_regions[i].off_signal);
                                break;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            safe_printf("error reading touch controller\r\n");
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

void screensaver_loop() {
    while(1) {
        wait(SCREENSAVER_WAIT_TIME);
        float elapsed_time = screensaver_timer.read();
        if ( elapsed_time > SCREENSAVER_WAIT_TIME) {
            safe_printf("Turning on screensaver\n");
            turn_on_screensaver();
        } 
    }
}