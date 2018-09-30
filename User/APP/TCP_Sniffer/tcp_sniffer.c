#include "tcp_sniffer.h"

#include "lwip/ip4_addr.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"

#include <stdio.h>
#include <string.h>

#include "display.h"


#include "MCU_NetworkInterface.h"

static void tcp_sniffer_err(void *arg, err_t err)
{
	printf("connect error! closed by core!!\r\n");
	printf("try to connect to server again!!\r\n");
	tcp_sniffer_init();
}

static u16_t errorCode = 0;
static char errorStr[256];

static u8_t rdata[256];
static u16_t rlen;

static void process_data(void)
{
	char c[8];
	u8_t i, len;
	u16_t err;
	
	i = 5;
	len = 0;
	while(rdata[i]!='s') {
		c[len] = rdata[i];
		len++;
		i++;
	}
	err = 0;
	for(i=0 ; i<len ; i++) {
		err = err*10 + c[i] - '0';
	}
	errorCode = err;
	
	rdata[rlen] = 0;
	strcpy(errorStr, (char *)rdata+5+len+5);
	
	rlen = 0;
}

static void sniffer_recv(struct pbuf *p)
{
	u16_t idx;
	u16_t nidx;
	u16_t cpylen;
	
	if(p->tot_len != p->len) {
		printf("tcp sniffer recv an un-normal packet: p->tot_len != p->len\r\n");
		return;
	}
	
	idx = 0;
	nidx = pbuf_memfind(p, "\r\n", 2, idx);
	while(nidx != 0xffff) 
	{
		cpylen = nidx - idx;
		pbuf_copy_partial(p, rdata+rlen, cpylen, idx);
		rlen += cpylen;
		process_data();
		display_num(errorCode);
		printf("errorCode: %d\r\n", errorCode);
		printf("errorStr:  %s\r\n", errorStr);
		
		idx += 2;
		nidx = pbuf_memfind(p, "\r\n", 2, idx);
	}
	cpylen = p->tot_len-idx;
	pbuf_copy_partial(p, rdata+rlen, cpylen, idx);
	rlen += cpylen;
}



static err_t tcp_sniffer_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	char pBuf[64];
	int numByte;
	
	/* We perform here any necessary processing on the pbuf */
	if (p != NULL) {
		/* We call this function to tell the LwIp that we have processed the data */
		/* This lets the stack advertise a larger window, so more data can be received*/
		tcp_recved(pcb, p->tot_len);
		
		numByte=pbuf_copy_partial(p, pBuf, 64, 0);
//		printf("data recv: %s\r\n", pBuf);
		if(1==MCU_ProcessRevcString(pBuf,numByte))
		{
			numByte=MCU_GetSendString(pBuf); //get string which will be sent to mc
			tcp_write(pcb, pBuf, numByte, 1);
			tcp_output(pcb);
		}
		
		
		pbuf_free(p);
	} 
	else if (err == ERR_OK) 
	{
		/* When the pbuf is NULL and the err is ERR_OK, the remote end is closing the connection. */
		/* We free the allocated memory and we close the connection */
		tcp_close(pcb);
		tcp_sniffer_init();
		return ERR_OK;
	}
	return ERR_OK;
}

static err_t tcp_sniffer_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	printf("tcp sniffer send data OK! sent len = [%d]\n", len);
	return ERR_OK;
}

static err_t tcp_sniffer_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
//	char GREETING[] = "Hi, I am a new Client!\n";

	tcp_recv(pcb, tcp_sniffer_recv);
	tcp_sent(pcb, tcp_sniffer_sent);
	ip_set_option(pcb, SOF_KEEPALIVE);
    
//	tcp_write(pcb, GREETING, sizeof(GREETING), 1); 
	return ERR_OK;
}

void tcp_sniffer_init(void)
{
	struct tcp_pcb *pcb = NULL;	            		
	ip4_addr_t server_ip;
	/* Create a new TCP control block  */
	pcb = tcp_new();
	IP4_ADDR(&server_ip, 192,168,0,2);
	tcp_connect(pcb, &server_ip, 9999, tcp_sniffer_connected); 
	tcp_err(pcb, tcp_sniffer_err);

	rlen = 0;
}


