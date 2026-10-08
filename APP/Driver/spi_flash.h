#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

#include "HeaderFiles.h"

#define SPI_FLASH_PAGE_SIZE            256
#define SPI_FLASH_SECTOR_SIZE          4096
#define SPI_FLASH_CS_LOW()             gpio_bit_reset(GPIOA, GPIO_PIN_15)
#define SPI_FLASH_CS_HIGH()            gpio_bit_set(GPIOA, GPIO_PIN_15)

void spi_flash_init(void);
void spi_flash_sector_erase(uint32_t sector_addr);
void spi_flash_bulk_erase(void);
void spi_flash_page_write(uint8_t* pbuffer, uint32_t write_addr, uint16_t num_byte_to_write);
void spi_flash_buffer_write(uint8_t* pbuffer, uint32_t write_addr, uint32_t num_byte_to_write);
void spi_flash_buffer_read(uint8_t* pbuffer, uint32_t read_addr, uint16_t num_byte_to_read);
uint32_t spi_flash_read_id(void);
void spi_flash_start_read_sequence(uint32_t read_addr);
uint8_t spi_flash_read_byte(void);
uint8_t spi_flash_send_byte(uint8_t byte);
uint16_t spi_flash_send_halfword(uint16_t half_word);
void spi_flash_write_enable(void);
void spi_flash_wait_for_write_end(void);
void spi_flash_buffer_erase(uint32_t sector_addr, uint32_t num_byte_to_erase);

#endif
