/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 2000. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : osadap.c
    VERSION     :
    AUTHOR      : bk1472 (baekwon.choi@lge.com)
    DATE        : 2010/01/28
    DESCRIPTION :

*******************************************************************************/

/*---------------------------------------------------------
    전역 제어 상수 정의
    (Global Control Constant Definitions)
---------------------------------------------------------*/
#define CHECK_DEADLOCK		1

#if     (CHECK_DEADLOCK == 2)
#undef SMT_ACC
#define SMT_ACC			SMT_MTX
#endif

/*---------------------------------------------------------
    #include 파일들
    (File Inclusions)
---------------------------------------------------------*/
#include <osa/frtos/FreeRTOS.h>
#include <common.h>
#include <board.h>
#include <bsp_arch.h>
#include <serial.h>
#include <osadap.h>
#include <dbgprint.h>
#include <exception.h>

/*---------------------------------------------------------
    전역 제어 상수 정의
    (Global Control Constant Definitions)
---------------------------------------------------------*/
#define PRIO_GROWTH_H		(0)
#define PRIO_GROWTH_L		(1)

/*---------------------------------------------------------
    상수 정의
    (Constant Definitions)
---------------------------------------------------------*/
#define ID_MASK_SEMA			0x4000
#define ID_MASK_MSGQ			0x8000
#define	ID_MASK_EVNT			0xC001	/* Task is waiting for task event		*/
#define ID_MASK_CHAR			0xC003	/* Task is waiting for key input		*/
#define	ID_MASK_TIME			0xC00f	/* Task is sleeping 					*/

#define SIG_WAKEUP				SIGRTMIN
#define INVALID_TID				0xEEEEFFFF

#define HASH_FUNC(__s)			( (((UINT32)(__s))/0x28)&(NUM_OF_HASH_ROWS-1) )
#define NUM_OF_TASKS			256

/*---------------------------------------------------------
    형 정의
    (Type Definitions)
---------------------------------------------------------*/

/*---------------------------------------------------------
    Extern 전역변수와 함수 prototype 선언
    (Extern Variables & Function Prototype Declarations)
---------------------------------------------------------*/
/*Task*/
extern	TaskHandle_t	FRTOS_getCurr			(void);
extern	unsigned int	FRTOS_getTid			(TaskHandle_t pHandle);
extern	void			FRTOS_setTid			(TaskHandle_t pHandle, unsigned int osa_tid);
extern	char*			FRTOS_getTskNm			(TaskHandle_t pHandle);
extern	uint32_t		FRTOS_getPrio			(TaskHandle_t pHandle);
extern	uint32_t		FRTOS_getStSize			(TaskHandle_t pHandle);
extern	void* 			FRTOS_getSp				(TaskHandle_t pHandle);
extern	void*			FRTOS_getSpBTM			(TaskHandle_t pHandle);


/*Queue*/
extern	char*			get_QueueBuffer			(QueueHandle_t pHandle);


extern	int				pollPrint				(char *format, ...);
extern	UINT64			sysClkReadNanoSec		(void);

/*---------------------------------------------------------
    전역 변수와 함수 prototype 선언
    (Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
osv_t			osvHash[NUM_OF_HASH_ROWS][NUM_OF_HASH_COLS];
osv_t			osvTask[NUM_OF_TASKS];
unsigned long	critical_section_count = 0;
unsigned long	sem_error;
int         	gWaitWarnTicks         = 50;


/*---------------------------------------------------------
    Static 변수와 함수 prototype 선언
    (Static Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
static SemaphoreHandle_t	osaUartMutex  = NULL;
static SemaphoreHandle_t	osvHashMutex  = NULL;
static SemaphoreHandle_t	osvTCreateMtx = NULL;
static int			bInExcHook     = FALSE;

static const char	*smTypes[] = { "Acc", "Mtx", "Ser" };

static osv_t		*addOsvEntry(void *pObj, char *name, int type, int grId);
static osv_t		*findOsvEntry(void *pObj, int isDelete );

osv_t 				*findMyOsvEntry(int myId);
/******************************************************************************/
/* New task entry point.                                                      */
/******************************************************************************/
void new_task_entry_point(void* p_argument)
{
	typedef void (*proc_t)(void*);

	uint32_t* p_arg = p_argument;
	proc_t proc     = (proc_t)p_arg[0];
	int* arg        = (int*  )p_arg[1];

	free(p_arg);
	(void)xSemaphoreGive(osvTCreateMtx);

	if(proc)
		(*proc)((void*)arg);
}

/******************************************************************************/
/* Enter a critical section.                                                  */
/******************************************************************************/
kernel_state beginCriticalSection(void)
{
    kernel_state  cpu_sr;

	cpu_sr = port_SR_Save();

	return( cpu_sr );
}

/******************************************************************************/
/* Exit a critical section.                                                   */
/******************************************************************************/
void endCriticalSection(kernel_state cpu_sr)
{
	port_SR_Restore(cpu_sr);
}

/******************************************************************************/
/* Conversion function between milli second time and system ticks             */
/*                                                                            */
/******************************************************************************/
static int ms2Ticks(int ms)
{
	int tick = 0;

	if ((ms != WAIT_FOREVER) && (ms != NO_WAIT))
	{
		tick = (UINT32)(((UINT64)ms * configTICK_RATE_HZ*2 + 999) / 1000);
		if (tick < 1) tick = 1;
	}
	else
	{
		tick = ms;
	}

	return(tick);
}

/******************************************************************************/
/* map createTask to VxWorks taskSpawn.                                       */
/*                                                                            */
/* If the priority parameter is 0, set the task priority equal to the         */
/* DEFAULT_TASK_PRIORITY. If the priority is negative or greater than the     */
/* maximum supported priority level, set the priority to the maximum supported*/
/* priority level.                                                            */
/*                                                                            */
/* If the stack_size is set to 0, set the task stack size equal to the        */
/* DEFAULT_STACK_SIZE.                                                        */
/*                                                                            */
/* Convert task priority.  For process_create, the larger the priority number */
/* the greater the priority. For VxWorks, the lower the priority number the   */
/* greater the priority.                                                      */
/*                                                                            */
/* Return 0 if taskSpawn fails.                                               */
/*                                                                            */
/******************************************************************************/
TID_TYPE createTask(
           char    *name,
           void    (*entry_point)(void *),
           void    *arg,
           void    *rsv,                    /* must be 0 */
           size_t  stack_size,
           int     prio,
		   char    group )					/* TODO : makeup for dbgprint */
{
	BaseType_t		err;
	int				n;
	osv_t			*pOsv;
	TaskHandle_t	pTcb = NULL;
	uint32_t		stkSz = stack_size/sizeof(uint32_t);
	uint32_t		*p_argument;

	/* Coverity Warning */
	if (name != NULL)                        /* maximum of OSA_NAME_SZ char for task name */
	{
		for(n=0; name[n] != 0; n++)
		{
			if(n >= OSA_NAME_SZ)
				return(0);
		}
	}
	if (prio <= 0)
		prio=DEFAULT_TASK_PRIORITY;

	if (stkSz==0)
		stkSz=0x800;

	(void)xSemaphoreTake(osvTCreateMtx, portMAX_DELAY);

	p_argument = (uint32_t*)malloc(sizeof(uint32_t)*3);
	p_argument[0] = (uint32_t)entry_point;
	p_argument[1] = (uint32_t)arg;
	err = xTaskCreate(new_task_entry_point, name, (uint16_t)stkSz, p_argument, prio, &pTcb);

	if (err != pdPASS)
	{
		pollPrint("OSAD: (E) Cannot create task, err 0x%x\n", err);
		free(p_argument);
		return (0);
	}

	if ((pOsv = addOsvEntry(pTcb, name, OSV_TASK_T, group)) == NULL)
	{
		#ifdef DBG_ON
		printf ( "OSAD: (E) in createTask(): %8.8s\n", name);
		#endif
	}
	else
	{
		pOsv->u.t.ov_prio = 0;
		pOsv->u.t.ov_wait = 0;
		pOsv->u.t.ov_mSec = 0;
		pOsv->u.t.ov_mUse = 0;
		pOsv->u.t.ov_addr = 0;
	}

	pOsv->u.t.ov_prio = prio;

	#ifdef DBG_ON
	printf("CreateTask : %08x", (UINT32)entry_point);
	#endif

	return((TID_TYPE)pOsv->ov_pObj);
}

/******************************************************************************/
/* map deleteTask to  uCos OSTaskDel.                                         */
/******************************************************************************/
int deleteTask( TID_TYPE tid )
{
	printf("(OS ERR)atomThreads system can't delete running thread!\n");
	return -1;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
TID_TYPE getTaskId(char *name)
{
	osv_t*			pOsv = &osvTask[0];
	int				n    = 0;

	if (name == NULL)
	{
		n = (int)FRTOS_getTid(FRTOS_getCurr());
	}
	else
	{
		for (n = 0; n < NUM_OF_TASKS; n++, pOsv++)
		{
			if (strcasecmp(name, pOsv->ov_name) == 0)
				break;
		}
	}


	if (n == NUM_OF_TASKS)
		return -1;

 	return n;
}

char *getTaskName(void)
{
	char			*tname = "";
	osv_t			*pOsv  = findMyOsvEntry(-1);
	TaskHandle_t	data   = NULL;

	if (pOsv != NULL)
	{
		data = pOsv->u.t.ov_tcb;
	}
	else
	{
		data = FRTOS_getCurr();
	}

	if (data != NULL)
		tname = FRTOS_getTskNm(data);
	else
		tname = "-Tsk     ";

	return tname;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void setTaskPriority(TID_TYPE procId,int priority)
{
	printf("(OS ERR) atomThreads system can't change thread's priority!\n");
	return;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int  getTaskPriority(TID_TYPE procId)
{
	osv_t			*pOsv = findOsvEntry((void*)procId, 0);
	TaskHandle_t	pTcb  = NULL;

	if (pOsv == NULL)
		return -1;

	pTcb = pOsv->u.t.ov_tcb;

	return (int)FRTOS_getPrio(pTcb);
}

/******************************************************************************/
/* New OS Adaptation Layer API.  This may be used to wait on events on local  */
/*    processor or remote processor (in which case a msg from remote typically*/
/*    issues the wakeup.  This version also allows status to be returned      */
/*    as well as one 32-bit value from the caller (who issues the             */
/*    resumeTask() call.                                                      */
/*                                                                            */
/* Returns:  status of the operation                                          */
/*                                                                            */
/* Upon return, the process issuing the resumeTask call has filled            */
/* in any return data at pValue (unless pValue is pointer to NULL).           */
/*                                                                            */
/******************************************************************************/
unsigned long ipcsuspendTask(unsigned int ms, WAKE_INFO *pwake)
{
	if (ms == NO_WAIT)
	{
		/* Nothing to do, just ignore it */
	}
	else if (ms == WAIT_FOREVER)
	{
		/* Nothing to do, just ignore it */
		while(1)
		{
			vTaskDelay(100);
		}
	}
	else
	{
		#if 0
		UINT16	hours, minutes, seconds;
		UINT32	milli;
		osv_t	*pOsv = findOsvEntry((void *)getTaskId(NULL), 0);

		if (pOsv)
		{
			pOsv->u.t.ov_wait = ID_MASK_TIME;
			pOsv->u.t.ov_mSec = ms;
		}
		ms *=2;

		hours   = (ms / (1000 * 60 * 60));
		minutes = (ms % (1000 * 60 * 60) ) / (1000 * 60);
		seconds = (ms % (1000 * 60 ) ) / (1000);
		milli	= (ms % 1000);

		OSTimeDlyHMSM(hours, minutes, seconds, milli, OS_OPT_TIME_DLY, &err);

		if (pOsv)
		{
			pOsv->u.t.ov_wait = 0;
			pOsv->u.t.ov_mSec = 0;
		}
		#else
		osv_t	*pOsv = findOsvEntry((void *)getTaskId(NULL), 0);
		int		tick  = ms2Ticks(ms);

		if (pOsv)
		{
			pOsv->u.t.ov_wait = ID_MASK_TIME;
			pOsv->u.t.ov_mSec = ms;
		}

		vTaskDelay((TickType_t)tick);

		if (pOsv)
		{
			pOsv->u.t.ov_wait = 0;
			pOsv->u.t.ov_mSec = 0;
		}
		#endif
	}
  return 0;
}
/******************************************************************************/
/* ipcresumeTask                                                              */
/*        Status is result of remote wakeup.                                  */
/*        Value may or may not have meaning, depending on remote command type.*/
/******************************************************************************/
void ipcresumeTask(TID_TYPE tid, WAKE_INFO *pwake)
{
	printf("(OS ERR) atomThreads system doesn't have thread resume API!\n");
	return;
}

int	sleepTask(int waitSec)
{
	ipcsuspendTask(waitSec*1000, NULL);

	return 0;
}

int suspendTask_by_tid(short int tid)
{
	printf("(OS ERR) atomThreads system doesn't have thread suspend by Thread API!\n");
	return -1;
}

/******************************************************************************/
/*	Update wait status of task                                                */
/******************************************************************************/
void updateWaitStatus(TID_TYPE taskId, int waitMs)
{
	#if 0
	osv_t	*pOsvTask = NULL;

	if (taskId <= 0)
		taskId = getTaskId(NULL);

	pOsvTask = findOsvEntry((void *)taskId, 0);

	if (pOsvTask)
	{
		if (waitMs == 0)
		{
			pOsvTask->u.t.ov_wait = 0;
			pOsvTask->u.t.ov_mSec = 0;
		}
		else
		{
			pOsvTask->u.t.ov_wait = ID_MASK_CHAR;
			pOsvTask->u.t.ov_mSec = waitMs;
		}
	}
	#endif
	return;
}

/******************************************************************************/
/* Add Semaphore Hash Entry                                                   */
/******************************************************************************/
static osv_t *addOsvEntry( void *pObj, char *name, int type, int grId )
{
	osv_t		*retEntry = NULL;
	osv_t		*osvCol   = NULL;
	BaseType_t	err;

	err = xSemaphoreTake(osvHashMutex, portMAX_DELAY);
	if (err == pdPASS)
	{
		int		i, n = 0, start, nCols;
		UINT32	hash = HASH_FUNC(pObj);

		if (type == OSV_TASK_T)
		{
			start = hash;
			nCols = NUM_OF_TASKS;
			//dbgprint("^Y^hash: %d,  %s, 0x%08x", hash, ((OS_TCB*)pObj)->NamePtr, (UINT32)pObj);
		}
		else
		{
			start = 0;
			nCols = NUM_OF_HASH_COLS;
		}

		for (i = 0, n = 0; i < nCols; i++)
		{
			n = (i + start) % nCols;

			/* For task skip index */
			if (type == OSV_TASK_T)
			{
				if (n == 0) continue;
				osvCol = &osvTask[n];
			}
			else
			{
				osvCol = &osvHash[hash][n];
			}

			if (osvCol->ov_pObj == 0)
			{
				retEntry = osvCol;

				strncpy(retEntry->ov_name, name, OSA_NAME_SZ);
				retEntry->ov_pObj    = (void *)((type == OSV_TASK_T) ? (int)n : (int)pObj);
				retEntry->ov_type    = type;
				retEntry->ov_grId    = grId;
				retEntry->ov_myId    = n*NUM_OF_HASH_ROWS + hash;
				if (type == OSV_TASK_T)
				{
					retEntry->u.t.ov_tcb = (TaskHandle_t)pObj;
					FRTOS_setTid(retEntry->u.t.ov_tcb,(unsigned int)n);
				}
				//pollPrint("Info] addObj(0x%x, %s, %d) at %d(%d)\n", pObj, name, type, n, hash);
				break;
			}
		}

		if (retEntry == NULL)
		{
			dbgprint("^R^Error!!!! too many entries in row %d", hash);
		}
		(void)xSemaphoreGive(osvHashMutex);
	}

	return(retEntry);
}

/******************************************************************************/
/* Find Semaphore Hash Entry with given sid                                   */
/******************************************************************************/
static osv_t *findOsvEntry( void *pObj, int isDelete )
{
	osv_t		*retEntry = NULL;
	osv_t		*osvCol   = NULL;
	BaseType_t	err       = pdPASS;	/* Coverity Warning */
	UINT08		inIsr;
	UINT32		intStatus = 0;
	int			taskId = (int)pObj;

	inIsr = intContext();

	if (!inIsr)
	{
		intStatus = intLock();
		err = xSemaphoreTake(osvHashMutex, portMAX_DELAY);
	}

	if (intContext() || (err == pdPASS))
	{
		int		i, n, start, nCols;
		UINT32	hash;

_AGAIN:
		hash = HASH_FUNC(pObj);

		if (taskId < NUM_OF_TASKS)
		{
			start = hash;
			nCols = NUM_OF_TASKS;
		}
		else
		{
			start = 0;
			nCols = NUM_OF_HASH_COLS;
		}

		for (i = 0, n = start; i < nCols; i++, n++)
		{
			n %= nCols;

			/* For task skip index */
			if (taskId < NUM_OF_TASKS)
			{
				if (n == 0)continue;
				osvCol = &osvTask[n];
			}
			else
			{
				osvCol = &osvHash[hash][n];
			}

			if (osvCol->ov_pObj == pObj)
			{
				if (isDelete)
				{
					osvCol->ov_pObj = 0;
					osvCol->ov_type = OSV_UNUSED;

					if (taskId < NUM_OF_TASKS)
					{
						osvCol->u.t.ov_wait = 0;
						osvCol->u.t.ov_mSec = 0;
					}
				}
				else
				{
					retEntry = osvCol;
				}
				break;
			}
		}

		if ((retEntry == NULL) && (isDelete == FALSE))
		{
			if (taskId >= NUM_OF_TASKS)
				return NULL;

			pObj = (TaskHandle_t)FRTOS_getCurr();
			goto _AGAIN;
		}

		if (!inIsr)
		{
			(void)xSemaphoreGive(osvHashMutex);
			intUnlock(intStatus);
		}
	}

	if (retEntry == NULL)
	{
		printf("ERR : no hash entry exist for %08x\n", pObj);
	}

	return(retEntry);
}

osv_t *findMyOsvEntry(int myId)
{
	osv_t	*pRetOsv = NULL;

	if (myId < 0)
		myId = getTaskId(NULL);

	pRetOsv = findOsvEntry((void *)myId, 0);

	return(pRetOsv);
}

/******************************************************************************/
/* Map createSem to VxWorks sem[BCM]Create.                                   */
/* semType should be one of SMT_SER, SMT_ACC, SMT_MTX                         */
/******************************************************************************/
static SID_TYPE createSem( void * sem_ptr, char *name, int sFlag, int iCount, int semType)
{
	static SID_TYPE		sem_id = 0x1000;
	SID_TYPE			myId;
	char				*sm_name;
	SemaphoreHandle_t	pOsSem = NULL;
	osv_t				*pOsv;
	BaseType_t			err;


	if (sem_ptr != NULL)
		pOsSem = (SemaphoreHandle_t)sem_ptr;

	if     (iCount == 1 && semType == SMT_MTX) //Mutex
		pOsSem = xSemaphoreCreateMutex();
	else if(iCount == 1)
		pOsSem = xSemaphoreCreateBinary();
	else
		pOsSem = xSemaphoreCreateCounting((UBaseType_t)iCount, (UBaseType_t)iCount);

	if (name == NULL)
	{
		sm_name = (char*)malloc(16);
		sprintf(&sm_name[0], "KS%02d", sem_id & 0xfff);
		name = &sm_name[0];
	}

	if (pOsSem == NULL)
	{
		printf("OSAD: (E) in CreateSem[%3.3s] : %8.8s, %d, err=0x%x\n",
					smTypes[semType], name, iCount, err);
	}
	else if ((pOsv = addOsvEntry((void *)sem_id, name, semType, 0)) == NULL)
	{
		#ifdef DBG_ON
        printf ( "OSAD: (E) in createSem[%3.3s] : %8.8s\n", smTypes[semType], name);
		#endif
	}
	else
	{
		pOsv->u.s.ov_iVal = iCount;
		pOsv->u.s.ov_iCnt = iCount;
		pOsv->u.s.ov_addr = 0;

		if (sFlag & SMF_RECURSIVE)
			pOsv->u.s.ov_recu = 0;
		else
			pOsv->u.s.ov_recu = 0xFFFF;

		pOsv->u.s.ov_task = 0;
		pOsv->u.s.ov_sem  = pOsSem;
		myId = sem_id;
		sem_id += 0x01;
	}

	return(myId);
}

/******************************************************************************/
/* osamem semaphore                                                           */
/******************************************************************************/
SID_TYPE createArgMtxSem(void *sem_ptr, char *name, int sFlag)
{
	SID_TYPE sem = createSem(sem_ptr, name, sFlag, 1, SMT_MTX);
	return (sem);
}

/******************************************************************************/
/* Map createAccSem to VxWorks semCCreate- by contigo.                        */
/******************************************************************************/
SID_TYPE createSerSem(char *name, int sFlag, int iCount)
{
	SID_TYPE sem = createSem(NULL, name, sFlag, iCount, SMT_SER);
	return (sem);
}

/******************************************************************************/
/* Map createAccSem to VxWorks semBCreate- by contigo.                        */
/******************************************************************************/
SID_TYPE createAccSem(char *name, int sFlag)
{
	SID_TYPE sem = createSem(NULL, name, sFlag, 1, SMT_ACC);
	return (sem);
}

/******************************************************************************/
/* Map createMtxSem to VxWorks semMCreate- by contigo.                        */
/******************************************************************************/
SID_TYPE createMtxSem(char *name, int sFlag)
{
	SID_TYPE sem = createSem(NULL, name, sFlag, 1, SMT_MTX);
	return (sem);
}

/******************************************************************************/
/* Map createBinSem to VxWorks semBCreate- by contigo.                        */
/******************************************************************************/
SID_TYPE createBinSem(char *name, int sFlag, int iCount)
{
	SID_TYPE sem = createSem(NULL, name, sFlag, iCount, SMT_ACC);
	return (sem);
}

/******************************************************************************/
/* Map postSem to uC/OS OSSemPost.                                            */
/******************************************************************************/
#if	(CHECK_DEADLOCK > 0)
static int _checkDeadLock(osv_t *pOsvSema, osv_t *pOsvTask)
{
	UINT32		taskSet[NUM_OF_TASKS/32] = {0, };
	osv_t		*pInOsvSema = pOsvSema;
	osv_t		*pInOsvTask = pOsvTask;
	BaseType_t	err;
	int			i;
	int			bDead  = FALSE;
	int			bPrint = FALSE;
	int			sid0, sid1, sid2;
	int			tid0, tid1, tid2;
	int			row, col;
	char		*msg = "";

	err = xSemaphoreTake(osvHashMutex, portMAX_DELAY);

	sid0 = (int)pOsvSema->ov_myId;
	tid0 = (int)pOsvTask->ov_pObj;

	if (bDead == TRUE)
	{
RestartForPrint:
		bPrint = TRUE;
		dbgprint("^r^waitSem() :: Task '%s' is %s", pOsvTask->ov_name, msg);
		dbgprint("^r^%-2d] '%8.8s' is waiting for '%8.8s'(0x%02x)", 0, pInOsvTask->ov_name, pInOsvSema->ov_name, sid0);
	}
	memset(taskSet, 0, sizeof(taskSet));

	sid1 = sid0;
	tid1 = tid0;
	for (i = 1; i < 30; i++)
	{
		taskSet[tid1/32] |= 1 << (tid1 % 32);
		row = sid1 % NUM_OF_HASH_ROWS;
		col = sid1 / NUM_OF_HASH_ROWS;

		pOsvSema = &osvHash[row][col];
		tid2 = pOsvSema->u.s.ov_task;
		if (tid2 == 0)
			break;

		pOsvTask = &osvTask[tid2];
		sid2     = pOsvTask->u.t.ov_wait;

		if (bPrint) dbgprint("^r^%-2d] '%8.8s' owned by task  '%8.8s'(0x%02x)", i, pOsvSema->ov_name, pOsvTask->ov_name, tid2);

		/* Task is not waiting mutex/semaphore/msg */
		if (sid2 == 0)
			break;

		/* Finite timeout, treat safe from deadlock */
		if (pOsvTask->u.t.ov_mSec != -1)
			break;
		if		(sid2 == ID_MASK_EVNT)
		{
			msg = (char *)"waiting event with SEM/MTX locked";
			if (bPrint == FALSE) goto RestartForPrint;
			else                 break;
		}
		else if	(sid2 == ID_MASK_CHAR)
		{
			msg = (char *)"waiting character with SEM/MTX locked";
			if (bPrint == FALSE) goto RestartForPrint;
			else                 break;
		}
		else if (sid2 & ID_MASK_MSGQ)
		{
			msg = (char *)"waiting message with SEM/MTX locked";
			if (bPrint == FALSE) goto RestartForPrint;
			else                 break;
		}

		sid2 &= 0xfff;
		pOsvSema = &osvHash[sid2%NUM_OF_HASH_ROWS][sid2/NUM_OF_HASH_ROWS];

		if (bPrint) dbgprint("^r^%-2d] '%8.8s' is waiting for '%8.8s'(0x%02x)", i, pOsvTask->ov_name, pOsvSema->ov_name, sid2);

		/* Trace task waiting for message or event */
		if ( taskSet[tid2/32] & (1 << (tid2 % 32)) )
		{
			msg = (char *)"blocked by deadlock";
			bDead = TRUE;
			if (bPrint == FALSE) goto RestartForPrint;
			else                 break;
		}
		sid1 = sid2;
		tid1 = tid2;
	}
	(void)xSemaphoreGive(osvHashMutex);

	return bDead;
}
#endif

void postSem( SID_TYPE which )
{
	BaseType_t		err;
	void			*caller;
	osv_t			*pOsv;
	TID_TYPE		tId;
	kernel_state	state;

	if (which == 0)
	{
		dbgprint("^R^postSem(ID): ID is NULL");
		return;
	}

	str_lr(&caller);
	pOsv     = findOsvEntry((void *)which, 0);
	tId      = getTaskId(NULL);

	/* Mutex Type 인 경우 owner 이외의
	 * task가 Post를 못하도록 설정.
	 */
	if ( (pOsv != NULL)           &&
		 (pOsv->u.s.ov_task != 0) &&
		 (pOsv->ov_type == SMT_MTX)
	   )
	{
		if (tId != pOsv->u.s.ov_task)
		{
			dbgprint("^R^This mtx (ID:%x) is locked by different Task return Err", pOsv->ov_pObj);
			dbgprint("Locked by Task(%03d), Post Try by Task(%03d)", pOsv->u.s.ov_task, tId);
			PrintStack();
			return;
		}
	}

	state = intLock();

	if (pOsv != NULL)	/* Accept KLOCWORK warning */
	{
		pOsv->u.s.ov_addr = (UINT32) caller;
		if ((pOsv->u.s.ov_recu != 0xFFFF) && (pOsv->u.s.ov_recu > 0))
		{
			pOsv->u.s.ov_recu--;
		}
		else
		{
			pOsv->u.s.ov_task	= 0;
			err = xSemaphoreGive(pOsv->u.s.ov_sem);
			if(pdPASS != err)
				sem_error++;
		}
	}
	else
	{
		err = xSemaphoreGive(pOsv->u.s.ov_sem);
		if(pdPASS != err)
			sem_error++;
	}
	if(pdPASS == err)
	{
		pOsv->u.s.ov_iCnt++;
	}

	intUnlock(state);

	return;
}

/******************************************************************************/
/* Map waitSem to uC/OS semTake.  This version specifies                      */
/*       the wait time.                                                       */
/******************************************************************************/
int waitSem( SID_TYPE which, int wait_time )
{
	BaseType_t		err;
	int				number_of_ticks = 0;
	void			*caller;
	osv_t			*pOsvSem, *pOsvTask = NULL;
	TID_TYPE		tId;
	UINT32			cnt;
	kernel_state	state;
	UINT64			requestTick, startingTick;

	str_lr(&caller);

	if (which == 0)
	{
		dbgprint("^R^waitSem(ID): ID is NULL");
		PrintStack();
		return (OSADAP_SEM_NOTVALID);
	}

	requestTick = readMsTicks();

	pOsvSem  = findOsvEntry((void *)which, 0);
	if (pOsvSem == NULL)
	{
		dbgprint("^R^waitSem(ID): no semaphre ID found");
		PrintStack();
		return (OSADAP_SEM_NOTVALID);
	}

	number_of_ticks = ms2Ticks(wait_time);
	if (wait_time == WAIT_FOREVER)
		number_of_ticks = portMAX_DELAY;


	if (intContext() && (wait_time == WAIT_FOREVER) && !bInExcHook)
	{
		pollPrint("waitSem(%x, FOREVER), while in intContext() %08x\n", which, caller);
		PrintStack();
	}

	/*
	 * If getting sem is failed, sem is recursive, and sem owner is this task,
	 * don't wait for sem
	 */
	state = intLock();
	/*
	 * Just try to get semaphore
	 */
	if ((cnt = pOsvSem->u.s.ov_iCnt) > 0)
	{
		err = xSemaphoreTake(pOsvSem->u.s.ov_sem, 1);
	}
	tId      = getTaskId(NULL);
	pOsvTask = findOsvEntry((void*)tId, 0);

	if (pOsvTask != NULL) /* Accept "KLOCWORK" warning */
	{
		pOsvTask->u.t.ov_wait = pOsvSem->ov_myId + ID_MASK_SEMA;
		pOsvTask->u.t.ov_mSec = wait_time;
		pOsvTask->u.t.ov_addr = (UINT32)caller;
	}

	if (cnt == 0)
	{
//		dbgprint("Sem accept failed %08x %08x", pOsvSem->u.s.ov_sem, caller);
		err = pdFAIL;
		if (((UINT32)tId == pOsvSem->u.s.ov_task) && (pOsvSem->u.s.ov_recu != 0xFFFF))
		{
//			dbgprint("Sem recursive %08x %08x %08x", pOsvSem->u.s.ov_sem, caller, pOsvSem->u.s.ov_recu);
			pOsvSem->u.s.ov_recu++;
			err = pdPASS;
		}
	}
	else
	{
//		dbgprint("Sem accept success %08x", pOsvSem->u.s.ov_sem);
	    pOsvSem->u.s.ov_task	= (UINT32)tId;
	    err = pdPASS;
	}

	if(pdPASS == err)
		pOsvSem->u.s.ov_iCnt--;
	intUnlock(state);

	/*
	 * If wait flag is enabled, wait
	 */
	if ((err != pdPASS) && (wait_time != NO_WAIT))
	{
		startingTick = readMsTicks();
		if ((startingTick - requestTick) > gWaitWarnTicks)
		{
			dbgprint("^r^%s(%s,%d) :: Started with %d ms delay", __func__, pOsvSem->ov_name, wait_time, startingTick - requestTick);
		}
		#if (CHECK_DEADLOCK > 0)
		if (pOsvSem->ov_type == SMT_SER)
		{
			_BACK_2:
			err = xSemaphoreTake(pOsvSem->u.s.ov_sem, number_of_ticks);
			if(    number_of_ticks == portMAX_DELAY /*forever*/
				&& err != pdPASS)
				goto _BACK_2;
		}
		else
		{
			#define POLL_PERIOD		(   1000)
			#define CHECK_TIME0		( 5*1000)
			#define CHECK_TIME1		(60*1000)
			UINT32	current = POLL_PERIOD;
			int		count   = 0;
			int		bDead   = FALSE;
			int		remain;

			if (wait_time != WAIT_FOREVER)
				remain = wait_time - (SINT32)(startingTick-requestTick);
			else
				remain = 1;

			while ( (err != pdPASS) && (remain > 0) )
			{
				int	curr_tick;

				count += POLL_PERIOD;

				if (wait_time != WAIT_FOREVER)
				{
					current = min(remain, POLL_PERIOD);
					remain -= current;
				}

				/* Check deadlock every 15 seconds */
				if ( !bDead && ((count % CHECK_TIME0) == 0) )
				{
					if ((count % CHECK_TIME1) == 0)
						dbgprint("^r^%s(%s, %d) :: Checking deadlock, count=%d", __func__, pOsvSem->ov_name, wait_time, count);
					bDead = _checkDeadLock(pOsvSem, pOsvTask);
				}
				/*
				 * Jackee's Note: 2007/11/15
				 *	pthread_mutex_timedlock() will not timed out if the type of
				 *	mutex is not TIMED_MUTEX. To use time out, we should set
				 *	mutex attribute as timed.
				 */
				curr_tick = ms2Ticks(current);
				err = xSemaphoreTake(pOsvSem->u.s.ov_sem, curr_tick);
			}
		}
		#else
		_BACK_3:
		err = xSemaphoreTake(pOsvSem->u.s.ov_sem, number_of_ticks);
		if(    number_of_ticks == portMAX_DELAY /*forever*/
			&& err != pdPASS)
			goto _BACK_3;
		#endif
	}

	if (pOsvTask != NULL)
	{
		pOsvTask->u.t.ov_wait = 0;
		pOsvTask->u.t.ov_mSec = 0;
	}

	if (err != pdPASS)
	{
		sem_error++;
		return (OSADAP_SEM_TIMEOUT);
	}
	else
	{
		pOsvSem->u.s.ov_iCnt--;
	}

	if (pOsvSem != NULL)
	{
		pOsvSem->u.s.ov_addr = (UINT32) caller;
		pOsvSem->u.s.ov_task = (UINT32)tId;
	}

	return (OSADAP_SEM_SUCCESS);
}

/*---------------------------------------------------------------------------+
| Message Related functions
|	createMsg()
|	sendMsg()
|	sendUrgentMsg()
|	recvMsg()
+----------------------------------------------------------------------------*/
#define QUEUE_UNUSED	0x80000000
QID_TYPE createMsg(char *name, int maxNum, int maxLen, int prio)
{
	static QID_TYPE	queue_id = 0x2000;
	QID_TYPE		myId;
	QueueHandle_t	qid = NULL;
	osv_t			*pOsv;

	if (maxNum < 0)
		maxNum = -maxNum;
	if (maxLen < 4)
		maxLen = 4;
	maxLen = (((maxLen+3)>>2)<<2);

	qid = xQueueCreate((BaseType_t)maxNum, (BaseType_t)maxLen);
	if (qid == NULL)
	{
		return 0;
	}

	if ((pOsv = addOsvEntry((void *)queue_id, name, OSV_MSGQ_T, 0)) == NULL)
	{
		#ifdef DBG_ON
        printf ( "OSAD: (E) in createMsg : %8.8s\n", name);
		#endif
	}
	else
	{
		pOsv->u.q.ov_nSnd = 0;
		pOsv->u.q.ov_nRcv = 0;
		pOsv->u.q.ov_nErr = 0;
		pOsv->u.q.ov_nMax    = maxLen;
		pOsv->u.q.ov_nMaxnum = maxNum;
		pOsv->u.q.ov_ptr  = (UINT32)get_QueueBuffer(qid);
		pOsv->u.q.ov_q    = qid;
		myId = queue_id;
		queue_id += 0x01;
	}

	return(myId);
}

int sendMsg(QID_TYPE qid, void *msg, ULONG msz)
{
	osv_t		*pOsvMsg;
	uint8_t		*buffer;
	BaseType_t	err;

	if ((qid & 0x2000) == 0)
	{
		return (OSADAP_MSG_NOTVALID);
	}

	pOsvMsg  = findOsvEntry((void *)qid, 0);

	if (pOsvMsg == NULL)
	{
		dbgprint("^R^sendMsg: Queue not found %x", qid);
		return -1;
	}

	buffer = (uint8_t*)msg;

	if (msz > pOsvMsg->u.q.ov_nMax)
	{
		dbgprint("^R^sendMsg: msg size is over Max value %s(%x): %d > %d", pOsvMsg->ov_name, qid, msz, pOsvMsg->u.q.ov_nMax);
		err = pdFAIL;
	}
	else
	{
		err = xQueueSend(pOsvMsg->u.q.ov_q, (uint8_t*)buffer, portMAX_DELAY);
	}

	if (err == pdPASS)
	{
		if (pOsvMsg) pOsvMsg->u.q.ov_nSnd++;
		return  0;
	}
	else
	{
		if (pOsvMsg) pOsvMsg->u.q.ov_nErr++;
		return OSADAP_MSG_NOTAVAILABLE;
	}
}

int recvMsg(QID_TYPE qid, void *msg, ULONG msz, int waitMs)
{
	uint8_t 		*buffer = (uint8_t*)msg;
	int				length;
	void			*caller;
	osv_t			*pOsvMsg, *pOsvTask;
	int				number_of_ticks=0;
	BaseType_t		err;

	str_lr(&caller);

	if ((qid & 0x2000) == 0)
	{
		return (OSADAP_MSG_NOTVALID);
	}

	pOsvMsg  = findOsvEntry((void *)qid, 0);

	pOsvTask = findOsvEntry((void *)(FRTOS_getTid(FRTOS_getCurr())), 0);

	number_of_ticks = ms2Ticks(waitMs);
    if (waitMs == WAIT_FOREVER)
      number_of_ticks = portMAX_DELAY;

	if ( (pOsvTask != NULL) && (pOsvMsg != NULL) )
	{
		pOsvTask->u.t.ov_wait = pOsvMsg->ov_myId + ID_MASK_MSGQ;
		pOsvTask->u.t.ov_mSec = waitMs;
		pOsvTask->u.t.ov_addr = (UINT32)caller;
	}

	if(pOsvMsg->u.q.ov_nMax < msz)
	{
		err = pdFAIL;
	}
	else
	{
		if (waitMs == NO_WAIT) err = xQueueReceive(pOsvMsg->u.q.ov_q, (void*)buffer, 0);
		else                   err = xQueueReceive(pOsvMsg->u.q.ov_q, (void*)buffer, number_of_ticks);
	}

	if (err != pdPASS)
	{
		length = 0;
	}

	if (pOsvTask != NULL)
	{
		pOsvTask->u.t.ov_wait = 0;
		pOsvTask->u.t.ov_mSec = 0;
	}

    if (err == pdPASS)
	{
		if (pOsvMsg) pOsvMsg->u.q.ov_nRcv++;
		return (length);
	}
	return (OSADAP_MSG_NOTVALID);
}

/*---------------------------------------------------------------------------+
| Message Related functions
|	recvEvent()
|	sendEvent()
+----------------------------------------------------------------------------*/
int recvEvent(UINT32 events, UINT32 options, int waitMs, UINT32 *pRetEvents)
{
	printf("(OS ERR)atomThreads system doesn't give recvEvent API!\n");
	return -1;
}

int sendEvent(TID_TYPE tid, UINT32 ev)
{
	printf("(OS ERR)atomThreads system doesn't give sendEvent API!\n");
	return -1;
}

/*---------------------------------------------------------------------------+
| Initialize Operating System ADAPtation Layer
|	osadap_init()
+----------------------------------------------------------------------------*/

void osadap_init(unsigned char defPri, size_t dummy)
{
	extern int dbgFreeRTOSExcBaseHook(int vect, void *sp, REG_SET *pRegs, EXC_INFO *pExcInfo);
	osv_t			*pOsv;
	int				index;
	TaskHandle_t	pTcb = FRTOS_getCurr();

	FrtosExcHandle = dbgFreeRTOSExcBaseHook;

	pOsv = &osvHash[0][0];
	for (index = 0; index < NUM_OF_TASKS; index++, pOsv++)
		pOsv->ov_type = OSV_UNUSED;

	pOsv = &osvTask[0];
	for (index = 0; index < NUM_OF_TASKS; index++, pOsv++)
		pOsv->ov_type = OSV_UNUSED;

	if (strcmp(FRTOS_getTskNm(pTcb), "root") == 0)
	{
		UINT32	hash = HASH_FUNC(pTcb);
		pOsv = &osvTask[1];
		strncpy(pOsv->ov_name, "root", OSA_NAME_SZ);
	    pOsv->ov_pObj     = (void*)1;
	    pOsv->ov_type     = OSV_TASK_T;
	    pOsv->ov_grId     = 0;
	    pOsv->ov_myId     = 1*NUM_OF_HASH_ROWS + hash;
		pOsv->u.t.ov_prio = 20;
		pOsv->u.t.ov_wait = 0;
		pOsv->u.t.ov_mSec = 0;
		pOsv->u.t.ov_mUse = 0;
		pOsv->u.t.ov_addr = 0;
		pOsv->u.t.ov_tcb  = pTcb;
		FRTOS_setTid(pTcb, 1u);
	}

	osaUartMutex = xSemaphoreCreateMutex();
	if (osaUartMutex == NULL)
	{
		pollPrint("OSAD: (E) Cannot create mutex for dbgprint\n");
	}

	/* Initialize mutex for Object Hash Table Entries	*/
	osvHashMutex = xSemaphoreCreateMutex();
	if (osvHashMutex == NULL)
	{
		pollPrint("OSAD: (E) Cannot create mutex for hash table\n");
	}

	osvTCreateMtx = xSemaphoreCreateCounting(1, 1);
	if (osvTCreateMtx == NULL)
	{
		pollPrint("OSAD: (E) Cannot create mutex for dbgprint\n");
	}
	return;
}



/*---------------------------------------------------------------------------+
| Some utility functions for debugging
|	semIdListGet(int idList[], int maxSems)
|	msgIdListGet(int idList[], int maxMsgs)
|	dbgPrintTask1(int)
|	dbgPrintTasks(void)
|	dbgPrintSemas(void)
|	dbgPrintMsgQs(void)
+----------------------------------------------------------------------------*/
int semIdListGet(int idList[], int maxSems)
{
	int			i, n = 0;
  	BaseType_t	err;
	osv_t	*pHash = &osvHash[0][0];

	err = xSemaphoreTake(osvHashMutex, portMAX_DELAY);
	if (err == pdPASS)
	{
		for (i=0, n = 0; (i < NUM_OF_HASH_ROWS*NUM_OF_HASH_COLS) && (n < maxSems); i++, pHash++)
		{
			if ((pHash->ov_pObj != 0) && (pHash->ov_type <= SMT_SER))
			{
				idList[n] = (int) pHash->ov_pObj;
				n++;
			}
		}
		(void)xSemaphoreGive(osvHashMutex);
	}
	return(n);
}

int msgIdListGet(int idList[], int maxMsgs)
{
	int			i, n = 0;
  	BaseType_t	err;
	osv_t	*pHash = &osvHash[0][0];

	err = xSemaphoreTake(osvHashMutex, portMAX_DELAY);
	if (err == pdPASS)
	{
		for (i=0, n = 0; (i < NUM_OF_HASH_ROWS*NUM_OF_HASH_COLS) && (n < maxMsgs); i++, pHash++)
		{
			if ((pHash->ov_pObj != 0) && (pHash->ov_type == 0xff))
			{
				idList[n] = (int) pHash->ov_pObj;
				n++;
			}
		}
		(void)xSemaphoreGive(osvHashMutex);
	}
	return(n);
}

void dbgPrintTask1(char *taskName)
{
	extern char		gDebugCtrl[16];

	static char		msg[] = " *";
	int				i, n = 0, sumMUse = 0;
	int				taskId = -1;
	TaskHandle_t	pTcb;

	if (taskName != NULL)
	{
		if ( (('a' <= taskName[0]) && (taskName[0] <= 'z')) ||
		     (('A' <= taskName[0]) && (taskName[0] <= 'Z')) )
		{
			taskId = getTaskId(taskName);
		}
		else
		{
			sscanf(taskName, "%x", &taskId);
			if (taskId == 0) taskId = getTaskId(NULL);
		}
	}

	for (i = 1; i < NUM_OF_TASKS; i++)
	{
		UINT32	stack_size, stack_usage, stack_used, waitFor;
		UINT32	grId, mUse, caller;
		osv_t	*pOsv = NULL;
		void*	stk_ptr;
		void*	stk_btm;

		pOsv = &osvTask[i];
		if (pOsv->ov_type != OSV_TASK_T)
			continue;

    	pTcb = pOsv->u.t.ov_tcb;

		stack_size  = (UINT32) FRTOS_getStSize(pTcb) * sizeof(uint8_t);
		if (stack_size == 0)
		{
			stack_usage = 0;
			stack_used  = 0;
		}
		else
		{
			stk_ptr = FRTOS_getSp(pTcb);
			stk_btm = FRTOS_getSpBTM(pTcb);
			stack_used  = (UINT32)stk_ptr;
			stack_used  = abs(stack_used - (UINT32)stk_btm);
			stack_usage = (stack_used * 100) / stack_size;
		}

		if ( (taskId != -1) && (taskId != FRTOS_getTid(pTcb)) )
		{
			continue;
		}

		/*
		 * TODO:
		 *	Add memory usage summary
		 */
		grId   = pOsv->ov_grId;
		mUse   = pOsv->u.t.ov_mUse;
		caller = pOsv->u.t.ov_addr;
		waitFor= pOsv->u.t.ov_wait & 0x00000fff;

		printf(" TI[%-8.8s] %cC+F%02d, tid=%02x(w=%03x,p=%3d), m=%6d, s[%06x-%06x:%06x]%3d%%(%04x/%04x)\n",
				FRTOS_getTskNm(pTcb), (gDebugCtrl[grId])?msg[1]:msg[0], grId,
				(UINT08)FRTOS_getTid(pTcb), waitFor, pOsv->u.t.ov_prio, mUse,
				(UINT32)stk_ptr, (UINT32)stk_btm, stack_size * sizeof(uint8_t),
				stack_usage, stack_size - stack_used, stack_size
				);

		sumMUse += mUse;
		n++;

		/**
		 *
		 * TODO : later Fix(2015.12.23)
		 *
		 */
		if (taskId != -1)
		{
			extern UINT32 *excGetRegList(void);
			UINT32	*pRegList;

			if (taskId != getTaskId(NULL))
			{
				UINT32	stack[3];

				stack[0] = (UINT32)(stk_ptr) + (4*9); /*sp*/
				stack[1] = 0;                                   /*lr*/
				stack[2] = (UINT32)(stk_ptr) + (4*8); /*pc*/
				StackTrace(0x2000, 32, NULL, &stack[-13], taskId);
			}
			else if ((pRegList = excGetRegList()) != NULL)
			{
				StackTrace(0x2000, 32, NULL, (UINT32 *)pRegList, 0);
			}
			else
			{
				StackTrace(0x2000, 32, NULL, NULL, 0);
			}
		}
	}
	printf(" Total %d Tasks, %d bytes allocated\n", n, sumMUse);

	return;
}

void dbgPrintTasks(void)
{
	dbgPrintTask1((char *)NULL);
}

void dbgPrintSemas(void)
{
	int					i, n = 0;
	osv_t				*pHash = &osvHash[0][0];
	SemaphoreHandle_t	pSema;
	BaseType_t			err;

	err = xSemaphoreTake(osvHashMutex, portMAX_DELAY);
	if (err == pdPASS)
	{
		for (i=0; (i < NUM_OF_HASH_ROWS*NUM_OF_HASH_COLS); i++, pHash++)
		{
			if ((pHash->ov_pObj != 0) && (pHash->ov_type <= SMT_SER))
			{
				n++;
				pSema = (SemaphoreHandle_t)pHash->u.s.ov_sem;

//				hexdump("Sem", (void *)pSema, sizeof(SEMAPHORE));

				if (pHash->u.s.ov_recu == 0xFFFF)
				{
					printf(" SI[%-8.8s] sid=%03x(%06x) %s, iVal=%2d, cVal=%2d\n",
							pHash->ov_name, pHash->ov_myId, (int)pHash->ov_pObj, smTypes[pHash->ov_type],
							pHash->u.s.ov_iVal, pHash->u.s.ov_iCnt);
				}
				else
				{
					printf(" SI[%-8.8s] sid=%03x(%06x) %s, nest=%2d, owner=0x%06x, lockfrom=0x%06x\n",
							pHash->ov_name, pHash->ov_myId, (int)pHash->ov_pObj, smTypes[pHash->ov_type],
							pHash->u.s.ov_recu, pHash->u.s.ov_task, pHash->u.s.ov_addr);
				}
			}
		}
		(void)xSemaphoreGive(osvHashMutex);
	}
	printf(" Total %d Semaphores\n", n);

	return;
}

void dbgPrintMsgQs(void)
{
	int				i, n = 0;
	osv_t			*pHash = &osvHash[0][0];
	QueueHandle_t	pMsgq;
	BaseType_t		err;

	err = xSemaphoreTake(osvHashMutex, portMAX_DELAY);
	if (err == pdPASS)
	{
		for (i=0; (i < NUM_OF_HASH_ROWS*NUM_OF_HASH_COLS); i++, pHash++)
		{
			if ((pHash->ov_pObj != 0) && (pHash->ov_type == OSV_MSGQ_T))
			{
				n++;
				pMsgq = (QueueHandle_t)pHash->u.q.ov_q;

//				msgQShow(pMsgq, 1);
//				hexdump("Msg", (void *)pMesq, sizeof(MSG_Q));

				printf(" MI[%-8.8s] qid=%03x(%06x), mQue=%3d, mLen=%2d, nQue=%-4d, nSnd=%-6d, nErr=%d\n",
						pHash->ov_name, pHash->ov_myId, (int)pHash->ov_pObj,
						pHash->u.q.ov_nMaxnum, pHash->u.q.ov_nMax,
						pHash->u.q.ov_nSnd - pHash->u.q.ov_nRcv,
						pHash->u.q.ov_nSnd,  pHash->u.q.ov_nErr);
			}
		}
		(void)xSemaphoreGive(osvHashMutex);
	}
	printf(" Total %d Message Queues\n", n);

	return;
}

/*---------------------------------------------------------------------------+
| 	Read fault address/status register in coprocessor register 15
+----------------------------------------------------------------------------*/
void lockUartPrint(void)
{
	if (!intContext() && (osaUartMutex != NULL))
	{
		(void)xSemaphoreTake(osaUartMutex, portMAX_DELAY);
	}
	return;
}

void unlockUartPrint(void)
{
	if (!intContext() && (osaUartMutex != NULL))
	{
		(void)xSemaphoreGive(osaUartMutex);
	}
	return;
}

/*---------------------------------------------------------------------------+
|  Some timer related fucntions moved from timer.c
+----------------------------------------------------------------------------*/
UINT64 readUsTicks(void)
{
	UINT64 value = sysClkReadNanoSec();

	return (value/(UINT64)1000);
}

UINT64 readMsTicks(void)
{
	UINT64 value = sysClkReadNanoSec();

	return (value/(UINT64)1000000);
}

#define	MS2US(__ms)				((__ms) * 1000)

void delayMicroSecond(unsigned int usWait)
{
	UINT64	endUs;

	endUs = readUsTicks() + usWait;

	while (endUs > readUsTicks())
		;

	return;
}

void delayMilliSecond(unsigned int ms)
{
	delayMicroSecond(MS2US(ms));
	return;
}

/* UTC 값(1970.1.1 기준)을 GPS값(1980.1.6 기준)으로 바꾸기 위한 offset */
#define 	GPS2UTC		0x12D53D80
UINT64 timeGap = 0;

UINT64 getSystemGpsTime(void)
{
	UINT64	gpsTime;
	#if 0
	struct timespec seed;
	clock_gettime(CLOCK_REALTIME, &seed);

	gpsTime = seed.tv_sec - GPS2UTC;
	#else
	gpsTime = (UINT64)(xTaskGetTickCount()/100) + timeGap;
	#endif

	return gpsTime;
}

int setSystemGpsTime(UINT64 gpsTime)
{
	#if 0
	struct timespec seed;

	seed.tv_sec = gpsTime + GPS2UTC;
	seed.tv_nsec = 0;
	clock_settime(CLOCK_REALTIME, &seed);
	#else
	timeGap = gpsTime - (UINT64)(xTaskGetTickCount()/100);
	#endif

	return 0;
}
