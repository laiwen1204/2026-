#ifndef __PARAM_STORE_H
#define __PARAM_STORE_H

#include "HeaderFiles.h"

#define PARAM_BASE_ADDR      0x08010000
#define PARAM_PAGE_SIZE      0x1000

#define PARAM_MAGIC          0x5AA5C33C
#define MAX_ALARM_RECORDS    30

typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint8_t  channel;
    uint8_t  reserved[3];
    float    threshold_value;
    float    actual_value;
    uint8_t  _pad[111];
} alarm_record_t;

typedef struct __attribute__((packed)) {
    uint32_t magic_word;
    uint32_t struct_version;
    uint16_t device_id;
    uint8_t  baud_rate_code;
    uint8_t  alarm_mode;
    uint8_t  report_interval;
    uint8_t  reserved_sys[3];
    uint8_t  fw_version[4];
    uint32_t reserved_model[3];
    uint32_t reserved1[3];
    uint32_t comm_baud_code;
    uint32_t reserved1_rem[3];

    float    ch0_ratio;
    float    ch1_ratio;
    float    ch0_threshold;
    float    ch1_threshold;
    float    ch2_threshold;
    uint32_t update_flag;
    uint32_t app_version;
    uint32_t app_crc32;
    uint32_t app_size;
    uint8_t  reserved2[0xF4 - 0x60];
    uint32_t reserved3;
    uint32_t param_crc32;
    uint32_t param_tail_magic;

    alarm_record_t alarm_records[MAX_ALARM_RECORDS];
} param_t;


void param_init(void);
void param_save(void);
void param_load(void);

uint16_t param_get_device_id(void);
void     param_set_device_id(uint16_t id);
uint8_t  param_get_baud_rate(void);
void     param_set_baud_rate(uint8_t code);
uint8_t  param_get_alarm_mode(void);
void     param_set_alarm_mode(uint8_t mode);
uint8_t  param_get_report_interval(void);
void     param_set_report_interval(uint8_t interval);
void     param_get_fw_version(uint8_t *buf);
uint32_t param_get_update_flag(void);
void     param_set_update_flag(uint32_t flag);
uint32_t param_get_app_version(void);
void     param_set_app_version(uint32_t ver);
uint32_t param_get_app_crc32(void);
void     param_set_app_crc32(uint32_t crc);
uint32_t param_get_app_size(void);
void     param_set_app_size(uint32_t size);

float    param_get_ch0_ratio(void);
void     param_set_ch0_ratio(float ratio);
float    param_get_ch1_ratio(void);
void     param_set_ch1_ratio(float ratio);

float    param_get_ch0_threshold(void);
void     param_set_ch0_threshold(float threshold);
float    param_get_ch1_threshold(void);
void     param_set_ch1_threshold(float threshold);
float    param_get_ch2_threshold(void);
void     param_set_ch2_threshold(float threshold);

int      param_alarm_count(void);
void     param_alarm_add(uint8_t channel, float threshold, float actual);
void     param_alarm_clear(void);
void     param_alarm_get_recent(alarm_record_t *buf, int max_count, int *out_count);

#endif
