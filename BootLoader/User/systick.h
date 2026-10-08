#ifndef SYS_TICK_H
#define SYS_TICK_H

#include <stdint.h>

void systick_config(void);
void delay_1ms(uint32_t count);
void delay_decrement(void);

#endif
