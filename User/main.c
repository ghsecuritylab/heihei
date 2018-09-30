#include "global.h"

#include "malloc.h"
#include "ff.h"
#include "exfuns.h"
#include "usart1.h"

#include "usbh_usr.h"

#include <string.h>

#include "Timer/timer7.h"

#include "lwip/init.h"
#include "netif/ethernet.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"

#include "display.h"
#include "MCU_Display.h"
#include "key.h"

#include "tftp_server.h"
#include "udp_trigger.h"
#include "tcp_sniffer.h"

//rock you

extern void lwip_ticks_init(void);
extern err_t ethernetif_init(struct netif *netif);
extern void process_mac(void);

struct netif enc28j60_netif;

static void *tftp_file_open(char* fname, char* mode, u8_t write)
{
	FIL *pfil;
	FRESULT res;
	
	pfil = mem_malloc(sizeof(FIL));
	if(!pfil) {
		return NULL;
	}
	if(write) {
		res = f_open(pfil, fname, FA_WRITE | FA_CREATE_ALWAYS);
		if(res) {
			mem_free(pfil);
			return NULL;
		}
	} else {
		res = f_open(pfil, fname, FA_READ);
		if(res) {
			mem_free(pfil);
			return NULL;
		}
	}
	return pfil;
}

static void tftp_file_close(void *handle)
{
	FIL *pfil = (FIL *)handle;
	
	f_close(pfil);
	mem_free(pfil);
}

static int tftp_file_read(void* handle, u8_t* buf, int bytes)
{
	FRESULT res;
	FIL *pfil = (FIL *)handle;
	unsigned int br;
	
	res = f_read(pfil, buf, bytes, &br);
	if(res) {
		return -1;
	}
	return br;
}

static int tftp_file_write(void* handle, u8_t *buf, int bytes)
{
	FRESULT res;
	FIL *pfil = (FIL *)handle;
	unsigned int bw;
	
	res = f_write(pfil, buf, bytes, &bw);
	if(res || bw!=bytes) {
		return -1;
	}
	return bw;
}

static struct tftp_file_op fop = {tftp_file_open, tftp_file_close, tftp_file_read, tftp_file_write};


#define  UDISK_STATS_FLASH_INTERVAL  200

USBH_HOST  USB_Host;
USB_OTG_CORE_HANDLE  USB_OTG_Core;

//用户测试主程序
//返回值:0,正常
//       1,有问题
u8 USH_User_App(void)
{
	u32 total,free;
	u8 res=0;
	
	printf("设备连接成功!\r\n");	 
  	f_mount(fs[0],"0:",1); 	//重新挂载U盘
	res=exf_getfree("0:",&total,&free);
	if(res==0)
	{
		printf("FATFS OK!");	
		printf("U Disk Total Size: %d MB\r\n", total>>10);	 
		printf("U Disk  Free Size: %d MB\r\n", free>>10);
	}
	
	{
		u8 stats = led_status;
		
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 1);
		delay_ms(UDISK_STATS_FLASH_INTERVAL);
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 0);
		delay_ms(UDISK_STATS_FLASH_INTERVAL);
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 1);
		delay_ms(UDISK_STATS_FLASH_INTERVAL);
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 0);
		
		display_led_ctrl(stats, 1);
	}
	
	while(HCD_IsDeviceConnected(&USB_OTG_Core))//设备连接成功
	{
		process_mac();
		
		//process LwIP timeout
		sys_check_timeouts();
		
		//todo: add your own user code here
		if(!ft_flag) {
			if(Key0_Scan(0)) {
				printf("key pressed\r\n");
				udp_trigger();
			}
			MCU_DisplayRunCycle();
		}
	}
	
	{
		u8 stats = led_status;
		
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 1);
		delay_ms(UDISK_STATS_FLASH_INTERVAL);
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 0);
		delay_ms(UDISK_STATS_FLASH_INTERVAL);
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 1);
		delay_ms(UDISK_STATS_FLASH_INTERVAL);
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 0);
		
		display_led_ctrl(stats, 1);
	}
	
 	f_mount(0,"0:",1); 	//卸载U盘
	printf("设备连接中...\r\n");
	return res;
}


//设置向量表偏移地址
//NVIC_VectTab:基址
//Offset:偏移量			 
void MY_NVIC_SetVectorTable(u32 NVIC_VectTab, u32 Offset)	 
{ 	   	 
	SCB->VTOR = NVIC_VectTab|(Offset & (u32)0x1FFFFF80);//设置NVIC的向量表偏移寄存器
	//用于标识向量表是在CODE区还是在RAM区
}

//不能在这里执行所有外设复位!否则至少引起串口不工作.		    
//把所有时钟寄存器复位		  
void MYRCC_DeInit(void)
{	
 	RCC->APB1RSTR = 0x00000000;//复位结束			 
	RCC->APB2RSTR = 0x00000000; 
	  
  	RCC->AHBENR = 0x00000014;  //睡眠模式闪存和SRAM时钟使能.其他关闭.	  
  	RCC->APB2ENR = 0x00000000; //外设时钟关闭.			   
  	RCC->APB1ENR = 0x00000000;   
	RCC->CR |= 0x00000001;     //使能内部高速时钟HSION	 															 
	RCC->CFGR &= 0xF8FF0000;   //复位SW[1:0],HPRE[3:0],PPRE1[2:0],PPRE2[2:0],ADCPRE[1:0],MCO[2:0]					 
	RCC->CR &= 0xFEF6FFFF;     //复位HSEON,CSSON,PLLON
	RCC->CR &= 0xFFFBFFFF;     //复位HSEBYP	   	  
	RCC->CFGR &= 0xFF80FFFF;   //复位PLLSRC, PLLXTPRE, PLLMUL[3:0] and USBPRE 
	RCC->CIR = 0x00000000;     //关闭所有中断		 
	//配置向量表				  
#ifdef  VECT_TAB_RAM
	MY_NVIC_SetVectorTable(0x20000000, 0x0);
#else   
	MY_NVIC_SetVectorTable(0x08000000,0x0);
#endif
}

//系统时钟初始化函数
//pll:选择的倍频数，从2开始，最大值为16		 
void Stm32_Clock_Init(u8 PLL)
{
	unsigned char temp=0;   
	MYRCC_DeInit();		  //复位并配置向量表
 	RCC->CR|=0x00010000;  //外部高速时钟使能HSEON
	while(!(RCC->CR>>17));//等待外部时钟就绪
	RCC->CFGR=0X00000400; //APB1=DIV2;APB2=DIV1;AHB=DIV1;
	PLL-=2;				  //抵消2个单位（因为是从2开始的，设置0就是2）
	RCC->CFGR|=PLL<<18;   //设置PLL值 2~16
	RCC->CFGR|=1<<16;	  //PLLSRC ON 
	FLASH->ACR|=0x32;	  //FLASH 2个延时周期
	RCC->CR|=0x01000000;  //PLLON
	while(!(RCC->CR>>25));//等待PLL锁定
	RCC->CFGR|=0x00000002;//PLL作为系统时钟	 
	while(temp!=0x02)     //等待PLL作为系统时钟设置成功
	{   
		temp=RCC->CFGR>>2;
		temp&=0x03;
	}    
}

void display_test(void)
{
	while(1) {
		display_number(1, 0);
		display_number(2, 0);
		display_number(3, 0);
		display_number(4, 0);
		delay_ms(1000);
		
		display_number(1, 1);
		display_number(2, 1);
		display_number(3, 1);
		display_number(4, 1);
		delay_ms(1000);
		
		display_number(1, 2);
		display_number(2, 2);
		display_number(3, 2);
		display_number(4, 2);
		delay_ms(1000);
		
		display_number(1, 3);
		display_number(2, 3);
		display_number(3, 3);
		display_number(4, 3);
		delay_ms(1000);
		
		display_number(1, 4);
		display_number(2, 4);
		display_number(3, 4);
		display_number(4, 4);
		delay_ms(1000);
		
		display_number(1, 5);
		display_number(2, 5);
		display_number(3, 5);
		display_number(4, 5);
		delay_ms(1000);
		
		display_number(1, 6);
		display_number(2, 6);
		display_number(3, 6);
		display_number(4, 6);
		delay_ms(1000);
		
		display_number(1, 7);
		display_number(2, 7);
		display_number(3, 7);
		display_number(4, 7);
		delay_ms(1000);
		
		display_number(1, 8);
		display_number(2, 8);
		display_number(3, 8);
		display_number(4, 8);
		delay_ms(1000);
		
		display_number(1, 9);
		display_number(2, 9);
		display_number(3, 9);
		display_number(4, 9);
		delay_ms(1000);
		
		display_led_ctrl(LED1_FLAG, 1);
		delay_ms(500);
		display_led_ctrl(LED1_FLAG, 0);
		delay_ms(500);
		
		display_led_ctrl(LED2_FLAG, 1);
		delay_ms(500);
		display_led_ctrl(LED2_FLAG, 0);
		delay_ms(500);
		
		display_led_ctrl(LED3_FLAG, 1);
		delay_ms(500);
		display_led_ctrl(LED3_FLAG, 0);
		delay_ms(500);
		
		display_led_ctrl(LED4_FLAG, 1);
		delay_ms(500);
		display_led_ctrl(LED4_FLAG, 0);
		delay_ms(500);
		
		display_led_ctrl(LED5_FLAG, 1);
		delay_ms(500);
		display_led_ctrl(LED5_FLAG, 0);
		delay_ms(500);
		
		display_led_ctrl(LED6_FLAG, 1);
		delay_ms(500);
		display_led_ctrl(LED6_FLAG, 0);
		delay_ms(500);
	}
}

int main(void)
{
	ip4_addr_t ipaddr, netmask, gw;
	
//	Stm32_Clock_Init(9);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	delay_init();
	
	usart1_init(115200);
	MCU_DisplayInit();
	Key_Init();
	
	my_mem_init(SRAMIN);		//初始化内部内存池
	exfuns_init();				//为fatfs相关变量申请内存
	
	USBH_Init(&USB_OTG_Core,USB_OTG_FS_CORE_ID,&USB_Host,&USBH_MSC_cb,&USR_cb);
	
	lwip_ticks_init();
	
	lwip_init();
	
	/*register a netif to lwip*/
	IP4_ADDR(&ipaddr, 192, 168, 0, 10);
	IP4_ADDR(&netmask, 255, 255, 255, 0);
	IP4_ADDR(&gw, 192, 168, 0, 1);
	netif_add(&enc28j60_netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, ethernet_input);
	netif_set_default(&enc28j60_netif);
	netif_set_up(&enc28j60_netif);
	
	tftp_server_init(&fop);
	udp_trigger_init();
	tcp_sniffer_init();
	
	while(1)
	{
		USBH_Process(&USB_OTG_Core, &USB_Host);
		delay_ms(10);
		
		process_mac();
		
		//process LwIP timeout
		sys_check_timeouts();
		
		//todo: add your own user code here
		MCU_DisplayRunCycle();
	}
}
