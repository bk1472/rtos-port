/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 1998-2003. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : dbgprint.c
    VERSION     : 2.0
    AUTHOR      : 이재길(jackee@lge.com)
    DATE        : 2003.01.22
    DESCRIPTION : Customized print, crc and stacktrace  utilities.

*******************************************************************************/

/*-----------------------------------------------------------------------------
	제어 상수
	(Control Constants)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
    #include 파일들
    (File Inclusions)
------------------------------------------------------------------------------*/
#include <common.h>
#include <osa/osadap.h>
#include <osa/dbgprint.h>
#include <bsp_arch.h>
#include <serial.h>

/*----------------------------------------------------------------------------
    형 정의
    (Type Definitions)
-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
	Extern 전역변수와 함수 prototype 선언
	(Extern Variables & Function Prototype Declarations)
------------------------------------------------------------------------------*/
extern SINT16	GetFrameSize(void (*pFunc)(), UINT32 *sp, UINT32 *pc, UINT32 **ra, int nTryStmdb);
extern int		isValidPc(UINT32 *pc);
extern osv_t *	findMyOsvEntry(int myId);

/*-----------------------------------------------------------------------------
	Static 변수와 함수 prototype 선언
	(Static Variables & Function Prototypes Declarations)
------------------------------------------------------------------------------*/
static void (*pmPrintf)() = (void (*)(void))pollPrint;

/******************************************************************************
**		Utility for Stack Tracer
******************************************************************************/
UINT32			_begOsaMem = 0;		/* Begin Address of OsaMem		*/
UINT32			_endOsaMem = 0;		/* End   Address of OsaMem		*/
UINT32			_verboseTr = 0;		/* Display stack trace message	*/

/*
 * Hard limit is 7, but should be defined with proper value less than 7
 * by caller.  It is expected to be defined in $(DEST_OS)/osamem.c
 */
#define	SM_NCALLS				7				/* Should be 7				*/

void
SetAllocatorRange(UINT32 addr1, UINT32 addr2)
{
	_begOsaMem = addr1;
	_endOsaMem = addr2;
	return;
}

char	trace_fmt1[] = ">>    [%02d: sp=0x%08x, pc=0x%08x]\n";
char	trace_fmt2[] = ">>    [%02d: sp=0x%08x, pc=0x%08x(0x%06x+0x%04x), %s]\n";
char	trace_fmt3[] = ">>    [%02d: sp=0x%08x, pc=0x%08x,%s()@%s:%d]\n";

typedef struct {
	UINT32 addr;
	UINT32 end;
	UINT32 ptr;
} symEntry_t;

UINT32	nTxtSyms = 0;
UINT32	*pSymTabBase = NULL;
char	*pSymStrBase = NULL;
UINT32	*pSymHashBase = NULL;

UINT32 findSymByAddr(UINT32 addr, char **pSymName)
{
	int			x, l = 0, r = nTxtSyms-1, matched = 0;
	symEntry_t	*pSyms = (symEntry_t *)pSymTabBase;

	do
	{
		x = (l + r) / 2;
		if      (addr < pSyms[x].addr) { matched = 0; r = x - 1; }
		else if (addr < pSyms[x].end ) { matched = 1; l = x + 1; }
		else                           { matched = 0; l = x + 1; }

	} while ((l <= r) && (matched == 0));

	if (matched)
	{
		*pSymName = &pSymStrBase[pSyms[x].ptr];
		return(pSyms[x].addr);
	}
	else
	{
		*pSymName = (char *)"Not Found";
		return 0;
	}
}

static void build_SymHashTable(void)
{
	symEntry_t	*pSyms = (symEntry_t *)pSymTabBase;
	UINT32 i, j;

	pSymHashBase = (UINT32*)malloc(sizeof(UINT32)*nTxtSyms);

	for(i = 0; i < nTxtSyms; i++)
		pSymHashBase[i] = i;

	for(i = 0; i < nTxtSyms; i++)
	{
		for(j = 1; j < nTxtSyms; j++)
		{
			UINT32 _data;
			if( strcmp(&pSymStrBase[pSyms[pSymHashBase[j-1]].ptr], &pSymStrBase[pSyms[pSymHashBase[j]].ptr]) > 0 )
			{
				_data = pSymHashBase[j];
				pSymHashBase[j] = pSymHashBase[j-1];
				pSymHashBase[j-1] = _data;
			}
		}
	}
}

UINT32 findSymByName(char *symName)
{
	int			x, l = 0, r = nTxtSyms-1, matched = 0;
	symEntry_t	*pSyms = (symEntry_t *)pSymTabBase;

	if (pSymHashBase == NULL)
		build_SymHashTable();

	do
	{
		x = (l + r) / 2;
		if      (strcmp(symName, &pSymStrBase[pSyms[pSymHashBase[x]].ptr]) <  0) { matched = 0; r = x - 1; }
		else if (strcmp(symName, &pSymStrBase[pSyms[pSymHashBase[x]].ptr]) == 0) { matched = 1; l = x + 1; }
		else                           { matched = 0; l = x + 1; }

	} while ((l <= r) && (matched == 0));

	if (matched)
	{
		return(pSyms[pSymHashBase[x]].addr);
	}

	return(0);
}

void PrintTracedFrame(void (*pFunc)(), int idx, UINT32 *tSP, UINT32 *tPC)
{
	if ((nTxtSyms > 0) && (pSymStrBase != NULL))
	{
		extern int addr2line(unsigned int addr, char **ppFileName);
		extern int nDwarfLst;
		char	*pFileName;
		int		lineNo = 0;
		char	*symName;
		UINT32	symValue;

		if ((symValue = findSymByAddr((UINT32)tPC, &symName)) != 0)
		{
			if (nDwarfLst)
				lineNo = addr2line((unsigned int)tPC, &pFileName);
			if (lineNo == 0)
				(*pFunc)(trace_fmt2, idx, tSP, tPC, symValue, (int)tPC-symValue, symName);
			else
				(*pFunc)(trace_fmt3, idx, tSP, tPC, symName, pFileName, lineNo);
			return;
		}
	}

	(*pFunc)(trace_fmt1, idx, tSP, tPC);
	return;
}

void
SetVerboseTrace(UINT32 new)
{
	_verboseTr = new;
}


void StackTrace(UINT32 size, UINT32 count, UINT32 *res, UINT32 *pStack, int taskId)
{
	UINT32		*pc, *ra, *sp;
	UINT32		*limit;				/* Upper limit of stack search	*/
	UINT32		*sptr = NULL;		/* Saved sp for more debug		*/
	UINT32		*susl = NULL;		/* Supervisor Upper Stack Limit	*/
	SINT16		fs = 0;				/* Frame Size in bytes			*/
	int			j=0, k=0;			/* Counter temporally used		*/
	int			doTraceAll = 0;		/* Flag for do burst tracing	*/
	void		(*pFunc)();			/* Print function               */
	ATOM_TCB	*tDesc = (ATOM_TCB *)0;

	/*
	 *	Flush last pened line in stdio buffer
	 */
	if (res == NULL)
	{
		UINT32	lock;

		lock = intLock();
		serial_flush_buffer();
		intUnlock(lock);
	}

	/*
	 * Store return address first since calling intContext() will destroy it.
	 */
	str_ra(&ra);

	if		(res    != NULL) pFunc = (void (*)(void))NULL;
	else if (pStack != NULL) pFunc = (void (*)(void))pmPrintf;
	else if (intContext()  ) pFunc = (void (*)(void))pmPrintf;
	else                     pFunc = (void (*)(void))rprint0n;

	if (pStack == NULL)				/* Stack Trace from Normal task	*/
	{
		str_pc(&pc);				/* Store program counter		*/
		str_sp(&sp);				/* Store stack pointer			*/
	}
	else							/* Stack trace from ISR			*/
	{
		osv_t *pOsv = NULL;

		pc = (UINT32 *)pStack[REG_PC];	/* Read pc from Exception Stack	*/
		ra = (UINT32 *)pStack[REG_LR];	/* Read lr from Exception Stack	*/
		sp = (UINT32 *)pStack[REG_SP];	/* Read sp from Exception Stack	*/
		if (taskId == 0) taskId = getTaskId(NULL);
		else             pFunc = (void (*)(void))rprint0n;
		pOsv = findMyOsvEntry(taskId);

		if (pOsv != NULL)
			tDesc = pOsv->u.t.ov_tcb;

		if(tDesc == (ATOM_TCB *)0) {
			pollPrint("Task Control Block Get Error=> tcb(%x)\n", (UINT32)tDesc);
			return;
		}
		susl = (UINT32 *)(tDesc->stack_size + ((tDesc->stack_size & ~(STACK_ALIGN_SIZE - 1)) - STACK_ALIGN_SIZE));

		sptr = sp;

		(*pFunc)("\n");
		if (!isValidPc(pc))
		{
			/*
			 * 무효한 PC 면 LR를 대신 사용한다.
			 */
			(*pFunc)("      Changing pc: 0x%x-> 0x%x\n", pc, ra);
			pc = ra;
		}
		else if (isValidPc(ra))
		{
			UINT32		*tmp_ra;

			/*
			 * PC는 유효하지만, 3개 이상의 Frame을 추적할 수 없고,
			 * LR이 유효하면, LR을 대신 사용한다.
			 */
			fs = GetFrameSize(pFunc, sp, pc, &tmp_ra, 1);
			if ((fs > 0) && (GetFrameSize(pFunc, sp + fs/4, tmp_ra, &tmp_ra, 1) <= 0))
			{
				(*pFunc)("      Changing pc(2): 0x%x-> 0x%x\n", pc, ra);
				pc = ra;
			}
		}

		(*pFunc)(">>    Stack Trace for C functions\n");
		PrintTracedFrame(pFunc, j, sp, pc);
		j = 1;
	}

	if ((res != NULL) && (count > SM_NCALLS)) count = SM_NCALLS;

	fs = GetFrameSize(pFunc, sp, pc, &ra, 1);

	if ((pStack != NULL) && (fs <= 0))
	{
		int		callDepth, maxDepth = 0;
		int		x, y, z;
		int		fsFound, fsNew;
		UINT32	*lrFound, *lrNew;
		UINT32	*nsp, *npc;

		/* Intensive search of valid frame structure				*/
		/*	  1. Select function addrs in [sp, sp+1k)				*/
		/*	  2. For each addrs, detect start point of local frame 	*/
		/*	  3. Calculate depth of function call for this frame	*/
		/*	  4. Replace sp & pc with values having maximum depth	*/
		for (x = 0, y = 0; (x < 256) && (y < 8); x++)
		{
			nsp = (UINT32 *)(sptr+x);
			npc = (UINT32 *)(sptr[x]);

			/*	Select function addrs in [sp, sp+1k)				*/
			if (isValidPc(npc))
			{
				y++;
				for (z = 0, fsFound = 0; z < 128; z += fsFound/4)
				{
					nsp = (UINT32 *)(sptr+x+z);
					npc = (UINT32 *)(sptr[x]);

					/*	Detect start pointlocal frame				*/
					fsFound = GetFrameSize(pFunc, nsp, npc, &lrFound, 128);
					if (fsFound <= 0)
					{
						break;;
					}

					fsNew = fsFound;
					lrNew = lrFound;
					/*	Calculate depth of function call			*/
					for (callDepth = 0; fsNew > 0; callDepth++)
					{
						nsp  += fsNew/4;
						npc   = lrNew;
						fsNew = GetFrameSize(pFunc, nsp, npc, &lrNew, 1);

						if ( !isValidPc(lrNew) || (fsNew < 0) || ((fsNew == 0) && (npc == lrNew)) )
							break;
					}

					/* Update maximum depth							*/
					if (callDepth > maxDepth)
					{
						sp = (UINT32 *)(sptr+x+z);
						pc = (UINT32 *)(sptr[x]);
						ra = (UINT32 *)(lrFound);
						fs = fsFound;
						maxDepth = callDepth;
					}
				}
			}
		}

		if (maxDepth > 0)
		{
			for (z = 0; z < fs/4; z++)
			{
				if (isValidPc((UINT32*)sp[z]))
					PrintTracedFrame(pFunc, -1, sp+z, (UINT32 *)sp[z]);
			}
		}
	}

	limit = sp + (size/4);			/* Set search boundary			*/

	for (k = 0; (sp < limit) && (j < count) && (k < 64); k++)
	{
		/* Need Conditionally different function address.			*/
		/* check ram.map to select correct function					*/

		if (pFunc != NULL)			/* Printer has been binded		*/
		{
			PrintTracedFrame(pFunc, j, sp, ra);
			j++;
		}
		else if ( !_begOsaMem || ((UINT32)ra < _begOsaMem) || ((UINT32)ra > _endOsaMem) )
		{									/* From SMalloc Log		*/
			res[j] = (UINT32)ra;
			j++;
		}

		if (fs < 0)
			break;

		sp += fs / 4;
		pc  = ra;
		fs = GetFrameSize(pFunc, sp, pc, &ra, 1);
		if (!isValidPc(ra))
			break;

		if ((fs == 0) && (pc == ra))
		{
		   	if ((sp + 0x14) < susl) doTraceAll = 1;
			break;
		}
	}

	if ( (pStack != NULL) && ((doTraceAll != 0) || (j < 3)) )
	{
		(*pFunc)("\n>>    List of PCs in stack [%06x-%06x]\n", sptr, susl);
		for (j = 0, k = 0; ((sptr+k) < susl) && (j < 128); k++)
		{
			pc = (UINT32 *)sptr[k];
			if (isValidPc(pc))
			{
				PrintTracedFrame(pFunc, k, sptr+k, pc);
				j++;
			}
		}
		return;
	}

	return;
}

void PrintStack(void)
{
	StackTrace(0x2000, 32, NULL, NULL, 0);
	return;
}

void PrintLoggedFrame(UINT32 *pFrame, UINT32 count)
{
	int		i;
	void	(*pFunc)();			/* Print function               */

	if (intContext()) pFunc = (void (*)(void))pmPrintf;
	else              pFunc = (void (*)(void))rprint0n;

	for (i = 0; (i < count) && (pFrame[i] > 0); i++)
	{
		PrintTracedFrame(pFunc, i, 0, (UINT32 *)pFrame[i]);
	}

	return;
}
