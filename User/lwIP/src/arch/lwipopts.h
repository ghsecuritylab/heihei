/*
;*****************************************************************************************************

;*	      
;*****************************************************************************************************
;*/

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

	 
/* 有系统 */  
#define NO_SYS                       1
//#define NO_SYS_NO_TIMERS             1

#define LWIP_SOCKET  0
#define LWIP_NETCONN 0

/* 4字节对齐 */
#define MEM_ALIGNMENT                4
	
/* 4K的内存 */  
#define MEM_SIZE                     1024*20


/* */
#define ETH_PAD_SIZE				 0 

#define TCP_OVERSIZE  0

#define SYS_LIGHTWEIGHT_PROT         0

//#define ETHARP_DEBUG                LWIP_DBG_ON
//#define ICMP_DEBUG                  LWIP_DBG_ON
//#define MEM_DEBUG                       LWIP_DBG_ON
//#define MEMP_DEBUG                      LWIP_DBG_ON
//#define TCP_DEBUG					  LWIP_DBG_ON
//#define MEMP_MEM_MALLOC              1
//#define MEM_USE_POOLS 0


//#define ARP_TABLE_SIZE  10;
//#define ARP_QUEUEING  1;
//#define MEMP_NUM_ARP_QUEUE  30;

#define  LWIP_UDP  1
#define  MEMP_NUM_UDP_PCB  8

#define MEMP_NUM_SYS_TIMEOUT            (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + MEMP_NUM_UDP_PCB + 4)

#define  LWIP_TCP  1
#define  MEMP_NUM_TCP_PCB  2
#define  MEMP_NUM_TCP_PCB_LISTEN  0
#define  MEMP_NUM_TCP_SEG  16
//#define  TCP_SND_BUF                  4000    //允许TCP协议使用的最大发送缓冲长度
//#define  TCP_MSS                      1000


#endif /* __LWIPOPTS_H__ */

/*
;*****************************************************************************************************
;*                            			End Of File
;*****************************************************************************************************
;*/

