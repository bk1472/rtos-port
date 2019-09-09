#include <osa/osadap.h>
#include <common.h>
#include <exception.h>
#include <board.h>
#include <bsp_arch.h>
#include <serial.h>

extern void		cmdDebugMain(void);
extern void		dbgPrintTask1(char *taskName);

extern UINT32	_start_code[];
extern UINT32	__data_start__[];
extern UINT32	_verboseTr;		/* Display stack trace message	*/

#define _START	_start_code
#define _etext	__data_start__


/*
**	Function Call/Return related Instruction sets
*/
#define	ARM_INST_STM_SP_MASK	0xffff0000		/*	Mask for STMDB sp!, {..	*/
#define ARM_INST_STR_SP_MASK	0xfffff000		/*  Mask for STR lr,[sp,#]! */
#define ARM_INST_STR_SP_MASK2	0xffff0000		/*  Mask for STR Rd,[sp,#]! */
#define	ARM_INST_ADD_SP_MASK	0xfffff000		/*	Mask for ADD sp,sp,#n	*/
#define	ARM_INST_LDR2PC_MASK	0x0c00f000		/*	Mask for LDR pc,[rn,#n	*/
#define	ARM_INST_MOV2PC_MASK	0x0ffff000		/*	Mask for MOVxx pc,rn	*/
#define	ARM_INST_PC2LR_MASK		0x0fffffff		/*	Mask for MOVxx lr,pc	*/
#define	ARM_INST_BL_MASK		0x0f000000		/*	Mask for BL	xxxx		*/
#define ARM_INST_REG_LR_MASK	0x00004000		/*  Mask for LR is in RegSet*/
#define ARM_INST_REG_PC_MASK	0x00008000		/*  Mask for PC is in RegSet*/

#define	ARM_INST_STMDB_SP		0xe92d0000		/*	STMDB sp!, {r#-r#,lr}	*/
#define	ARM_INST_LDMIA_SP		0xe8bd8000		/*	LDIMA sp!, {r#-r#,pc}	*/
#define ARM_INST_STR_LR2SP		0xe52de000		/*  STR   lr,[sp,#]!        */
#define ARM_INST_STR_Rx2SP		0xe52d0000		/*  STR   Rd,[sp,#]!        */
#define	ARM_INST_MOV_LR2PC		0x01a0f00e		/*	MOVxx pc,lr             */
#define	ARM_INST_MOV_rx2PC		0x01a0f000		/*	MOVxx pc,rx             */
#define	ARM_INST_MOV_PC2LR		0x01a0e00f		/*	MOVxx lr,pc             */
#define	ARM_INST_BX_LR			0xe12fff1e		/*	BX    lr                */
#define	ARM_INST_ADD_SP			0xe28dd000		/*	ADD   sp,sp,#n			*/
#define	ARM_INST_SUB_SP			0xe24dd000		/*	SUB   sp,sp,#n			*/
#define	ARM_INST_BL				0x0b000000		/*	BL    label				*/
#define	ARM_INST_LDR2PC			0x0400f000		/*	LDR   pc,[rn,#			*/

#define SHOW_GFS_STAT(_t)		if (_verboseTr) (*pFunc)(_t ": pc=0x%08x, fs=%3d, lr=0x%08x, sp=0x%08x, spc=0x%08x\n", pc, fs, lr, sp, savedPc);
#define	FIND_METHOD				2


int isValidPc(UINT32 *pc)
{
	if (( (UINT32)pc >= (UINT32)_START) &&
		( (UINT32)pc <= (UINT32)_etext) &&
		(((UINT32)pc & 0x3) == 0      )
	   )
		return 1;

	return 0;
}

static SINT16 GetSubstractOffset(UINT32 inst)
{
	SINT16	rotate_imm;
	SINT16	immed_8;

	rotate_imm = ((inst & 0xf00) >> 8);
	immed_8    = ((inst & 0x0ff)     );

	if (rotate_imm<8) return((immed_8) >> (2 *     rotate_imm) );
	else              return((immed_8) << (2 * (16-rotate_imm)));
}

SINT16 GetFrameSize(void (*pFunc)(), UINT32 *sp, UINT32 *pc, UINT32 **ra, int nTryStmdb)
{
	UINT32	mask;			/* Mask variable to test regs set	*/
	#if	(FIND_METHOD == 0)
	UINT32	*savedPc = pc;	/* Saved input program counter		*/
	#else
	UINT32	*savedPc = 0;	/* Pc which has inst STR lr,[sp,#]	*/
	#endif /* (FIND_METHOD == 0) */
	UINT32	*pcSubSp = 0;   /* Pc which has inst SUB sp, #nn    */
	UINT32	*pcBound = pc;	/* Boundary to search STR_LR2SP inst*/
	UINT32	*lr = 0;		/* New Return address searched		*/
	SINT16	fs = 0, i;
	UINT08	bSTMInst = 0;

	/* 1: Check whether pc is valid or not						*/
	if (!isValidPc(pc))
		return 0;

	SHOW_GFS_STAT("1  ");

	/*	2: Search STMDB instruction backword					*/
	while (1)
	{
		if ((UINT32)pc < (UINT32)_START)
			return -1;

		if (((pc[0] & ARM_INST_STM_SP_MASK) == ARM_INST_STMDB_SP) &&
		    ((pc[0] & ARM_INST_REG_LR_MASK)                     ) )
		{
			bSTMInst = 1;
			break;
		}

		if ((pc[0] & ARM_INST_ADD_SP_MASK) == ARM_INST_SUB_SP)
		{
			pcSubSp = pc;
		}

		#if	(FIND_METHOD > 0)
		/*	Check function entry start with STR lr,[sp,#]	*/
		if (((UINT32)savedPc                ==                  0) &&
			((pc[0] & ARM_INST_STR_SP_MASK) == ARM_INST_STR_LR2SP) )
		{
			savedPc = pc;
			#if	(FIND_METHOD > 1)
			i  = 0;
			while ((pc[i] & ARM_INST_STR_SP_MASK2) == ARM_INST_STR_Rx2SP)
			{
				i  += 1;
				fs += 4;
			}
			break;
			#endif /* (FIND_METHOD > 1) */
		}
		#endif /* (FIND_METHOD > 0) */

		pc--;
	}
	SHOW_GFS_STAT("2  ");

	/* 3-1: Get size of frame to save registers via STMDB inst	*/
	if (bSTMInst)
	{
		for (mask = 0x4000; mask > 0; mask >>= 1)
		{
			if (pc[0] & mask) fs += 4;
		}
	}
	SHOW_GFS_STAT("3-1");

	/* 3-2: Get size of frame decremented by SUB inst			*/
	/* psip_main.c:Psip_Main() 함수에서 9번째에 SUB inst가 있음 */
	/*   12개로 늘이고, 중간에 return 명령어가 있으면 종료하는	*/
	/*   방식으로 수정, LDMxx 명령에 대해서도 처리해야 하나 	*/
	/*   STMxx 를 사용하는 함수는 12개 이상의 Instruction으로	*/
	/*   이루어지니까 처리하지 않아도 좋을 듯.					*/
	/* xm_hd3_draw.c:XmDRV_DrawPixmap() 에서는 13번째에 SUB inst*/
	/*	 가 있음 --> 16개로 늘이자.								*/
	for (i = 1; (pcSubSp != NULL) && (i <= 16); i++)
	{
		if ( ((pc[i] & ARM_INST_BL_MASK    ) == ARM_INST_BL       ) ||
		 	 ((pc[i] & ARM_INST_LDR2PC_MASK) == ARM_INST_LDR2PC   ) ||
			 ((pc[i] & ARM_INST_MOV2PC_MASK) == ARM_INST_MOV_rx2PC) )
			break;

		if ((pc + i) >= pcBound)
			break;

		if ((pc[i] & ARM_INST_ADD_SP_MASK) == ARM_INST_SUB_SP)
		{
			fs += GetSubstractOffset(pc[i]);
		}
	}
	SHOW_GFS_STAT("3-2");

	/* 4-0: Update return address stored in top of frame		*/
	pcBound = pc;
	lr = (UINT32 *)sp[(fs-4)/4]-1;
	SHOW_GFS_STAT("4-0");

	/* 5-1: Check whether new pc is valid or not				*/
	if ((nTryStmdb <= 1) && !isValidPc(lr))
	{
		/*	STMDB instruction is not for current function		*/
		/*	Check other type of function calls					*/
		pc = savedPc;
		#if	(FIND_METHOD == 0)
		while (pc > pcBound)
		{
			/*	Check function entry start with STR lr,[sp,#]	*/
			if ((pc[0] & ARM_INST_STR_SP_MASK) == ARM_INST_STR_LR2SP)
				break;
			pc--;
		}
		#endif /* (FIND_METHOD == 0) */

		fs = 0;
		if ((pc > pcBound) && ((pc[1] & ARM_INST_ADD_SP_MASK) == ARM_INST_SUB_SP))
		{
			fs += GetSubstractOffset(pc[1]);
		}

		lr = (UINT32 *)sp[fs/4];
		if ((pc > pcBound) && isValidPc(lr))
		{
			/*	Function entry start with STR lr,[sp,#]			*/
			if (ra) *ra = lr;
			SHOW_GFS_STAT("5-0");
			return(fs+4);
		}

		/* Function is small enough not to save link register	*/
		SHOW_GFS_STAT("5-1");
		return 0;
	}

	for (i = 0; i < nTryStmdb; i++, fs += 4)
	{
		lr = (UINT32 *)sp[(fs-4)/4] - 1;

		if (!isValidPc(lr))
			continue;

		/* 6: Check function call type								*/
		if		 ((lr[ 0] & ARM_INST_BL_MASK) == ARM_INST_BL       )
		{
			/* Function call via BL instruction */
			break;
		}
		else if (((lr[-1] & ARM_INST_PC2LR_MASK ) == ARM_INST_MOV_PC2LR) &&
				 ((lr[ 0] & ARM_INST_LDR2PC_MASK) == ARM_INST_LDR2PC   ) )
		{
			/* Function call via MOV lr,pc & LDR pc,[rx,$x] pair	*/
			break;
		}
		else if (((lr[-1] & ARM_INST_PC2LR_MASK ) == ARM_INST_MOV_PC2LR) &&
				 ((lr[ 0] & ARM_INST_MOV2PC_MASK) == ARM_INST_MOV_rx2PC) )
		{
			/* Function call via MOV lr,pc & MOV pc,rx		pair	*/
			break;
		}
		else
		{
			// Print("0x%08x: 0x%08x/0x%08x\n", pc, pc[-1], pc[0]);
		}
	}

	if (i >= nTryStmdb)
	{
		fs = -1;
	}

	SHOW_GFS_STAT("6-1");

	if (fs > 0)
	{
		/* 6-2: Update frame size for PC just above link reg	*/
	    if (bSTMInst && (pc[0] & ARM_INST_REG_PC_MASK))
		{
			fs += 4;
			SHOW_GFS_STAT("6-2");
		}

		for (i = -1; i >= -4; i--)
		{
			/* 6-3: Update frame size for inst "SUB   sp,sp,#n"		*/
			if ((pc[i] & ARM_INST_ADD_SP_MASK) == ARM_INST_SUB_SP)
			{
				fs += GetSubstractOffset(pc[i]);
				SHOW_GFS_STAT("6-3");
			}

			/* 6-4: Get size of frame to save r0-r3 via STMDB inst	*/
			else if ((pc[i] & ARM_INST_STM_SP_MASK) == ARM_INST_STMDB_SP)
			{
				for (mask = 0x8000; mask > 0; mask >>= 1)
					if (pc[i] & mask) fs += 4;
				SHOW_GFS_STAT("6-4");
			}

			/* 6-5: Update frame size for inst "LDR   rn,[sp,#4]!"		*/
			else if ((pc[i] & ARM_INST_STR_SP_MASK2) == ARM_INST_STR_Rx2SP)
			{
				fs += 4;
				SHOW_GFS_STAT("6-5");
			}
			else
			{
				break;
			}
		}
	}

	/* 7: Store searched return address							*/
	if (ra) *ra = lr;

	return(fs);
}

#define DSM_WINDOW_SIZE		(8)
#define	IS_VALID_PC(x, v)	( ((v) & EXC_INFO_PC) &&   !((UINT32)(x) & 0x3) && \
							  ((UINT32)(x)>=(UINT32)_START) &&                    \
		                      ((UINT32)(x)<=(UINT32)_etext  )  )


typedef struct _exc_msg_tbl
{
	int		id;
	char	*msg;
} EXC_MSG_TBL;

static EXC_MSG_TBL excmsgTbl [] =
{
	{EXC_OFF_RESET,		"RESET CONDITION"				},
	{EXC_OFF_UNDEF,		"UNDEFINED INSTRUCTION"			},
	{EXC_OFF_SWI,		"SOFTWARE INSTRUCTION"			},
	{EXC_OFF_PREFETCH,	"PREFETCH ABORT"				},
	{EXC_OFF_DATA,		"DATA ABORT"					},
	{EXC_OFF_IRQ,		"INTERRUPT"						},
	{EXC_OFF_FIQ,		"FAST INTERRUPT"				},
	{0xFF,				"Trap to uninitialized vector"	}
};

static EXC_MSG_TBL	sFltMsgTbl [] =
{
    { 0x01,				"Alignment"						},
    { 0x00,				"TLB miss"						},
    { 0x0c,				"External abort on L1"			},
    { 0x0e,				"External abort on L2"			},
    { 0x05,				"Translation Section"			},
    { 0x07,				"Translation Page"				},
    { 0x09,				"Domain Section"				},
    { 0x0b,				"Domain Page"					},
    { 0x0d,				"Permission Section"			},
    { 0x0f,				"Permission Page"				},
    { 0x0a,				"External Abort"				},
    { 0x02,				"Debug Event"					},
    { -1,				"Unknown fault status"			}
};

static char		_gExcMsg[2048];
static char		*_pExcMsg = NULL;
static UINT32	except_pc = 0;
static UINT32	rsvd_pc   = 0;

extern int dsmInst(
    FAST INSTR 		*binInst,	/* Pointer to the instruction */
    int				address,	/* Address prepended to instruction */
    pVOIDFUNCPTR	prtAddress	/* Address printing function */
    );

static void printExceptDsmMsg(UINT32 pc, UINT32 rsvd_pc, int valid)
{
	UINT32 *dsmPc = NULL;
	int i;

	if (IS_VALID_PC(pc, valid))
	{
		extern int regPrintDsmMode(UINT32 print_func);
		dsmPc = (UINT32 *)pc - DSM_WINDOW_SIZE;
HasValidLR:
		regPrintDsmMode((UINT32)pollPrint);
		pollPrint("\n");
		for (i = 0; i < 2*DSM_WINDOW_SIZE+1; i++, dsmPc++)
		{
			if (dsmPc == (UINT32 *)pc) pollPrint("     ** ");
			else                    pollPrint("        ");
			dsmInst((void *)dsmPc, (UINT32)dsmPc, NULL);
		}
		pollPrint("\n");
	}
	else if (IS_VALID_PC(rsvd_pc, EXC_INFO_PC))
	{
		dsmPc = (UINT32 *)rsvd_pc - DSM_WINDOW_SIZE;
		goto HasValidLR;
	}
}

static void printRegsMap(REG_SET *pRegs)
{
	sprintf( _pExcMsg,   "r0/a1  %08x   r1/a2  %08x    r2/a3 %08x    r3/a4 %08x\n",
						pRegs->r[ 0], pRegs->r[ 1], pRegs->r[ 2], pRegs->r[ 3]);
	sprintf( _pExcMsg, "%sr4/v1  %08x   r5/v2  %08x    r6/v3 %08x    r7/v4 %08x\n", _pExcMsg,
						pRegs->r[ 4], pRegs->r[ 5], pRegs->r[ 6], pRegs->r[ 7]);
	sprintf( _pExcMsg, "%sr8/v5  %08x   r9/v6  %08x   r10/v7 %08x   r11/v8 %08x\n", _pExcMsg,
						pRegs->r[ 8], pRegs->r[ 9], pRegs->r[10], pRegs->r[11]);
	sprintf( _pExcMsg, "%sip/r12 %08x   sp/r13 %08x   lr/r14 %08x   pc/r15 %08x\n\n", _pExcMsg,
						pRegs->r[12], pRegs->r[13], pRegs->r[14], pRegs->r[15]);

	pollPrint(_pExcMsg);
}

int exc_Mode = 0;
int gfxPrintMode = 0;
int dbgAtomExcBaseHook(int vect, void *sp, REG_SET *pRegs, EXC_INFO *pExcInfo)
{
	extern osv_t  *findMyOsvEntry(int myId);
	extern UINT16 mmuCP15R5GetIFS(void);
	extern UINT16 mmuCP15R5GetDFS(void);
	extern UINT32 mmuCP15R1Get(void);
	extern UINT32 mmuCP15R6GetDFA(void);
	FAST int	valid = pExcInfo->valid;
	FAST int	i = 7, n;
	UINT08		*taskName = NULL;
	UINT16		iFaultStatus = mmuCP15R5GetIFS();
	UINT16		dFaultStatus = mmuCP15R5GetDFS();
	char		*excMsg      = excmsgTbl[ 7].msg;
	char		*iFaultMsg   = sFltMsgTbl[12].msg;
	char		*dFaultMsg   = sFltMsgTbl[12].msg;
	osv_t		*pOsv        = NULL;
	ATOM_TCB	*tDesc       = NULL;
	int			taskId;
	//extern char buildPath[];

	/*
	 *	다른 디버그 메시지가 완전히 출력되도록 Flush 해준다.
	 */
	n = serial_get_buf_status();
	serial_flush_buffer();
	pollPrint("---------- %d bytes in io buffer has been flushed\n", n);

    if (valid & EXC_INFO_VECADDR)
	{
		for (i = 0; excmsgTbl[i].id != vect; i++)
	    {
	    	if (excmsgTbl[i].id < 0)
	        	break;
	    }
	}
	excMsg = excmsgTbl[i].msg;
	for (i = 0; sFltMsgTbl[i].id != (iFaultStatus & 0xf); i++)
    {
    	if (sFltMsgTbl[i].id < 0)
        	break;
    }
	iFaultMsg = sFltMsgTbl[i].msg;

	for (i = 0; sFltMsgTbl[i].id != (dFaultStatus & 0xf); i++)
    {
    	if (sFltMsgTbl[i].id < 0)
        	break;
    }
	dFaultMsg = sFltMsgTbl[i].msg;

	taskId = getTaskId(NULL);
	pOsv = findMyOsvEntry(taskId);

	if (pOsv != NULL) tDesc = pOsv->u.t.ov_tcb;
	else              tDesc = NULL;

	sprintf(_gExcMsg,"\n$$\n##\n$$\n");

	if(tDesc != (ATOM_TCB*)0)
		taskName = (UINT08*)&pOsv->ov_name[0];//tDesc->NamePtr;
	else
		taskName = (UINT08*)"None";

	sprintf( _gExcMsg, "\n>> %s[%s:%d] at 0x%06x(Task tcb:0x%x, %-8s), uptime %d ms <<\n",
						_gExcMsg, excMsg, vect/4, pRegs->pc, (UINT32)tDesc,
						taskName, (UINT32)readMsTicks());

    if (valid & EXC_INFO_PC)
		sprintf(_gExcMsg, "%s>> Program counter at Exception       : 0x%08x\n", _gExcMsg, pExcInfo->pc);

	if (valid & EXC_INFO_CPSR)
		sprintf(_gExcMsg, "%s>> Current Processor Status Register  : 0x%08x(0x%08x)\n", _gExcMsg,
						pExcInfo->cpsr, pRegs->cpsr);
	sprintf(_gExcMsg,     "%s>> MMU Control Register               : 0x%06x\n", _gExcMsg, mmuCP15R1Get());
	sprintf(_gExcMsg, "%s>> INST \"%s(0x%02x)\" DATA \"%s(0x%02x)\" \n", _gExcMsg, iFaultMsg, iFaultStatus, dFaultMsg, dFaultStatus);
	sprintf(_gExcMsg, "%s>> at 0x%08x\n>>\n", _gExcMsg, mmuCP15R6GetDFA());
	gfxPrintMode = 1;
	pollPrint(_gExcMsg);

//
//	Exception이 발생한 이후에 debug port를 연결해도 메시지를 보기 위해,
//	메시지를 지우지 않게 한다.
//	_gExcMsg[0] = '\0';

	_pExcMsg = &_gExcMsg[strlen(_gExcMsg)+1];

	printRegsMap(pRegs);

	gfxPrintMode = 0;

	except_pc = (UINT32)pRegs->pc;
	rsvd_pc   = (UINT32)pRegs->r[14];

	printExceptDsmMsg(except_pc, rsvd_pc, valid);

	HEXDUMP("Exception Stack", (void *)pRegs, 0xc0);

	//pollPrint("From Binary : %s\n", buildPath);
	StackTrace(0x2000, 32, NULL, (UINT32 *)pRegs, 0);
	pollPrint("\n----------\n");

	gfxPrintMode = 1;
	exc_Mode = 1;
	while (1)
	{
		extern SINT32 gnDebugLevel;

		SINT32 hook_gnDebugLevel = gnDebugLevel;

		gnDebugLevel = 0;
		cmdDebugMain();
		gnDebugLevel = hook_gnDebugLevel;
	}
	exc_Mode = 0;
	/*
	 *	NEVER REACHE TO HERE
	 */
	return 0;
}

void dbgPrintExcMsg(void)
{
	pollPrint("\n\nReprinting Exception Message\n");
	if(_pExcMsg == NULL)
	{
		rprint1n("Exception is not occurred!");
		return;
	}
	pollPrint(_gExcMsg);
	pollPrint(_pExcMsg);

	printExceptDsmMsg(except_pc, rsvd_pc, EXC_INFO_PC);

	dbgPrintTask1("0");
}
