;/********************************************************************************************************
;                                               uC/OS-II
;                                         The Real-Time Kernel
;
;                               (c) Copyright 1992-2004, Micrium, Weston, FL
;                                          All Rights Reserved
;
;                                           Generic ARM Port
;
; File      : OS_CPU_A.ASM
; Version   : V1.60
; By        : Jean J. Labrosse
;
; For       : ARM7 or ARM9
; Mode      : ARM or Thumb
; Toolchain : IAR's EWARM V4.11a and higher
;********************************************************************************************************/

            .extern  OSRunning
            .extern  OSPrioCur
            .extern  OSPrioHighRdy
            .extern  OSTCBCurPtr
            .extern  OSTCBHighRdyPtr
            .extern  OSIntNestingCtr
            .extern  OSIntEnter
            .extern  OSIntExit
            .extern  OSTaskSwHook
            .extern  OS_CPU_IRQ_ISR_Handler
            .extern  OS_CPU_FIQ_ISR_Handler
			.extern  intContextEnter
			.extern  intContextExit


			.text
			.globl  OSStartHighRdy
            .globl  OSCtxSw
            .globl  OSIntCtxSw
            .globl  OS_CPU_IRQ_ISR
            .globl  OS_CPU_FIQ_ISR
            .globl  CPU_SR_Save               /* Functions declared in this file */
            .globl  CPU_SR_Restore
			.globl	CPU_CntLeadZeros


#define	NO_INT      0xC0                         /* Mask used to disable interrupts (Both FIR and IRQ) */
#define SVC32_MODE	0x13
#define FIQ32_MODE	0x11
#define IRQ32_MODE	0x12


;/*********************************************************************************************************
;                                          START MULTITASKING
;                                       void OSStartHighRdy(void)
;
; Note(s) : 1) OSStartHighRdy() MUST:
;              a) Call OSTaskSwHook() then,
;              b) Set OSRunning to TRUE,
;              c) Switch to the highest priority task.
;*********************************************************************************************************/

;/*
;        RSEG CODE:CODE:NOROOT(2)
;        CODE32
;*/
	.type OSStartHighRdy, %function
OSStartHighRdy:

        LDR     r0, L$OS_TaskSwHook     /* OSTaskSwHook() */
        MOV     lr, PC
        MOV     PC, r0

        MSR     CPSR, #(NO_INT | SVC32_MODE)        /* Switch to SVC mode with IRQ and FIQ disabled */

        LDR     r4, L$OS_Running        /* OSRunning = TRUE */
        MOV     r5, #1
        STRB    r5, [r4]

                                        /* SWITCH TO HIGHEST PRIORITY TASK */
        LDR     r4, L$OS_TCBHighRdy     /*    Get highest priority task TCB address */
        LDR     r4, [r4]                /*    get stack pointer */
        LDR     SP, [r4]                /*    switch to the new stack */

        LDR     r4,  [SP], #4           /*    pop new task's CPSR */
        MSR     SPSR, r4
        LDMFD   SP!, {r0-r12,lr,PC}^    /*    pop new task's context */
	.size OSStartHighRdy, .-OSStartHighRdy

;/**********************************************************************************************************
;                         PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
;
; Note(s) : 1) OSCtxSw() is called in SYS mode with BOTH FIQ and IRQ interrupts DISABLED
;
;           2) The pseudo-code for OSCtxSw() is:
;              a) Save the current task's context onto the current task's stack
;              b) OSTCBCurPtr->StkPtr   = SP;
;              c) OSTaskSwHook();
;              d) OSPrioCur             = OSPrioHighRdy;
;              e) OSTCBCurPtr           = OSTCBHighRdyPtr;
;              f) SP                    = OSTCBHighRdyPtr->StkPtr;
;              g) Restore the new task's context from the new task's stack
;              h) Return to new task's code
;
;           3) Upon entry:
;              OSTCBCurPtr      points to the OS_TCB of the task to suspend
;              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
;*********************************************************************************************************/
;/*
;        RSEG CODE:CODE:NOROOT(2)
;        CODE32
;*/
	.type OSCtxSw, %function
OSCtxSw:
                                        /* SAVE CURRENT TASK'S CONTEXT */
        STMFD   SP!, {lr}               /*     Push return address */
        STMFD   SP!, {lr}
        STMFD   SP!, {r0-r12}           /*     Push registers */
        MRS     r4,  CPSR               /*     Push current CPSR */
        TST     lr, #1                  /*     See if called from Thumb mode */
        ORRNE   r4,  r4, #0x20          /*     If yes, Set the T-bit */
        STMFD   SP!, {r4}

        LDR     r4, L$OS_TCBCur         /* OSTCBCurPtr->StkPtr = SP; */
        LDR     r5, [r4]
        STR     SP, [r5]

        LDR     r0, L$OS_TaskSwHook     /* OSTaskSwHook(); */
        MOV     lr, PC
        MOV     PC, r0

        LDR     r4, L$OS_PrioCur        /* OSPrioCur = OSPrioHighRdy */
        LDR     r5, L$OS_PrioHighRdy
        LDRB    r6, [r5]
        STRB    r6, [r4]

        LDR     r4, L$OS_TCBCur         /* OSTCBCurPtr  = OSTCBHighRdyPtr; */
        LDR     r6, L$OS_TCBHighRdy
        LDR     r6, [r6]
        STR     r6, [r4]

        LDR     SP, [r6]                /* SP = OSTCBHighRdy->StkPtr; */

                                        /* RESTORE NEW TASK'S CONTEXT */
        LDMFD   SP!, {r4}               /*    Pop new task's CPSR */
        MSR     SPSR, r4

        LDMFD   SP!, {r0-r12,lr,PC}^    /*    Pop new task's context */
	.size OSCtxSw, .-OSCtxSw

;/**********************************************************************************************************
;                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
;
; Note(s) : 1) OSIntCtxSw() is called in SYS mode with BOTH FIQ and IRQ interrupts DISABLED
;
;           2) The pseudo-code for OSCtxSw() is:
;              a) OSTaskSwHook();
;              b) OSPrioCur             = OSPrioHighRdy;
;              c) OSTCBCurPtr           = OSTCBHighRdyPtr;
;              d) SP                    = OSTCBHighRdyPtr->StkPtr;
;              e) Restore the new task's context from the new task's stack
;              f) Return to new task's code
;
;           3) Upon entry:
;              OSTCBCurPtr      points to the OS_TCB of the task to suspend
;              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume
;*********************************************************************************************************/
;/*
;        RSEG CODE:CODE:NOROOT(2)
;        CODE32
;*/
	.type OSIntCtxSw, %function
OSIntCtxSw:
        LDR     r0, L$OS_TaskSwHook     /* OSTaskSwHook() */
        MOV     lr, PC
        MOV     PC, r0

        LDR     r4,L$OS_PrioCur         /* OSPrioCur = OSPrioHighRdy */
        LDR     r5,L$OS_PrioHighRdy
        LDRB    r6,[r5]
        STRB    r6,[r4]

        LDR     r4,L$OS_TCBCur          /* OSTCBCurPtr  = OSTCBHighRdyPtr; */
        LDR     r6,L$OS_TCBHighRdy
        LDR     r6,[r6]
        STR     r6,[r4]

        LDR     SP,[r6]                 /* SP = OSTCBHighRdyPtr->StkPtr; */

                                        /* RESTORE NEW TASK'S CONTEXT */
        LDMFD   SP!, {r4}               /*    Pop new task's CPSR */
        MSR     SPSR, r4

        LDMFD   SP!, {r0-r12,lr,PC}^    /*    Pop new task's context */
	.size OSIntCtxSw, .-OSIntCtxSw

/*

;/********************************************************************************************************
;                                      IRQ Interrupt Service Routine
;*********************************************************************************************************/

;/*
;        RSEG CODE:CODE:NOROOT(2)
;        CODE32
;*/
	.type OS_CPU_IRQ_ISR, %function
OS_CPU_IRQ_ISR:
											   /* FIQ interrupt disable */
		MSR     CPSR_c, #(NO_INT | IRQ32_MODE) /* Change to IRQ mode (to use the IRQ stack to handle interrupt) */
        STMFD   SP!, {r1-r3}                   /* PUSH WORKING REGISTERS ONTO IRQ STACK */
        MOV     r1, SP                         /* Save   IRQ stack pointer */
        ADD     SP, SP,#12                     /* Adjust IRQ stack pointer */
        SUB     r2, lr,#4                      /* Adjust PC for return address to task */
        MRS     r3, SPSR                       /* Copy SPSR (i.e. interrupted task's CPSR) to r3 */
        MSR     CPSR_c, #(NO_INT | SVC32_MODE) /* Change to SVC mode */

                                               /* SAVE TASK'S CONTEXT ONTO TASK'S STACK */
        STMFD   SP!, {r2}                      /*    Push task's Return PC */
        STMFD   SP!, {lr}                      /*    Push task's lr */
        STMFD   SP!, {r4-r12}                  /*    Push task's r12-r4 */

        LDMFD   r1!, {r4-r6}                   /*    Move task's r1-r3 from IRQ stack to SVC stack */
        STMFD   SP!, {r4-r6}
        STMFD   SP!, {r0}                      /*    Push task's r0    onto task's stack */
        STMFD   SP!, {r3}                      /*    Push task's CPSR (i.e. IRQ's SPSR) */

		LDR		r0,	L$INT_CONTEXT_ENTER
		MOV		lr, PC
		MOV		PC,	r0
                                               /* HANDLE NESTING COUNTER */
        LDR     r0, L$_OS_IntEnter				/* OSIntEnter() */
		MOV     lr, PC
        MOV		PC, r0

        LDR     r0, L$OS_IntNesting            /* OSIntNestingCtr++; */
        LDRB    r1, [r0]

        CMP     r1, #1                         /* if (OSIntNestingCtr == 1) { */
        BNE     OS_CPU_IRQ_ISR_1

        LDR     r4, L$OS_TCBCur                /*     OSTCBCurPtr->StkPtr = SP */
        LDR     r5, [r4]
        STR     SP, [r5]                       /* } */

OS_CPU_IRQ_ISR_1:
        MSR     CPSR_c, #(NO_INT | IRQ32_MODE) /* Change to IRQ mode (to use the IRQ stack to handle interrupt) */

        LDR     r0, L$OS_CPU_IRQ_ISR_Handler   /* OS_CPU_IRQ_ISR_Handler(); */
        MOV     lr, PC
        MOV		PC, r0

        MSR     CPSR_c, #(NO_INT | SVC32_MODE) /* Change to SVC mode */

		LDR		r0,	L$INT_CONTEXT_EXIT
		MOV		lr, PC
		MOV		PC,	r0

        LDR     r0, L$_OS_IntExit				/* OSIntExit() */
		MOV     lr, PC
        MOV		PC, r0

                                               /* RESTORE NEW TASK'S CONTEXT */
        LDMFD   SP!, {r4}                      /*    Pop new task's CPSR */
        MSR     SPSR, r4

        LDMFD   SP!, {r0-r12,lr,PC}^           /*    Pop new task's context */
	.size OS_CPU_IRQ_ISR, .-OS_CPU_IRQ_ISR

;/**********************************************************************************************************
;                                      FIQ Interrupt Service Routine
;*********************************************************************************************************/

;/*
;        RSEG CODE:CODE:NOROOT(2)
;        CODE32
;*/
	.type OS_CPU_FIQ_ISR, %function
OS_CPU_FIQ_ISR:
											   /* FIQ interrupt disable */
		MSR     CPSR_c, #(NO_INT | FIQ32_MODE) /* Change to FIQ mode (to use the FIQ stack to handle interrupt) */
        STMFD   SP!, {r1-r3}                   /* PUSH WORKING REGISTERS ONTO FIQ STACK */
        MOV     r1, SP                         /* Save   FIQ stack pointer */
        ADD     SP, SP,#12                     /* Adjust FIQ stack pointer */
        SUB     r2, lr,#4                      /* Adjust PC for return address to task */
        MRS     r3, SPSR                       /* Copy SPSR (i.e. interrupted task's CPSR) to r3 */
        MSR     CPSR_c, #(NO_INT | SVC32_MODE) /* Change to SVC mode */

                                               /* SAVE TASK'S CONTEXT ONTO TASK'S STACK */
        STMFD   SP!, {r2}                      /*    Push task's Return PC */
        STMFD   SP!, {lr}                      /*    Push task's lr */
        STMFD   SP!, {r4-r12}                  /*    Push task's r12-r4 */

        LDMFD   r1!, {r4-r6}                   /*    Move task's r1-r3 from FIQ stack to SVC stack */
        STMFD   SP!, {r4-r6}
        STMFD   SP!, {r0}                      /*    Push task's r0    onto task's stack */
        STMFD   SP!, {r3}                      /*    Push task's CPSR (i.e. FIQ's SPSR) */

		LDR		r0, L$INT_CONTEXT_ENTER
		MOV		lr, PC
		MOV		PC, r0

        LDR     r0, L$_OS_IntEnter				/* OSIntEnter() */
		MOV     lr, PC
        MOV		PC, r0
                                               /* HANDLE NESTING COUNTER */
        LDR     r0, L$OS_IntNesting            /* OSIntNestingCtr++; */
        LDRB    r1, [r0]

        CMP     r1, #1                         /* if (OSIntNestingCtr == 1) { */
        BNE     OS_CPU_FIQ_ISR_1

        LDR     r4, L$OS_TCBCur                /* OSTCBCurPtr->StkPtr = SP */
        LDR     r5, [r4]
        STR     SP, [r5]                       /* } */

OS_CPU_FIQ_ISR_1:
        MSR     CPSR_c, #(NO_INT | FIQ32_MODE) /* Change to FIQ mode (to use the FIQ stack to handle interrupt) */

        LDR     r0, L$OS_CPU_FIQ_ISR_Handler   /* OS_CPU_FIQ_ISR_Handler(); */
        MOV     lr, PC
        MOV		PC, r0

        MSR     CPSR_c, #(NO_INT | SVC32_MODE) /* Change to SVC mode */

		LDR		r0,	L$INT_CONTEXT_EXIT
		MOV		lr, PC
		MOV		PC,	r0

        LDR     r0, L$_OS_IntExit               /* OSIntExit(); */
        MOV     lr, PC
        MOV		PC, r0

                                               /* RESTORE NEW TASK'S CONTEXT */
        LDMFD   SP!, {r4}                      /*    Pop new task's CPSR */
        MSR     SPSR, r4

        LDMFD   SP!, {r0-r12,lr,PC}^           /*    Pop new task's context */
	.size OS_CPU_FIQ_ISR, .-OS_CPU_FIQ_ISR

;/**********************************************************************************************************
;                                   CRITICAL SECTION METHOD 3 FUNCTIONS
;
; Description: Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
;              would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
;              disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to
;              disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
;              into the CPU's status register.
;
; Prototypes :     OS_CPU_SR  CPU_SR_Save(void);
;                  void       CPU_SR_Restore(OS_CPU_SR cpu_sr);
;
;
; Note(s)    : 1) These functions are used in general like this:
;
;                 void Task (void *p_arg)
;                 {
;                 #if OS_CRITICAL_METHOD == 3          /* Allocate storage for CPU status register /
;                     CPU_SR  cpu_sr;
;                 #endif
;
;                          :
;                          :
;                     OS_CRITICAL_ENTER();             /* cpu_sr = CPU_SaveSR();                /
;                          :
;                          :
;                     OS_CRITICAL_EXIT();              /* CPU_RestoreSR(cpu_sr);                /
;                          :
;                          :
;                 }
;
;              2) CPU_SaveSR() is implemented as recommended by Atmel's application note:
;
;                    "Disabling Interrupts at Processor Level"
;*********************************************************************************************************/

;/*      RSEG CODE:CODE:NOROOT(2)
;        CODE32
;*/
	.type CPU_SR_Save, %function
CPU_SR_Save:
        MRS     r0,CPSR                     /* Set IRQ and FIQ bits in CPSR to disable all interrupts */
        ORR     r1,r0,#NO_INT
        MSR     CPSR_c,r1
        MRS     r1,CPSR                     /* Confirm that CPSR contains the proper interrupt disable flags */
        AND     r1,r1,#NO_INT
        CMP     r1,#NO_INT
        BNE     CPU_SR_Save              /* Not properly disabled (try again)*/
        MOV		PC,      lr                          /* Disabled, return the original CPSR contents in r0 */
	.size CPU_SR_Save, .-CPU_SR_Save

	.type CPU_SR_Restore, %function
CPU_SR_Restore:
        MSR     CPSR_c,r0
        MOV     PC, lr
	.size CPU_SR_Restore, .-CPU_SR_Restore

#if (HAS_ASSEM_CLZ == 1)
	.type CPU_CntLeadZeros, %function
CPU_CntLeadZeros:
		CLZ		r0, r0
		MOV		PC, lr
	.size CPU_CntLeadZeros, .-CPU_CntLeadZeros
#endif

;/**********************************************************************************************************
;                                     POINTERS TO VARIABLES
;*********************************************************************************************************/

L$OS_TaskSwHook:
        .long    OSTaskSwHook

L$OS_CPU_IRQ_ISR_Handler:
        .long    OS_CPU_IRQ_ISR_Handler

L$OS_CPU_FIQ_ISR_Handler:
        .long    OS_CPU_FIQ_ISR_Handler

L$_OS_IntEnter:
        .long    OSIntEnter

L$_OS_IntExit:
        .long    OSIntExit

L$OS_IntNesting:
        .long   OSIntNestingCtr

L$OS_PrioCur:
        .long    OSPrioCur

L$OS_PrioHighRdy:
        .long    OSPrioHighRdy

L$OS_Running:
        .long    OSRunning

L$OS_TCBCur:
        .long    OSTCBCurPtr

L$OS_TCBHighRdy:
        .long    OSTCBHighRdyPtr

L$INT_CONTEXT_ENTER:
		.long	intContextEnter

L$INT_CONTEXT_EXIT:
		.long	intContextExit

        .end
