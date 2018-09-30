#ifndef __TM1637_H
#define __TM1637_H

#include "stm32f10x.h"


/* just for reference */
#define  DIGIT_ADDR1  0xc0
#define  DIGIT_ADDR2  0xc1
#define  DIGIT_ADDR3  0xc2
#define  DIGIT_ADDR4  0xc3
#define  DIGIT_ADDR5  0xc4
#define  DIGIT_ADDR6  0xc5

#define  DIGIT_CHAR_0  0x3f
#define  DIGIT_CHAR_1  0x06
#define  DIGIT_CHAR_2  0x5b
#define  DIGIT_CHAR_3  0x4f
#define  DIGIT_CHAR_4  0x66
#define  DIGIT_CHAR_5  0x6d
#define  DIGIT_CHAR_6  0x7d
#define  DIGIT_CHAR_7  0x07
#define  DIGIT_CHAR_8  0x7f
#define  DIGIT_CHAR_9  0x6f


void TM1637_Init(void);
void TM1637_SetDigit(u8 digit, u8 c);
u8 TM1637_Scan_Key(void);

#endif
