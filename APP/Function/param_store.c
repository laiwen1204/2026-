#include "param_store.h"

static param_t g_param;
static uint32_t g_alarm_count = 0;


static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 1U) ? ((crc >> 1) ^ 0xEDB88320UL) : (crc >> 1);
        }
    }
    return crc;
}

static uint32_t param_calc_crc(void)
{
    return (crc32_update(0xFFFFFFFFUL, (const uint8_t *)&g_param,
                        (uint32_t)offsetof(param_t, param_crc32))
            ^ 0xFFFFFFFFUL);
}


static void param_set_defaults(void)
{
    memset(&g_param, 0, sizeof(g_param));
    g_param.magic_word       = PARAM_MAGIC;
    g_param.struct_version   = 0x00010001;
    g_param.device_id        = 0x0001;
    g_param.baud_rate_code   = 0x13;
    g_param.comm_baud_code   = 0xFFFFFFFF;
    g_param.alarm_mode       = 2;
    g_param.report_interval  = 1;
    g_param.fw_version[0]    = 2;
    g_param.fw_version[1]    = 0;
    g_param.fw_version[2]    = 1;
    g_param.fw_version[3]    = 0;
    g_param.ch0_ratio        = 1.0f;
    g_param.ch1_ratio        = 1.0f;
    g_param.ch0_threshold    = 3.0f;
    g_param.ch1_threshold    = 3.0f;
    g_param.ch2_threshold    = 100.0f;
    g_param.param_crc32      = 0;
    g_param.param_tail_magic = 0xA5A5C3C3;
    g_param.param_crc32      = param_calc_crc();
    g_alarm_count            = 0;
}


void param_load(void)
{
    uint8_t *p = (uint8_t *)&g_param;
    for (uint32_t i = 0; i < sizeof(param_t); i++) {
        p[i] = internal_flash_read_Char(PARAM_BASE_ADDR + i);
    }
}

void param_save(void)
{
    g_param.param_crc32 = param_calc_crc();
    internal_flash_erase(PARAM_BASE_ADDR);
    internal_flash_write_str_Char(PARAM_BASE_ADDR, (uint8_t *)&g_param, sizeof(param_t));
}

void param_init(void)
{
    param_load();
    if (g_param.magic_word != PARAM_MAGIC) {
        param_set_defaults();
        param_save();
    }
    {
        bool need_save = false;
        if ((uint8_t)g_param.comm_baud_code != g_param.baud_rate_code) {
            g_param.comm_baud_code = g_param.baud_rate_code;
            need_save = true;
        }
        if (g_param.struct_version != 0x00010001) {
            g_param.struct_version = 0x00010001;
            need_save = true;
        }
        if (g_param.param_tail_magic != 0xA5A5C3C3) {
            g_param.param_tail_magic = 0xA5A5C3C3;
            need_save = true;
        }
        if (need_save || g_param.param_crc32 != param_calc_crc()) {
            param_save();
        }
    }
    g_alarm_count = 0;
    for (int i = 0; i < MAX_ALARM_RECORDS; i++) {
        if (g_param.alarm_records[i].timestamp != 0) {
            g_alarm_count++;
        }
    }
}


uint16_t param_get_device_id(void)           { return g_param.device_id; }
void     param_set_device_id(uint16_t id)     { g_param.device_id = id; param_save(); }

uint8_t  param_get_baud_rate(void)            { return g_param.baud_rate_code; }
void     param_set_baud_rate(uint8_t code)    { g_param.baud_rate_code = code; g_param.comm_baud_code = code; param_save(); }

uint8_t  param_get_alarm_mode(void)           { return g_param.alarm_mode; }
void     param_set_alarm_mode(uint8_t mode)   { g_param.alarm_mode = mode; param_save(); }

uint8_t  param_get_report_interval(void)       { return g_param.report_interval; }
void     param_set_report_interval(uint8_t i)  { g_param.report_interval = i; param_save(); }

void     param_get_fw_version(uint8_t *buf)    { memcpy(buf, g_param.fw_version, 4); }

uint32_t param_get_update_flag(void)           { return g_param.update_flag; }
void     param_set_update_flag(uint32_t flag)  { g_param.update_flag = flag; param_save(); }

uint32_t param_get_app_version(void)           { return g_param.app_version; }
void     param_set_app_version(uint32_t ver)   { g_param.app_version = ver; param_save(); }

uint32_t param_get_app_crc32(void)             { return g_param.app_crc32; }
void     param_set_app_crc32(uint32_t crc)     { g_param.app_crc32 = crc; param_save(); }

uint32_t param_get_app_size(void)              { return g_param.app_size; }
void     param_set_app_size(uint32_t size)     { g_param.app_size = size; param_save(); }


float param_get_ch0_ratio(void)                { return g_param.ch0_ratio; }
void  param_set_ch0_ratio(float r)             { g_param.ch0_ratio = r; param_save(); }
float param_get_ch1_ratio(void)                { return g_param.ch1_ratio; }
void  param_set_ch1_ratio(float r)             { g_param.ch1_ratio = r; param_save(); }


float param_get_ch0_threshold(void)            { return g_param.ch0_threshold; }
void  param_set_ch0_threshold(float t)         { g_param.ch0_threshold = t; param_save(); }
float param_get_ch1_threshold(void)            { return g_param.ch1_threshold; }
void  param_set_ch1_threshold(float t)         { g_param.ch1_threshold = t; param_save(); }
float param_get_ch2_threshold(void)            { return g_param.ch2_threshold; }
void  param_set_ch2_threshold(float t)         { g_param.ch2_threshold = t; param_save(); }


int param_alarm_count(void)
{
    return (int)g_alarm_count;
}

void param_alarm_add(uint8_t channel, float threshold, float actual)
{
    if (g_alarm_count >= MAX_ALARM_RECORDS) {
        memmove(&g_param.alarm_records[0], &g_param.alarm_records[1],
                sizeof(alarm_record_t) * (MAX_ALARM_RECORDS - 1));
        g_alarm_count = MAX_ALARM_RECORDS - 1;
    }

    uint16_t y, mo, d, h, mi, s;
    rtc_get_time(&y, &mo, &d, &h, &mi, &s);
    uint32_t now = bcd2ts(y, mo, d, h, mi, s);

    alarm_record_t *rec = &g_param.alarm_records[g_alarm_count];
    memset(rec, 0, sizeof(alarm_record_t));
    rec->timestamp       = now;
    rec->channel         = channel;
    rec->threshold_value = threshold;
    rec->actual_value    = actual;
    g_alarm_count++;
    param_save();
}

void param_alarm_clear(void)
{
    memset(g_param.alarm_records, 0, sizeof(g_param.alarm_records));
    g_alarm_count = 0;
    param_save();
}

void param_alarm_get_recent(alarm_record_t *buf, int max_count, int *out_count)
{
    int total = (int)g_alarm_count;
    int count = (total < max_count) ? total : max_count;
    *out_count = count;

    for (int i = 0; i < count; i++) {
        memcpy(&buf[i], &g_param.alarm_records[total - 1 - i],
               sizeof(alarm_record_t));
    }
}
