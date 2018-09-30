#ifndef __UDP_TRIGGER_H
#define __UDP_TRIGGER_H

#include "lwip/udp.h"

extern u8 ft_flag;

void udp_trigger_init(void);
void udp_trigger(void);

#endif
