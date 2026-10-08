#ifndef SYS_TICK_H
#define SYS_TICK_H

#include <stdint.h>

void systick_config(void);
void delay_1ms(uint32_t count);
void delay_us(uint32_t t);
void delay_decrement(void);
uint64_t GetSysRunTime(void);

#endif
