/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 2000. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : exception_handler_c.c
    VERSION     :
    AUTHOR      : bk1472 (baekwon.choi@lge.com)
    DATE        : 2010/01/28
    DESCRIPTION :

*******************************************************************************/

/*---------------------------------------------------------
    (Global Control Constant Definitions)
---------------------------------------------------------*/
#define __DEBUGGING_STATE__

/*---------------------------------------------------------
    (File Inclusions)
---------------------------------------------------------*/
#include <common.h>
#include <osadap.h>
#include <stdio.h>
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
extern void OS_CPU_IRQ_ISR(void);
extern void OS_CPU_FIQ_ISR(void);

/*---------------------------------------------------------
    (Static Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
static void		excRSVDerr(void);
static int		exception_count_critical = 0;
static int		exception_count_all      = 0;
static UINT32	*pExcRegList             = 0;

static void		excResethandler		(void);
static int		excExcBaseHandle	(int vect, void *sp, REG_SET *pRegs, EXC_INFO *pExcInfo);
static void		excExcHandle		(ESF *pEsf, REG_SET *pRegs, int code_mode);

static EXC_TBL	excEnterTbl[NUM_EXC_VECS] =
{
	{EXC_OFF_RESET, 	excEnterReset         },
	{EXC_OFF_UNDEF, 	excEnterUndef         },
	{EXC_OFF_SWI, 		excEnterSwi           },
	{EXC_OFF_PREFETCH,	excEnterPrefetchAbort },
	{EXC_OFF_DATA,		excEnterDataAbort     },
	{EXC_OFF_RSVD,		excRSVDerr            },
	{EXC_OFF_IRQ,		OS_CPU_IRQ_ISR        },
	{EXC_OFF_FIQ,		OS_CPU_FIQ_ISR        }

};

/*---------------------------------------------------------
    (Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
int							exception_vector_init (void);
volatile PUCOS_EXC_HANDLE   uCosExcHandle   = excExcBaseHandle;
volatile PUCOS_EXC_RESET    uCosResetHandle = excResethandler;



int exception_vector_init (void)
{
	#define FIRST_VECTOR	EXC_OFF_RESET
	FAST int	i;
	#if (USE_HIGH_VECTOR > 0)
	UINT32		vecOffset    = 0xffff0000;
	#else
	UINT32		vecOffset    = 0xA0000000;
	#endif


	armInitExceptionModes();

	for (i = 0 ; i < NUM_EXC_VECS; ++i)
	{
		*(UINT32*)(excEnterTbl[i].vecAddr + vecOffset) = 0xe59ff000 | (EXC_VEC_TABLE_BASE - 8 - FIRST_VECTOR);
		*(pVOIDFUNCPTR*)
			(excEnterTbl[i].vecAddr+vecOffset + EXC_VEC_TABLE_BASE - FIRST_VECTOR) =
									excEnterTbl[i].fn;
	}

	*(UINT32*)(EXC_OFF_RESET + vecOffset) = EXC_RESET_CODE;
	return 0;
}

void excExcContinue
     (
	 ESF     * pEsf,
	 REG_SET * pRegs
	 )
{
	extern void	intContextEnter(void);
	extern void	intContextExit(void);
	extern int	intLock(void);
	extern void	intUnlock(int);
	extern void	setIntNum(int num);

	EXC_INFO	exc_info;
	FAST UINT32	vec = pEsf->vecAddr;
	FAST int	code_mode = 0;
    CPU_SR  cpu_sr;

    cpu_sr = 0;                                            /* Prevent compiler warning                 */

	CPU_CRITICAL_ENTER();
	intContextEnter();
	setIntNum(0xff);

	/**
	 * if Program Counter is Null...
	 */
	if (pRegs->pc == 0)
		vec = pEsf->vecAddr = EXC_OFF_RESET;

	exception_count_all++;
	if (0 == code_mode && vec != EXC_OFF_SWI)
	{
		if(++exception_count_critical >=EXC_ACEPT_MAX_COUNT)
		{
			#ifdef __DEBUGGING_STATE__
			#else
			uCosResetHandle();
			#endif
		}
	}

	pExcRegList = &pRegs->r[0];

	switch (vec)
	{
		case EXC_OFF_PREFETCH:
		case EXC_OFF_DATA:
		case EXC_OFF_UNDEF:
		case EXC_OFF_SWI:
			excExcHandle(pEsf, pRegs, code_mode);
			break;
		default:
			exc_info.valid   = EXC_INFO_VECADDR|EXC_INFO_PC|EXC_INFO_CPSR;
			exc_info.vecAddr = pEsf->vecAddr;
			exc_info.pc      = pRegs->pc;
			exc_info.cpsr    = pRegs->cpsr;
			uCosExcHandle(vec, pEsf, pRegs, &exc_info);
	}
	pExcRegList = NULL;

	intContextExit();
	CPU_CRITICAL_EXIT();

	return;
}

static void excResethandler(void)
{
//	system_reset();
}

static void excRSVDerr(void)
{
	pollPrint("exception occurred at Reserved Area!\n");
}

static int excExcBaseHandle(int vect, void *sp, REG_SET *pRegs, EXC_INFO *pExcInfo)
{
	pollPrint("[Default exception handler]\n");

	return 0;
}

static void excExcHandle
     (
	 ESF     * pEsf,
	 REG_SET * pRegs,
	 int     code_mode
	 )
{
	EXC_INFO excInfo;
	int vec = pEsf->vecAddr;

	excInfo.valid   = EXC_INFO_VECADDR|EXC_INFO_PC|EXC_INFO_CPSR;
	excInfo.vecAddr = pEsf->vecAddr;
	excInfo.pc      = pRegs->pc;
	excInfo.cpsr    = pRegs->cpsr;

	uCosExcHandle(vec, pEsf, pRegs, &excInfo);
}

UINT32 *excGetRegList(void)
{
	if (exception_count_all > 0) return(pExcRegList);
	else                         return(NULL);
}
