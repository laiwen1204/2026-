#include "app_main.h"
#include "cmd_handler.h"

system_state_t g_sys = {
    .year = 0x26, .month = 0x06, .day = 0x03,
    .hour = 0x00, .minute = 0x00, .second = 0x00,
};


void system_init(void)
{
    __disable_irq();
    SCB->VTOR = 0x08011000;
    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    __DSB();
    __ISB();

    systick_config();
    __enable_irq();

    LED_Init();
    LED1_ON();

    rtc_init_hw();
    rtc_get_time(&g_sys.year, &g_sys.month, &g_sys.day,
                 &g_sys.hour, &g_sys.minute, &g_sys.second);

    OLED_Init();
    OLED_Clear();

    ADC_Init();

    PT100_Init();
    g_sys.pt100_ok = PT100_IsOK();

    spi_flash_init();

    my_dac_init();

    usart_init();

    boot_if_init();

    param_init();

    uint8_t code = param_get_baud_rate();
    uint32_t baud = 19200;
    if (code == 0x0B)      baud = 4800;
    else if (code == 0x0C) baud = 9600;
    else if (code == 0x13) baud = 19200;
    else if (code == 0x14) baud = 115200;
    rs485_set_baud(baud);

    proto_set_id(param_get_device_id());

    send_hb();

    LED1_OFF();
}


void oled_proc(void)
{
    static uint8_t last_active = 0xFF;
    uint8_t active = ar_active() ? 1 : 0;

    if (active != last_active) {
        last_active = active;
        OLED_Clear();
        OLED_Printf(0, 0, 16, "2026661795");
        if (active) {
            OLED_Printf(0, 16, 16, "AutoSample");
        } else {
            OLED_Printf(0, 16, 16, "IDLE");
        }
        OLED_Refresh();
    }
}


void led_blink(void)
{
    static uint8_t state = 0;
    state = !state;

    if (state) {
        LED1_ON();
    } else {
        LED1_OFF();
    }

    if (ar_active()) {
        LED2_ON();
    } else {
        LED2_OFF();
    }
}


void protocol_proc(void)
{
    proto_ctx_t *ctx = get_ctx();

    if (recv_flag) {
        recv_flag = 0;

        uint8_t rx_buf[128];
        uint8_t rx_len = recv_real_len;
        memcpy(rx_buf, recv_real_buf, rx_len);

        while (rx_len > 0 &&
               (rx_buf[rx_len - 1] == '\r' ||
                rx_buf[rx_len - 1] == '\n')) {
            rx_len--;
        }

        int ret = proto_parse(ctx, rx_buf, rx_len);

        if (ret == 0 && ctx->frame_ready) {
            ctx->frame_ready = false;

            if (ar_active()) {
                uint8_t  ftype = ctx->rx_frame.type;
                uint16_t fcmd  = ctx->rx_frame.command;
                if (!(ftype == FRAME_TYPE_HEARTBEAT ||
                      (ftype == FRAME_TYPE_COMMAND && fcmd == 0x0303))) {
                    return;
                }
            }

            if (cmd_dispatch(ctx) != 0) {
                send_err(&ctx->rx_frame);
            }
        } else if (ret < 0 || ctx->frame_error) {
            ctx->frame_error = false;
            ctx->frame_ready = false;
            send_err(&ctx->rx_frame);
        }
    }
}


void adc_proc(void)
{
    g_sys.adc_voltage = adc_dma_buffer * 3.3f / 4095.0f;
}


void rtc_proc(void)
{
    rtc_get_time(&g_sys.year, &g_sys.month, &g_sys.day,
                 &g_sys.hour, &g_sys.minute, &g_sys.second);
}


void pt100_proc(void)
{
    if (!g_sys.pt100_ok) return;

    uint16_t raw = PT100_ReadADC();
    g_sys.pt100_voltage    = ((float)raw / 32767.0f) * 1.024f;
    g_sys.pt100_resistance = (g_sys.pt100_voltage / 52.0f) / 0.0001f;
    g_sys.pt100_temperature = (g_sys.pt100_resistance - 100.0f) / 0.385f;
}

