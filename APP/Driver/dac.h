#ifndef __DAC_H
#define __DAC_H

#include "HeaderFiles.h"

void my_dac_init(void);
void dac_set_output_value(uint16_t val);
uint16_t dac_get_output_value(void);

#endif
