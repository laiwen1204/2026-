#include "RTC.h"

#define BKP_VALUE 0x32F0

static rtc_parameter_struct rtc_initpara;
static __IO uint32_t prescaler_a = 0, prescaler_s = 0;
static uint32_t RTCSRC_FLAG = 0;

uint8_t rtc_was_cfg(void)
{
    return (BKP_VALUE == RTC_BKP0) && (0x00 == RTCSRC_FLAG);
}

void rtc_init_hw(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    RTCSRC_FLAG = !GET_BITS(RCU_BDCTL, 8, 9);

    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    prescaler_s = 0xFF;
    prescaler_a = 0x7F;

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();

    if (!rtc_was_cfg())
		{
        rtc_initpara.factor_asyn = prescaler_a;
        rtc_initpara.factor_syn  = prescaler_s;
        rtc_initpara.year        = 0x26;
        rtc_initpara.month       = 0x06;
        rtc_initpara.date        = 0x03; 
        rtc_initpara.day_of_week = RTC_WEDSDAY;
        rtc_initpara.hour        = 0x00;
        rtc_initpara.minute      = 0x00;
        rtc_initpara.second      = 0x00;
        rtc_initpara.display_format = RTC_24HOUR;
        rtc_initpara.am_pm       = RTC_AM;

        if (ERROR == rtc_init(&rtc_initpara)) {
            while (1);
        }
        RTC_BKP0 = BKP_VALUE;
    }
    rcu_all_reset_flag_clear();
}

void rtc_get_time(uint16_t *y, uint16_t *m, uint16_t *d,uint16_t *hh, uint16_t *mm, uint16_t *ss)
{
    rtc_current_time_get(&rtc_initpara);
    *y  = rtc_initpara.year;
    *m  = rtc_initpara.month;
    *d  = rtc_initpara.date;
    *hh = rtc_initpara.hour;
    *mm = rtc_initpara.minute;
    *ss = rtc_initpara.second;
}

void rtc_set_time_now(uint16_t y, uint16_t m, uint16_t d, uint16_t hh, uint16_t mm, uint16_t ss)
{
    pmu_backup_write_enable();

    rtc_initpara.factor_asyn = prescaler_a;
    rtc_initpara.factor_syn  = prescaler_s;
    rtc_initpara.year        = y;
    rtc_initpara.month       = m;
    rtc_initpara.date        = d;
    rtc_initpara.day_of_week = RTC_SATURDAY;
    rtc_initpara.hour        = hh;
    rtc_initpara.minute      = mm;
    rtc_initpara.second      = ss;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.am_pm       = RTC_AM;

    if (ERROR == rtc_init(&rtc_initpara)) {
        return;
    }
    RTC_BKP0 = BKP_VALUE;
}


uint32_t bcd2ts(uint16_t y, uint16_t mo, uint16_t d, uint16_t h, uint16_t mi, uint16_t s)
{
    uint32_t by = ((y >> 4) & 0x0F) * 10 + (y & 0x0F);
    uint32_t bm = ((mo >> 4) & 0x0F) * 10 + (mo & 0x0F);
    uint32_t bd = ((d >> 4) & 0x0F) * 10 + (d & 0x0F);
    uint32_t bh = ((h >> 4) & 0x0F) * 10 + (h & 0x0F);
    uint32_t bmi = ((mi >> 4) & 0x0F) * 10 + (mi & 0x0F);
    uint32_t bs = ((s >> 4) & 0x0F) * 10 + (s & 0x0F);

    static const uint16_t days_before_month[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    uint32_t year_since_2000 = 2000 + by;
    uint32_t days = (year_since_2000 - 2000) * 365;
    days += (year_since_2000 - 2000 + 3) / 4;
    days += days_before_month[bm - 1] + bd - 1;
    if (bm > 2 && ((year_since_2000 % 4 == 0 && year_since_2000 % 100 != 0) ||
                    year_since_2000 % 400 == 0)) {
        days += 1;
    }

    return days * 86400UL + bh * 3600UL + bmi * 60UL + bs + 946684800UL;
}

void ts2bcd(uint32_t ts, uint16_t *y, uint16_t *mo, uint16_t *d, uint16_t *h, uint16_t *mi, uint16_t *s)
{
    uint32_t secs = (ts >= 946684800UL) ? (ts - 946684800UL) : ts;
    *s = secs % 60; secs /= 60;
    *mi = secs % 60; secs /= 60;
    *h = secs % 24; secs /= 24;

    uint32_t days_since_2000 = secs;
    uint32_t year = 2000;
    while (1) {
        uint32_t days_in_year = 365;
        if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) days_in_year = 366;
        if (days_since_2000 < days_in_year) break;
        days_since_2000 -= days_in_year;
        year++;
    }

    static const uint8_t month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint32_t month = 0;
    while (month < 12) {
        uint32_t md = month_days[month];
        if (month == 1 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) md = 29;
        if (days_since_2000 < md) break;
        days_since_2000 -= md;
        month++;
    }

    *y  = (uint16_t)(((year - 2000) / 10) << 4) | ((year - 2000) % 10);
    *mo = (uint16_t)(((month + 1) / 10) << 4) | ((month + 1) % 10);
    *d  = (uint16_t)(((days_since_2000 + 1) / 10) << 4) | ((days_since_2000 + 1) % 10);
    *h  = (uint16_t)(((uint16_t)*h / 10) << 4) | ((uint16_t)*h % 10);
    *mi = (uint16_t)(((uint16_t)*mi / 10) << 4) | ((uint16_t)*mi % 10);
    *s  = (uint16_t)(((uint16_t)*s / 10) << 4) | ((uint16_t)*s % 10);
}
