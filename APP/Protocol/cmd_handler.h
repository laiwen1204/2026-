#ifndef __CMD_HANDLER_H
#define __CMD_HANDLER_H

#include "protocol.h"

int  cmd_dispatch(proto_ctx_t *ctx);
bool ar_active(void);
void ar_proc(void);
void alarm_proc(void);

#endif
