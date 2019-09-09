				.global		archIRQHandler
				.global		contextEnterCritical
				.global		contextExitCritical
				.global		contextEnableInterrupts
				.global		archContextSwitch
				.global		archFirstThreadRestore


				.extern		__interrupt_dispatcher
				.extern		intContextEnter
				.extern		intContextExit

				/**/
				.equ USR_MODE,	0x10
				.equ FIQ_MODE,	0x11
				.equ IRQ_MODE,	0x12
				.equ SVC_MODE,	0x13
				.equ ABT_MODE,	0x17
				.equ UND_MODE,	0x1B
				.equ SYS_MODE,	0x1F

				.equ I_BIT,		0x80        /* when I bit is set, IRQ is disabled */
				.equ F_BIT,		0x40        /* when F bit is set, FIQ is disabled */
				.equ N_BIT,		0xc0


				.text
				.code 32


/**
 * \b archContextSwitch
 *
 * Architecture-specific context switch routine.
 *
 * Note that interrupts are always locked out when this routine is
 * called. For cooperative switches, the scheduler will have entered
 * a critical region. For preemptions (called from an ISR), the
 * ISR will have disabled interrupts on entry.
 *
 * @param[in] old_tcb_ptr Pointer to the thread being scheduled out
 * @param[in] new_tcb_ptr Pointer to the thread being scheduled in
 *
 * @return None
 *
 * void archContextSwitch (ATOM_TCB *old_tcb_ptr, ATOM_TCB *new_tcb_ptr)
 */
	.type archContextSwitch, %function
archContextSwitch:
    STMFD       sp!, {r4 - r11, lr}             /* Save registers */

    STR         sp, [r0]                        /* Save old SP in old_tcb_ptr->sp_save_ptr (first TCB element) */
    LDR         r1, [r1]                        /* Load new SP from new_tcb_ptr->sp_save_ptr (first TCB element) */
    MOV         sp, r1

    LDMFD       sp!, {r4 - r11, pc}             /* Load new registers */
	.size archContextSwitch, .-archContextSwitch


/**
 * \b archFirstThreadRestore
 *
 * Architecture-specific function to restore and start the first thread.
 * This is called by atomOSStart() when the OS is starting.
 *
 * This function will be largely similar to the latter half of
 * archContextSwitch(). Its job is to restore the context for the
 * first thread, and finally enable interrupts (although we actually
 * enable interrupts in thread_shell() for new threads in this port
 * rather than doing it explicitly here).
 *
 * It expects to see the context saved in the same way as if the
 * thread has been previously scheduled out, and had its context
 * saved. That is, archThreadContextInit() will have been called
 * first (via atomThreadCreate()) to create a "fake" context save
 * area, containing the relevant register-save values for a thread
 * restore.
 *
 * Note that you can create more than one thread before starting
 * the OS - only one thread is restored using this function, so
 * all other threads are actually restored by archContextSwitch().
 * This is another reminder that the initial context set up by
 * archThreadContextInit() must look the same whether restored by
 * archFirstThreadRestore() or archContextSwitch().
 *
 * @param[in] new_tcb_ptr Pointer to the thread being scheduled in
 *
 * @return None
 *
 * void archFirstThreadRestore (ATOM_TCB *new_tcb_ptr)
 */
	.type archFirstThreadRestore, %function
archFirstThreadRestore:
    LDR         r0, [r0]                        /* Get SP (sp_save_ptr is conveniently first element of TCB) */
    MOV         sp, r0                          /* Load new stack pointer */
    LDMFD       sp!, {r4 - r11, pc}             /* Load new registers */
	.size archFirstThreadRestore, .-archFirstThreadRestore


/**
 *  \b contextEnableInterrupts
 *
 *  Enables interrupts on the processor
 *
 *  @return None
 */
	.type contextEnableInterrupts, %function
contextEnableInterrupts:
    MRS         r0, CPSR
    MOV         r1, #I_BIT
    BIC         r0, r0, r1
    MSR         CPSR_c, r0
    BX          lr
	.size contextEnableInterrupts, .-contextEnableInterrupts


/**
 *  \b contextExitCritical
 *
 *  Exit critical section (restores interrupt posture)
 *
 *  @param[in] r0 Interrupt Posture
 *
 *  @return None
 */
	.type contextExitCritical, %function
contextExitCritical:
    MSR         CPSR_c, r0
	MOV			pc, lr
	.size contextExitCritical, .-contextExitCritical


/**
 *  \b contextEnterCritical
 *
 *  Enter critical section (disables interrupts)
 *
 *  @return Current interrupt posture
 */
	.type contextEnterCritical, %function
contextEnterCritical:
    MRS         r0, CPSR
    ORR         r1, r0, #(N_BIT)
    MSR         CPSR_c, r1
	MRS			r1, CPSR
	AND			r1, r1, #(N_BIT)
	CMP			r1, #(N_BIT)
	BNE			contextEnterCritical
	MOV			pc, lr
	.size contextEnterCritical, .-contextEnterCritical


/**
 *  \b archIRQHandler
 *
 *  IRQ entry point.
 *
 *  Save the process/thread context onto its own stack before calling __interrupt_dispatcher().
 *  __interrupt_dispatcher() might switch stacks. On return the same context is popped from the
 *  stack and control is returned to the process.
 *
 *  @return None
 */
	.type archIRQHandler, %function
archIRQHandler:

    MSR         cpsr_c, #(SVC_MODE | N_BIT)     /* Save current process context in process stack */
    STMFD       sp!, {r0 - r3, ip, lr}

    MSR         cpsr_c, #(IRQ_MODE | N_BIT)     /* Save lr_irq and spsr_irq in process stack */
    SUB         lr, lr, #4
    MOV         r1, lr
    MRS         r2, spsr
    MSR         cpsr_c, #(SVC_MODE | N_BIT)
    STMFD       sp!, {r1, r2}

;	LDR			r0, L$INT_CONTEXT_ENTER
;	MOV			lr, pc
;	MOV			pc, r0

    LDR         r0, L$INTERRUPT_DISPATCH
	MOV			lr, pc
	MOV			pc, r0							/* Dispatch the interrupt to platform folder for
                                                   the timer tick interrupt or a simular function
                                                   for other interrupts. Some of those IRQs may
                                                   call Atomthreads kernel routines and cause a
                                                   thread switch. */

;	LDR			r0, L$INT_CONTEXT_EXIT
;	MOV			lr, pc
;	MOV			pc, r0

    LDMFD       sp!, {r1, r2}                   /* Restore lr_irq and spsr_irq from process stack */
    MSR         cpsr_c, #(IRQ_MODE | N_BIT)
    STMFD       sp!, {r1}
    MSR         spsr_cxsf, r2

    MSR         cpsr_c, #(SVC_MODE | N_BIT)     /* Restore process regs */
    LDMFD       sp!, {r0 - r3, ip, lr}

    MSR         cpsr_c, #(IRQ_MODE | N_BIT)     /* Exit from IRQ */
    LDMFD       sp!, {pc}^
	.size archIRQHandler, .-archIRQHandler

L$INTERRUPT_DISPATCH:
		.long	__interrupt_dispatcher

L$INT_CONTEXT_ENTER:
		.long	intContextEnter

L$INT_CONTEXT_EXIT:
		.long	intContextExit

		.end
