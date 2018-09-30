#include "key.h"
#include "global.h"

void Key_GPIO_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 	 
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void Key_Init(void)
{
	Key_GPIO_Config();
}



u8 Key0_Scan(u8 mode)    //mode=0表示不支持连续按，mode=1表示支持连续按
{
	static u8 key_up =1;
	if(mode) key_up =1;
	if(key_up && KEY0==0)
	{
		delay_ms(10);
		if(KEY0==0)
		{
			key_up =0;
			return 1;
		}	
	}
	else if(KEY0==1)
		key_up =1;
	
	return 0;
}

