#include "screensaver.h"
#include "mbed.h"
#include "lcd.h"
#include "fsl_lcdc.h"
#include "stdio_thread.h"

Timer screensaver_timer;
bool screensaver_on;
extern DigitalOut backlight;

static void turn_on_screensaver() {
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
        wait(MBED_CONF_APP_SCREENSAVER_WAIT_TIME);
        float elapsed_time = screensaver_timer.read();
        if ( !screensaver_on && (elapsed_time > MBED_CONF_APP_SCREENSAVER_WAIT_TIME) ) {
            safe_printf("Turning on screensaver\n");
            turn_on_screensaver();
        }
    }
}