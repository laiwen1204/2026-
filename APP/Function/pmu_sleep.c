#include "pmu_sleep.h"

void pmu_enter_sleep_10s(void)
{
    uint16_t y, mo, d, h, mi, s;
    rtc_get_time(&y, &mo, &d, &h, &mi, &s);

    uint8_t bin_sec  =((s  >> 4) & 0x0F) * 10 + (s  & 0x0F);
    uint8_t bin_min  = ((mi >> 4) & 0x0F) * 10 + (mi & 0x0F);
    uint8_t bin_hour = ((h  >> 4) & 0x0F) * 10 + (h  & 0x0F);
    uint8_t bin_date = ((d  >> 4) & 0x0F) * 10 + (d  & 0x0F);

    bin_sec += 10;
    if (bin_sec >= 60) { bin_sec -= 60; bin_min++; }
    if (bin_min >= 60) { bin_min -= 60; bin_hour++; }
    if (bin_hour >= 24) { bin_hour -= 24; bin_date++; }

    uint8_t alarm_sec  =((bin_sec  / 10) << 4) | (bin_sec  % 10);
    uint8_t alarm_min  = ((bin_min  / 10) << 4) | (bin_min  % 10);
    uint8_t alarm_hour = ((bin_hour / 10) << 4) | (bin_hour % 10);
    uint8_t alarm_date = ((bin_date / 10) << 4) | (bin_date % 10);

    rcu_periph_clock_enable(RCU_PMU);
    rcu_periph_clock_enable(RCU_RTC);
    pmu_backup_write_enable();

    rtc_alarm_disable(RTC_ALARM0);
    rtc_alarm_struct rtc_alarm;
    rtc_alarm.alarm_mask = RTC_ALARM_HOUR_MASK | RTC_ALARM_MINUTE_MASK;
    rtc_alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    rtc_alarm.alarm_day    = alarm_date;
    rtc_alarm.alarm_hour   = alarm_hour;
    rtc_alarm.alarm_minute = alarm_min;
    rtc_alarm.alarm_second = alarm_sec;
    rtc_alarm.am_pm        = RTC_AM;
    rtc_alarm_config(RTC_ALARM0, &rtc_alarm);
    rtc_alarm_subsecond_config(RTC_ALARM0, RTC_MASKSSC_0_14, 0);
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_alarm_enable(RTC_ALARM0);

    exti_init(EXTI_17, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_enable(EXTI_17);
    nvic_irq_enable(RTC_Alarm_IRQn, 2, 0);

    pmu_flag_clear(PMU_FLAG_RESET_STANDBY);
    pmu_to_deepsleepmode(PMU_LDO_NORMAL, PMU_LOWDRIVER_DISABLE, WFI_CMD);


    rcu_all_reset_flag_clear();

    RCU_CTL |= RCU_CTL_HXTALEN;
    while (!(RCU_CTL & RCU_CTL_HXTALSTB));

    RCU_CTL |= RCU_CTL_PLLEN;
    while (!(RCU_CTL & RCU_CTL_PLLSTB));

    PMU_CTL |= PMU_CTL_HDEN;
    while (!(PMU_CS & PMU_CS_HDRF));
    PMU_CTL |= PMU_CTL_HDS;
    while (!(PMU_CS & PMU_CS_HDSRF));

    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLP;
    while (!(RCU_CFG0 & RCU_SCSS_PLLP));

    SystemCoreClockUpdate();

    rtc_alarm_disable(RTC_ALARM0);
    rtc_interrupt_disable(RTC_INT_ALARM0);
    exti_interrupt_disable(EXTI_17);

    rs485_send_raw((const uint8_t *)"instrument wakeup\r\n", 19);
}

void RTC_Alarm_IRQHandler(void)
{
    if (rtc_flag_get(RTC_FLAG_ALRM0) != RESET) {
        rtc_flag_clear(RTC_FLAG_ALRM0);
    }
    exti_flag_clear(EXTI_17);
}
