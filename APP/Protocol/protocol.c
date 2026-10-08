#include "HeaderFiles.h"
#include "protocol.h"

static proto_ctx_t g_proto_ctx = {
    .my_device_id = DEFAULT_DEVICE_ID,
    .frame_ready  = false,
    .frame_error  = false,
    .download_mode = false,
};

proto_ctx_t *get_ctx(void)
{
    return &g_proto_ctx;
}

void proto_set_id(uint16_t id)
{
    g_proto_ctx.my_device_id = id;
}


uint8_t hex_char_to_nibble(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFF;
}

uint8_t hex_pair_to_byte(const char *hex)
{
    return (hex_char_to_nibble(hex[0]) << 4) | hex_char_to_nibble(hex[1]);
}

void byte_to_hex_pair(uint8_t b, char *out)
{
    static const char digits[] = "0123456789ABCDEF";
    out[0] = digits[b >> 4];
    out[1] = digits[b & 0x0F];
}


static int build_binary_frame(uint8_t *out, uint16_t out_sz,
                               uint16_t dev_id, uint8_t type, uint16_t cmd,
                               const uint8_t *content, uint8_t content_len)
{
    uint16_t crc;
    uint32_t bin_len = 13 + content_len;

    if (bin_len > out_sz) return -1;

    out[0] = (uint8_t)(FRAME_HEADER >> 8);
    out[1] = (uint8_t)(FRAME_HEADER & 0xFF);
    out[2] = (uint8_t)(dev_id >> 8);
    out[3] = (uint8_t)(dev_id & 0xFF);
    out[4] = type;
    out[5] = (uint8_t)(cmd >> 8);
    out[6] = (uint8_t)(cmd & 0xFF);
    out[7] = content_len;
    out[8] = PROTOCOL_VERSION;

    if (content_len > 0) {
        memcpy(&out[9], content, content_len);
    }

    crc = crc16_modbus(out, 9 + content_len);
    out[9 + content_len]     = (uint8_t)(crc >> 8);
    out[9 + content_len + 1] = (uint8_t)(crc & 0xFF);

    out[9 + content_len + 2] = (uint8_t)(FRAME_TAIL >> 8);
    out[9 + content_len + 3] = (uint8_t)(FRAME_TAIL & 0xFF);

    return 13 + content_len;
}


int build_frame(uint8_t *out_ascii, uint16_t out_sz,
                uint16_t dev_id, uint8_t type, uint16_t cmd,
                const uint8_t *content, uint8_t content_len)
{
    static uint8_t binary[MAX_BINARY_FRAME];
    int bin_len = build_binary_frame(binary, sizeof(binary),
                                     dev_id, type, cmd, content, content_len);
    if (bin_len < 0 || (uint32_t)(bin_len * 2 + 2) > out_sz) return -1;

    for (int i = 0; i < bin_len; i++) {
        byte_to_hex_pair(binary[i], (char *)&out_ascii[i * 2]);
    }
    out_ascii[bin_len * 2]     = '\r';
    out_ascii[bin_len * 2 + 1] = '\n';
    return bin_len * 2 + 2;
}

int build_ok(uint8_t *out_ascii, uint16_t out_sz, const frame_t *req)
{
    uint8_t ok = 0xFF;
    return build_frame(out_ascii, out_sz,
                       g_proto_ctx.my_device_id,
                       FRAME_TYPE_RESPONSE, req->command,
                       &ok, 1);
}

int build_err(uint8_t *out_ascii, uint16_t out_sz, const frame_t *req)
{
    return build_frame(out_ascii, out_sz,
                       g_proto_ctx.my_device_id,
                       FRAME_TYPE_ERROR, CMD_ERROR_CMD,
                       NULL, 0);
}


void send_ok(const frame_t *req)
{
    static uint8_t ascii[MAX_ASCII_FRAME];
    int len = build_ok(ascii, sizeof(ascii), req);
    if (len > 0) rs485_send_raw(ascii, len);
}

void send_resp(const frame_t *req, const uint8_t *content, uint8_t len)
{
    static uint8_t ascii[MAX_ASCII_FRAME];
    int alen = build_frame(ascii, sizeof(ascii),
                           g_proto_ctx.my_device_id,
                           FRAME_TYPE_RESPONSE, req->command,
                           content, len);
    if (alen > 0) rs485_send_raw(ascii, alen);
}

void send_err(const frame_t *req)
{
    static uint8_t ascii[MAX_ASCII_FRAME];
    int len = build_err(ascii, sizeof(ascii), req);
    if (len > 0) rs485_send_raw(ascii, len);
}

void send_hb(void)
{
    static uint8_t ascii[MAX_ASCII_FRAME];
    int len = build_frame(ascii, sizeof(ascii),
                          g_proto_ctx.my_device_id,
                          FRAME_TYPE_HEARTBEAT, CMD_HEARTBEAT,
                          NULL, 0);
    if (len > 0) rs485_send_raw(ascii, len);
}

void send_str(const char *str)
{
    rs485_send_raw((const uint8_t *)str, strlen(str));
}


int check_crc(const uint8_t *binary_buf, uint32_t len)
{
    if (len < 11) return -1;

    uint8_t content_len = binary_buf[7];
    uint32_t calc_over = 9 + content_len;
    if (calc_over + 2 > len) return -1;
    if (calc_over < 9) return -1;

    uint16_t received_crc = ((uint16_t)binary_buf[calc_over] << 8) |
                            binary_buf[calc_over + 1];
    uint16_t calc_crc = crc16_modbus(binary_buf, calc_over);

    return (received_crc == calc_crc) ? 0 : -1;
}

static int ascii_to_binary(const uint8_t *ascii, uint32_t ascii_len,
                            uint8_t *binary, uint32_t binary_sz)
{
    if (ascii_len & 1) return -1;

    uint32_t bin_len = ascii_len / 2;
    if (bin_len > binary_sz) return -1;

    for (uint32_t i = 0; i < bin_len; i++) {
        uint8_t hi = hex_char_to_nibble((char)ascii[i * 2]);
        uint8_t lo = hex_char_to_nibble((char)ascii[i * 2 + 1]);
        if (hi == 0xFF || lo == 0xFF) return -1;
        binary[i] = (hi << 4) | lo;
    }
    return bin_len;
}

int proto_parse(proto_ctx_t *ctx, const uint8_t *ascii_buf, uint32_t len)
{
    static uint8_t binary[MAX_BINARY_FRAME];
    int bin_len;

    ctx->frame_ready = false;
    ctx->frame_error = false;

    if (len < 24) {
        ctx->error_reason = PROTO_ERR_FORMAT;
        return -1;
    }

    bin_len = ascii_to_binary(ascii_buf, len, binary, sizeof(binary));
    if (bin_len < 12) {
        ctx->error_reason = PROTO_ERR_FORMAT;
        return -1;
    }

    uint16_t hdr = ((uint16_t)binary[0] << 8) | binary[1];
    if (hdr != FRAME_HEADER) {
        ctx->error_reason = PROTO_ERR_FORMAT;
        return -1;
    }

    uint16_t tail = ((uint16_t)binary[bin_len - 2] << 8) | binary[bin_len - 1];
    if (tail != FRAME_TAIL) {
        ctx->error_reason = PROTO_ERR_FORMAT;
        return -1;
    }

    if (check_crc(binary, bin_len - 2) != 0) {
        ctx->error_reason = PROTO_ERR_CRC;
        ctx->frame_error = true;
        return -1;
    }

    uint16_t dev_id  = ((uint16_t)binary[2] << 8) | binary[3];
    uint8_t  type    = binary[4];
    uint16_t cmd     = ((uint16_t)binary[5] << 8) | binary[6];
    uint8_t  clen    = binary[7];
    uint8_t  ver     = binary[8];

    if (clen > sizeof(ctx->rx_frame.content)) {
        ctx->error_reason = PROTO_ERR_LENGTH;
        ctx->frame_error = true;
        return -1;
    }
    if (9 + clen + 2 + 2 != (uint32_t)bin_len) {
        ctx->error_reason = PROTO_ERR_LENGTH;
        ctx->frame_error = true;
        return -1;
    }

    if (dev_id != BROADCAST_ID && dev_id != ctx->my_device_id) {
        return -1;
    }

    ctx->rx_frame.dev_id  = dev_id;
    ctx->rx_frame.type    = type;
    ctx->rx_frame.command = cmd;
    ctx->rx_frame.length  = clen;
    ctx->rx_frame.version = ver;
    if (clen > 0) {
        memcpy(ctx->rx_frame.content, &binary[9], clen);
    }

    ctx->frame_ready = true;
    ctx->error_reason = PROTO_ERR_NONE;
    return 0;
}
