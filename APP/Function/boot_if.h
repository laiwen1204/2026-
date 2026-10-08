#ifndef __BOOT_IF_H
#define __BOOT_IF_H

#include "HeaderFiles.h"

void boot_if_init(void);
void boot_if_activate(void);
uint8_t boot_if_is_active(void);
void boot_if_rbne_handler(uint8_t byte);
void boot_if_idle_handler(void);

#endif
