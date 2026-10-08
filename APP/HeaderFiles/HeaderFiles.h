#ifndef __HEADERFILES_H
#define __HEADERFILES_H

#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "systick.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

typedef struct {
    uint8_t  pt100_ok;
    float    adc_voltage;
    float    pt100_voltage;
    float    pt100_resistance;
    float    pt100_temperature;
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
} system_state_t;

extern system_state_t g_sys;

extern uint16_t adc_dma_buffer;

extern uint8_t  recv_real_buf[128];
extern uint8_t  recv_real_len;
extern uint8_t  recv_flag;

#include "BootConfig.h"
#include "rom.h"
#include "oled.h"
#include "LED.h"
#include "dac.h"
#include "adc.h"
#include "RTC.h"
#include "rs485.h"
#include "pt100.h"
#include "spi_flash.h"
#include "app_main.h"
#include "scheduler.h"
#include "boot_if.h"
#include "crc16.h"
#include "protocol.h"
#include "param_store.h"
#include "cmd_handler.h"
#include "pmu_sleep.h"

#endif 
