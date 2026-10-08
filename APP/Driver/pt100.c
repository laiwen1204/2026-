#include "pt100.h"

static uint8_t g_pt100_ok = 0;

static void pt100_spi_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_SPI1);

    gpio_af_set(GPIOB, GPIO_AF_5, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    SPI_SET_CS();

    spi_parameter_struct spi_init_struct;
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_256;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;

    spi_init(SPI1, &spi_init_struct);
    spi_enable(SPI1);
}

static uint8_t spi_send_byte(uint8_t byte)
{
    while (RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI1, byte);
    while (RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE));
    return (spi_i2s_data_receive(SPI1));
}

static uint16_t spi_transmit16(uint16_t tx_data)
{
    uint8_t rxdataH, rxdataL;

    SPI_CLR_CS();
    while (spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) == RESET);
    spi_i2s_data_transmit(SPI1, (tx_data >> 8) & 0xFF);
    while (spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE) == RESET);
    rxdataH = spi_i2s_data_receive(SPI1);

    while (spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) == RESET);
    spi_i2s_data_transmit(SPI1, tx_data & 0xFF);
    while (spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE) == RESET);
    rxdataL = spi_i2s_data_receive(SPI1);
    SPI_SET_CS();

    return ((uint16_t)rxdataH << 8) | rxdataL;
}

static uint32_t spi_transmit32(uint32_t tx_data)
{
    uint8_t b1 = (tx_data >> 24) & 0xFF;
    uint8_t b2 = (tx_data >> 16) & 0xFF;
    uint8_t b3 = (tx_data >> 8) & 0xFF;
    uint8_t b4 = tx_data & 0xFF;

    SPI_CLR_CS();
    delay_us(2);

    uint8_t r1 = spi_send_byte(b1);
    uint8_t r2 = spi_send_byte(b2);
    uint8_t r3 = spi_send_byte(b3);
    uint8_t r4 = spi_send_byte(b4);

    delay_us(2);
    SPI_SET_CS();

    return ((uint32_t)r1 << 24) | ((uint32_t)r2 << 16) | ((uint32_t)r3 << 8) | r4;
}

static uint16_t read_conversion(void)
{
    return spi_transmit16(0x0000);
}

static uint16_t read_config(void)
{
    uint32_t ret = spi_transmit32(0x00010000);
    return ret & 0xFFFF;
}

static uint32_t write_config(uint16_t config)
{
    config |= REG_CONFIG_NOP_VALID;
    return spi_transmit32(((uint32_t)config << 16) | (uint32_t)config);
}

static uint16_t build_config(void)
{
    uint16_t config = CONFIG_DEFAULT;

    config &= ~REG_CONFIG_MUX_MASK;
    config |= REG_CONFIG_MUX_SINGLE_0;

    config &= ~REG_CONFIG_PGA_MASK;
    config |= REG_CONFIG_PGA_1_024V;

    config &= ~REG_CONFIG_MODE_MASK;
    config |= REG_CONFIG_MODE_CONTIN;

    return config;
}

#define FILTER_WIN 5

static void sort_buf(uint16_t a[], uint8_t n)
{
    uint8_t i, j;
    for (i = 0; i < n - 1; i++)
        for (j = 0; j < n - 1 - i; j++)
            if (a[j] > a[j + 1]) {
                uint16_t t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
            }
}

static uint16_t median_filter(uint16_t sample)
{
    static uint16_t buf[FILTER_WIN] = {0};
    static uint8_t idx = 0;
    static uint8_t cnt = 0;
    uint16_t sorted[FILTER_WIN];
    uint8_t i;

    buf[idx] = sample;
    idx = (idx + 1) % FILTER_WIN;
    if (cnt < FILTER_WIN) cnt++;

    for (i = 0; i < cnt; i++) sorted[i] = buf[i];
    sort_buf(sorted, cnt);
    return sorted[cnt / 2];
}

void PT100_Init(void)
{
    uint16_t config;
    uint16_t readback;

    pt100_spi_init();

    config = build_config();
    write_config(config);
    for (volatile uint32_t d = 0; d < 100000; d++);

    readback = read_config();
    if (readback != config) {
        return;
    }
    g_pt100_ok = 1;
}

uint16_t PT100_ReadADC(void)
{
    return median_filter(read_conversion());
}

uint8_t PT100_IsOK(void)
{
    return g_pt100_ok;
}
