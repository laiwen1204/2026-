#include "BootConfig.h"

void bootloader_config_init(BootParam_t *param, UpdateLog_t *mylog,
                            UserConfig_t *userconfig, CalibData_t *calibdata)
{
    BootParam_t  tmp_param    = { 0 };
    UpdateLog_t  tmp_log      = { 0 };
    UserConfig_t tmp_usercfg  = { 0 };
    CalibData_t  tmp_calib    = { 0 };

    tmp_param.magicWord    = 0x5AA5C33C;
    tmp_param.version      = 0x0001;
    tmp_param.structSize   = 256;
    tmp_param.updateMode   = 0x01;
    tmp_param.appStartAddr = 0x08011000;
    tmp_param.appEntryAddr = 0x08011000;
    tmp_param.appStackAddr = *(uint32_t *)0x08011000;
    tmp_param.bootVersion  = 0x01;
    tmp_param.bootSize     = 4096;
    tmp_param.backupAddr   = 0x08031000;
    tmp_param.backupSize   = 256;

    memcpy(tmp_param.deviceID,     "202601301528", 12);
    memcpy(tmp_param.productModel, "000000000001", 12);
    memcpy(tmp_param.serialNumber, "100000000000", 12);

    tmp_param.hwVersion  = 1;
    tmp_param.cpuID      = 1;
    tmp_param.flashSize  = 0x400;
    tmp_param.ramSize    = 0x2F;
    tmp_param.clockFreq  = 240000000;
    tmp_param.tailMagic  = 0xA5A5C3C3;

    tmp_log.mode = 1;

    tmp_usercfg.uart_baudrate = 19200;
    tmp_usercfg.uart_stopbit  = 1;

    tmp_calib.calib_magic = 0xCAC0FFEE;

    *param      = tmp_param;
    *mylog      = tmp_log;
    *userconfig = tmp_usercfg;
    *calibdata  = tmp_calib;
}
