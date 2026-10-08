#include "rom.h"

void internal_flash_erase(uint32_t addr)
{
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
    fmc_page_erase(addr);
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
    fmc_lock();
}

void internal_flash_earse_sector(uint32_t addr)
{
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
    fmc_sector_erase(addr);
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
    fmc_lock();
}

void internal_flash_write(uint32_t addr, uint16_t data)
{
    fmc_unlock();
    fmc_halfword_program(addr, data);
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
    fmc_lock();
}

void internal_flash_write_word(uint32_t addr, uint32_t data)
{
    fmc_unlock();
    fmc_word_program(addr, data);
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
    fmc_lock();
}

void internal_flash_write_str(uint32_t addr, uint16_t *data, uint16_t len)
{
    fmc_unlock();
    for (uint16_t i = 0; i < len; i++) {
        fmc_halfword_program(addr + i * 2, data[i]);
    }
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
    fmc_lock();
}

void internal_flash_write_str_Char(uint32_t addr, uint8_t *data, uint32_t len)
{
    fmc_unlock();
    for (uint32_t i = 0; i < len; i++) {
        fmc_byte_program(addr + i, data[i]);
        while (fmc_flag_get(FMC_FLAG_BUSY));
    }
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
    fmc_lock();
}

uint16_t internal_flash_read(uint32_t addr)
{
    return *(volatile uint16_t *)(addr);
}

uint8_t internal_flash_read_Char(uint32_t addr)
{
    return *(volatile uint8_t *)(addr);
}

uint32_t internal_flash_read_word(uint32_t addr)
{
    return *(volatile uint32_t *)(addr);
}
