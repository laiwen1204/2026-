#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define FRAME_HEADER        0xA5B6
#define FRAME_TAIL          0xB6A5
#define PROTOCOL_VERSION    0x02
#define BROADCAST_ID        0xFFFF
#define DEFAULT_DEVICE_ID   0x0001

#define FRAME_TYPE_COMMAND   0x01
#define FRAME_TYPE_RESPONSE  0x02
#define FRAME_TYPE_HEARTBEAT 0x05
#define FRAME_TYPE_ERROR     0xFF

#define CMD_HEARTBEAT        0x8888
#define CMD_BROADCAST_SEEK   0xFFFF
#define CMD_ERROR_CMD        0xEEEE

#define BAUD_4800            0x0B
#define BAUD_9600            0x0C
#define BAUD_19200           0x13
#define BAUD_115200          0x14
#define BAUD_4800_DEC        11
#define BAUD_9600_DEC        12
#define BAUD_19200_DEC       19
#define BAUD_115200_DEC      20

#define INTERVAL_1S          1
#define INTERVAL_3S          2
#define INTERVAL_5S          3

#define MAX_ASCII_FRAME      550
#define MAX_BINARY_FRAME     256

#define PROTO_ERR_NONE       0
#define PROTO_ERR_CRC        1
#define PROTO_ERR_LENGTH     2
#define PROTO_ERR_UNKNOWN_CMD 3
#define PROTO_ERR_FORMAT     4

typedef struct {
    uint16_t dev_id;
    uint8_t  type;
    uint16_t command;
    uint8_t  length;
    uint8_t  version;
    uint8_t  content[128];
} frame_t;

typedef struct {
    uint16_t my_device_id;
    frame_t  rx_frame;
    uint8_t  error_reason;
    bool     frame_ready;
    bool     frame_error;
    bool     download_mode;
} proto_ctx_t;

proto_ctx_t *get_ctx(void);
void proto_set_id(uint16_t id);

uint8_t hex_char_to_nibble(char c);
uint8_t hex_pair_to_byte(const char *hex);
void    byte_to_hex_pair(uint8_t b, char *out);

int  proto_parse(proto_ctx_t *ctx, const uint8_t *ascii_buf, uint32_t len);
int  check_crc(const uint8_t *binary_buf, uint32_t len);

int  build_frame(uint8_t *out_ascii, uint16_t out_sz,
                 uint16_t dev_id, uint8_t type, uint16_t cmd,
                 const uint8_t *content, uint8_t content_len);
int  build_ok(uint8_t *out_ascii, uint16_t out_sz, const frame_t *req);
int  build_err(uint8_t *out_ascii, uint16_t out_sz, const frame_t *req);

void send_ok(const frame_t *req);
void send_resp(const frame_t *req, const uint8_t *content, uint8_t len);
void send_err(const frame_t *req);
void send_hb(void);
void send_str(const char *str);

#endif
