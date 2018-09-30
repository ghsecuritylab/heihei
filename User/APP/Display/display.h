
#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "stm32f10x.h"

#define LED1_FLAG 0x01
#define LED2_FLAG 0x02
#define LED3_FLAG 0x04
#define LED4_FLAG 0x08
#define LED5_FLAG 0x10
#define LED6_FLAG 0x20

extern u8 led_status;

void display_init(void);
void display_number(u8 digit, u8 num);
void display_num(u16 num);
void display_led_ctrl(u8 led, u8 on);

#endif
