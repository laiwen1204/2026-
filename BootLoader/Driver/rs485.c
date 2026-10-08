#include "rs485.h"
#include "rom.h"

uint8_t recv_buf[128] = { 0 };
uint8_t recv_len = 0;
uint8_t recv_real_buf[128] = { 0 };
uint8_t recv_real_len = 0;
uint8_t recv_flag = 0;

static uint8_t  g_dl_mode = 0;
static uint8_t *g_dl_buf  = NULL;
static uint32_t g_dl_idx  = 0;
static uint8_t  g_dl_done = 0;

uint8_t g_boot_baud_v2 = 0x13;

static void usart_485_cs(uint8_t cs)
{
    if (cs == 1) {
        gpio_bit_set(GPIOE, GPIO_PIN_8);
    } else {
        gpio_bit_reset(GPIOE, GPIO_PIN_8);
    }
}

void my_usart_init(void)
{
    nvic_irq_enable(USART1_IRQn, 3, 2);

    rcu_periph_clock_enable(RCU_USART1);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);

    gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_8);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    gpio_bit_reset(GPIOE, GPIO_PIN_8);

    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_5);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);

    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_6);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);

    usart_deinit(USART1);

    {
        uint8_t  baud_code = internal_flash_read_Char(0x0801000A);
        uint32_t baud;
        switch (baud_code) {
            case 0x0B: baud = 4800;   g_boot_baud_v2 = 0x11; break;
            case 0x0C: baud = 9600;   g_boot_baud_v2 = 0x12; break;
            case 0x13: baud = 19200;  g_boot_baud_v2 = 0x13; break;
            case 0x14: baud = 115200; g_boot_baud_v2 = 0x14; break;
            default:   baud = 19200;  g_boot_baud_v2 = 0x13; break;
        }
        usart_baudrate_set(USART1, baud);
    }
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);

    usart_interrupt_enable(USART1, USART_INT_RBNE);
    usart_interrupt_enable(USART1, USART_INT_IDLE);
}

void usart_recv_buf(void)
{
}

int fputc(int ch, FILE* f)
{
    usart_485_cs(1);
    usart_data_transmit(USART1, (uint8_t)ch);
    while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
    usart_485_cs(0);
    return ch;
}

void USART1_IRQHandler(void)
{
    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET)
    {
        uint8_t byte = usart_data_receive(USART1);
        if (g_dl_mode && g_dl_buf != NULL) {
            g_dl_buf[g_dl_idx++] = byte;
        } else {
            recv_buf[recv_len++] = byte;
        }
    }
    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE) != RESET)
    {
        if (g_dl_mode) {
            usart_data_receive(USART1);
            g_dl_done = 1;
        } else if (recv_len != 0) {
            usart_data_receive(USART1);
            memcpy(recv_real_buf, recv_buf, recv_len);
            recv_real_len = recv_len;
            recv_len = 0;
            recv_flag = 1;
        }
    }
}

uint8_t usart_is_recv_ready(void)
{
    return recv_flag;
}

void usart_get_recv_data(uint8_t *buf, uint8_t *len)
{
    *len = recv_real_len;
    memcpy(buf, recv_real_buf, recv_real_len);
    recv_flag = 0;
    recv_real_len = 0;
}

void usart_enter_download_mode(uint8_t *buf)
{
    g_dl_mode = 1;
    g_dl_buf  = buf;
    g_dl_idx  = 0;
    g_dl_done = 0;
}

uint8_t usart_is_download_done(void)
{
    return g_dl_done;
}

void usart_exit_download_mode(void)
{
    g_dl_mode = 0;
    g_dl_buf  = NULL;
    g_dl_done = 0;
}

uint32_t usart_get_dl_idx(void)
{
    return g_dl_idx;
}
