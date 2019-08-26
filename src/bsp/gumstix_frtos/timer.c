/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 2000. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : timer.c
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

/*---------------------------------------------------------
    형 정의
    (Type Definitions)
---------------------------------------------------------*/
typedef struct TIMER_STR
{
	UINT32		osmr0;
	UINT32		osmr1;
	UINT32		osmr2;
	UINT32		osmr3;
	UINT32		oscr;	/* OS Timer Counter Register          */
	UINT32		ossr;	/* OS Timer Status Register           */
	UINT32		ower;	/* OS Timer Watchdong Register        */
	UINT32		oier;	/* OS Timer Interrupt Enable Register */
} PRIM_TMR_STR_T;

/*---------------------------------------------------------
    Extern 전역변수와 함수 prototype 선언
    (Extern Variables & Function Prototype Declarations)
---------------------------------------------------------*/
extern void				intConnect					(int intVector, PFNCT isr, int arg, char *name);
extern int				intEnable					(int intVector);

/*---------------------------------------------------------
    전역 변수와 함수 prototype 선언
    (Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
void					primecell_tmr_Init			(void);
void					primecell_tmr_TickInit		(void);
void					primecell_tmr_TickISR		(void);

/*---------------------------------------------------------
    Static 변수와 함수 prototype 선언
    (Static Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
static volatile PRIM_TMR_STR_T		*pTimer = (volatile PRIM_TMR_STR_T *)OS_TIMER_BASE;

void  primecell_tmr_Init (void)
{
	#if 0
	pTimer->oscr  = 0;
	pTimer->osmr1 = pTimer->oscr + SYSTEM_CLOCK;
	pTimer->ossr  = pTimer->osmr1;
	#endif
}

void  primecell_tmr_TickInit(void)
{
	pTimer->oscr  = 0;
	pTimer->osmr1 = pTimer->oscr + SYSTEM_CLOCK / configTICK_RATE_HZ;

	pTimer->ossr  = pTimer->osmr1;
	intConnect(INT_NUM_OS_TIMER1, (PFNCT)primecell_tmr_TickISR, 0, "tick");
	intEnable(INT_NUM_OS_TIMER1);

	pTimer->oier |= (1<<1);
	pTimer->ossr  = pTimer->osmr1;
}

void  primecell_tmr_TickISR(void)
{
	extern void vTickISR( void );
	pTimer->ossr  = pTimer->osmr1;
	pTimer->osmr1 = pTimer->oscr + SYSTEM_CLOCK / configTICK_RATE_HZ;
	vTickISR();
}

UINT64 sysClkReadNanoSec (void)
{
	extern TickType_t xTaskGetTickCount( void );
	register UINT32	tick_count, clk_count;
	register UINT32	int_pended;
	register UINT64	nsec;
	register UINT32	ticksPerSec = configTICK_RATE_HZ;

	intDisable(INT_NUM_OS_TIMER1);

	tick_count = xTaskGetTickCount();

	clk_count  = pTimer->oscr;
	int_pended = pTimer->ossr;
	if (int_pended)
	{
		clk_count = pTimer->oscr;

		if (clk_count > 1)
			tick_count++;
	}
	intEnable(INT_NUM_OS_TIMER1);

	clk_count = SYSTEM_CLOCK/ticksPerSec - clk_count;

	nsec = ((UINT64)tick_count * 1000000000) / ticksPerSec +
		   ((UINT64)clk_count  * 1000000000) / SYSTEM_CLOCK;

	return (nsec);
}
