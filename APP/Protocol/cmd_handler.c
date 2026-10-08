#include "HeaderFiles.h"
#include "cmd_handler.h"

static bool     g_auto_report_active = false;
static uint32_t g_auto_report_interval_ms = 1000;
static uint32_t g_auto_report_last = 0;

bool ar_active(void) { return g_auto_report_active; }

static void f2b(float val, uint8_t *out)
{
    uint32_t bits;
    memcpy(&bits, &val, 4);
    out[0] = (uint8_t)(bits >> 24);
    out[1] = (uint8_t)(bits >> 16);
    out[2] = (uint8_t)(bits >> 8);
    out[3] = (uint8_t)(bits);
}

static float b2f(const uint8_t *in)
{
    uint32_t bits = ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) |
                    ((uint32_t)in[2] << 8)  | ((uint32_t)in[3]);
    float val;
    memcpy(&val, &bits, 4);
    return val;
}


static void handle_reset(const frame_t *req)
{
    send_ok(req);
    NVIC_SystemReset();
}

static void handle_query_version(const frame_t *req)
{
    uint8_t ver[4];
    param_get_fw_version(ver);
    send_resp(req, ver, 4);
}

static void handle_set_time(const frame_t *req)
{
    if (req->length != 4) { send_err(req); return; }

    uint32_t ts = ((uint32_t)req->content[0] << 24) |
                  ((uint32_t)req->content[1] << 16) |
                  ((uint32_t)req->content[2] << 8)  |
                  ((uint32_t)req->content[3]);

    uint16_t y, mo, d, h, mi, s;
    ts2bcd(ts, &y, &mo, &d, &h, &mi, &s);
    rtc_set_time_now(y, mo, d, h, mi, s);

    g_sys.year = y; g_sys.month = mo; g_sys.day = d;
    g_sys.hour = h; g_sys.minute = mi; g_sys.second = s;

    send_ok(req);
}

static void handle_query_time(const frame_t *req)
{
    uint16_t y, mo, d, h, mi, s;
    rtc_get_time(&y, &mo, &d, &h, &mi, &s);
    uint32_t ts = bcd2ts(y, mo, d, h, mi, s);

    uint8_t buf[4];
    buf[0] = (uint8_t)(ts >> 24);
    buf[1] = (uint8_t)(ts >> 16);
    buf[2] = (uint8_t)(ts >> 8);
    buf[3] = (uint8_t)(ts);
    send_resp(req, buf, 4);
}

static void handle_set_device_id(const frame_t *req)
{
    if (req->length != 2) { send_err(req); return; }

    uint16_t new_id = ((uint16_t)req->content[0] << 8) | req->content[1];
    if (new_id == 0x0000 || new_id == 0xFFFF) { send_err(req); return; }

    param_set_device_id(new_id);
    proto_set_id(new_id);

    uint8_t ok = 0xFF;
    static uint8_t ascii[MAX_ASCII_FRAME];
    int len = build_frame(ascii, sizeof(ascii),
                                   new_id, FRAME_TYPE_RESPONSE, req->command,
                                   &ok, 1);
    if (len > 0) rs485_send_raw(ascii, len);
}

static void handle_query_device_id(const frame_t *req)
{
    uint16_t id = param_get_device_id();
    uint8_t buf[2];
    buf[0] = (uint8_t)(id >> 8);
    buf[1] = (uint8_t)(id & 0xFF);
    send_resp(req, buf, 2);
}

static void handle_set_baudrate(const frame_t *req)
{
    if (req->length != 1) { send_err(req); return; }

    uint8_t code = req->content[0];
    if (code != BAUD_4800 && code != BAUD_9600 &&
        code != BAUD_19200 && code != BAUD_115200) {
        send_err(req); return;
    }

    param_set_baud_rate(code);

    send_ok(req);

    uint32_t baud = 19200;
    if (code == BAUD_4800)       baud = 4800;
    else if (code == BAUD_9600)  baud = 9600;
    else if (code == BAUD_19200) baud = 19200;
    else if (code == BAUD_115200) baud = 115200;
    rs485_set_baud(baud);
}

static void handle_query_baudrate(const frame_t *req)
{
    uint8_t code = param_get_baud_rate();
    send_resp(req, &code, 1);
}


static void handle_query_ch0(const frame_t *req)
{
    float raw = (float)adc_dma_buffer * 3.3f / 4095.0f;
    float val = raw * param_get_ch0_ratio();
    uint8_t buf[4];
    f2b(val, buf);
    send_resp(req, buf, 4);
}

static void handle_query_ch1(const frame_t *req)
{
    float raw = (float)dac_get_output_value() * 3.3f / 4095.0f;
    float val = raw * param_get_ch1_ratio();
    uint8_t buf[4];
    f2b(val, buf);
    send_resp(req, buf, 4);
}

static void handle_query_pt100(const frame_t *req)
{
    float temperature = 0.0f;
    if (PT100_IsOK()) {
        uint16_t raw = PT100_ReadADC();
        float voltage = ((float)raw / 32767.0f) * 1.024f;
        float resistance = (voltage / 50.63f) / 0.0001f;
        temperature = (resistance - 100.0f) / 0.385f;
    }
    uint8_t buf[4];
    f2b(temperature, buf);
    send_resp(req, buf, 4);
}

static void handle_set_ch0_ratio(const frame_t *req)
{
    if (req->length != 4) { send_err(req); return; }
    float ratio = b2f(req->content);
    param_set_ch0_ratio(ratio);
    send_ok(req);
}

static void handle_set_ch1_ratio(const frame_t *req)
{
    if (req->length != 4) { send_err(req); return; }
    float ratio = b2f(req->content);
    param_set_ch1_ratio(ratio);
    send_ok(req);
}

static void handle_set_report_interval(const frame_t *req)
{
    if (req->length != 1) { send_err(req); return; }
    uint8_t code = req->content[0];
    if (code < 1 || code > 3) { send_err(req); return; }

    param_set_report_interval(code);

    uint32_t ms[] = {0, 1000, 3000, 5000};
    g_auto_report_interval_ms = ms[code];
    send_ok(req);
}


static void handle_set_dac(const frame_t *req)
{
    if (req->length != 2) { send_err(req); return; }

    uint16_t value = ((uint16_t)req->content[0] << 8) | req->content[1];
    if (value > 4095) value = 4095;

    dac_set_output_value(value);

    send_ok(req);
}

static void handle_start_auto_report(const frame_t *req)
{
    g_auto_report_active = true;
    g_auto_report_last = 0;
    send_ok(req);
}

static void handle_stop_auto_report(const frame_t *req)
{
    g_auto_report_active = false;
    send_ok(req);
}

static void handle_sleep(const frame_t *req)
{
    send_ok(req);
    for (volatile int i = 0; i < 100000; i++) {}
    OLED_WR_Byte(0xAE, OLED_CMD);
    pmu_enter_sleep_10s();
    OLED_WR_Byte(0xAF, OLED_CMD);
}


static void handle_read_threshold_batch(const frame_t *req)
{
    uint8_t buf[8];
    f2b(param_get_ch0_threshold(), buf);
    f2b(param_get_ch1_threshold(), buf + 4);
    send_resp(req, buf, 8);
}

static void handle_read_ch0_threshold(const frame_t *req)
{
    uint8_t buf[4];
    f2b(param_get_ch0_threshold(), buf);
    send_resp(req, buf, 4);
}

static void handle_read_ch1_threshold(const frame_t *req)
{
    uint8_t buf[4];
    f2b(param_get_ch1_threshold(), buf);
    send_resp(req, buf, 4);
}

static void handle_read_ch2_threshold(const frame_t *req)
{
    uint8_t buf[4];
    f2b(param_get_ch2_threshold(), buf);
    send_resp(req, buf, 4);
}

static void handle_write_ch0_threshold(const frame_t *req)
{
    if (req->length != 4) { send_err(req); return; }
    param_set_ch0_threshold(b2f(req->content));
    send_ok(req);
}

static void handle_write_ch1_threshold(const frame_t *req)
{
    if (req->length != 4) { send_err(req); return; }
    param_set_ch1_threshold(b2f(req->content));
    send_ok(req);
}

static void handle_write_ch2_threshold(const frame_t *req)
{
    if (req->length != 4) { send_err(req); return; }
    param_set_ch2_threshold(b2f(req->content));
    send_ok(req);
}


static void handle_upgrade_request(const frame_t *req)
{
    send_ok(req);
    delay_1ms(200);

    *(uint32_t *)0x2002FFFC = 0x424C5550;

    __set_FAULTMASK(1);
    NVIC_SystemReset();
}

static void handle_start_transfer(const frame_t *req)
{
    boot_if_activate();
    send_ok(req);
}


static void handle_set_alarm_reporting(const frame_t *req)
{
    if (req->length != 1) { send_err(req); return; }
    uint8_t mode = req->content[0];
    if (mode != 1 && mode != 2) { send_err(req); return; }
    param_set_alarm_mode(mode);
    send_ok(req);
}

static void handle_query_alarm_records(const frame_t *req)
{
    alarm_record_t records[10];
    int count = 0;
    param_alarm_get_recent(records, 10, &count);

    if (count == 0) {
        send_str("empty\r\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        alarm_record_t *r = &records[i];
        uint16_t y, mo, d, h, mi, s;
        ts2bcd(r->timestamp, &y, &mo, &d, &h, &mi, &s);

        uint32_t dy  = ((y >> 4) & 0x0F) * 10 + (y & 0x0F);
        uint32_t dmo = ((mo >> 4) & 0x0F) * 10 + (mo & 0x0F);
        uint32_t dd  = ((d >> 4) & 0x0F) * 10 + (d & 0x0F);
        uint32_t dh  = ((h >> 4) & 0x0F) * 10 + (h & 0x0F);
        uint32_t dmi = ((mi >> 4) & 0x0F) * 10 + (mi & 0x0F);
        uint32_t ds  = ((s >> 4) & 0x0F) * 10 + (s & 0x0F);

        char line[128];
        int n = snprintf(line, sizeof(line),
                         "%04lu-%02lu-%02lu %02lu:%02lu:%02lu|CH%u|%.2f|%.2f\r\n",
                         (unsigned long)(2000 + dy),
                         (unsigned long)dmo, (unsigned long)dd,
                         (unsigned long)dh, (unsigned long)dmi, (unsigned long)ds,
                         (unsigned int)r->channel,
                         (double)r->threshold_value, (double)r->actual_value);
        if (n > 0) send_str(line);
    }
}

static void handle_clear_alarms(const frame_t *req)
{
    param_alarm_clear();
    send_ok(req);
}


static void handle_heartbeat(const frame_t *req)
{
    send_hb();
}

static void handle_broadcast_seek(const frame_t *req)
{
    send_hb();
}


int cmd_dispatch(proto_ctx_t *ctx)
{
    frame_t *f = &ctx->rx_frame;
    uint16_t cmd = f->command;

    if (g_auto_report_active) {
        if (f->type == FRAME_TYPE_HEARTBEAT) {
            if (cmd == CMD_HEARTBEAT) { handle_heartbeat(f); return 0; }
            if (cmd == CMD_BROADCAST_SEEK) { handle_broadcast_seek(f); return 0; }
            return -1;
        }
        if (f->type == FRAME_TYPE_COMMAND && cmd == 0x0303) {
            handle_stop_auto_report(f);
            return 0;
        }
        return 0;
    }

    if (f->type == FRAME_TYPE_HEARTBEAT) {
        if (cmd == CMD_HEARTBEAT) { handle_heartbeat(f); return 0; }
        if (cmd == CMD_BROADCAST_SEEK) { handle_broadcast_seek(f); return 0; }
        return -1;
    }

    if (f->type != FRAME_TYPE_COMMAND && f->type != FRAME_TYPE_HEARTBEAT) {
        return -1;
    }

    switch (cmd) {
    case 0x0101: handle_reset(f);              break;
    case 0x0104: handle_query_version(f);      break;
    case 0x0105: handle_set_time(f);           break;
    case 0x0106: handle_query_time(f);         break;
    case 0x01A1: handle_set_device_id(f);      break;
    case 0x0111: handle_query_device_id(f);    break;
    case 0x01A2: handle_set_baudrate(f);       break;
    case 0x0112: handle_query_baudrate(f);     break;

    case 0x0201: handle_query_ch0(f);          break;
    case 0x0202: handle_query_ch1(f);          break;
    case 0x0221: handle_query_pt100(f);        break;
    case 0x0241: handle_set_ch0_ratio(f);      break;
    case 0x0242: handle_set_ch1_ratio(f);      break;
    case 0x0261: handle_set_report_interval(f); break;

    case 0x0301: handle_set_dac(f);            break;
    case 0x0302: handle_start_auto_report(f);  break;
    case 0x0303: handle_stop_auto_report(f);   break;
    case 0x03AA: handle_sleep(f);              break;

    case 0x0400: handle_read_threshold_batch(f); break;
    case 0x0401: handle_read_ch0_threshold(f); break;
    case 0x0402: handle_read_ch1_threshold(f); break;
    case 0x0403: handle_read_ch2_threshold(f); break;
    case 0x0411: handle_write_ch0_threshold(f); break;
    case 0x0412: handle_write_ch1_threshold(f); break;
    case 0x0413: handle_write_ch2_threshold(f); break;

    case 0x0501: handle_upgrade_request(f);    break;
    case 0x0502: handle_start_transfer(f);     break;

    case 0x0601: handle_set_alarm_reporting(f); break;
    case 0x0602: handle_query_alarm_records(f); break;
    case 0x0603: handle_clear_alarms(f);       break;

    default:
        return -1;
    }

    return 0;
}


void ar_proc(void)
{
    if (!g_auto_report_active) return;

    uint32_t now = GetSysRunTime();
    if (now - g_auto_report_last < g_auto_report_interval_ms) return;
    g_auto_report_last = now;

    uint16_t y, mo, d, h, mi, s;
    rtc_get_time(&y, &mo, &d, &h, &mi, &s);
    uint32_t ts = bcd2ts(y, mo, d, h, mi, s);

    float ch0_raw = (float)adc_dma_buffer * 3.3f / 4095.0f;
    float ch1_raw = (float)dac_get_output_value() * 3.3f / 4095.0f;
    float ch0_val = ch0_raw * param_get_ch0_ratio();
    float ch1_val = ch1_raw * param_get_ch1_ratio();

    uint8_t buf[12];
    buf[0] = (uint8_t)(ts >> 24);
    buf[1] = (uint8_t)(ts >> 16);
    buf[2] = (uint8_t)(ts >> 8);
    buf[3] = (uint8_t)(ts);
    f2b(ch0_val, buf + 4);
    f2b(ch1_val, buf + 8);

    static uint8_t ascii[MAX_ASCII_FRAME];
    uint16_t dev_id = param_get_device_id();
    int len = build_frame(ascii, sizeof(ascii),
                                   dev_id, FRAME_TYPE_RESPONSE, 0x0302,
                                   buf, 12);
    if (len > 0) rs485_send_raw(ascii, len);
}


void alarm_proc(void)
{
    float ch0_raw = (float)adc_dma_buffer * 3.3f / 4095.0f;
    float ch1_raw = (float)dac_get_output_value() * 3.3f / 4095.0f;
    float ch0_val = ch0_raw * param_get_ch0_ratio();
    float ch1_val = ch1_raw * param_get_ch1_ratio();

    uint16_t y, mo, d, h, mi, s;
    rtc_get_time(&y, &mo, &d, &h, &mi, &s);
    uint16_t by = ((y >> 4) & 0x0F) * 10 + (y & 0x0F);
    uint16_t bm = ((mo >> 4) & 0x0F) * 10 + (mo & 0x0F);
    uint16_t bd = ((d >> 4) & 0x0F) * 10 + (d & 0x0F);
    uint16_t bh = ((h >> 4) & 0x0F) * 10 + (h & 0x0F);
    uint16_t bmi = ((mi >> 4) & 0x0F) * 10 + (mi & 0x0F);
    uint16_t bs = ((s >> 4) & 0x0F) * 10 + (s & 0x0F);

    if (ch0_val > param_get_ch0_threshold()) {
        param_alarm_add(0, param_get_ch0_threshold(), ch0_val);
        if (param_get_alarm_mode() == 1) {
            char line[128];
            snprintf(line, sizeof(line),
                     "20%02d-%02d-%02d %02d:%02d:%02d|CH0|%.2f|%.2f\r\n",
                     by, bm, bd, bh, bmi, bs,
                     (double)param_get_ch0_threshold(), (double)ch0_val);
            send_str(line);
        }
    }

    if (ch1_val > param_get_ch1_threshold()) {
        param_alarm_add(1, param_get_ch1_threshold(), ch1_val);
        if (param_get_alarm_mode() == 1) {
            char line[128];
            snprintf(line, sizeof(line),
                     "20%02d-%02d-%02d %02d:%02d:%02d|CH1|%.2f|%.2f\r\n",
                     by, bm, bd, bh, bmi, bs,
                     (double)param_get_ch1_threshold(), (double)ch1_val);
            send_str(line);
        }
    }
}
