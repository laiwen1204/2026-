#ifndef __CRC16_H
#define __CRC16_H

#include <stdint.h>

uint16_t crc16_modbus(const uint8_t *data, uint32_t len);
uint16_t crc16_modbus_update(uint16_t crc, uint8_t byte);

#endif
