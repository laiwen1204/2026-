#include "spi_flash.h"

#define WRITE     0x02
#define WRSR      0x01
#define WREN      0x06
#define READ      0x03
#define RDSR      0x05
#define RDID      0x9F
#define SE        0x20
#define BE        0xC7

#define WIP_FLAG  0x01
#define DUMMY_BYTE 0xA5

void spi_flash_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_SPI0);

    gpio_af_set(GPIOB, GPIO_AF_5, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);

    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_15);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_15);
    SPI_FLASH_CS_HIGH();

    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_8;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(SPI0, &spi_init_struct);
    spi_enable(SPI0);
}

void spi_flash_sector_erase(uint32_t sector_addr)
{
    spi_flash_write_enable();
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(SE);
    spi_flash_send_byte((sector_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((sector_addr & 0xFF00) >> 8);
    spi_flash_send_byte(sector_addr & 0xFF);
    SPI_FLASH_CS_HIGH();
    spi_flash_wait_for_write_end();
}

void spi_flash_bulk_erase(void)
{
    spi_flash_write_enable();
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(BE);
    SPI_FLASH_CS_HIGH();
    spi_flash_wait_for_write_end();
}

void spi_flash_page_write(uint8_t* pbuffer, uint32_t write_addr, uint16_t num_byte_to_write)
{
    spi_flash_write_enable();
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(WRITE);
    spi_flash_send_byte((write_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((write_addr & 0xFF00) >> 8);
    spi_flash_send_byte(write_addr & 0xFF);
    while (num_byte_to_write--) {
        spi_flash_send_byte(*pbuffer);
        pbuffer++;
    }
    SPI_FLASH_CS_HIGH();
    spi_flash_wait_for_write_end();
}

void spi_flash_buffer_write(uint8_t* pbuffer, uint32_t write_addr, uint32_t num_byte_to_write)
{
    uint8_t num_of_page = 0, num_of_single = 0, addr = 0, count = 0;

    addr = write_addr % SPI_FLASH_PAGE_SIZE;
    count = SPI_FLASH_PAGE_SIZE - addr;
    num_of_page = num_byte_to_write / SPI_FLASH_PAGE_SIZE;
    num_of_single = num_byte_to_write % SPI_FLASH_PAGE_SIZE;

    if (0 == addr) {
        while (num_of_page--) {
            spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
            write_addr += SPI_FLASH_PAGE_SIZE;
            pbuffer += SPI_FLASH_PAGE_SIZE;
        }
        if (0 != num_of_single)
            spi_flash_page_write(pbuffer, write_addr, num_of_single);
    } else {
        if (num_byte_to_write < count) {
            spi_flash_page_write(pbuffer, write_addr, num_byte_to_write);
        } else {
            spi_flash_page_write(pbuffer, write_addr, count);
            num_of_page = (num_byte_to_write - count) / SPI_FLASH_PAGE_SIZE;
            num_of_single = (num_byte_to_write - count) % SPI_FLASH_PAGE_SIZE;
            write_addr += count;
            pbuffer += count;
            while (num_of_page--) {
                spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
                write_addr += SPI_FLASH_PAGE_SIZE;
                pbuffer += SPI_FLASH_PAGE_SIZE;
            }
            if (0 != num_of_single)
                spi_flash_page_write(pbuffer, write_addr, num_of_single);
        }
    }
}

void spi_flash_buffer_read(uint8_t* pbuffer, uint32_t read_addr, uint16_t num_byte_to_read)
{
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(READ);
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);
    spi_flash_send_byte(read_addr & 0xFF);
    while (num_byte_to_read--) {
        *pbuffer = spi_flash_send_byte(DUMMY_BYTE);
        pbuffer++;
    }
    SPI_FLASH_CS_HIGH();
}

uint32_t spi_flash_read_id(void)
{
    uint32_t temp = 0, temp0 = 0, temp1 = 0, temp2 = 0;

    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(RDID);
    temp0 = spi_flash_send_byte(DUMMY_BYTE);
    temp1 = spi_flash_send_byte(DUMMY_BYTE);
    temp2 = spi_flash_send_byte(DUMMY_BYTE);
    SPI_FLASH_CS_HIGH();

    temp = (temp0 << 16) | (temp1 << 8) | temp2;
    return temp;
}

void spi_flash_start_read_sequence(uint32_t read_addr)
{
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(READ);
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);
    spi_flash_send_byte(read_addr & 0xFF);
}

uint8_t spi_flash_read_byte(void)
{
    return (spi_flash_send_byte(DUMMY_BYTE));
}

uint8_t spi_flash_send_byte(uint8_t byte)
{
    while (RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI0, byte);
    while (RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));
    return (spi_i2s_data_receive(SPI0));
}

uint16_t spi_flash_send_halfword(uint16_t half_word)
{
    while (RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI0, half_word);
    while (RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));
    return spi_i2s_data_receive(SPI0);
}

void spi_flash_write_enable(void)
{
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(WREN);
    SPI_FLASH_CS_HIGH();
}

void spi_flash_wait_for_write_end(void)
{
    uint8_t flash_status = 0;

    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(RDSR);
    do {
        flash_status = spi_flash_send_byte(DUMMY_BYTE);
    } while ((flash_status & WIP_FLAG) == SET);
    SPI_FLASH_CS_HIGH();
}

void spi_flash_buffer_erase(uint32_t sector_addr, uint32_t num_byte_to_erase)
{
    uint8_t buffer_data[SPI_FLASH_SECTOR_SIZE] = {0};
    uint8_t buffer_data1[SPI_FLASH_SECTOR_SIZE] = {0};
    uint8_t num_of_sector = 0, num_of_single = 0, addr = 0, count = 0;

    addr = sector_addr % SPI_FLASH_SECTOR_SIZE;
    count = SPI_FLASH_PAGE_SIZE - addr;
    num_of_sector = num_byte_to_erase / SPI_FLASH_SECTOR_SIZE;
    num_of_single = num_byte_to_erase % SPI_FLASH_SECTOR_SIZE;

    if (0 == addr) {
        while (num_of_sector--) {
            spi_flash_sector_erase(sector_addr);
            sector_addr += SPI_FLASH_PAGE_SIZE;
        }
        if (0 != num_of_single) {
            spi_flash_buffer_read(buffer_data, sector_addr + num_of_single, SPI_FLASH_SECTOR_SIZE - num_of_single);
            spi_flash_sector_erase(sector_addr);
            spi_flash_buffer_write(buffer_data, sector_addr + num_of_single, SPI_FLASH_SECTOR_SIZE - num_of_single);
        }
    } else {
        if (num_byte_to_erase < count) {
            spi_flash_buffer_read(buffer_data, num_of_sector * SPI_FLASH_SECTOR_SIZE, addr);
            spi_flash_buffer_read(buffer_data1, num_of_sector * SPI_FLASH_SECTOR_SIZE + addr + num_byte_to_erase, SPI_FLASH_SECTOR_SIZE - addr - num_byte_to_erase);
            spi_flash_sector_erase(sector_addr);
            spi_flash_buffer_write(buffer_data, num_of_sector * SPI_FLASH_SECTOR_SIZE, addr);
            spi_flash_buffer_write(buffer_data1, num_of_sector * SPI_FLASH_SECTOR_SIZE + addr + num_byte_to_erase, SPI_FLASH_SECTOR_SIZE - addr - num_byte_to_erase);
        } else {
            spi_flash_buffer_read(buffer_data, num_of_sector * SPI_FLASH_SECTOR_SIZE, addr);
            spi_flash_sector_erase(sector_addr);
            spi_flash_buffer_write(buffer_data, num_of_sector * SPI_FLASH_SECTOR_SIZE, addr);

            num_byte_to_erase -= addr;
            num_of_sector = num_byte_to_erase / SPI_FLASH_SECTOR_SIZE;
            num_of_single = num_byte_to_erase % SPI_FLASH_SECTOR_SIZE;
            sector_addr += count;

            while (num_of_sector--) {
                spi_flash_sector_erase(sector_addr);
                sector_addr += SPI_FLASH_PAGE_SIZE;
            }
            if (0 != num_of_single) {
                spi_flash_buffer_read(buffer_data, sector_addr + num_of_single, SPI_FLASH_SECTOR_SIZE - num_of_single);
                spi_flash_sector_erase(sector_addr);
                spi_flash_buffer_write(buffer_data, sector_addr + num_of_single, SPI_FLASH_SECTOR_SIZE - num_of_single);
            }
        }
    }
}
