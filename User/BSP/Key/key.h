#include "stm32f10x.h"
#include "global.h"

#define KEY0   PBin(8)

void Key_Init(void);
u8 Key0_Scan(u8 mode);
