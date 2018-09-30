
#include "stm32f10x.h"
#include "display.h"
#include "tm1637.h"


const u8 digitCode[5] = {0x00, DIGIT_ADDR1, DIGIT_ADDR2, DIGIT_ADDR3, DIGIT_ADDR4};

const u8 numCode[10] = {DIGIT_CHAR_0, DIGIT_CHAR_1, DIGIT_CHAR_2, DIGIT_CHAR_3, DIGIT_CHAR_4, DIGIT_CHAR_5,\
					DIGIT_CHAR_6, DIGIT_CHAR_7, DIGIT_CHAR_8, DIGIT_CHAR_9};


/**
 * digit: 1~4;
 * num: 0~9;
 */
void display_number(u8 digit, u8 num)
{
	digit = digitCode[digit];
	num = numCode[num];
	
	TM1637_SetDigit(digit, num);
}

/**
 * num: 0 ~ 9999
 */
void display_num(u16 num)
{
	if(num < 10000) {
		display_number(4, num%10);
		num /= 10;
		display_number(3, num%10);
		num /= 10;
		display_number(2, num%10);
		num /= 10;
		display_number(1, num%10);
	}
}


u8 led_status = 0;

/**
 * led: 1~6
 * on: 0/1
 */
void display_led_ctrl(u8 led, u8 on)
{
	if(on)
		led_status |= led;
	else
		led_status &= ~led;
	
	TM1637_SetDigit(DIGIT_ADDR5, led_status);
}


void display_init(void)
{
	TM1637_Init();
	led_status = 0;
	
	TM1637_SetDigit (DIGIT_ADDR1, 0x00);
	TM1637_SetDigit (DIGIT_ADDR2, 0x00);
	TM1637_SetDigit (DIGIT_ADDR3, 0x00);		
	TM1637_SetDigit (DIGIT_ADDR4, 0x00);
	TM1637_SetDigit (DIGIT_ADDR5, 0x00);
	
	TM1637_SetDigit (DIGIT_ADDR5, led_status);
}
