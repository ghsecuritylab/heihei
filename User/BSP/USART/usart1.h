#ifndef __USART1_H
#define __USART1_H

#include <stdio.h>	
#include <stm32f10x.h>

#define EN_USART1_RX 			1		//ʹ�ܣ�1��/��ֹ��0������1����

#ifdef  EN_USART1_RX
#define USART1_REC_LEN  			200  	//�����������ֽ��� 200
extern u8  USART1_RX_BUF[USART1_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u16 USART1_RX_STA;         		//����״̬���	
#endif

void usart1_init(u32 bound);

#endif

