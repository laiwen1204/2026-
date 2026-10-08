#include "Function.h"
#include "rom.h"
#include "rs485.h"
#include "BootConfig.h"
#include "oled.h"



typedef void (*pFunction)(void);

pFunction jump2app;

#define CONFIG_SIZE 1024*4

#define CONFIG_APP_SIZE 1024*128

#define APP_DOWNLOAD_ADDR 0x08051000


typedef struct __attribute__((packed)) Parameter_SUM
{
	BootParam_t BootParam;
	BootParam_t BootParam_Reserved;
	UpdateLog_t UpdateLog;
	UserConfig_t UserConfig;
	CalibData_t CalibData;
}Parameter_t;

Parameter_t my_param_sum = { 0 };

uint8_t config_buf[CONFIG_APP_SIZE] = { 0 };


static void load_cfg(void);

static bool Download_Transport(uint32_t DownLoad_Addr);

uint32_t crc32_calc(uint8_t* data , uint32_t len);

void mcu_software_reset(void);

bool jump_to_app(void);

bool Backup_App(void);

static uint8_t hex_char_to_nibble(char c);
static int     ascii_to_binary(const uint8_t *ascii, uint32_t len, uint8_t *binary, uint32_t sz);
static bool    check_serial_commands(void);
static void    send_ok_response(uint16_t cmd);
static void    send_error_response(uint16_t cmd);
static bool    bootloader_receive_firmware(void);
static void    bootloader_install_firmware(void);
static uint16_t crc16_modbus(const uint8_t *data, uint32_t len);


void sys_init(void)
{
	systick_config();
	my_usart_init();

	OLED_Init();
	OLED_Clear();
	OLED_Printf(0, 0, 16, "2026661795");
	OLED_Printf(0, 16, 16, "Bootloader");
	OLED_Refresh();
}

void bl_main(void)
{
	bool fw_ready = false;

	load_cfg();
	memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));

	delay_1ms(100);



	if (my_param_sum.BootParam.appVersion != my_param_sum.BootParam_Reserved.appVersion)
		Backup_App();


	my_param_sum.BootParam.appStartAddr = 0x08011000;
	my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);
	my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);

	if (my_param_sum.BootParam.bootFailCount == 0xFFFF)
	{
		my_param_sum.BootParam.bootFailCount = 0;
	}

	if (my_param_sum.BootParam.bootFailCount >= 5)
	{

		for (uint16_t i = 0; i < CONFIG_SIZE; i++)
		{
			config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
		}
		memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));

		my_param_sum.BootParam.bootFailCount = 0;
		my_param_sum.BootParam.appVersion = my_param_sum.BootParam_Reserved.appVersion;

		memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
		internal_flash_erase(BOOT_CONFIG_ADDR);
		internal_flash_write_str_Char(BOOT_CONFIG_ADDR , config_buf , CONFIG_SIZE);


		my_param_sum.BootParam.appStartAddr = 0x08011000;
		my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);
		my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);


		for (uint8_t i = 0;i < 32; i++)
		{
			internal_flash_erase(0x08011000 + i * 4 * 1024);
		}
		internal_flash_write_str_Char(0x08011000 , (uint8_t*)0x08031000 , 128 * 1024);
	}



	delay_1ms(1000);


	if (my_param_sum.BootParam.magicWord != 0x5AA5C33C)
	{
		my_param_sum.BootParam.updateFlag   = 0x00;
		my_param_sum.BootParam.updateStatus = 0x00;
		my_param_sum.BootParam.bootFailCount = 0;
		my_param_sum.BootParam.backupCRC32  = 0;
		goto BootJump;
	}

	if (my_param_sum.BootParam.updateStatus == 0x01 && my_param_sum.BootParam.updateFlag == 0x5A)
	{

		bool Download_Transport_Result = Download_Transport(APP_DOWNLOAD_ADDR);


		for (uint16_t i = 0; i < CONFIG_SIZE; i++)
		{
			config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
		}
		memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));

		if (Download_Transport_Result == true)
		{
			my_param_sum.BootParam.appStartAddr = 0x08011000;
			my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);
			my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);
			my_param_sum.BootParam.updateStatus = 0x00;
			my_param_sum.BootParam.updateFlag = 0x00;
			my_param_sum.BootParam.updateCount++;

		}
		else
		{
			my_param_sum.BootParam.resetCount++;
			my_param_sum.BootParam.bootFailCount++;
		}
		memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
		internal_flash_erase(BOOT_CONFIG_ADDR);
		internal_flash_write_str_Char(BOOT_CONFIG_ADDR , config_buf , CONFIG_SIZE);
		mcu_software_reset();
	}
			else
	{
		int sec, tick;

		*(uint32_t *)0x2002FFFC = 0;

		if (*(uint32_t *)0x2002FFF8 == 0xB007B007)
		{
			*(uint32_t *)0x2002FFF8 = 0;
			goto BootJump;
		}

		for (sec = 10; sec > 0 && !fw_ready; sec -= 3)
		{
			for (tick = 0; tick < 3 && (sec - tick) > 0 && !fw_ready; tick++)
			{
				delay_1ms(500);
				fw_ready = check_serial_commands();
				delay_1ms(500);
			}
		}

	if (!fw_ready) {
	BootJump:
		{
			rcu_periph_clock_enable(RCU_GPIOA);
			gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_5);
			gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
			gpio_bit_reset(GPIOA, GPIO_PIN_5);
		}

		{
			uint32_t v2_expect = g_boot_baud_v2;
			uint32_t v2_cur    = internal_flash_read_word(0x0801002C);
			uint32_t ver_cur   = internal_flash_read_word(0x08010004);

			if (ver_cur != 0x00010001 || v2_cur != v2_expect) {
				uint16_t i;
				for (i = 0; i < 4096; i++) {
					config_buf[i] = internal_flash_read_Char(0x08010000 + i);
				}
				*(uint32_t *)(config_buf + 0x04) = 0x00010001;
				*(uint32_t *)(config_buf + 0x2C) = v2_expect;
				internal_flash_erase(0x08010000);
				internal_flash_write_str_Char(0x08010000, config_buf, 4096);
			} else if (v2_cur == 0xFFFFFFFF) {
				internal_flash_write_word(0x0801002C, v2_expect);
			}
		}

		OLED_Refresh();
		jump_to_app();

		gpio_bit_set(GPIOA, GPIO_PIN_5);
		OLED_Refresh();
		}

		while (1)
		{
			gpio_bit_toggle(GPIOA, GPIO_PIN_5);
			delay_1ms(250);
			check_serial_commands();
			delay_1ms(250);
		}
		}

}
static void load_cfg(void)
{
	for (uint16_t i = 0; i < 1024 * 4; i++)
	{
		config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
	}
}
static bool Download_Transport(uint32_t DownLoad_Addr)
{
	uint32_t pages;
	uint32_t i;
	uint32_t app_crc32;
	uint32_t vi;
	uint32_t mismatch_cnt;
	uint8_t flash_byte;

	if (my_param_sum.BootParam.appSize == 0)
	{
		return false;
	}
	if (my_param_sum.BootParam.appSize > CONFIG_APP_SIZE)
	{
		return false;
	}

	memset(config_buf, 0, sizeof(config_buf));
	for (i = 0; i < my_param_sum.BootParam.appSize; i++)
	{
		config_buf[i] = internal_flash_read_Char(APP_DOWNLOAD_ADDR + i);
	}

	app_crc32 = crc32_calc(config_buf, my_param_sum.BootParam.appSize);
	if (app_crc32 != my_param_sum.BootParam.appCRC32)
	{
		return false;
	}

	pages = (my_param_sum.BootParam.appSize + 4095) / 4096;
	for (i = 0; i < pages; i++)
	{
		internal_flash_erase(my_param_sum.BootParam.appStartAddr + i * 4096);
	}

	internal_flash_write_str_Char(my_param_sum.BootParam.appStartAddr, config_buf, my_param_sum.BootParam.appSize);

	mismatch_cnt = 0;
	for (vi = 0; vi < my_param_sum.BootParam.appSize; vi++)
	{
		flash_byte = internal_flash_read_Char(my_param_sum.BootParam.appStartAddr + vi);
		if (flash_byte != config_buf[vi])
		{
			if (mismatch_cnt < 10)
			mismatch_cnt++;
		}
	}
	return true;
}

static const uint16_t crc16_tab[256] = {
    0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
    0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
    0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
    0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
    0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
    0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
    0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
    0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
    0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
    0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
    0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
    0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
    0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
    0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
    0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
    0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
    0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
    0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
    0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
    0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
    0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
    0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
    0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
    0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
    0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
    0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
    0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
    0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
    0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
    0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
    0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
    0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

static uint16_t crc16_modbus(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;
    uint32_t i;
    for (i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc16_tab[(crc ^ data[i]) & 0xFF];
    }
    return crc;
}

static uint8_t hex_char_to_nibble(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFF;
}


static int ascii_to_binary(const uint8_t *ascii, uint32_t len, uint8_t *binary, uint32_t sz)
{
    uint32_t i, bin_len;
    if (len & 1) return -1;
    bin_len = len / 2;
    if (bin_len > sz) return -1;
    for (i = 0; i < bin_len; i++) {
        uint8_t hi = hex_char_to_nibble((char)ascii[i * 2]);
        uint8_t lo = hex_char_to_nibble((char)ascii[i * 2 + 1]);
        if (hi == 0xFF || lo == 0xFF) return -1;
        binary[i] = (hi << 4) | lo;
    }
    return (int)bin_len;
}

static void byte_to_hex_pair(uint8_t b, char *out)
{
    static const char digits[] = "0123456789ABCDEF";
    out[0] = digits[b >> 4];
    out[1] = digits[b & 0x0F];
}


static void send_ok_response(uint16_t cmd)
{
    uint8_t bin[14];
    uint16_t crc;
    char ascii[30];
    int i;

    bin[0]  = 0xA5; bin[1]  = 0xB6;
    bin[2]  = 0x00; bin[3]  = 0x01;
    bin[4]  = 0x02;
    bin[5]  = (uint8_t)(cmd >> 8); bin[6]  = (uint8_t)(cmd);
    bin[7]  = 0x01;
    bin[8]  = 0x02;
    bin[9]  = 0xFF;
    crc = crc16_modbus(bin, 10);
    bin[10] = (uint8_t)(crc >> 8);
    bin[11] = (uint8_t)(crc & 0xFF);
    bin[12] = 0xB6; bin[13] = 0xA5;

    for (i = 0; i < 14; i++) {
        byte_to_hex_pair(bin[i], &ascii[i * 2]);
    }
    ascii[28] = '\r';
    ascii[29] = '\n';

    {
        int j;
        delay_1ms(5);
        __disable_irq();
        gpio_bit_set(GPIOE, GPIO_PIN_8);
        for (j = 0; j < 30; j++) {
            usart_data_transmit(USART1, (uint8_t)ascii[j]);
            while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
        }
        while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
        gpio_bit_reset(GPIOE, GPIO_PIN_8);
        __enable_irq();
    }
}

static void send_error_response(uint16_t cmd)
{
    uint8_t bin[13];
    uint16_t crc;
    char ascii[28];
    int i;

    bin[0]  = 0xA5; bin[1]  = 0xB6;
    bin[2]  = 0x00; bin[3]  = 0x01;
    bin[4]  = 0xFF;
    bin[5]  = (uint8_t)(cmd >> 8); bin[6]  = (uint8_t)(cmd);
    bin[7]  = 0x00;
    bin[8]  = 0x02;
    crc = crc16_modbus(bin, 9);
    bin[9]  = (uint8_t)(crc >> 8);
    bin[10] = (uint8_t)(crc & 0xFF);
    bin[11] = 0xB6; bin[12] = 0xA5;

    for (i = 0; i < 13; i++) {
        byte_to_hex_pair(bin[i], &ascii[i * 2]);
    }
    ascii[26] = '\r';
    ascii[27] = '\n';

    {
        int j;
        __disable_irq();
        gpio_bit_set(GPIOE, GPIO_PIN_8);
        for (j = 0; j < 28; j++) {
            usart_data_transmit(USART1, (uint8_t)ascii[j]);
            while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
        }
        while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
        gpio_bit_reset(GPIOE, GPIO_PIN_8);
        __enable_irq();
    }
}

static bool check_serial_commands(void)
{
    uint8_t local_buf[128];
    uint8_t local_len;
    uint8_t binary[64];
    int bin_len;
    uint16_t hdr, tail, cmd;
    uint8_t type;

    if (!usart_is_recv_ready())
        return false;

    usart_get_recv_data(local_buf, &local_len);
    if (local_len < 22)
        return false;

    bin_len = ascii_to_binary(local_buf, local_len, binary, sizeof(binary));
    if (bin_len < 12)
        return false;

    hdr  = ((uint16_t)binary[0] << 8) | binary[1];
    tail = ((uint16_t)binary[bin_len - 2] << 8) | binary[bin_len - 1];
    type = binary[4];
    cmd  = ((uint16_t)binary[5] << 8) | binary[6];

    if (hdr != 0xA5B6 || tail != 0xB6A5 || type != 0x01)
        return false;

    if (cmd == 0x0502) {
        bootloader_receive_firmware();
        return true;
    }
    if (cmd == 0x0503) {
        bootloader_install_firmware();
        return false;
    }
    return false;
}

static bool bootloader_receive_firmware(void)
{
    uint32_t i, fw_size, calc_crc;
    uint32_t timeout;

    usart_enter_download_mode(config_buf);

    {
        uint32_t last_idx = 0;
        uint32_t idle_cnt = 0;
        timeout = 0;
        while (1) {
            delay_1ms(100);
            timeout++;

            uint32_t cur_idx = usart_get_dl_idx();
            if (cur_idx != last_idx) {
                last_idx = cur_idx;
                idle_cnt = 0;
            } else if (cur_idx > 0) {
                idle_cnt++;
            }

            if (idle_cnt >= 5) {
                break;
            }

            if (timeout > 300) {
                usart_exit_download_mode();

                send_error_response(0x0502);
                return false;
            }
        }
    }

    fw_size = usart_get_dl_idx();
    usart_exit_download_mode();

    if (fw_size < 12) {

        send_error_response(0x0502);
        return false;
    }

    {
        uint32_t magic = ((uint32_t)config_buf[0] << 24) | ((uint32_t)config_buf[1] << 16) |
                         ((uint32_t)config_buf[2] << 8)  | ((uint32_t)config_buf[3] << 0);
        if (magic != 0x5AA5C33C) {

            send_error_response(0x0502);
            return false;
        }
    }

    calc_crc = crc32_calc(config_buf + 4, fw_size - 4);

    {
        uint32_t pages = ((fw_size - 4) + 4095) / 4096;
        for (i = 0; i < pages; i++) {
            internal_flash_erase(APP_DOWNLOAD_ADDR + i * 4096);
        }
    }

    usart_disable(USART1);
    usart_enable(USART1);
    usart_interrupt_enable(USART1, USART_INT_RBNE);
    usart_interrupt_enable(USART1, USART_INT_IDLE);


    internal_flash_write_str_Char(APP_DOWNLOAD_ADDR, config_buf + 4, fw_size - 4);


    for (i = 0; i < fw_size - 4; i++) {
        if (internal_flash_read_Char(APP_DOWNLOAD_ADDR + i) != config_buf[i + 4]) {

            send_error_response(0x0502);
            return false;
        }
    }


    send_ok_response(0x0502);

    {
        uint8_t param_buf[4096];
        Parameter_t *params;
        uint32_t j;

        for (j = 0; j < sizeof(Parameter_t); j++) {
            param_buf[j] = internal_flash_read_Char(BOOT_CONFIG_ADDR + j);
        }
        params = (Parameter_t *)param_buf;

        params->BootParam.magicWord    = 0x5AA5C33C;
        params->BootParam.updateFlag   = 0x5A;
        params->BootParam.updateStatus = 0x01;
        params->BootParam.appSize      = fw_size - 4;
        params->BootParam.appCRC32     = calc_crc;
        params->BootParam.appVersion   = 0;
        params->BootParam.appStartAddr = 0x08011000;

        params->BootParam.appStackAddr =
            ((uint32_t)config_buf[4] << 24) | ((uint32_t)config_buf[5] << 16) |
            ((uint32_t)config_buf[6] << 8)  | ((uint32_t)config_buf[7] << 0);
        params->BootParam.appEntryAddr =
            ((uint32_t)config_buf[8] << 24) | ((uint32_t)config_buf[9] << 16) |
            ((uint32_t)config_buf[10] << 8) | ((uint32_t)config_buf[11] << 0);

        internal_flash_erase(BOOT_CONFIG_ADDR);
        internal_flash_write_str_Char(BOOT_CONFIG_ADDR, param_buf, sizeof(Parameter_t));
    }

    return true;
}

static void bootloader_install_firmware(void)
{
    uint32_t j;
    for (j = 0; j < sizeof(Parameter_t); j++) {
        ((uint8_t *)&my_param_sum)[j] = internal_flash_read_Char(BOOT_CONFIG_ADDR + j);
    }

    if (my_param_sum.BootParam.updateStatus != 0x01 ||
        my_param_sum.BootParam.updateFlag != 0x5A) {

        send_error_response(0x0503);
        return;
    }


    if (Download_Transport(APP_DOWNLOAD_ADDR)) {
        for (j = 0; j < sizeof(Parameter_t); j++) {
            ((uint8_t *)&my_param_sum)[j] = internal_flash_read_Char(BOOT_CONFIG_ADDR + j);
        }
        my_param_sum.BootParam.updateStatus = 0x00;
        my_param_sum.BootParam.updateFlag   = 0x00;
        my_param_sum.BootParam.updateCount++;

        my_param_sum.BootParam.appStartAddr = 0x08011000;
        my_param_sum.BootParam.appStackAddr = *(__IO uint32_t *)0x08011000;
        my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t *)(0x08011000 + 4);

        memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
        internal_flash_erase(BOOT_CONFIG_ADDR);
        internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, sizeof(Parameter_t));

        send_ok_response(0x0503);

        OLED_Clear();
        OLED_Printf(0, 0, 16, "2026661795");
        OLED_Printf(0, 16, 16, "IDLE");
        OLED_Refresh();

        delay_1ms(500);

        *(uint32_t *)0x2002FFF8 = 0xB007B007;

        mcu_software_reset();
    } else {
        my_param_sum.BootParam.resetCount++;
        my_param_sum.BootParam.bootFailCount++;
        memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
        internal_flash_erase(BOOT_CONFIG_ADDR);
        internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, sizeof(Parameter_t));


        send_error_response(0x0503);
    }
}

static const uint32_t crc32_tab[256] = {
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
    0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
    0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
    0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
    0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C50,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
    0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
    0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
    0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
    0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
    0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
    0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
    0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
    0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB30A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
    0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
    0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x6162FFD3,0x1665CF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
    0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
    0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D
};

uint32_t crc32_calc(uint8_t* data , uint32_t len)
{
	uint32_t crc = 0xFFFFFFFF;
	uint32_t i;

	for (i = 0; i < len; i++)
	{
		crc = crc32_tab[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	}

	return crc ^ 0xFFFFFFFF;
}
void mcu_software_reset(void)
{
	__set_FAULTMASK(1);
	NVIC_SystemReset();
}
void iap_load_app(uint32_t appxaddr)
{
	uint32_t sp = *(__IO uint32_t*)appxaddr;
	if ((sp & 0x2FFC0000) == 0x20000000)
	{
		__disable_irq();

		SysTick->CTRL = 0;
		SysTick->LOAD = 0;
		SysTick->VAL = 0;

		for (uint32_t i = 0; i < 8; i++)
		{
			NVIC->ICER[i] = 0xFFFFFFFF;
			NVIC->ICPR[i] = 0xFFFFFFFF;
		}

		__DSB();
		__ISB();

		SCB->VTOR = appxaddr;

		__set_MSP(*(__IO uint32_t*)appxaddr);

		__enable_irq();

		jump2app = (pFunction)(*(__IO uint32_t*)(appxaddr + 4));
		jump2app();

		return;
	}
}

bool jump_to_app(void)
{
	uint32_t app_addr = 0x08011000;
	uint32_t sp, pc;
	int retry;

	for (retry = 0; retry < 10; retry++) {
		sp = *(__IO uint32_t*)(app_addr);
		pc = *(__IO uint32_t*)(app_addr + 4);
		if (sp != 0xFFFFFFFF && pc != 0xFFFFFFFF) break;
		delay_1ms(5);
	}

	if ((sp & 0x2FFC0000) == 0x20000000 && (pc & 0xFF000000) == 0x08000000)
	{
		usart_disable(USART1);
		iap_load_app(app_addr);
	}

	return false;
}

bool Backup_App(void)
{
	uint32_t crc32 = crc32_calc(((uint8_t*)0x08011000) , 128 * 1024);
	if (crc32 != my_param_sum.BootParam.backupCRC32)
	{
		return false;
	}

	for (uint8_t i = 0; i < 32; i++)
	{
		internal_flash_erase(0x08031000 + i * 1024 * 4);
		delay_1ms(30);
	}


	internal_flash_write_str_Char(0x08031000 , (uint8_t*)0x08011000 , 128 * 1024);
	uint32_t backupCRC32_1 = crc32_calc(((uint8_t*)0x08031000) , 128 * 1024);
	if (backupCRC32_1 != my_param_sum.BootParam.backupCRC32) return false;

	for (uint16_t i = 0; i < CONFIG_SIZE; i++)
	{
		config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
	}
	memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));
	my_param_sum.BootParam_Reserved.appVersion = my_param_sum.BootParam.appVersion;
	memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
	internal_flash_erase(BOOT_CONFIG_ADDR);
	internal_flash_write_str_Char(BOOT_CONFIG_ADDR , config_buf , sizeof(Parameter_t));
	return true;
}
