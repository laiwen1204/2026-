#include "dac.h"

static uint16_t g_dac_current_value = 0;

void my_dac_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_DAC);

    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);

    dac_deinit(DAC0);
    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_SOFTWARE);
    dac_trigger_enable(DAC0, DAC_OUT0);
    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE);
    dac_output_buffer_enable(DAC0, DAC_OUT0);
    dac_enable(DAC0, DAC_OUT0);
}

void dac_set_output_value(uint16_t val)
{
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, val);
    dac_software_trigger_enable(DAC0, DAC_OUT0);
    g_dac_current_value = val;
}

uint16_t dac_get_output_value(void)
{
    return g_dac_current_value;
}
