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
    전역 제어 상수 정의
    (Global Control Constant Definitions)
---------------------------------------------------------*/
#define __DEBUGGING_STATE__

/*---------------------------------------------------------
    #include 파일들
    (File Inclusions)
---------------------------------------------------------*/
#include <common.h>
#include <osadap.h>
#include <stdio.h>
#include <atomport.h>
#include "exception.h"

/*---------------------------------------------------------
    상수 정의
    (Constant Definitions)
---------------------------------------------------------*/
/*---------------------------------------------------------
    형 정의
    (Type Definitions)
---------------------------------------------------------*/

/*---------------------------------------------------------
    Extern 전역변수와 함수 prototype 선언
    (Extern Variables & Function Prototype Declarations)
---------------------------------------------------------*/
extern void archIRQHandler(void);

/*---------------------------------------------------------
    Static 변수와 함수 prototype 선언
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
	{EXC_OFF_IRQ,		archIRQHandler        },
	{EXC_OFF_FIQ,		NULL                  }

};

/*---------------------------------------------------------
    전역 변수와 함수 prototype 선언
    (Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
int							exception_vector_init (void);
volatile PUCOS_EXC_HANDLE   atomExcHandle   = excExcBaseHandle;
volatile PUCOS_EXC_RESET    atomResetHandle = excResethandler;



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
	CRITICAL_STORE;

	CRITICAL_START();
	intContextEnter();
	setIntNum(0xff);

	/**
	 * if Program Counter is Null...
	 */
	if (pRegs->pc == 0)
		vec = pEsf->vecAddr = EXC_OFF_RESET;

	/**
	 * Code 삽입 해야 함....
	 * MMU mapped 된 영역이면서 break point code 일 때.
	 */
	exception_count_all++;
	if (0 == code_mode && vec != EXC_OFF_SWI)
	{
		if(++exception_count_critical >=EXC_ACEPT_MAX_COUNT)
		{
			#ifdef __DEBUGGING_STATE__
			#else
			atomResetHandle();
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
			atomExcHandle(vec, pEsf, pRegs, &exc_info);
	}
	pExcRegList = NULL;

	intContextExit();
	CRITICAL_END();

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
	/**
	 * Break point 기능이 있을 때는 기능 추가.
	 */
	atomExcHandle(vec, pEsf, pRegs, &excInfo);
}

UINT32 *excGetRegList(void)
{
	if (exception_count_all > 0) return(pExcRegList);
	else                         return(NULL);
}
