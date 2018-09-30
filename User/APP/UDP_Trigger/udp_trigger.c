
#include "stm32f10x.h"
#include "udp_trigger.h"

#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"

#include "display.h"
#include "usart1.h"


#define  UDP_TRIGGER_PORT  10

#define  TRIGGER_TIMER_INTERVAL  200
#define  TRIGGER_TIMER_MAXAGE  150

u8 ft_flag = 0;

static u8 trigger_time;
static u8 trigger_flag;

void trigger_timeout(void *parg)
{
	if(trigger_flag == 0)
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 0);
	else
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 1);
	
	trigger_flag = !trigger_flag;
	trigger_time++;
	
	if(trigger_time > TRIGGER_TIMER_MAXAGE) {
		display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 0);
		//restore led status
		printf("trigger timeout\r\n");
		//light error led
		display_led_ctrl(LED1_FLAG, 1);
		ft_flag = 0;
	} else {
		sys_timeout(TRIGGER_TIMER_INTERVAL, trigger_timeout, NULL);
	}
}

void trigger_timer_start(void)
{
	trigger_time = 0;
	trigger_flag = 0;
	sys_timeout(TRIGGER_TIMER_INTERVAL, trigger_timeout, NULL);
	display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 1);
}

void udp_trigger_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip4_addr_t *addr, u16_t port)
{
//   udp_sendto(upcb, p, addr, port);
	printf("udp_trigger_callback: get a packet from %d.%d.%d.%d: %d\r\n", (addr->addr)>>24, \
	                                                  (addr->addr)>>16&0xff, (addr->addr)>>8&0xff, (addr->addr)&0xff, port);
	sys_untimeout(trigger_timeout, NULL);
	
	display_led_ctrl(LED4_FLAG|LED5_FLAG|LED6_FLAG, 0);
	
	ft_flag = 0;
	
	pbuf_free(p);
}

static struct udp_pcb *tupcb;

void udp_trigger_init(void)
{
   /* Create a new UDP control block  */
   tupcb = udp_new();
   /* Bind the upcb to any IP address and the UDP_PORT port*/
   udp_bind(tupcb, IP_ADDR_ANY, UDP_TRIGGER_PORT);
   /* Set a receive callback for the upcb */
   udp_recv(tupcb, udp_trigger_callback, NULL);
}

void udp_trigger(void)
{
	struct pbuf *p;
	u8 *pb;
	ip4_addr_t addr;
	u16 port;
	
	addr.addr = (100<<24)|(0<<16)|(168<<8)|192;
	port = 8080;
	
	p = pbuf_alloc(PBUF_TRANSPORT, 9, PBUF_RAM);
	pb = (u8 *)p->payload;
	pb[0] = 'm';
	pb[1] = 'c';
	pb[2] = 0;
	pb[3] = 192;
	pb[4] = 168;
	pb[5] = 0;
	pb[6] = 10;
	pb[7] = 0;
	pb[8] = 69;
	
	udp_sendto(tupcb, p, &addr, port);
	pbuf_free(p);
	
	trigger_timer_start();
	
	ft_flag = 1;
}

