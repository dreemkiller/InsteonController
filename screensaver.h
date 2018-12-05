#ifndef __SCREENSAVER_LOOP_H__
#define __SCREENSAVER_LOOP_H__

#include "mbed.h"
extern Timer screensaver_timer;
extern bool screensaver_on;

void screensaver_loop();

void turn_off_screensaver();

#endif // __SCREENSAVER_LOOP_H__