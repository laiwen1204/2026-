#ifndef __LED_H
#define __LED_H

#include "HeaderFiles.h"

void LED_Init(void);

#define LED1_ON()   gpio_bit_set(GPIOA, GPIO_PIN_5)
#define LED1_OFF()  gpio_bit_reset(GPIOA, GPIO_PIN_5)
#define LED2_ON()   gpio_bit_set(GPIOA, GPIO_PIN_6)
#define LED2_OFF()  gpio_bit_reset(GPIOA, GPIO_PIN_6)

#endif
