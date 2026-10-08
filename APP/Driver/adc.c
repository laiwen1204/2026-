#include "adc.h"

uint16_t adc_dma_buffer;

void ADC_Init(void)
{

    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_ADC0);
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);

    adc_deinit();
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1);
    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_56);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);
    adc_enable(ADC0);
    delay_1ms(1);
    adc_calibration_enable(ADC0);

    dma_single_data_parameter_struct dma_struct;
    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH0);
    dma_single_data_para_struct_init(&dma_struct);
    dma_struct.periph_addr         = (uint32_t)&ADC_RDATA(ADC0);
    dma_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_struct.memory0_addr        = (uint32_t)&adc_dma_buffer;
    dma_struct.memory_inc          = DMA_MEMORY_INCREASE_DISABLE;
    dma_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_struct.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;
    dma_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_struct.number              = 1;
    dma_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH0, &dma_struct);
    dma_channel_subperipheral_select(DMA1, DMA_CH0, DMA_SUBPERI0);
    dma_circulation_enable(DMA1, DMA_CH0);
    dma_channel_enable(DMA1, DMA_CH0);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);

    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_1);
}


uint16_t adc_read_ch1(void)
{
    uint16_t val;

    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_11, ADC_SAMPLETIME_56);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
    while (!adc_flag_get(ADC0, ADC_FLAG_EOC));
    val = (uint16_t)adc_routine_data_read(ADC0);

    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_56);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);

    return val;
}
