#include "rs485.h"

#define DMA_RX_BUF_SIZE  256

static uint8_t dma_rx_buf[DMA_RX_BUF_SIZE];

uint8_t recv_real_buf[128] = { 0 };
uint8_t recv_real_len = 0;
uint8_t recv_flag = 0;

static void usart_485_CS(uint8_t cs)
{
    if (cs == 1) {
        gpio_bit_set(GPIOE, GPIO_PIN_8);
    } else {
        gpio_bit_reset(GPIOE, GPIO_PIN_8);
    }
}

void usart_init(void)
{
    nvic_irq_enable(USART1_IRQn, 5, 0);

    rcu_periph_clock_enable(RCU_USART1);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_DMA0);

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
    usart_baudrate_set(USART1, 19200U);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);

    dma_single_data_parameter_struct dma_rx_struct;
    dma_deinit(DMA0, DMA_CH5);
    dma_single_data_para_struct_init(&dma_rx_struct);
    dma_rx_struct.direction         = DMA_PERIPH_TO_MEMORY;
    dma_rx_struct.memory0_addr      = (uint32_t)dma_rx_buf;
    dma_rx_struct.memory_inc        = DMA_MEMORY_INCREASE_ENABLE;
    dma_rx_struct.periph_addr       = (uint32_t)&USART_DATA(USART1);
    dma_rx_struct.periph_inc        = DMA_PERIPH_INCREASE_DISABLE;
    dma_rx_struct.periph_memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_rx_struct.number            = DMA_RX_BUF_SIZE;
    dma_rx_struct.priority          = DMA_PRIORITY_HIGH;
    dma_rx_struct.circular_mode     = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_rx_struct);
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI4);
    dma_channel_enable(DMA0, DMA_CH5);

    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);

    usart_interrupt_enable(USART1, USART_INT_IDLE);
    usart_enable(USART1);
}

void usart_dma_rx_reconfig(void)
{
    dma_channel_disable(DMA0, DMA_CH5);
    dma_memory_address_config(DMA0, DMA_CH5, DMA_MEMORY_0,
                              (uint32_t)dma_rx_buf);
    dma_transfer_number_config(DMA0, DMA_CH5, DMA_RX_BUF_SIZE);
    dma_flag_clear(DMA0, DMA_CH5, DMA_FLAG_FTF);
    dma_channel_enable(DMA0, DMA_CH5);
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);
}

int fputc(int ch, FILE *f)
{
    usart_485_CS(1);
    usart_data_transmit(USART1, (uint8_t)ch);
    while (usart_flag_get(USART1, USART_FLAG_TBE) == RESET) {
    }
    while (usart_flag_get(USART1, USART_FLAG_TC) == RESET) {
    }
    usart_485_CS(0);
    return ch;
}

void rs485_send_raw(const uint8_t *data, uint32_t len)
{
    gpio_bit_toggle(GPIOA, GPIO_PIN_7);

    for (volatile uint32_t d = 0; d < 720000; d++) {}

    usart_485_CS(1);
    for (volatile uint32_t d = 0; d < 24000; d++) {}
    for (uint32_t i = 0; i < len; i++) {
        usart_data_transmit(USART1, data[i]);
        while (usart_flag_get(USART1, USART_FLAG_TBE) == RESET) {
        }
    }
    while (usart_flag_get(USART1, USART_FLAG_TC) == RESET) {
    }
    usart_485_CS(0);
}

void rs485_set_baud(uint32_t baud)
{
    usart_disable(USART1);
    usart_baudrate_set(USART1, baud);
    usart_enable(USART1);
}

void USART1_IRQHandler(void)
{
    if (boot_if_is_active()) {
        if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE) != RESET) {
            if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET) {
                boot_if_rbne_handler((uint8_t)usart_data_receive(USART1));
            }
            (void)USART_STAT0(USART1);
            (void)USART_DATA(USART1);
            boot_if_idle_handler();
        } else if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET) {
            boot_if_rbne_handler((uint8_t)usart_data_receive(USART1));
        }
        return;
    }

    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE) != RESET) {
        uint32_t recv_len = 0;

        (void)USART_STAT0(USART1);
        (void)USART_DATA(USART1);

        dma_channel_disable(DMA0, DMA_CH5);
        recv_len = DMA_RX_BUF_SIZE - dma_transfer_number_get(DMA0, DMA_CH5);

        if (recv_len > 0 && recv_len < DMA_RX_BUF_SIZE) {
            uint32_t copy_len = (recv_len > sizeof(recv_real_buf))
                                ? sizeof(recv_real_buf) : recv_len;
            memcpy(recv_real_buf, dma_rx_buf, copy_len);
            recv_real_len = (uint8_t)copy_len;
            recv_flag = 1;
        }

        dma_memory_address_config(DMA0, DMA_CH5, DMA_MEMORY_0,
                                  (uint32_t)dma_rx_buf);
        dma_transfer_number_config(DMA0, DMA_CH5, DMA_RX_BUF_SIZE);
        dma_flag_clear(DMA0, DMA_CH5, DMA_FLAG_FTF);
        dma_channel_enable(DMA0, DMA_CH5);
    }
}

