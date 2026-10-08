#ifndef __PT100_H
#define __PT100_H

#include "HeaderFiles.h"

#define PT100_CS_PORT    GPIOC
#define PT100_CS_PIN     GPIO_PIN_2

#define SPI_SET_CS()    gpio_bit_set(PT100_CS_PORT, PT100_CS_PIN)
#define SPI_CLR_CS()    gpio_bit_reset(PT100_CS_PORT, PT100_CS_PIN)

#define REG_CONFIG_OS_MASK               (0x8000)
#define REG_CONFIG_OS_SINGLE             (0x8000)
#define REG_CONFIG_OS_NOTBUSY            (0x8000)

#define REG_CONFIG_MUX_MASK              (0x7000)
#define REG_CONFIG_MUX_SINGLE_0          (0x4000)

#define REG_CONFIG_PGA_MASK              (0x0E00)
#define REG_CONFIG_PGA_1_024V            (0x0600)

#define REG_CONFIG_MODE_MASK             (0x0100)
#define REG_CONFIG_MODE_CONTIN           (0x0000)
#define REG_CONFIG_MODE_SINGLE           (0x0100)

#define REG_CONFIG_DR_MASK               (0x00E0)
#define REG_CONFIG_DR_100SPS             (0x0080)

#define REG_CONFIG_PULL_UP_EN_MASK       (0x0008)
#define REG_CONFIG_PULL_UP_EN            (0x0008)

#define REG_CONFIG_NOP_MASK              (0x0006)
#define REG_CONFIG_NOP_VALID             (0x0002)

#define CONFIG_DEFAULT                   ((uint16_t)0x058b)

void PT100_Init(void);
uint16_t PT100_ReadADC(void);
uint8_t PT100_IsOK(void);

#endif
