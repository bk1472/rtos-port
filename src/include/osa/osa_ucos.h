#ifndef __OSA_UCOS_HEADER__
#define __OSA_UCOS_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <ucos/os.h>
#include <ucos/os_cfg.h>
#include <ucos/os_cfg_app.h>
#include <osa/osa_types.h>

#define FAST	register

/*----------------------------------------------------------------------------+
| Return codes
+----------------------------------------------------------------------------*/
#define OSA_OK		(0)
#define OSA_NOT_OK	(1)
#define ERROR		NOT_OK

/*----------------------------------------------------------------------------+
| Semaphore/Mutex related defines and macros.
+----------------------------------------------------------------------------*/
#define	SMT_ACC			0				/* Semaphore for access control 	*/
#define	SMT_MTX			1				/* Special Binary Sema4 for Mutex	*/
#define	SMT_SER			2				/* Semaphore for serialization		*/

#define	SMF_NONE		0				/* No flag are set				*/

#define	SMF_BOUNDED		0				/* No meaning in vxworks		*/
#define SMF_RECURSIVE	0x00000001		/* Recursive, default for semM	*/
#define SMF_PI_CONTROL	0				/* Priority Inversion Control	*/

/*----------------------------------------------------------------------------+
| Defines comes from vxworks message/semaphore/events
+----------------------------------------------------------------------------*/
#define MSG_Q_FIFO 0
#define SEM_Q_FIFO 0
#define SEM_EMPTY 0
#define SEM_DELETE_SAFE 0
#define EVENTS_SEND_IF_FREE 0
#define EVENTS_WAIT_ALL		0
#define EVENTS_WAIT_ANY		0


/*----------------------------------------------------------------------------+
| Defines for tty control flags
| Copied from /usr/include/sys/termios.h
+----------------------------------------------------------------------------*/
/* Flag bis for uartLFlags */
#define ISIG	0x0001
#define ICANON	0x0002
#define ECHO	0x0004
#define ECHOE	0x0008
#define ECHOK	0x0010
#define ECHONL	0x0020
#define NOFLSH	0x0040
#define TOSTOP	0x0080
#define IEXTEN	0x0100
#define FLUSHO	0x0200
#define ECHOKE	0x0400
#define ECHOCTL	0x0800

/* Flag bis for uartOFlags */
#define OPOST	0x00001
#define OLCUC	0x00002
#define OCRNL	0x00004
#define ONLCR	0x00008
#define ONOCR	0x00010
#define ONLRET	0x00020
#define OFILL	0x00040
#define CRDLY	0x00180

/*----------------------------------------------------------------------------+
| Tick related macros.
+----------------------------------------------------------------------------*/
#define	KC_MS_PER_TICK	 (1000 / sysClkRateGet())				// 100Hz
#define MS2TICKS(time)	 ((time+KC_MS_PER_TICK-1) / KC_MS_PER_TICK)

/*----------------------------------------------------------------------------+
| Other miscellaneous macros.
+----------------------------------------------------------------------------*/
#define NELEMENTS(array)	(sizeof(array) / sizeof((array)[0]))

/*----------------------------------------------------------------------------+
| Global Type declaration wrapper for Task,Queue,Semaphore
+----------------------------------------------------------------------------*/
typedef	unsigned int	SID_TYPE;		/* Type for id of semaphore			*/
typedef	unsigned int	TID_TYPE;		/* Type for id of task(process)		*/
typedef	unsigned int	QID_TYPE;		/* Type for id of message quue		*/
typedef unsigned long	MSG_TYPE[4];	/* Type for fixed size of msg		*/
typedef const unsigned	int	FILE;

typedef long int	__time_t;
typedef __time_t	time_t;
struct	timespec
{
	__time_t	tv_sec;
	long int	tv_nsec;
};

struct tm
{
  int tm_sec;			/* Seconds.	[0-60] (1 leap second) */
  int tm_min;			/* Minutes.	[0-59] */
  int tm_hour;			/* Hours.	[0-23] */
  int tm_mday;			/* Day.		[1-31] */
  int tm_mon;			/* Month.	[0-11] */
  int tm_year;			/* Year	- 1900.  */
  int tm_wday;			/* Day of week.	[0-6] */
  int tm_yday;			/* Days in year.[0-365]	*/
  int tm_isdst;			/* DST.		[-1/0/1]*/

#ifdef	__USE_BSD
  long int tm_gmtoff;		/* Seconds east of UTC.  */
  __const char *tm_zone;	/* Timezone abbreviation.  */
#else
  long int __tm_gmtoff;		/* Seconds east of UTC.  */
  __const char *__tm_zone;	/* Timezone abbreviation.  */
#endif
};

/*----------------------------------------------------------------------------+
| Global variables.
+----------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------+
| Object Specific Variables
|  Keep track of general stats of calls to these APIs.
|  1. Keep full name objects(task, semaphore, message queue)
|  2. Log address of caller of binary or mutex semaphore
|  3. Log success/fail count of messsage queue
+----------------------------------------------------------------------------*/
#define NUM_OF_HASH_ROWS		128
#define NUM_OF_HASH_COLS		  4

#define	OSV_A_SM_T		 		0x00	// Object type is Binary Semaphore(Access Control)
#define	OSV_M_SM_T		 		0x01	// Object type is Mutex Semaphore(Mutual Exclusion)
#define	OSV_S_SM_T		 		0x02	// Object type is Count Semaphore(Serialization)
#define	OSV_MSGQ_T		 		0x04	// Object type is Message Queue
#define	OSV_TASK_T		 		0x05	// Object type is Task
#define OSV_UNUSED		 		0xFF

typedef struct {						// Total 0x0c bytes
	UINT16		ov_wait;				// 0x14: Semaphore/Message id waiting for
	UINT16		ov_rsv1;				// 0x16: _reserved for future use.
	UINT32		ov_mUse;				// 0x18: Memory usage of task.
	UINT32		ov_addr;				// 0x1c: Where has waitSem() been called
	UINT32		ov_prio;				// 0x20: Task Priority
	OS_TCB		*ov_tcb;				// 0x24: Task TCB Pointer
	int			ov_mSec;				// 0x28: Wait time in milli second
} osv_tt;

typedef struct {						// Total 0x0c bytes
	UINT16	ov_iVal;					// 0x14: Initial Lock Counter
	UINT16	ov_recu;					// 0x16: Recursive Counter
	UINT32	ov_task;					// 0x18: Task ID
	UINT32	ov_addr;					// 0x1c: Where has waitSem() been called
	OS_SEM  *ov_sem;					// 0x20: Semaphore id pointer
} osv_st;

typedef struct {						// Total 0x0c bytes
	UINT16	ov_nErr;					// 0x14: Number of failures in sendMsg()
	UINT16	ov_nSnd;					// 0x16: Number of messages sent
	UINT16	ov_nRcv;					// 0x18: Number of messages received
	UINT16	ov_nMax;					// 0x1a: Maximum length of message
	UINT32	ov_ptr;						// 0x1c: Pointer to MSG buffer
	OS_Q	*ov_q;						// 0x20: Queue id pointer
} osv_qt;

typedef	struct {						// Total 0x20 bytes.
	char	ov_name[OSA_NAME_SZ+1];		// 0x00: Name of object
	void	*ov_pObj;					// 0x08: Pointer to object struct
	UINT08	ov_type;					// 0x0c: Type of this object.
	UINT08	ov_grId;					// 0x0d: Group id for task.
	UINT16	ov_myId;					// 0x0e: Object ID of task/semaphore.
	UINT32	ov_errno;					// 0x10: Last error code for this object.
	union
	{
		osv_tt	t;						// 0x14: OSV Task    Specific Variable
		osv_st	s;						// 0x14: OSV Sema4   Specific Variable
		osv_qt	q;						// 0x14: OSV Message Specific Variable
	} u;
} osv_t;

typedef CPU_SR		kernel_state;

/*----------------------------------------------------------------------------+
| File related globals declared in libucos.c
+----------------------------------------------------------------------------*/
extern FILE *stdin, *stdout, *stderr;

/*----------------------------------------------------------------------------+
| Function Prototypes.
+----------------------------------------------------------------------------*/

/*========================================================================
 *
 *                   System Call Definition ( uC/OS specific )
 *
 *=======================================================================*/
/*========================================================================
 * Defined in libucos.c
 *=======================================================================*/
extern int		printf (const char *fmt, ...);
extern int		pollPrint(char *fmt, ...);
extern int		fprintf(FILE *fp, char *fmt, ...);
extern int		memFindMax();
extern int		intLock();
extern void		intUnlock(int status);
extern int		getc(FILE *fp);
extern int		fflush(FILE *fp);
extern char*	fgets(char *c, int n, FILE *fp);
extern void		putchar(UINT08 digit);
extern void		puts(char *c);
extern void		fputs(char *c, FILE *fp);
extern int		suspendTask_by_tid(short tid);
extern void		memShow(int in);
extern float	atof(char *in);
extern int		max(int a, int b);
extern int		min(int a, int b);
extern int		abs(int a);
extern UINT64	sysClkReadNanoSec (void);

/*========================================================================
 * Defined in vsprintf.c
 *=======================================================================*/
extern int		sprintf(char * buf, const char *fmt, ...);
extern int		sscanf(const char * buf, const char * fmt, ...);
extern int		atoi(const char *s);
extern ULONG	strtoul(const char *cp,char **endp,unsigned int base);
extern long		strtol(const char *cp,char **endp,unsigned int base);

/*========================================================================
 * Defined in snprintf.c
 *=======================================================================*/
extern int snprintf (char *, size_t, const char *, ...);
extern int vsnprintf (char *, size_t, const char *, va_list);

/*========================================================================
 * Defined in bsp_c.c
 *=======================================================================*/
extern void		intConnect(int intNum, PFNCT func, int arg, char*name);
extern void		fastintConnect(int intVector, PFNCT isr, int arg, char*name);
extern int		intEnable(int intNum);
extern int		intDisable(int intNum);
extern int		intContext();
extern void		setIntNum(int num);
extern int		getIntNum(void);
extern int		isr_manager_init(void);

/*========================================================================
 * Defined in osadap.c
 *=======================================================================*/
extern int		msgIdListGet(int idList[], int maxMsgs);
extern int		semIdListGet(int idList[], int maxSems);
extern void		SETBRK(void);

/*========================================================================
 * Defined in osamem.c
 *=======================================================================*/
extern void*	malloc(size_t);
extern void*	calloc(size_t, size_t);
extern void*	realloc(void *, size_t);
extern void		free(void *);

/*========================================================================
 * Defined in string.c
 *=======================================================================*/
extern char*	strcpy(char * dest,const char *src);
extern char*	strncpy(char * dest,const char *src,size_t count);
extern char*	strcat(char * dest, const char * src);
extern char*	strncat(char *dest, const char *src, size_t count);
extern int		strcmp(const char * cs,const char * ct);
extern int		strcasecmp(const char * cs,const char * ct);
extern int		strncmp(const char * cs,const char * ct,size_t count);
extern char*	strchr(const char * s, int c);
extern char*	strrchr(const char * s, int c);
extern size_t	strlen(const char * s);
extern size_t	strnlen(const char * s, size_t count);
extern char*	strdup(const char *s);
extern void*	memset(void * s,int c,size_t count);
extern char*	bcopy(const char * src, char * dest, int count);
extern void*	memcpy(void * dest,const void *src,size_t count);
extern void*	memmove(void * dest,const void *src,size_t count);
extern int		memcmp(const void * cs,const void * ct,size_t count);
extern char*	strstr(const char * s1,const char * s2);

#ifdef __cplusplus
}
#endif

#endif/*__OSA_UCOS_HEADER__*/
