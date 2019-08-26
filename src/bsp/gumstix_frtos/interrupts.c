/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 2000. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : interrupts.c
    VERSION     :
    AUTHOR      : bk1472 (baekwon.choi@lge.com)
    DATE        : 2010/01/28
    DESCRIPTION :

*******************************************************************************/

/*---------------------------------------------------------
    전역 제어 상수 정의
    (Global Control Constant Definitions)
---------------------------------------------------------*/

/*---------------------------------------------------------
    #include 파일들
    (File Inclusions)
---------------------------------------------------------*/
#include <common.h>
#include <board.h>
#include <osadap.h>

/*---------------------------------------------------------
    상수 정의
    (Constant Definitions)
---------------------------------------------------------*/
#define MAX_ISR_COUNT		(TOTAL_INT_COUNT) /*defined at pxa255.h*/
#define IRQ_MODE			(0)
#define FIQ_MODE			(1)

/*---------------------------------------------------------
    형 정의
    (Type Definitions)
---------------------------------------------------------*/
typedef struct ISR_STR
{
	UINT32 icip;	/* IC IRQ pending register */
	UINT32 icmr;	/* IC Mask Register        */
	UINT32 iclr;	/* IC Level Register       */
	UINT32 icfp;	/* IC FIQ pending Register */
	UINT32 icpr;	/* IC Pending Register     */
	UINT32 iccr;	/* IC Control Register     */
} ISR_STR_T;

typedef struct
{
	PFNCT	isrAddr;
	int		isrArg;
	int		isrCnt;
	char	isrName[8];
} VEC_ENTRY;

/*---------------------------------------------------------
    Extern 전역변수와 함수 prototype 선언
    (Extern Variables & Function Prototype Declarations)
---------------------------------------------------------*/

/*---------------------------------------------------------
    전역 변수와 함수 prototype 선언
    (Variables & Function Prototypes Declarations)
---------------------------------------------------------*/

/*---------------------------------------------------------
    Static 변수와 함수 prototype 선언
    (Static Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
static volatile		ISR_STR_T	*VIC           = (volatile ISR_STR_T *)IC_BASE;
static int					 	InIntContext   = 0;
static int						intNum         = 0xff;
static const int				isrOrder[]     = INT_ORDER_LIST;
static VEC_ENTRY				intVecTable[MAX_ISR_COUNT];

static void						interrupt_process		(UINT32 intMode);
static int						int_connect				(VEC_ENTRY *pVec, PFNCT isr, int arg, char *name);



int init_interrupt_manager (void)
{
	int		i;

	VIC->iclr = 0x0000;
	VIC->icmr = 0x0000;
	VIC->iccr = 0x0000;
//	VIC->icip = 0x0000;
//	VIC->icfp = 0x0000;

	for (i = 0; i < MAX_ISR_COUNT; i++)
	{
		intVecTable[i].isrAddr    = 0;
		intVecTable[i].isrArg     = 0;
		intVecTable[i].isrCnt     = 0;
		intVecTable[i].isrName[0] = '\0';
	}

	return 0;
}

void intConnect(int intVector, PFNCT isr, int arg, char *name)
{
	if (intVector >= MAX_ISR_COUNT)
		return;

	int_connect(&intVecTable[intVector], isr, arg, name);
}

int intEnable(int intVector)
{
	if (intVector >= MAX_ISR_COUNT)
		return -1;

	VIC->iccr  = 0x01;
	VIC->iclr &= ~(1 << intVector); // IC level register set to IRQ mode
	VIC->icmr |=  (1 << intVector); // IC Mask Register set for enabling interrupt

	return 0;
}

int intDisable(int intVector)
{
	if (intVector >= MAX_ISR_COUNT)
		return -1;

	VIC->iccr  = 0x01;
	VIC->iclr &= ~(1 << intVector); // IC level register set to IRQ mode
	VIC->icmr &= ~(1 << intVector); // IC Mask Register reset for disabling interrupt

	return 0;
}

int intContext(void)
{
	if (InIntContext > 0)
		return 1;

	return 0;
}

void intContextEnter(void)
{
	InIntContext++;
}

void intContextExit(void)
{
	InIntContext--;
}

void setIntNum(int num)
{
	intNum = num;
}

int getIntNum(void)
{
	return intNum;
}

char *getIntName(void)
{
	if (intNum == 0xff || intNum >= MAX_ISR_COUNT)
		return NULL;

	return intVecTable[intNum].isrName;
}

static void interrupt_process(UINT32 intMode)
{
	UINT32	i;
	UINT32	intStatus;
	PFNCT	handler;
	int		arg;

	while (1)
	{
		if (intMode == IRQ_MODE) intStatus = VIC->icip;
		else                     intStatus = VIC->icfp;

		if (intStatus == 0)
			break;
		i = 0;
		while (isrOrder[i] != INT_INVALID_ORDER)
		{
			if (isrOrder[i] < 0 || isrOrder[i] >= MAX_ISR_COUNT)
			{
				i++;
				continue;
			}
			if ( intStatus & (1 << isrOrder[i]) )
			{
				setIntNum(isrOrder[i]);
				handler = intVecTable[isrOrder[i]].isrAddr;
				arg     = intVecTable[isrOrder[i]].isrArg;

				if( 0 != handler )
				{
					intVecTable[isrOrder[i]].isrCnt++;
					handler(arg);
				}
				break;
			}
			i++;
		}
		if (isrOrder[i] == INT_INVALID_ORDER)
		{
			/**
			 * NOT REGISTERED INTERRUPT OCCURED
			 */
			break;
		}
	}
}

static int int_connect(VEC_ENTRY *pVec, PFNCT isr, int arg, char *name)
{
	static const char defIsrName[8] = "nm-Isr";
	int		i;
	char	*isrName = NULL;

	if (pVec == NULL)
		return -1;

	if (isr == 0)
	{
		pVec->isrAddr    = 0;
		pVec->isrArg     = 0;
		pVec->isrName[0] = '\0';
		return -2;
	}
	pVec->isrAddr = isr;
	pVec->isrArg  = arg;

	if (name != NULL) isrName = name;
	else              isrName = (char *)defIsrName;

	for (i = 0; isrName[i] != '\0'; i++)
	{
		if (i == 7) break;
		pVec->isrName[i] = isrName[i];
	}
	pVec->isrName[i] = '\0';

	return 0;
}

void portIRQ_ISR_Handler(void)
{
	int status = intLock();

	interrupt_process(IRQ_MODE);

	intUnlock(status);
}

int intLock (void)
{
	return port_SR_Save();
}

void intUnlock (int status)
{
	port_SR_Restore(status);
	return;
}
