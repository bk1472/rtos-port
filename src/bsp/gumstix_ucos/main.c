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
    (Global Control Constant Definitions)
---------------------------------------------------------*/

/*---------------------------------------------------------
     (File Inclusions)
---------------------------------------------------------*/
#include <common.h>
#include <stdio.h>
#include <osadap.h>
#include <board.h>
#include <bsp_arch.h>
#include "exception.h"

/*---------------------------------------------------------
    (Constant Definitions)
---------------------------------------------------------*/

/*---------------------------------------------------------
    (Type Definitions)
---------------------------------------------------------*/

/*---------------------------------------------------------
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
    (Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
OS_TCB			RootTCB;
CPU_STK			initialStk[0x600];
CPU_STK			RootTaskStk[0x800];

MEM_MAP_T		mem_map[] =
{
	{ "RAM",   CPU_MEM_BASE,  CPU_MEM_SIZE, DATA_CACHE },
	{ "Timer", OS_TIMER_BASE, 0x00100000,   0          },
	{ "UART",  FFUART_BASE,   0x00100000,   0          },
	{ "none",  0,             0,            0          }
};

/*---------------------------------------------------------
    (Static Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
static void		RootTask						(void *p_arg);


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
	OS_ERR	err;
	int		ver[3];

	board_init(115200);

	ver[0] = OS_VERSION / 10000;
	ver[1] = OS_VERSION % 10000;
	ver[2] = ver[1] % 100;
	ver[1] = ver[1] / 100;
	pollPrint("\x1b[35mstart uC/OS-iii(ver %d.%d.%d)\x1b[0m\n", ver[0], ver[1], ver[2]);

	OSInit(&err);
	EnableInterrupts();

	if (err == OS_ERR_NONE)
	{
		OSTaskCreate(&RootTCB, "root", (OS_TASK_PTR)RootTask, (void*)0, 20, &RootTaskStk[0],
				0x600, 0x800, 40, 10, NULL, OS_OPT_TASK_NONE, &err);

		OSStart(&err);                              /* Start multitasking (i.e. give control to uC/OS-II)      */
	}
	else
	{
		pollPrint("\x1b[37m\x1b[41mOS Start Failed!\x1b[0m\n");

		while (1)
			;
	}

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

static void  RootTask (void *p_arg)
{
	extern int root (void);
	excInitLGE();

	root();

	while(1)
		;
}
