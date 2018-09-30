#define outputData_MCUNetwork_GLOBALS
#define inputData_MCUNetwork_GLOBALS
#define operateData_MCUNetwork_GLOBALS


#include "MCU_NetworkInterface.h"

#include <string.h>


int MCU_InputData2Buffer(MCU_NetworkInputDataType *pInputData,char *pBuf)
{
	int i=0;

	MCU_itoa((int)pInputData->triggerBackup,&pBuf[i],1);
	i +=1;
	MCU_itoa((int)pInputData->click,&pBuf[i],4);
	i +=4;
	pBuf[i++]=0;
	return i;
}

void MCU_SetOutputData(char *pBuf,MCU_NetworkOutputDataType *pOutputData)
{
	int i,k;
	char tmpString[12];
	i=4;
	tmpString[0]=pBuf[i++];
	tmpString[1]=0;
	pOutputData->modeOfMC=(char)MCU_atoi(tmpString);

	tmpString[0]=pBuf[i++];
	tmpString[1]=0;
	pOutputData->emg=(char)MCU_atoi(tmpString);
	
	tmpString[0]=pBuf[i++];
	tmpString[1]=0;
	pOutputData->feedbackBackUp=(char)MCU_atoi(tmpString);
	
	
	for(k=0;k<4;k++)
	{
		tmpString[k]=pBuf[i++];
	}
	tmpString[k]=0;
	pOutputData->errorNo=MCU_atoi(tmpString);
	
//	printf("modeOfMC: %d\r\nemg: %d\r\nerrNo: %d\r\n", pOutputData->modeOfMC, pOutputData->emg, pOutputData->errorNo);
}

int MCU_GetSendString(char *pInputBuf)
{
	int numByte;
	MCU_NetworkInputDataType *pInputData=&inputData_MCUNetwork;
	
	numByte=MCU_InputData2Buffer(pInputData,&pInputBuf[4]);
	numByte+=4;
	
	MCU_itoa(numByte,pInputBuf,4);
	
	return numByte;
}

int MCU_ProcessRevcString(char *pInputBuf, int lenBuf)
{
	MCU_NetworkOperateDataType *pOperateData=&operateData_MCUNetwork;
	MCU_NetworkOutputDataType *pOutputData=&outputData_MCUNetwork;
	int flagRet=0;
	char *pBuf=pOperateData->pBuf_Revc;
	int flagPos=pOperateData->flagPos_Recv;
	int revSize;
	char pTmp[8];
	
	if(lenBuf<=0)
		return 0;
	
	if (flagPos<4)
	{
		memcpy(&pBuf[flagPos],pInputBuf,lenBuf);
		
		flagPos+=lenBuf;
		if (flagPos>=4)
		{
			pTmp[0]=pBuf[0];pTmp[1]=pBuf[1];pTmp[2]=pBuf[2];pTmp[3]=pBuf[3];
			pTmp[4]=0;
			pOperateData->size_Recv=MCU_atoi(pTmp);
			revSize=pOperateData->size_Recv;
		}
		else
			revSize=0xffff;
	}
	else
	{
		memcpy(&pBuf[flagPos],pInputBuf,lenBuf);
		
		flagPos+=lenBuf;
		revSize=pOperateData->size_Recv;
	}

	if (flagPos>=revSize)//yes done
	{
		MCU_SetOutputData(pBuf,pOutputData);
		
		flagPos=0;
		flagRet=1;
	}
	else
	{
		if (flagPos==2)//MC
		{
			flagPos=0;
		}
	}
	pOperateData->flagPos_Recv=flagPos;
	return flagRet;
}


int MCU_NetworkInit(MCU_NetworkOperateDataType *pOpData,MCU_NetworkInputDataType *pInput,MCU_NetworkOutputDataType *pOutput)
{
	return 0;
}


void MCU_NetworkExecuteCycle(MCU_NetworkOperateDataType *pOpData,MCU_NetworkInputDataType *pInput,MCU_NetworkOutputDataType *pOutput)
{
	return;
}


void MCU_NetworkClose(MCU_NetworkOperateDataType *pOpData,MCU_NetworkInputDataType *pInput,MCU_NetworkOutputDataType *pOutput)
{
	return;
}


/**
 * @param str ended with null (range 0 ~ 2G-1, 1048575)
 * @return conversion result / -1 if there is an error
 */
int MCU_atoi(char *str)
{
	int res=0, i=0;
	
	while(str[i] != 0)
	{
		if(str[i]<'0' || str[i]>'9')
			return -1;
		res = res*10 + str[i] - '0';
		i++;
	}
	return res;
}


/**
 * @param n number to be convered into str
 * @param str pointer to a buffer provided by caller
 * @param numChar 返回字符串的长度，即整数的个数
 */
void MCU_itoa(int n, char *str, int numChar)
{
	int len, i;
	int tmp;
	
	tmp = 1;
	for(i=0 ; i<numChar ; i++)
		tmp *= 10;

	n=n % tmp;
	len=numChar;
	
	i = 0;
	tmp=len-1-i;
	while(tmp>=0)
	{
			if(n==0)
			{
				str[tmp]='0';
			}
			else
			{
				str[tmp] = n%10 + '0';
				n /= 10;
			}
			i++;
			tmp=len-1-i;
	}
//	str[len] = 0;
}

