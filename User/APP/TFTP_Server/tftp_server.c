
#include "tftp_server.h"
#include "tftp_server_opts.h"

#include "lwip/udp.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/def.h"
#include "lwip/timeouts.h"

#include <string.h>


#if TFTP_SERVER_DEBUG
#define TFTP_DEBUG_PRINT(x); printf x;
#else
#define TFTP_DEBUG_PRINT(x);  
#endif


/* TFTP opcodes as specified in RFC1350 */
#define  TFTP_RRQ   1
#define  TFTP_WRQ   2
#define  TFTP_DATA  3
#define  TFTP_ACK   4
#define  TFTP_ERROR 5

/* TFTP error codes as specified in RFC1350 */
#define  TFTP_ERR_NOTDEFINED  0
#define  TFTP_ERR_FILE_NOT_FOUND  1
#define  TFTP_ERR_ACCESS_VIOLATION  2
#define  TFTP_ERR_DISKFULL  3
#define  TFTP_ERR_ILLEGALOP  4
#define  TFTP_ERR_UKNOWN_TRANSFER_ID  5
#define  TFTP_ERR_FILE_ALREADY_EXISTS  6
#define  TFTP_ERR_NO_SUCH_USER  7


#define TFTP_OPCODE_LEN         2
#define TFTP_BLKNUM_LEN         2
#define TFTP_ERRCODE_LEN        2
#define TFTP_DATA_LEN_MAX       512
#define TFTP_DATA_PKT_HDR_LEN   (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_ERR_PKT_HDR_LEN    (TFTP_OPCODE_LEN + TFTP_ERRCODE_LEN)
#define TFTP_ACK_PKT_LEN        (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_DATA_PKT_LEN_MAX   (TFTP_DATA_PKT_HDR_LEN + TFTP_DATA_LEN_MAX)


typedef struct
{
	u16_t op;    /* RRQ/WRQ */
	
	ip4_addr_t remote_ip;
	u16_t remote_port;
	
	/* File system related args */
	struct tftp_file_op fop;
	void *handle;

	/* last block */
	u8_t data[TFTP_DATA_LEN_MAX];
	u16_t  data_len;
	
	/* next block number */
	u16_t blknum;
	
	/* timeout related variables */
	u32_t timer;
	u32_t last_time;
	u8_t retries;

} tftp_connection_args;


static void tftp_server_tmr(void* arg);

static void
close_tftp_connection(struct udp_pcb *upcb)
{
	tftp_connection_args *arg = (tftp_connection_args *)upcb->recv_arg;
	
	sys_untimeout(tftp_server_tmr, upcb);
	
	((arg->fop).close)(arg->handle);
	mem_free(arg);
	
	/* Disconnect the udp_pcb*/
	udp_disconnect(upcb);
	/* close the connection */
	udp_remove(upcb);
	
	TFTP_DEBUG_PRINT(("close tftp connection\r\n"));
}

static void
tftp_send_error(struct udp_pcb *upcb, u16_t errCode, char *errMsg)
{
	u16_t str_length = strlen(errMsg);
	struct pbuf* p;
	u16_t* payload;

	p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)(TFTP_ERR_PKT_HDR_LEN + str_length + 1), PBUF_RAM);
	if(p == NULL) {
		TFTP_DEBUG_PRINT(("tftp_send_error: cannot alloc pbuf\r\n"));
		return;
	}

	payload = (u16_t*)(p->payload);
	payload[0] = PP_HTONS(TFTP_ERROR);
	payload[1] = PP_HTONS(errCode);
	memcpy((char *)&payload[2], errMsg, str_length + 1);

	udp_send(upcb, p);
	pbuf_free(p);
}
static void
tftp_send_error_to(struct udp_pcb *upcb, ip4_addr_t *to_ip, u16_t to_port, u16_t errCode, char *errMsg)
{
	u16_t str_length = strlen(errMsg);
	struct pbuf* p;
	u16_t* payload;

	p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)(TFTP_ERR_PKT_HDR_LEN + str_length + 1), PBUF_RAM);
	if(p == NULL) {
		TFTP_DEBUG_PRINT(("tftp_send_error: cannot alloc pbuf\r\n"));
		return;
	}

	payload = (u16_t*)(p->payload);
	payload[0] = PP_HTONS(TFTP_ERROR);
	payload[1] = PP_HTONS(errCode);
	memcpy((char *)&payload[2], errMsg, str_length + 1);

	udp_sendto(upcb, p, to_ip, to_port);
	pbuf_free(p);
}

static void
tftp_send_ack(struct udp_pcb *upcb, u16_t blknum)
{
	struct pbuf* p;
	u16_t* payload;

	p = pbuf_alloc(PBUF_TRANSPORT, TFTP_ACK_PKT_LEN, PBUF_RAM);
	if(p == NULL) {
		TFTP_DEBUG_PRINT(("tftp_send_ack: cannot alloc pbuf\r\n"));
		return;
	}
	payload = (u16_t*)(p->payload);

	payload[0] = PP_HTONS(TFTP_ACK);
	payload[1] = PP_HTONS(blknum);
	udp_send(upcb, p);
	pbuf_free(p);
}

static void
tftp_resend_data(struct udp_pcb *upcb)
{
	tftp_connection_args *arg = (tftp_connection_args *)upcb->recv_arg;
	struct pbuf *p;
	u16_t *payload;
	
	p = pbuf_alloc(PBUF_TRANSPORT, TFTP_DATA_PKT_HDR_LEN + arg->data_len, PBUF_RAM);
	if(p == NULL) {
		TFTP_DEBUG_PRINT(("tftp_resend_data: cannot alloc pbuf\r\n"));
		return;
	}
	payload = (u16_t*)(p->payload);
	payload[0] = PP_HTONS(TFTP_DATA);
	payload[1] = PP_HTONS(arg->blknum);

	memcpy((char *)&payload[2], arg->data, arg->data_len);

	udp_send(upcb, p);
	pbuf_free(p);
}

static void
tftp_send_data(struct udp_pcb *upcb)
{
	tftp_connection_args *arg = (tftp_connection_args *)upcb->recv_arg;
	struct pbuf *p;
	u16_t *payload;
	int ret;
	
	arg->blknum++;
	ret = ((arg->fop).read)(arg->handle, arg->data, TFTP_DATA_LEN_MAX);
	if(ret < 0) {
		TFTP_DEBUG_PRINT(("tftp_send_data: error occured while reading the file.\r\n"));
		tftp_send_error(upcb, TFTP_ERR_ACCESS_VIOLATION, "Error occured while reading the file.");
		close_tftp_connection(upcb);
		return;
	}
	arg->data_len = ret;
	
	p = pbuf_alloc(PBUF_TRANSPORT, TFTP_DATA_PKT_HDR_LEN + arg->data_len, PBUF_RAM);
	if(!p) {
		TFTP_DEBUG_PRINT(("tftp_send_data: cannot alloc pbuf\r\n"));
		return;
	}
	payload = (u16_t *)(p->payload);
	payload[0] = PP_HTONS(TFTP_DATA);
	payload[1] = PP_HTONS(arg->blknum);
	memcpy((char *)&payload[2], arg->data, arg->data_len);
	
	udp_send(upcb, p);
	pbuf_free(p);
}

/**
 * TFTP callback functions.
 */

/**
 *
 */
static void data_ack_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                       ip4_addr_t *addr, u16_t port)
{
	tftp_connection_args *parg = (tftp_connection_args *)arg;
	u16_t opcode;
	u16_t blknum;
	u16_t *payload;
	int ret;
	
	if(port != parg->remote_port || !ip4_addr_cmp(addr, &(parg->remote_ip)))
	{
		TFTP_DEBUG_PRINT(("data_recv_callback: something impossible just happened\r\n"));
		tftp_send_error_to(upcb, addr, port, TFTP_ERR_NOTDEFINED, "what the hell are you doing?");
		tftp_send_error_to(upcb, &(parg->remote_ip), parg->remote_port, TFTP_ERR_NOTDEFINED, "something impossible just happened");
		close_tftp_connection(upcb);
		pbuf_free(p);
		return;
	}
	
	parg->last_time = parg->timer;
	parg->retries = 0;
	
	payload = (u16_t *)p->payload;
	opcode = PP_NTOHS(payload[0]);
	
	switch(opcode) {
		case TFTP_WRQ:
		case TFTP_RRQ:
		{
			TFTP_DEBUG_PRINT(("data_recv_callback: should not receive RQ\r\n"));
			break;
		}
		case TFTP_ERROR:
		{
			TFTP_DEBUG_PRINT(("data_recv_callback: receive error packet\r\n"));
			close_tftp_connection(upcb);
			break;
		}
		case TFTP_DATA:
		{
			if(parg->op == TFTP_RRQ) {
				TFTP_DEBUG_PRINT(("data_recv_callback: should not receive DATA\r\n"));
				tftp_send_error(upcb, TFTP_ERR_ACCESS_VIOLATION, "Not a write connection");
				close_tftp_connection(upcb);
				break;
			}
			blknum = PP_NTOHS(payload[1]);
			if(blknum == parg->blknum+1) {
				parg->data_len = p->tot_len - TFTP_DATA_PKT_HDR_LEN;
				pbuf_copy_partial(p, parg->data, parg->data_len, TFTP_DATA_PKT_HDR_LEN);
				ret = ((parg->fop).write)(parg->handle, parg->data, parg->data_len);
				if(ret < 0) {
					TFTP_DEBUG_PRINT(("data_recv_callback: error writing file\r\n"));
					tftp_send_error(upcb, TFTP_ERR_ACCESS_VIOLATION, "error writing file");
					close_tftp_connection(upcb);
					break;
				}
				parg->blknum++;
				tftp_send_ack(upcb, parg->blknum);
				if(parg->data_len < TFTP_DATA_LEN_MAX) {
					close_tftp_connection(upcb);
				}
			} else {
				TFTP_DEBUG_PRINT(("data_recv_callback: receive wrong block data, block %d\r\n", blknum));
				tftp_send_ack(upcb, parg->blknum);
			}
			break;
		}
		case TFTP_ACK:
		{
			if(parg->op == TFTP_WRQ) {
				TFTP_DEBUG_PRINT(("data_recv_callback: should not receive ACK\r\n"));
				tftp_send_error(upcb, TFTP_ERR_ACCESS_VIOLATION, "Not a read connection");
				close_tftp_connection(upcb);
				break;
			}
			blknum = PP_NTOHS(payload[1]);
			if(blknum == parg->blknum) {
				tftp_send_data(upcb);
				if(parg->data_len < TFTP_DATA_LEN_MAX) {
					close_tftp_connection(upcb);
				}
			} else {
				TFTP_DEBUG_PRINT(("data_recv_callback: receive wrong block ack\r\n"));
			}
			break;
		}
		default:
			tftp_send_error(upcb, TFTP_ERR_ILLEGALOP, "Unknown operation");
			break;
	}
	
	pbuf_free(p);
}

/**
 * the recv_callback function is called when there is a packet received
 * on the main tftp server port (69)
 */
static void request_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                        ip4_addr_t *addr, u16_t port)
{
	tftp_connection_args *parg = (tftp_connection_args *)arg;
	
	char tftp_null = 0;
	char filename[TFTP_SERVER_MAX_FILENAME_LEN];
	char mode[TFTP_SERVER_MAX_MODE_LEN];
	u16_t filename_end_offset;
	u16_t mode_end_offset;
	u16_t opcode;
	
	struct udp_pcb *new_pcb;
	
	opcode = PP_NTOHS(*(u16_t *)(p->payload));
	
	if(opcode!=TFTP_RRQ && opcode !=TFTP_WRQ) {
		TFTP_DEBUG_PRINT(("request_recv_callback: illegal request\r\n"));
		tftp_send_error_to(upcb, addr, port, TFTP_ERR_ILLEGALOP, "illegal request");
		pbuf_free(p);
		return;
	}
	
	/* find \0 in pbuf -> end of filename string */
	filename_end_offset = pbuf_memfind(p, &tftp_null, sizeof(tftp_null), 2);
	if((u16_t)(filename_end_offset-2+1) > sizeof(filename)) {
		TFTP_DEBUG_PRINT(("request_recv_callback: Filename too long/not NULL terminated\r\n"));
		tftp_send_error_to(upcb, addr, port, TFTP_ERR_ACCESS_VIOLATION, "Filename too long/not NULL terminated");
		pbuf_free(p);
		return;
	}
	pbuf_copy_partial(p, filename, filename_end_offset-2+1, 2);

	/* find \0 in pbuf -> end of mode string */
	mode_end_offset = pbuf_memfind(p, &tftp_null, sizeof(tftp_null), filename_end_offset+1);
	if((u16_t)(mode_end_offset-filename_end_offset) > sizeof(mode)) {
		TFTP_DEBUG_PRINT(("request_recv_callback: Mode too long/not NULL terminated\r\n"));
		tftp_send_error_to(upcb, addr, port, TFTP_ERR_ACCESS_VIOLATION, "Mode too long/not NULL terminated");
		pbuf_free(p);
		return;
	}
	pbuf_copy_partial(p, mode, mode_end_offset-filename_end_offset, filename_end_offset+1);
	
	parg = mem_malloc(sizeof(tftp_connection_args));
	if(!parg) {
		TFTP_DEBUG_PRINT(("request_recv_callback: cannot alloc tftp_connection_args\r\n"));
		tftp_send_error_to(upcb, addr, port, TFTP_ERR_NOTDEFINED, "Server is out of mem.");
		pbuf_free(p);
		return;
	}
	parg->fop = *(struct tftp_file_op *)arg;
	parg->handle = ((parg->fop).open)(filename, mode, opcode == TFTP_WRQ);
	if (!parg->handle) {
		TFTP_DEBUG_PRINT(("request_recv_callback: Unable to open requested file.\r\n"));
		tftp_send_error_to(upcb, addr, port, TFTP_ERR_FILE_NOT_FOUND, "Unable to open requested file.");
		mem_free(parg);
		pbuf_free(p);
		return;
	}
	parg->op = opcode;
	ip4_addr_copy(parg->remote_ip, *addr);
	parg->remote_port = port;
	parg->blknum = 0;
	parg->timer = 0;
	parg->last_time = 0;
	parg->retries = 0;
	
	new_pcb = udp_new();
	if(!new_pcb)
	{   /* Error creating PCB. Out of Memory  */
		TFTP_DEBUG_PRINT(("request_recv_callback: cannot alloc new udp pcb\r\n"));
		tftp_send_error_to(upcb, addr, port, TFTP_ERR_NOTDEFINED, "Server is out of udp memp.");
		((parg->fop).close)(parg->handle);
		mem_free(parg);
		pbuf_free(p);
		return;
	}
	TFTP_DEBUG_PRINT(("tftp: %s request from ", (opcode == TFTP_WRQ) ? "write" : "read"));
	TFTP_DEBUG_PRINT(("%d.%d.%d.%d", ip4_addr1(addr), ip4_addr2(addr), ip4_addr3(addr), ip4_addr4(addr)));
	TFTP_DEBUG_PRINT((" for '%s' mode '%s'\r\n", filename, mode));
	
	udp_connect(new_pcb, addr, port);
	udp_recv(new_pcb, (udp_recv_fn)data_ack_recv_callback, parg);
	sys_timeout(TFTP_SERVER_TIMER_MSECS, tftp_server_tmr, new_pcb);

	if (parg->op == TFTP_WRQ) {
		tftp_send_ack(new_pcb, 0);
	} else {
		tftp_send_data(new_pcb);
	}

	pbuf_free(p);
}

err_t tftp_server_init(struct tftp_file_op *p)
{
	err_t ret;
	struct udp_pcb *upcb;
	
	upcb = udp_new();
	if (upcb == NULL) {
		return ERR_MEM;
	}
	ret = udp_bind(upcb, IP4_ADDR_ANY, TFTP_SERVER_PORT);
	if (ret != ERR_OK) {
		udp_remove(upcb);
		return ret;
	}
	udp_recv(upcb, (udp_recv_fn)request_recv_callback, p);
	
	return ERR_OK;
}

static void
tftp_server_tmr(void* arg)
{
	struct udp_pcb *upcb = (struct udp_pcb *)arg;
	tftp_connection_args *parg = (tftp_connection_args *)upcb->recv_arg;

	parg->timer++;
	
	sys_timeout(TFTP_SERVER_TIMER_MSECS, tftp_server_tmr, upcb);

	if ((parg->timer - parg->last_time) > (TFTP_SERVER_TIMEOUT_MSECS / TFTP_SERVER_TIMER_MSECS)) {
		if ((parg->op==TFTP_RRQ) && (parg->retries < TFTP_SERVER_MAX_RETRIES)) {
			TFTP_DEBUG_PRINT(("tftp: timeout, resend a data pkt\r\n"));
			tftp_resend_data(upcb);
			parg->retries++;
		} else {
			TFTP_DEBUG_PRINT(("tftp: timeout\r\n"));
			close_tftp_connection(upcb);
		}
	}
}

