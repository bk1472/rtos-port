/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 2000. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : main.c
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
#include <stdio.h>
#include <osadap.h>
#include <board.h>
#include <bsp_arch.h>
#include "exception.h"

/*---------------------------------------------------------
    상수 정의
    (Constant Definitions)
---------------------------------------------------------*/
#define __KB						(1024)
#define IDLE_STACK_SIZE_BYTES       (1*__KB)

/*---------------------------------------------------------
    형 정의
    (Type Definitions)
---------------------------------------------------------*/

/*---------------------------------------------------------
    Extern 전역변수와 함수 prototype 선언
    (Extern Variables & Function Prototype Declarations)
---------------------------------------------------------*/
extern int		init_interrupt_manager			(void);
extern int		serial_init						(int baudrate);
extern void		primecell_tmr_Init				(void);
extern void		primecell_tmr_TickInit			(void);
extern void		EnableInterrupts				(void);
extern void		Cache_Init						(UINT32 intMask);
extern void		MMU_Enable						(MEM_MAP_T *);
extern void		Cache_Enable					(void);

/*---------------------------------------------------------
    전역 변수와 함수 prototype 선언
    (Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
ATOM_TCB		RootTCB;
uint8_t			initialStk[0x600*4];
uint8_t			idle_thread_stack[IDLE_STACK_SIZE_BYTES];
uint8_t			RootTaskStk[0x800*4];

MEM_MAP_T		mem_map[] =
{
	{ "RAM",   CPU_MEM_BASE,  CPU_MEM_SIZE, DATA_CACHE },
	{ "Timer", OS_TIMER_BASE, 0x00100000,   0          },
	{ "UART",  FFUART_BASE,   0x00100000,   0          },
	{ "none",  0,             0,            0          }
};

/*---------------------------------------------------------
    Static 변수와 함수 prototype 선언
    (Static Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
static void		RootTask						(uint32_t param);


static void board_init (int baudrate)
{
	Cache_Init((UINT32)(F_BIT | I_BIT));
	MMU_Enable(mem_map);
	Cache_Enable();

	exception_vector_init();
	init_interrupt_manager();

	primecell_tmr_Init();
	primecell_tmr_TickInit();
	serial_init(baudrate);
}

int main (void)
{
	int8_t	status;

	board_init(115200);

	pollPrint("\x1b[35mStart Atomthreads\x1b[0m\n");

	status = atomOSInit(&idle_thread_stack[0], IDLE_STACK_SIZE_BYTES, TRUE);
	if(ATOM_OK== status)
	{
		status = atomThreadCreate_ext(&RootTCB, 20, RootTask, 0, &RootTaskStk[0], (0x800*4), TRUE, "root");

		if(ATOM_OK == status)
			atomOSStart();
	}
	while (1)
		;

	/**
	 * Never Return Here !!!
	 */
	return 0;
}

void sym_check(void)
{
	extern char __bss_end__[];
	extern char _start_code[];
	UINT32 *src = (UINT32*)(_start_code + 0x100000);
	UINT32 *dst = (UINT32*)__bss_end__;


	if (src[0] == 0xB12791EE)
	{
		UINT32 size = src[2];

		memcpy((void*)dst, (void*)src, size);
	}
}

/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppStartTask()' by 'OSTaskCreate()'.
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

void excInitLGE(void)
{
	extern void	SM_InitResource(void);
	/* Exception hook installation has been moved to osadap_init */
    osadap_init(0,0);

	#if (USE_OSAMEM > 0)
	suspendTask(20);
	SM_InitResource();
	#endif /* USE_OSAMEM > 0 */

	return;
}

static void  RootTask (uint32_t param)
{
	extern int root (void);
	excInitLGE();

	root();

	while(1)
		;
}
