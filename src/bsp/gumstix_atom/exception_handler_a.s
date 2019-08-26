#include "exception.h"

		/* globals */
		.globl	excEnterReset
		.globl	excEnterSwi
		.globl	excEnterUndef
		.globl	excEnterPrefetchAbort
		.globl	excEnterDataAbort
		.globl	armInitExceptionModes
		.globl	gotoResetVector
		/* externs */
		.extern	excExcContinue
		.text
		.code 32

/*******************************************************************************
*
* excEnterReset -  enter an Reset handler
*
* This routine is initialized on the Undefined Instruction vector
* by excVecInit.
*
* This routine can NEVER be called from C.
*
* SEE ALSO: excVecInit
*
* void excEnterReset()
*
* INTERNAL
* MODE = RESET CONDITION
* IRQs disabled
* sp -> register save area (see excEnterCommon)
*/
		.type excEnterReset, %function
excEnterReset:
		sub lr, lr, #4				/* adjust return address so it points to instruction that faulted */
		stmib sp,{r0-r3,lr}			/* save regs in save area */
		mov r0,#EXC_OFF_RESET		/* set r0 -> exception vector and join main thread */
		b L$excEnterCommon
		.size excEnterReset, .-excEnterReset

/**************************************************************************
*
* excEnterSwi - enter a Software Interrupt exception
*
* This routine is installed on the Software Interrupt vector by excVecInit.
*
* This routine can NEVER be called from C.
*
* SEE ALSO: excVecInit

* void excEnterSwi ()

* INTERNAL
* MODE = SVC32
* IRQs disabled
*/
		.type excEnterSwi, %function
excEnterSwi:
	msr		cpsr, #(I_BIT|F_BIT|MODE_SVC32)
	stmfd	sp!,{r2}			/* save r2 - there must be SVC stack */
	sub		lr,lr,#4			/* make lr -> faulting instruction */

/*
 * save some registers in the same order as other exception handlers
 * (see excEnterCommon)
 */

	ldr		r2,L$_swiSaveArea	/* r2 -> save area */
	stmib	r2,{r0-r3,lr}		/* incorrect r2 stored */

/*
 * r2 -> register save area containing <dummy>,r0,r1,<dummy>,r3,lr
 * original r2 on stack
 *
 * now save real r2 value
 */

	ldmfd	sp!,{r1}			/* get original r2 */
	str		r1,[r2,#4*3]		/* put in save area */


/*
 * IMB handler - this is now assembled for all Architecture 4 CPUs
 * lr-> faulting instruction
 */

	ldr	r0,[lr]					/* get instruction */

/* r0 = instruction, known to be a SWI */

	bic		r0,r0,#0xF0000001	/* remove condition and bit 0 */
	teq	r0,#0x0FF00000		/* IMB? */
	ldmeqib	r2,{r0-r3,lr} 		/* if yes, return to next.. */
	addeqs	pc,lr,#4			/* ..instruction */


	mrs		r3, spsr			/* r3 = svc_spsr */
	str		r3,[r2]				/* save spsr in save area */
	mov		r0,#EXC_OFF_SWI		/* r0-> exception vector */

/*
 * now join main thread with
 * r0-> exception vector
 * r1 = [scratch]
 * r2-> save area (containing spsr,r0-r3,lr)
 * r3 = original CPSR of exception mode
 * MODE=SVC32
 */

	b		L$excEnterCommon2
		.size excEnterSwi, .-excEnterSwi

/*******************************************************************************
*
* excEnterUndef -  enter an undefined instruction exception
*
* This routine is initialized on the Undefined Instruction vector
* by excVecInit.
*
* This routine can NEVER be called from C.
*
* SEE ALSO: excVecInit
*
* void excEnterUndef()
*
* INTERNAL
* MODE = UNDEF32
* IRQs disabled
* sp -> register save area (see excEnterCommon)
*/

		.type excEnterUndef, %function
excEnterUndef:
		msr	cpsr, #(I_BIT|F_BIT|MODE_UNDEF32)
		sub lr, lr, #4				/* adjust return address so it points to instruction that faulted */
		stmib sp,{r0-r3,lr}			/* save regs in save area */
		mov r0,#EXC_OFF_UNDEF		/* set r0 -> exception vector and join main thread */
		b L$excEnterCommon
		.size excEnterUndef, .-excEnterUndef

/*******************************************************************************
*
* excEnterPrefetchAbort -  enter an prefetch abort exception
*
* This routine is initialized on the Prefetch Abort exception vector
* by excVecInit.
*
* This routine can NEVER be called from C.
*
* SEE ALSO: excVecInit
*
* void excEnterPrefetchAbort()
*
* INTERNAL
* MODE = ABORT32
* IRQs disabled
* sp -> register save area (see excEnterCommon)
*/

		.type excEnterPrefetchAbort, %function
excEnterPrefetchAbort:
		msr	cpsr, #(I_BIT|F_BIT|MODE_ABORT32)
		sub lr, lr, #4				/* adjust return address so it points to instruction that faulted */
		stmib sp,{r0-r3,lr}			/* save regs in save area */
		mov r0,#EXC_OFF_PREFETCH	/* set r0 -> exception vector and join main thread */
		b L$excEnterCommon
		.size excEnterPrefetchAbort, .-excEnterPrefetchAbort

/*******************************************************************************
*
* excEnterDataAbort -  enter an data abort exception
*
* This routine is initialized on the Data Abort exception vector
* by excVecInit.
*
* This routine can NEVER be called from C.
*
* SEE ALSO: excVecInit
*
* void excEnterDataAbort()
*
* INTERNAL
* MODE = ABORT32
* IRQs disabled
* sp -> register save area (see excEnterCommon)
*/

		.type excEnterDataAbort, %function
excEnterDataAbort:
		msr	cpsr, #(I_BIT|F_BIT|MODE_ABORT32)
		sub lr, lr, #8				/* adjust return address so it points to instruction that faulted */
		stmib sp,{r0-r3,lr}			/* save regs in save area */
		mov r0,#EXC_OFF_DATA		/* set r0 -> exception vector and join main thread */
		.size excEnterDataAbort, .-excEnterDataAbort

/*******************************************************************************
*
* excEnterCommon - enter an exception handler
*
* Control passes to this routine from the exception vectors, after the address
* in lr has been adjusted to point to the faulting instruction.
*
* This routine can NEVER be called from C.
*

* void excEnterCommon ()

* INTERNAL
*
* The exception modes of the ARM have their own stack pointers. However,
* VxWorks exception handlers expect to be called in the context of the faulting
* task so this veneer switches to SVC mode, saving necessary context in an
* exception stack frame.
* The exception stack pointers are only used to point to a few words
* for register saving. THIS IS NOT A STACK and should not be used as one.
* It is laid out as follows
*
*  -------------------------------
* | SPSR | r0 | r1 | r2 | r3 | lr |
*  -------------------------------
* ^
* | sp points here
*
*
* Entry:
*    r0 -> exception vector
*    lr -> faulting instruction
*
*/

L$excEnterCommon:
		mrs r3, spsr 						/* save SPSR insave area */
		str r3, [sp]

		/*
		 * save sp in non-banked reg so can access saved registers after
		 * switching to SVC mode
		 */
		mov r2, sp

		mrs r3, cpsr						/* switch to SVC mode with interrupts (IRQs) disabled */
		bic r1, r3, #MASK_MODE
		orr r1, r1, #MODE_SVC32 | I_BIT | F_BIT
		msr cpsr, r1
L$excEnterCommon2:

/*
 **** INTERRUPTS DISABLED
 *
 * MODE = SVC32
 * r0 -> exception vector
 * r1 = [scratch]
 * r2 -> where exception mode SPSR,r0-r3,lr are saved
 * r3 = CPSR of exception mode
 * lr = svc_lr at time of exception
 *
 * save the following
 *    exception vector address
 *    svc_sp
 *    sp of exception mode (save area pointer)
 *    CPSR of exception mode
 * NOTE: if anything else gets added to this, the stack
 * addressing later will need adjusting
 */
		mov r1, sp
		stmfd sp!,{r0-r3}
/*
 * put registers of faulting task on stack in order defined
 * in REG_SET so can pass pointer to C handler
 */
		ldr r1, [r2,#4*5]			/* get LR of exception mode */
		ldr r3, [r2]				/* get SPSR of exception mode */
		stmfd sp!,{r1,r3}
		sub sp, sp, #4*(14-4+1)		/* make room for r4..r14 */
/*
 * check for USR mode exception - SYSTEM is handled as other modes
 * r3 = SPSR of exception mode
 */
		tst r3, #MASK_SUBMODE
		stmeqia sp,{r4-r14}^		/* EQ => USER mode */
		beq L$regsSaved
/*
 * not USR mode so must change to mode to get sp,lr
 * SYSTEM mode is also handled this way (but needn't be)
 * r3 = PSR of faulting mode
 */
		mov r1, sp					/* r1 -> where to put regs */
		mrs r0, cpsr				/* save current mode */
		orr r3, r3, #I_BIT
		orr r3, r3, #F_BIT
		bic r3, r3, #T_BIT
		msr cpsr, r3

		/* in faulting mode - interrupts still disabled */
		stmia r1, {r4-r14}			/* save regs */

/*
 * check if it's SVC mode and, if so, overwrite stored sp
 * stack pointed to by r3 contains
 *    r4..r14, PC, PSR of faulting mode
 *    address of exception vector
 *    svc_sp at time of exception
 *    sp of exception mode
 *    CPSR of exception mode
 */

	and	r3,r3,#MASK_SUBMODE					/* examine mode bits */
	teq	r3,#MODE_SVC32 & MASK_SUBMODE	/* SVC? */
	ldreq	r3,[r1,#4*(11+3)]				/* yes, get org svc_sp */
	streq	r3,[r1,#4*(13-4)]				/* and overwrite */

/* switch back to SVC mode with interrupts still disabled (r0) */

	msr	cpsr,r0

/* back in SVC mode - interrupts still disabled */

L$regsSaved:
/* transfer r0-r3 to stack */

	ldmib	r2,{r0-r3}						/* get other regs */
	stmfd	sp!,{r0-r3}

/*
 * exception save area can now be reused
 * stack contains
 *    r0..r14, PC, PSR of faulting mode
 *    address of exception vector
 *    svc_sp at time of exception
 *    sp of exception mode
 *    CPSR of exception mode
 * interrupts still disabled
 *
 * restore interrupt state of faulting code
 */

	ldr	r0,[sp,#4*16]						/* get PSR */
	bic	r0,r0,#MASK_MODE					/* clear mode bits */
	orr	r0,r0,#MODE_SVC32					/* select svc32 */
	msr	cpsr,r0								/* and write it to CPSR */

/*
 **** INTERRUPTS RESTORED to how they were when exception occurred
 *
 * call generic exception handler
 */

	mov	r1,sp								/* r1 -> REG_SET */
	add	r0,r1,#4*15							/* r0 -> ESF (PC, PSR, vector) */
	ldr	fp,[r1,#4*11]
	bl	excExcContinue						/* call C routine to continue */

/* exception handler returned (in SVC32) - disable interrupts (IRQs) again */

	mrs	r0,cpsr
	orr	r0,r0,#I_BIT | F_BIT
	msr	cpsr,r0

/*
 **** INTERRUPTS DISABLED
 *
 * restore regs from stack, putting some into the exception save area
 */

	ldr	r2,[sp,#4*19]						/* r2 -> exception save area */
	ldmfd	sp!,{r3-r6}						/* get r0-r3 */
	stmib	r2,{r3-r6}

/* determine mode in which exception occurred so can restore regs */

	ldr	r3,[sp,#4*(16-4)]					/* get PSR of faulting mode */
	tst	r3,#MASK_SUBMODE
	ldmeqia	sp,{r4-r14}^					/* EQ => USR mode */
	beq	L$regsRestored

/*
 * exception was not in USR mode so switch to mode to restore regs
 * r0 = PSR we can use to return to this mode
 * r3 = PSR of faulting mode
 */

	mov	r1,sp								/* r1 -> from where to load regs */
	orr	r3,r3,#I_BIT | F_BIT
	bic	r3,r3,#T_BIT
	msr	cpsr,r3

/*
 * in faulting mode - interrupts still disabled
 * r1 -> svc stack where r4-r14 are stored
 */

	ldmia	r1,{r4-r14}						/* load regs */

/*
 * If it's SVC mode, reset sp as we've just overwritten it
 * The correct value is in r1
 */

	and	r3,r3,#MASK_SUBMODE					/* examine mode bits */
	teq	r3,#MODE_SVC32 & MASK_SUBMODE	/* SVC? */
	moveq	sp,r1

/* switch back to SVC mode with interrupts still disabled (r0) */

	msr	cpsr,r0

/* back in SVC mode - interrupts still disabled */

L$regsRestored:

/* r4..r14 of faulting mode now restored */

	add	sp,sp,#4*(14-4+1)					/* strip r4..r14 from stack */

	ldmfd	sp!,{r1,r3}						/* get LR and SPSR of exception mode */
	str	r1,[r2,#4*5]						/* save LR in exception save area */
	str	r3,[r2]								/* ..with SPSR */

/* get the remaining stuff off the stack */

	ldmfd	sp!,{r0-r3}

/*
 * r0 = address of exception vector - discarded
 * r1 = svc_sp at time of exception - discarded
 * r2 = sp of exception mode
 * r3 = CPSR of exception mode
 *
 * switch back to exception mode
 */

	msr	cpsr,r3

/*
 * back in exception mode
 * restore remaining registers and return to task that faulted
 */

	ldr	r0,[r2]
	msr	spsr,r0
	ldmib	r2,{r0-r3,pc}^


		.data
		.balign 4

undefSaveArea:	.fill 6,4 /* 6 registers: SPSR, r0-r3, lr */
abortSaveArea:	.fill 6,4 /* 6 registers: SPSR, r0-r3, lr */
swiSaveArea:	.fill 6,4 /* 6 registers: SPSR, r0-r3, lr */

/*
 * ARM IRQ stack
 */
				.fill 1024,4
irqStack:

/*
 * ARM FIQ stack
 */
				.fill 1024,4
fiqStack:

		.text
		.balign 4

L$_undefSaveArea:	.long undefSaveArea
L$_abortSaveArea:	.long abortSaveArea
L$_swiSaveArea:		.long swiSaveArea
L$_irqStack:		.long irqStack
L$_fiqStack:		.long fiqStack
/*******************************************************************************
*
* armInitExceptionModes - initialise ARM exception modes
*
* This routine initialises the registers for the ARM exception modes.
*

* void armInitExceptionModes (void)

* INTERNAL
* The SP used by exception modes does not point to a (full-descending)
* stack but to an area just big enough to hold critical registers before
* the processor is switched to SVC mode for exception processing. The SP
* of the exception mode must be left pointing to the base of this area and
* the values in that area must be copied out so that the next exception can
* use it.
*
* Entered in SVC32 mode.
*/


		.type armInitExceptionModes, %function
armInitExceptionModes:
		mrs r0, cpsr
		bic r1, r0, #MASK_MODE
		orr r1, r1, #(I_BIT | F_BIT)

/*
 * switch to each mode in turn with interrupts disabled and set SP
 * r0 = original CPSR
 * r1 = CPSR with IRQ/FIQ disabled and mode bits clear
 */

		orr r2, r1, #MODE_UNDEF32
		msr cpsr, r2
		ldr sp, L$_undefSaveArea

		orr r2, r1, #MODE_ABORT32
		msr cpsr, r2
		ldr sp, L$_abortSaveArea

		orr r2, r1, #MODE_IRQ32
		msr cpsr, r2
		ldr sp, L$_irqStack

		orr r2, r1, #MODE_FIQ32
		msr cpsr, r2
		ldr sp, L$_fiqStack
/*
 * back to SVC mode
 */
	   msr cpsr, r0

/*
 **** INTERRUPTS RESTORED
 *
 * zero user_sp - it should never be used
 */
	   mov   r1, #0
	   stmfd sp!, {r1}
	   ldmfd sp, {sp}^
	   nop
	   add   sp, sp, #4
	   mov   pc, lr
	   .size armInitExceptionModes, .-armInitExceptionModes

		.type gotoResetVector, %function
gotoResetVector:
		mrs  r1, cpsr
		bic  r1, r1, #MASK_MODE
		orr  r1, r1, #MODE_SVC32
		orr  r1, r1, #I_BIT | F_BIT
		msr  cpsr, r1

		ldr  pc, =0x00 /*FLASH_BASE*/
		.size gotoResetVector, .-gotoResetVector
