#ifndef MCU_NetworkInterface_H
#define MCU_NetworkInterface_H


//#define MCU_Network_SERVER_IP "127.0.0.1"  //使用本机为 "127.0.0.1";示教盒:"192.168.1.206"

//#define MCU_Network_SERVER_PORT 6000


//#define		MCU_Network_Error_Disconnect					-300	//网络连接断开
//#define		MCU_Network_Error_DataMismatch				-299	//网络数据包不匹配
//#define		MCU_Network_Error_DownloadNoPath			-298 //网络升级找不到文件
//#define		MCU_Network_Error_DownloadOpenFile		-297 //网络升级打开文件错误

#if defined(__cplusplus)
extern "C"
{
#endif


//功能说明：
//1、传输报警信息和MC状态信息（初始化、信息变化时传输）
//2、备份按键信息回传，文件传输功能
//3、网络通讯异常，需要有自动连接功能（保证传输永不中断）,信息发送异常需要进行


//通讯方式，内存有变化时发送，还需要定期发送，用于检查网络状态。
//数据类型需根据 MCU 和 MC 的数据类型占用空间大小，对齐结构体
typedef struct MCU_NetworkOutputDataType   //相对MC
{
	char modeOfMC;	//0 program, 1 autoRun, 2 Teach , 3 Remote
	char emg;				//0 表示无急停，1表示急停状态
	char feedbackBackUp;//0常态或备份结束，1准备备份，2备份中
	int errorNo;
	
	

//	int click;	//MC更新一次数据后会把这个标示+1 也可用于检测通讯异常
}MCU_NetworkOutputDataType;

typedef struct MCU_NetworkInputDataType
{	
	char triggerBackup;//0常态或备份结束，1触发备份，2可以开始备份，
	
	int click;	//MCU更新一次数据后把这个标识+1 也可用于检测通讯异常
}MCU_NetworkInputDataType;

typedef struct MCU_NetworkOperateDataType
{	
	int tmp;
	
	
	char pBuf_Revc[64];
	int flagPos_Recv;
	int size_Recv;
	
}MCU_NetworkOperateDataType;



#ifdef outputData_MCUNetwork_GLOBALS
#define outputData_MCUNetwork_EXT
#else
#define outputData_MCUNetwork_EXT extern
#endif
outputData_MCUNetwork_EXT MCU_NetworkOutputDataType outputData_MCUNetwork;


#ifdef inputData_MCUNetwork_GLOBALS
#define inputData_MCUNetwork_EXT
#else
#define inputData_MCUNetwork_EXT extern
#endif
inputData_MCUNetwork_EXT MCU_NetworkInputDataType inputData_MCUNetwork;

#ifdef operateData_MCUNetwork_GLOBALS
#define operateData_MCUNetwork_EXT
#else
#define operateData_MCUNetwork_EXT extern
#endif
operateData_MCUNetwork_EXT MCU_NetworkOperateDataType operateData_MCUNetwork;

//返回负数，表示异常，>=0为初始化成功
int MCU_NetworkInit(MCU_NetworkOperateDataType *pOpData,MCU_NetworkInputDataType *pInput,MCU_NetworkOutputDataType *pOutput);


void MCU_NetworkExecuteCycle(MCU_NetworkOperateDataType *pOpData,MCU_NetworkInputDataType *pInput,MCU_NetworkOutputDataType *pOutput);


void MCU_NetworkClose(MCU_NetworkOperateDataType *pOpData,MCU_NetworkInputDataType *pInput,MCU_NetworkOutputDataType *pOutput);


int MCU_ProcessRevcString(char *pInputBuf, int lenBuf);
//返回发送字节数
int MCU_GetSendString(char *pInputBuf);


int MCU_atoi(char *str);


//@param n number to be convered into str
//@param str pointer to a buffer provided by caller
//@param numChar 返回字符串的长度，即整数的个数
void MCU_itoa(int n, char *str, int numChar);
#if defined(__cplusplus)
}
#endif 

#endif
