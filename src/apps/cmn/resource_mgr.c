/*-----------------------------------------------------------------------------
	제어 상수
	(Control Constants)
------------------------------------------------------------------------------*/
#define __RESOURCE_CONF_C__

/*-----------------------------------------------------------------------------
    #include 파일들
    (File Inclusions)
------------------------------------------------------------------------------*/
#include <common.h>
#include <osa/osadap.h>
#include "resource_mgr.h"

/*-----------------------------------------------------------------------------
	매크로 함수 정의
	(Macro Definitions)
------------------------------------------------------------------------------*/
/*	Message Queue defines	*/
#define	DEF_MSG_LEN			 16
#define DEF_MSG_PRIO		  0

/*----------------------------------------------------------------------------
    형 정의
    (Type Definitions)
-----------------------------------------------------------------------------*/
typedef struct {
	char		name[OSA_NAME_SZ];	/* Name of task							*/
	TID_TYPE	*pTid;				/* Pointer to return Task ID			*/
	void		(*fp)();			/* Start funciton pointer				*/
	void		*arg;				/* Arguement for start function			*/
	int			priority;			/* Initial priority of task				*/
	int			stack_size;			/* Initial stack size					*/
	UINT32		opts;				/* Option to control tasks				*/
	char		group;				/* Group id of Task						*/
} task_conf_t;

typedef struct {
	char		name[OSA_NAME_SZ];	/* Name of queue						*/
	QID_TYPE	*pQid;				/* Pointer to return Queue ID			*/
	int			max_num;			/* Max No. of msg can be queued			*/
	int			max_len;			/* Max Length of message				*/
	int			priority;			/* Priority of message					*/
} queue_conf_t;

typedef struct {
	char		name[OSA_NAME_SZ];	/* Name of semaphore					*/
	SID_TYPE	*pSid;				/* Pointer to return Semaphore ID		*/
	int			sTyp;				/* Type of semaphore					*/
	int			flag;				/* Additional flags for semaphore		*/
	int			vDef;				/* Default value						*/
} sema4_conf_t;

extern void keyboardTask(void);
extern void tTask1(void);
extern void tTask2(void);
extern void tTask3(void);
extern void tTask4(void);
extern void tTask5(void);

task_conf_t startup_tasks[] =
{
	{ "keyboard", (TID_TYPE *)NULL,  keyboardTask,           NULL,30,                      8192, 0x00, 0},
	{ "-tTask01", (TID_TYPE *)NULL,  tTask1,          		 NULL,35,                      8192, 0x00, 0},
	{ "-tTask02", (TID_TYPE *)NULL,  tTask2,          		 NULL,35,                      8192, 0x00, 0},
	{ "-tTask03", (TID_TYPE *)NULL,  tTask3,          		 NULL,35,                      8192, 0x00, 0},
	{ "-tTask04", (TID_TYPE *)NULL,  tTask4,          		 NULL,35,                      8192, 0x00, 0},
	{ "-tTask05", (TID_TYPE *)NULL,  tTask5,          		 NULL,35,                      8192, 0x00, 0},
	{ "        ", (TID_TYPE *)NULL,  NULL,                   NULL, 0,                         0, 0x00, 0}
};

sema4_conf_t startup_sema4s[] =
{
	/*Name,       &pSid,                     Type,    Flags,	       Value */
	{ "smTest01", &lSMidTest01,             SMT_SER, SMF_NONE,         0 },
	{ "smTest02", &lSMidTest02,             SMT_SER, SMF_NONE,         0 },
	{ "",         NULL,                      0,       0,               0 }
};

queue_conf_t startup_queues[] =
{
	/*Name,       &pQid,             nMsgs, LengthOfMsg  Priority      */
	{ "qTest01", &lQMidTest01,              20,   DEF_MSG_LEN,         0 },
	{ "qTest02", &lQMidTest02,              20,   DEF_MSG_LEN,         0 },
	{ "",         NULL,                      0,       0,               0 }
};

/*****************************************************************************/
/* createTasks                                                               */
/*****************************************************************************/
void createTasks(void)
{
	task_conf_t 	*pTask = &startup_tasks[0];
	TID_TYPE		tid;
	int 			i, n;

	for (i = 0, n = 0; pTask->fp != NULL; i++, pTask++)
	{
		if (pTask->stack_size > 0)
		{
			tid = createTask(	pTask->name,
								(void (*)(void*))pTask->fp,
								pTask->arg, NULL,
								pTask->stack_size,
								pTask->priority,
								pTask->group
							);
			if (tid == 0)
			{
				printf("Creating Task[%-8.8s] failed\n", pTask->name);
			}
			else
			{
				if (pTask->pTid != NULL) *pTask->pTid = tid;
				if (pTask->opts &  0x01) suspendTask_by_tid(tid);
				n++;
			}
		}
		else
		{
			((void (*)(void))pTask->fp)();
		}
	}

	dbgprint("%3d Tasks are created", n);
	return;
}

/*****************************************************************************/
/* createSemas                                                               */
/*****************************************************************************/
void createSemas(void)
{
	sema4_conf_t	*pSema = &startup_sema4s[0];
	int				i, n;

	for (i = 0, n = 0; pSema->pSid != NULL; i++, pSema++)
	{
		if		(pSema->sTyp == SMT_SER)
		{
			*pSema->pSid = createSerSem(pSema->name, pSema->flag, pSema->vDef);
		}
		else if (pSema->sTyp == SMT_ACC)
		{
			*pSema->pSid = createAccSem(pSema->name, pSema->flag);
		}
		else if (pSema->sTyp == SMT_MTX)
		{
			*pSema->pSid = createMtxSem(pSema->name, pSema->flag);
		}
		if (pSema->pSid == NULL)
		{
			printf("Create Semaphore[%8.8s] failed\n", pSema->name);
		}
		else
		{
			n++;
		}
	}
	dbgprint("%3d Semaphores are created", n);

	return;
}

/*****************************************************************************/
/* createMsgQs                                                               */
/*****************************************************************************/
void createMsgQs(void)
{
	queue_conf_t	*pQueue = &startup_queues[0];
	int				i, n;

	for (i = 0, n = 0; pQueue->pQid != NULL; i++, pQueue++)
	{
		*pQueue->pQid = createMsg(pQueue->name,
								  pQueue->max_num,
								  pQueue->max_len,
								  pQueue->priority );
		if (*pQueue->pQid == 0)
		{
			printf("Creating Queue[-%8.8s] failed\n", pQueue->name);
		}
		else
		{
			n++;
		}
	}
	dbgprint("%3d Message queues are created", n);
	return;
}
