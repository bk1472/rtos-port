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
TaskHandle_t		RootTaskHandle;
uint32_t			initialStk[0x600];

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
	serial_init(baudrate);
}

int main (void)
{
	extern int pollPrint(char *fmt, ...);
	board_init(115200);

	pollPrint("\x1b[35mstart FreeRTOS (v10.4.3)\x1b[0m\n");

	if(pdPASS != xTaskCreate(RootTask, "root", (uint16_t)0x800, NULL, 4, &RootTaskHandle))
	{
		pollPrint("\x1b[31mERROR: Root Task Create!!\x1b[0m\n");
	}
	else
	{
	    /* Start the FreeRTOS scheduler */
	    vTaskStartScheduler();
	}
	/**
	 * Never Return Here !!!
	 */
	while(1)
		;
	return 0;
}

void sym_check(void)
{
	extern char __bss_end__[];
	extern char _start_code[];
	UINT32 *src = (UINT32*)(_start_code + 0x400000);
	UINT32 *dst = (UINT32*)((char*)__bss_end__ + configTOTAL_HEAP_SIZE);


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
