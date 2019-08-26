#include <bsp_arch.h>
	.global	_start_code
	.global	system_init
	.global	EnableInterrupts

	.extern	main
	.extern initialStk
	.extern sym_check

#define SYM_MAGIC	0xB12791EE

	.type _start_code, %function
_start_code:
    b   system_init


system_init:

	msr		cpsr_c, #0x40|MODE_SVC32

clear_bss:
	ldr		r0, L$__bss_start__		/* find start of bss segment        */
	ldr		r1, L$__bss_end__		/* stop here                        */
	sub		r1, r1, r0				/* get size of bss					*/
	mov		r2, #0x00000000			/* r2 is zero						*/
	mov		r3, r2					/* r3 is zero						*/
	mov		r4, r2					/* r4 is zero						*/
	mov		r5, r2					/* r5 is zero						*/
	mov		r6, r2					/* r6 is zero						*/
	stmia	r0, {r2-r6}				/* save r2-r6						*/
	ldmia	r0, {r7-r11}			/* read r7-r11						*/
									/* bss is always greater than 40	*/

	subs	r1, r1, #40				/* Decrement counter				*/
.clbss_l40:
	stmia	r0!, {r2-r11}			/* Clear 40 bytes loop				*/
	subs	r1, r1, #40				/* Decrement counter				*/
	bge		.clbss_l40

	mov		r4, r0
	add		r1, r1, #40-4
	cmp		r1, #0
.clbss_l:
	strge	r3, [r0], #4			/* clear 4 bytes loop                */
	subs	r1, r1, #4
	bge		.clbss_l

	/*
	 * Disable Interrupts
	 */
	mrs		r0, cpsr
	bic		r0, r0, #I_BIT | F_BIT
	msr		cpsr_c, r0

	/*
	 *	Set C level Initial Stack Pointer
	 */
	ldr		sp, L$initialStk
	add		sp, sp, #(0x600*4)
	add		sp, sp, #-4

	bl		sym_check

    bl      main
    b       _start_code

	.size _start_code, .-_start_code


	.type EnableInterrupts, %function
EnableInterrupts:
	mrs		r0, cpsr
	bic		r0, r0, #I_BIT | F_BIT
	msr		cpsr_c, r0
	mov		pc, lr
	.size EnableInterrupts, .-EnableInterrupts

L$__bss_start__:
	.long	__bss_start__

L$__bss_end__:
	.long	__bss_end__

L$initialStk:
	.long	initialStk

L$_start_code:
	.long	_start_code

L$_symMagic:
	.long	SYM_MAGIC
