#ifndef __RS485_H
#define __RS485_H

#include "HeaderFiles.h"

void usart_init(void);
void usart_dma_rx_reconfig(void);
void rs485_send_raw(const uint8_t *data, uint32_t len);
void rs485_set_baud(uint32_t baud);

#endif
