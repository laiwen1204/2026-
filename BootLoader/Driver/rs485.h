#ifndef __USART_H__
#define __USART_H__

#include "HeaderFiles.h"

extern uint8_t g_boot_baud_v2;

void my_usart_init(void);
void usart_recv_buf(void);

uint8_t usart_is_recv_ready(void);
void    usart_get_recv_data(uint8_t *buf, uint8_t *len);
void    usart_enter_download_mode(uint8_t *buf);
uint8_t usart_is_download_done(void);
void    usart_exit_download_mode(void);
uint32_t usart_get_dl_idx(void);

#endif
