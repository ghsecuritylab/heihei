#include "tm1637.h"
#include "global.h"


//SDA方向设置
#define TM_SDA_IN()  {GPIOB->CRL&=0X0FFFFFFF;GPIOB->CRL|=(u32)8<<28;}
#define TM_SDA_OUT() {GPIOB->CRL&=0X0FFFFFFF;GPIOB->CRL|=(u32)3<<28;}

//IO操作
#define TM_IIC_SCL    PBout(6) //SCL
#define TM_IIC_SDA    PBout(7) //SDA	 
#define TM_READ_SDA   PBin(7)  //输入SDA


u8 CODE[19] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,\
	0x6f,0x77,\
	0x7C,0x39,0x5E,0x79,0x71,0x76,0x38,0x73};	 //0--18

	
//初始化IIC
static void TM_IIC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );	//使能GPIOB时钟
	   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_6|GPIO_Pin_7); 	//PB6,PB7 输出高
}
//产生IIC起始信号
static void TM_IIC_Start(void)
{
	TM_SDA_OUT();		//SDA线输出模式
	TM_IIC_SDA=1;
	TM_IIC_SCL=1;
	delay_us(4);
 	TM_IIC_SDA=0;		//start singal: when SCL is high, SDA change form high to low 
	delay_us(4);
	TM_IIC_SCL=0;		//钳住I2C总线，准备发送或接收数据
	delay_us(4);
}
//产生IIC停止信号
static void TM_IIC_Stop(void)
{
	TM_SDA_OUT();		//SDA线输出模式
	TM_IIC_SCL=0;
	delay_us(2);
	TM_IIC_SDA=0;
 	delay_us(2);
	TM_IIC_SCL=1;
	delay_us(4);
	TM_IIC_SDA=1;		//stop singal: when SCL is high, SDA change form low to high
	delay_us(4);
}

/*
//IIC发送一个字节
//需要处理从机返回的ACK
//返回值：0表示成功收到从机应答，非0值表示没有收到从机应答，通信出错
static u8 TM_IIC_Send_Byte(u8 txd)
{
    u8 t;
	u8 ErrTime=0;
	
	TM_SDA_OUT();
    for(t=0;t<8;t++)
    {
		TM_IIC_SCL=0;		//拉低时钟开始数据传输
		delay_us(2);
        TM_IIC_SDA = txd&0x01;
        txd>>=1;
		delay_us(4);
		TM_IIC_SCL=1;
		delay_us(2);
    }

	TM_SDA_IN();		//SDA设置为输入  
	TM_IIC_SDA=1;delay_us(2);	   
	TM_IIC_SCL=0;delay_us(2);	//在第8个时钟下降沿，从机返回应答
	while(TM_READ_SDA)
	{
		ErrTime++;
		if(ErrTime>250)
			return 1;
	}
	TM_IIC_SCL=1;
	delay_us(2);
	return 0;
} */

static u8 TM_IIC_Send_Byte(u8 txd)
{
	u8 t;
	u8 ErrTime=0;
	
	TM_SDA_OUT(); 	    
    TM_IIC_SCL=0;//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {
        TM_IIC_SDA=txd&0x01;
        txd>>=1;
		delay_us(4);
		TM_IIC_SCL=1;
		delay_us(2); 
		TM_IIC_SCL=0;	
		delay_us(2);
    }

	TM_SDA_IN();		//SDA设置为输入  
	TM_IIC_SDA=1;delay_us(2);	   
	TM_IIC_SCL=0;delay_us(2);	//在第8个时钟下降沿，从机返回应答
	while(TM_READ_SDA)
	{
		ErrTime++;
		if(ErrTime>250)
			return 1;
	}
	TM_IIC_SCL=1;
	delay_us(2);
	return 0;
}
//IIC读取一个字节
//需要在读取完一个字节后，给从机返回一个ACK
static u8 TM_IIC_Read_Byte(void)
{
	u8 i, recv=0;
	
	TM_SDA_IN();	//SDA设置为输入模式
    for(i=0;i<8;i++)
	{
        TM_IIC_SCL=0; 
        delay_us(2);
		TM_IIC_SCL=1;
		delay_us(2);
        recv>>=1;
        if(TM_READ_SDA) recv|=0x80;
    }
	TM_IIC_SCL=0;
	TM_SDA_OUT();
	TM_IIC_SDA=0;
	delay_us(2);
	TM_IIC_SCL=1;
	delay_us(2);
	TM_IIC_SCL=0;
	delay_us(2);

    return recv;
}

void TM1637_Init(void)
{
	TM_IIC_Init();
	TM_IIC_Start();
	TM_IIC_Send_Byte(0x8C);	//显示控制命令：开显示，脉冲宽度为11/16.
	TM_IIC_Stop();
}

void TM1637_SetDigit(u8 digit, u8 c)
{
	TM_IIC_Start();
	TM_IIC_Send_Byte(0x44);		//数据命令设置：固定地址，写数据到显示寄存器
	TM_IIC_Stop();

	TM_IIC_Start();
	TM_IIC_Send_Byte(digit);	//地址命令设置：写入addr对应地址
	TM_IIC_Send_Byte(c);		//给addr地址写数据
	TM_IIC_Stop();
}

u8 TM1637_Scan_Key(void)
{
	u8 key;
	
	TM_IIC_Start();
	TM_IIC_Send_Byte(0x42);	//写读键指令0x42
	key = TM_IIC_Read_Byte();
	TM_IIC_Stop();

	return key;
}
