#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#include "HeaderFiles.h"

void system_init(void);
void adc_proc(void);
void oled_proc(void);
void rtc_proc(void);
void pt100_proc(void);
void protocol_proc(void);
void led_blink(void);

#endif
