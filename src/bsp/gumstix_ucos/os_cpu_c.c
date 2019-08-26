/*
*********************************************************************************************************
*                                               uC/OS-II
*                                         The Real-Time Kernel
*
*
*                             (c) Copyright 1992-2004, Micrium, Weston, FL
*                                          All Rights Reserved
*
*                                           Generic ARM Port
*
* File      : OS_CPU_C.C
* Version   : V1.60
* By        : Jean J. Labrosse
*
* For       : ARM7 or ARM9
* Mode      : ARM or Thumb
* Toolchain : IAR's EWARM V4.11a and higher
*********************************************************************************************************
*/

#define   OS_CPU_GLOBALS

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_cpu_c__c = "$Id: $";
#endif


#include <os.h>


#define	ARM_MODE_ARM			0x00000000
#define	ARM_MODE_THUMB			0x00000020

                                                  /* __CPU_MODE__ is an IAR built-in constant indicating whether ... */
                                                  /* ... code of this file was compiled using ARM or Thumb mode      */
#define	ARM_SVC_MODE_THUMB		(0x00000013L + ARM_MODE_THUMB)
#define	ARM_SVC_MODE_ARM		(0x00000013L + ARM_MODE_ARM)

/*
*********************************************************************************************************
*                                           IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do
*              such things as STOP the CPU to conserve power.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSIdleTaskHook (void)
{
	return;
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
    CPU_STK_SIZE   i;
    CPU_STK       *p_stk;


    p_stk = OSCfg_ISRStkBasePtr;                            /* Clear the ISR stack                                    */

    for (i = 0u; i < OSCfg_ISRStkSize; i++)
	{
        *p_stk++ = (CPU_STK)0u;
    }
    OS_CPU_ExceptStkBase = (CPU_STK *)(OSCfg_ISRStkBasePtr + OSCfg_ISRStkSize - 1u);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                         STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-III's statistics task.  This allows your
*              application to add functionality to the statistics task.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSStatTaskHook (void)
{
	return;
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                            TASK RETURN HOOK
*
* Description: This function is called if a task accidentally returns.  In other words, a task should
*              either be an infinite loop or delete itself when done.
*
* Arguments  : p_tcb        Pointer to the task control block of the task that is returning.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskReturnHook (OS_TCB  *p_tcb)
{
    (void)p_tcb;                                            /* Prevent compiler warning                               */
	return;
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : None.
*
* Note(s)    : 1) This function is assumed to be called from the Tick ISR.
*********************************************************************************************************
*/

void  OSTimeTickHook (void)
{
	return;
}

/*
*********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
*
* Description: This function is called by either OSTaskCreate() or OSTaskCreateExt() to initialize the
*              stack frame of the task being created.  This function is highly processor specific.
*
* Arguments  : task          is a pointer to the task code
*
*              p_arg         is a pointer to a user supplied data area that will be passed to the task
*                            when the task first executes.
*
*              ptos          is a pointer to the top of stack.  It is assumed that 'ptos' points to
*                            a 'free' entry on the task stack.  If OS_STK_GROWTH is set to 1 then
*                            'ptos' will contain the HIGHEST valid address of the stack.  Similarly, if
*                            OS_STK_GROWTH is set to 0, the 'ptos' will contains the LOWEST valid address
*                            of the stack.
*
*              opt           specifies options that can be used to alter the behavior of OSTaskStkInit().
*                            (see uCOS_II.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when your task starts executing.
*              2) All tasks run in SYS mode.
*********************************************************************************************************
*/

CPU_STK *OSTaskStkInit (OS_TASK_PTR p_task, void *p_arg, CPU_STK *p_stk_base, CPU_STK *p_stk_limit, CPU_STK_SIZE stk_size, OS_OPT opt)
{
	CPU_STK *p_stk;


	(void)opt;		                         	/* 'opt' is not used, prevent warning                      */
	p_stk      = &p_stk_base[stk_size-1];      	/* Load stack pointer                                      */
	*(p_stk)   = (CPU_STK)p_task;              	/* Entry Point                                             */
	*(--p_stk) = (CPU_STK)OS_TaskReturn;       	/* R14 (LR)                                                */
	*(--p_stk) = (CPU_STK)0x12121212L;         	/* R12                                                     */
	*(--p_stk) = (CPU_STK)0x11111111L;         	/* R11                                                     */
	*(--p_stk) = (CPU_STK)0x10101010L;         	/* R10                                                     */
	*(--p_stk) = (CPU_STK)0x09090909L;         	/* R9                                                      */
	*(--p_stk) = (CPU_STK)0x08080808L;         	/* R8                                                      */
	*(--p_stk) = (CPU_STK)0x07070707L;         	/* R7                                                      */
	*(--p_stk) = (CPU_STK)0x06060606L;         	/* R6                                                      */
	*(--p_stk) = (CPU_STK)0x05050505L;         	/* R5                                                      */
	*(--p_stk) = (CPU_STK)0x04040404L;         	/* R4                                                      */
	*(--p_stk) = (CPU_STK)0x03030303L;         	/* R3                                                      */
	*(--p_stk) = (CPU_STK)0x02020202L;         	/* R2                                                      */
	*(--p_stk) = (CPU_STK)p_stk_limit;         	/* R1                                                      */
	*(--p_stk) = (CPU_STK)p_arg;               	/* R0 : argument                                           */
	*(--p_stk) = (CPU_STK)ARM_SVC_MODE_ARM;		/* CPSR  (Enable both IRQ and FIQ interrupts, ARM-mode)    */

	return (p_stk);
}

/*
*********************************************************************************************************
*                                           TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                 will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                 task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/
void  OSTaskSwHook (void)
{
	return;
}

#if (HAS_ASSEM_CLZ == 0)
CPU_DATA CPU_CntLeadZeros(CPU_DATA data)
{
    CPU_SR		cpu_sr;
	CPU_DATA	rc = 0, sz = sizeof(CPU_DATA) * 8;
	CPU_DATA	cmp = 0x80 << (sz - 8);

    OS_CRITICAL_ENTER();
	while (rc < sz)
	{
		if (data & cmp)
			break;

		rc++;
		data <<= 1;
	}
    OS_CRITICAL_EXIT();

	return rc;
}
#endif
