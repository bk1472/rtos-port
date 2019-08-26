#ifndef __OSADAP_HEADER__
#define __OSADAP_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------+
| Constant for task creation
+---------------------------------------------------------------------------*/
#define DEFAULT_STACK_SIZE      (4*1024)		/* Default 4k byte stack	*/
#define DEFAULT_TASK_PRIORITY   (    25)		/* Default Task Priority	*/

/*--------------------------------------------------------------------------+
| Constant to give timeout value
+---------------------------------------------------------------------------*/
#define OSADAP_WAIT_FOREVER     (    -1)		/* Wait for ever			*/
#define OSADAP_NOWAIT           (     0)		/* No wait					*/
#define OSADAP_NO_WAIT          (     0)		/* No wait					*/

#define WAIT_FOREVER			OSADAP_WAIT_FOREVER
#define NO_WAIT					OSADAP_NOWAIT
#define NOWAIT					OSADAP_NOWAIT

/*--------------------------------------------------------------------------+
| Return code for timeout in waiting
+---------------------------------------------------------------------------*/
#define OSADAP_ERR_TIMEOUT     0xFFFFFFFF

/*--------------------------------------------------------------------------+
| Semaphore related OSA return codes
+---------------------------------------------------------------------------*/
#define OSADAP_SEM_TIMEOUT      OSADAP_ERR_TIMEOUT		/*	-1				*/
#define OSADAP_SEM_NOTVALID     0xFFFFFFFE				/*	-2				*/
#define OSADAP_SEM_NOTCALLABLE  0xFFFFFFFD				/*	-3				*/
#define OSADAP_SEM_NOTAVAILABLE 0xFFFFFFFC				/*	-4				*/
#define OSADAP_SEM_DELETED      0xFFFFFFFB				/*	-5				*/

/*--------------------------------------------------------------------------+
| Message Queue related OSA return codes
+---------------------------------------------------------------------------*/
#define OSADAP_MSG_TIMEOUT      OSADAP_ERR_TIMEOUT		/*	-1				*/
#define OSADAP_MSG_NOTVALID     0xFFFFFFFE				/*	-2				*/
#define OSADAP_MSG_NOTCALLABLE  0xFFFFFFFD				/*	-3				*/
#define OSADAP_MSG_NOTAVAILABLE 0xFFFFFFFC				/*	-4				*/
#define OSADAP_MSG_DELETED      0xFFFFFFFB				/*	-5				*/
#define OSADAP_MSG_LENGTH_ERROR 0xFFFFFFFA				/*	-6				*/

/*--------------------------------------------------------------------------+
| Message Queue related OSA return codes
+---------------------------------------------------------------------------*/
#define OSADAP_EVT_TIMEOUT      OSADAP_ERR_TIMEOUT		/*	-1				*/
#define OSADAP_EVT_NOTVALID     0xFFFFFFFE				/*	-2				*/
#define OSADAP_EVT_NOTCALLABLE  0xFFFFFFFD				/*	-3				*/
#define OSADAP_EVT_NOTAVAILABLE 0xFFFFFFFC				/*	-4				*/
#define OSADAP_EVT_DELETED      0xFFFFFFFB				/*	-5				*/
#define OSADAP_EVT_ZERO_SIZE    0xFFFFFFFA				/*	-6				*/
#define OSADAP_EVT_NOT_RCVD_ALL 0xFFFFFFF9				/*	-7				*/

/*--------------------------------------------------------------------------+
| Other adaptation codes
+---------------------------------------------------------------------------*/
#define OSADAP_SEM_SUCCESS      0x0
#define OSADAP_SLEEP_ON_LOCAL   0
#define OSADAP_SLEEP_ON_REMOTE  1
#define OS_ERROR                ERROR            /* ERROR = -1 for VxWorks */
#define OS_REMOTE_SUCCESS       OK               /* OK    =  0 for VxWorks */

#define OSA_NAME_SZ				(8)

#if defined (_UCOS_)
#include <osa/osa_ucos.h>
#elif defined (_ATOM_)
#include <osa/osa_atom.h>
#elif defined (_FREERTOS_)
#include <osa/osa_frtos.h>
#else
#error "Operating system is not selected"
#endif

typedef struct mem_pool {
	void *	mem_ptr;
	size_t	mem_size;
	char *	name;
} MEM_POOL;

typedef struct wake_info {
     unsigned long type;
     unsigned long typeId;
     unsigned long status;
     unsigned long data[3];
/* RLB version has these too -- JPC */
     void          *pwaitList;
     void          *pwaitNode;
} WAKE_INFO;


/*------------------------------------------------------------------------+
| uCos Adaptation Layer API prototypes
+-------------------------------------------------------------------------*/
int				deleteTask		( TID_TYPE pid );
TID_TYPE		getTaskId		(char *name);
TID_TYPE		createTask		(
           			char    *name,
           			void    (*entry_point)(void *),
           			void    *arg,
           			void    *rsv,                    /* must be 0 */
           			size_t  stack_size,
           			int     prio,
           			char    group );

void			setTaskPriority	(TID_TYPE procId,int priority);
int				getTaskPriority	(TID_TYPE procId);

unsigned long	ipcsuspendTask	(unsigned int ms,WAKE_INFO *pwake);
void			ipcresumeTask	(TID_TYPE pid, WAKE_INFO *pwake);
void			updateWaitStatus(TID_TYPE taskId, int waitMs);

#define suspendTask(a)		ipcsuspendTask(a,NULL)
#define resumeTask(a)		ipcresumeTask(a,NULL)

int				getTaskStatus	(TID_TYPE procId, int *ptype, int *pId);
int				setTaskParam	(int paramtype, int value);



/*------------------------------------------------------------------------+
| Initializer of OS Adaptation Layer
+-------------------------------------------------------------------------*/
extern void osadap_init (unsigned char, size_t);


/*---------------------------------------------------------------------------+
| Some utility functions for debugging
+----------------------------------------------------------------------------*/
extern void	dbgPrintTasks(void);
extern void	dbgPrintSemas(void);
extern void	dbgPrintMsgQs(void);


/*------------------------------------------------------------------------+
| Task  Related O/S Wrapper
+-------------------------------------------------------------------------*/
extern kernel_state beginCriticalSection(void);
extern void         endCriticalSection(kernel_state previous_state);
extern int          sleepTask(int	waitTime);


/*------------------------------------------------------------------------+
| Semaphore Related O/S Wrapper
+-------------------------------------------------------------------------*/
extern SID_TYPE createSerSem(char *name, int sFlag, int iCount);
extern SID_TYPE createAccSem(char *name, int sFlag);
extern SID_TYPE createMtxSem(char *name, int sFlag);
extern SID_TYPE createBinSem(char *name, int sFlag, int iCount);
extern void     postSem( SID_TYPE which );
extern int      waitSem( SID_TYPE which,int wait_time );
extern void     deleteSem( SID_TYPE which );


/*------------------------------------------------------------------------+
| Message Queue  Related O/S Wrapper
+-------------------------------------------------------------------------*/
extern QID_TYPE createMsg(char *name, int maxQue, int maxLen, int prio);
extern int      sendMsg(QID_TYPE qid, void *msg, ULONG msz);
extern int		sendUrgentMsg(QID_TYPE qid, void *msg, ULONG msz);
extern int      recvMsg(QID_TYPE qid, void *msg, ULONG msz, int wait);


/*------------------------------------------------------------------------+
| Event  Related O/S Wrapper
+-------------------------------------------------------------------------*/
extern int      recvEvent(UINT32 events, UINT32 options, int timeout, UINT32 *rEv);
extern int      sendEvent( TID_TYPE tid, UINT32 ev);


/*------------------------------------------------------------------------+
| Terminal I/O Control Related O/S Wrapper
+-------------------------------------------------------------------------*/
extern void    setTerminalMode(int);
extern void    setTerminalExtISR(void (*pFunction)(UINT08));
extern void	   lockUartPrint(void);
extern void	   unlockUartPrint(void);


/*------------------------------------------------------------------------+
| Timer operation Related O/S Wrapper
+-------------------------------------------------------------------------*/
extern UINT64 __rdUsTicks(void);		/* Read # of 1M Hz ticks(locked)*/
extern UINT64 readUsTicks(void);		/* Read # of 1M Hz ticks        */
extern UINT64 readMsTicks(void);		/* Read # of 1K Hz ticks        */

extern void	  delayMicroSecond(UINT32);	/* Busy wait for given time		*/
extern void	  delayMilliSecond(UINT32);	/* Busy wait for given time		*/

extern UINT64	getSystemGpsTime();
extern int		setSystemGpsTime(UINT64 gpsTime);


/*--------------------------------------------------------------------------+
| Defines used in osamem.c
+---------------------------------------------------------------------------*/
#define			SM_FMAT_POOL		  0		// Pool Id for Fast Mat
#define			SM_SBDY_POOL		  1		// Pool Id for System buddy
#define			SM_SDEC_POOL		  2		// Pool Id for SDEC buddy

/*--------------------------------------------------------------------------+
| Functions in osamem.c
+---------------------------------------------------------------------------*/
extern void*	mallocByPoolId(size_t, UINT08 poolId, int where);
extern void		freeByPoolId(UINT08 poolId);
extern size_t	overheadSize(size_t);
extern int		partition(void *buf);

extern void		SM_DoLogCaller(int);
extern void		SM_InitPool(UINT08 poolId, MEM_POOL *);
extern int		SM_aliveAddr(void *buf);
extern int		SM_aValid(UINT32);
extern int		SM_BuddyMerge(int ppid);
extern UINT32	SM_BuddyStatus(int);
extern void		SM_PrintRecord(void *);
extern void		SM_PrintBuddy(int);
extern void		SM_ShowStatus(int);
extern void		SM_MarkRange(void);
extern void		SM_TestFuncs(void);

#ifdef __cplusplus
}
#endif

#endif/*__OSADAP_HEADER__*/
