
#include "MCU_Display.h"
#include "MCU_NetworkInterface.h"

#include "display.h"



void MCU_DisplayInit(void)
{
	//Init IIC and relative display
	display_init();
}

void MCU_DisplayRunCycle(void)
{
	//Display the error numble and LED
	MCU_NetworkOutputDataType *p = &outputData_MCUNetwork;
	
	switch(p->modeOfMC)
	{
		case 0:
			display_led_ctrl(LED4_FLAG, 1);
			display_led_ctrl(LED5_FLAG, 0);
			display_led_ctrl(LED6_FLAG, 0);
			display_led_ctrl(LED3_FLAG, 0);
			break;
		case 1:
			display_led_ctrl(LED4_FLAG, 0);
			display_led_ctrl(LED5_FLAG, 1);
			display_led_ctrl(LED6_FLAG, 0);
			display_led_ctrl(LED3_FLAG, 0);
			break;
		case 2:
			display_led_ctrl(LED4_FLAG, 0);
			display_led_ctrl(LED5_FLAG, 0);
			display_led_ctrl(LED6_FLAG, 1);
			display_led_ctrl(LED3_FLAG, 0);
			break;
		case 3:
			display_led_ctrl(LED4_FLAG, 0);
			display_led_ctrl(LED5_FLAG, 0);
			display_led_ctrl(LED6_FLAG, 0);
			display_led_ctrl(LED3_FLAG, 1);
			break;
		default:
			break;
	}
	
	if(p->emg)
		display_led_ctrl(LED2_FLAG, 1);
	if(p->errorNo > 0)
	{
		display_led_ctrl(LED1_FLAG, 1);
		display_num(p->errorNo);
	}
}

void MCU_DisplayClose(void)
{
	return;
}


