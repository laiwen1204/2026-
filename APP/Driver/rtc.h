#ifndef __RTC_H
#define __RTC_H

#include "HeaderFiles.h"

void rtc_init_hw(void);
void rtc_get_time(uint16_t *y, uint16_t *m, uint16_t *d, uint16_t *hh, uint16_t *mm, uint16_t *ss);
uint8_t rtc_was_cfg(void);
void rtc_set_time_now(uint16_t y, uint16_t m, uint16_t d, uint16_t hh, uint16_t mm, uint16_t ss);
uint32_t bcd2ts(uint16_t y, uint16_t mo, uint16_t d, uint16_t h, uint16_t mi, uint16_t s);
void ts2bcd(uint32_t ts, uint16_t *y, uint16_t *mo, uint16_t *d, uint16_t *h, uint16_t *mi, uint16_t *s);

#endif
