/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 1998-2003. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : osadap.c
    VERSION     : 1.0
    AUTHOR      : 이재길(jackee@lge.com)
    DATE        : 2003.01.29
    DESCRIPTION : OS Adaptation Memory Driver

*******************************************************************************/

/*-----------------------------------------------------------------------------
	전역 제어 상수
	(Control Constants)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
    #include 파일들
    (File Inclusions)
------------------------------------------------------------------------------*/
#include <common.h>
#include <osa/osadap.h>
#include <osa/osa_utils.h>
#include <board.h>
#include <bsp_arch.h>

/*-----------------------------------------------------------------------------
	제어 상수
	(Control Constant)
-------------------------------------------------------------------------------
	Check Points.

	  1. Checking memory leaks.
		Define SM_LOG_ALLOC. It will log caller's stack information on the
		header of allocated memory, and each memory is added to allocation
		list. By using SM_ShowStatus(), memory allocation within period can be
		displayed. The period will be set via remocon task using key '`'.
		If SM_USE_FAST_MAT is defined, memory request smaller than 65 byte
		will not logged. so, if you want to trace that size of memory request,
		undefine SM_USE_FAST_MAT.

	  2. Tuning fast allocation table.
		MAT_xx_MAX is used to make pre-allocated small size memory pool.
		Since it's allocation/deallocation speed is decided by number of
		entries to use, it is fixed at compile time. So, It must be tuned at
		the maximum request of memory from all applications. Since this
		allocation table has no header information, memory is used more
		efficent than another one(buddy).

	  3. Deciding Control Constants
		SM_LOG_ALLOC	0	기본적인 로그만 남김, 64k 이상 또는 FAST_MAT에
							메모리가 없어서 Buddy로 넘어온 것들만 추적함
						1	Buddy에서 할당되는 모든 메모리를 추적함.
						2	모든 메모리를 Buddy에서 할당하게 하여 모든 메모리를
							추적함.
		SM_FILL_MODE	0	표준 malloc()함수와 동일하게 동작함.
						1	malloc()을 calloc() 처럼 동작하게 함.
						2	할당된 메모리의 Internal fragment에 magic pattern을
							기록하여, 이 영역을 검사하여 Buffer overrun을 감지함.
						3	할당된 메모리의 전 영역에 magic pattern을 기록하여,
							이곳의 변경 유무에 따라 Buffer Overrun 또는
							Uninialized Pointer의 사용을 감지함.

------------------------------------------------------------------------------*/
#define	SM_LOG_ALLOC		   1	// Define it to trace allocation path
#define	SM_FILL_MODE		   1	// 0 : No fill, default behavior of malloc
									// 1 : Clear on alloc, treat malloc as calloc
									// 2 : Fill Magic Pattern on free(),
									//     Fill Zero  Pattern on alloc()
									// 3 : Fill Magic Pattern on free(),
									//     Keep Magic Pattern on alloc()
									//   2,3 is for checking usage after free
#define SM_LOG_ONBOOT		FALSE	// Enable/Disable log on boot
#define SM_FILL_MAGIC_B		0xee	// Magic pattern to be filled
									//   LST4200A는 0x81xxxxxx를 사용하면 Crash가
									//   발생하므로 디버깅이 용이하당.

#define	SM_FILL_MAGIC_S		( (SM_FILL_MAGIC_B <<  8) | SM_FILL_MAGIC_B )
#define	SM_FILL_MAGIC_L		( (SM_FILL_MAGIC_S << 16) | SM_FILL_MAGIC_S )

#define	SM_USE_FAST_MAT				// Define it to use fast memory alloc tbls.
#define	SM_LOG_ORG_SIZE				// Log Original size of fast memory alloc.


#if 	(SM_LOG_ALLOC > 0)
#define	SM_NTRACES			 32		// Number of call-stack trace to print.
#define	SM_NCALLS			  7		// Depth of stack trace.

  #if	(SM_LOG_ALLOC > 1)
  #undef SM_USE_FAST_MAT			// Log small size allocations.
  #endif /* SM_LOG_ALLOC > 1 */

  #if defined(SM_USE_FAST_MAT) && !defined(SM_LOG_ORG_SISE)
  #define	SM_LOG_ORG_SIZE
  #endif /* SM_USE_FAST_MAT */
#endif	/* SM_LOG_ALLOC > 0 */


/*-----------------------------------------------------------------------------
	상수 정의
	(Constant Definitions)
------------------------------------------------------------------------------*/
#ifdef	SM_USE_FAST_MAT

#define	MAT_NUM_LEVELS		   4	/* Number of levels in fast MAT tree	  */
#define	SM_NUM_MATS			   5	/* Number size entries fast MAT			  */
#define	SM_MAT_MAX			(0x0002 << SM_NUM_MATS)
#define	ALLOCATOR(_sz,_id)	(((_sz <= SM_MAT_MAX) && (_id<=SM_SBDY_POOL)) ? SM_FMAT_POOL : _id)

#define	MAT_04_MAX			4096	/*   4 Byte Entries:  4* 4K = (0x004000)  */
#define	MAT_08_MAX			6144	/*   8 Byte Entries:  8* 6K = (0x004000)  */
#define	MAT_10_MAX			6144	/*  16 Byte Entries: 16* 6K = (0x018000)  */
#define	MAT_20_MAX			6144	/*  32 Byte Entries: 32* 6K = (0x030000)  */
#define	MAT_40_MAX		   10240	/*  64 Byte Entries: 64*10K = (0x0A0000)  */
#define	MAT_80_MAX			4096	/* 128 Byte Entries:128* 4K = (0x080000)  */

#if		(MAT_04_MAX>=32768) || (MAT_08_MAX>=32768) || (MAT_10_MAX>=32768)	\
	 ||	(MAT_20_MAX>=32768) || (MAT_40_MAX>=32768) || (MAT_80_MAX>=32768)
  #define	MAT_BASE_LEVEL	   0
#else
  #define	MAT_BASE_LEVEL	   1
#endif	/* MAT_nn_MAX	*/

#else

#define	ALLOCATOR(_sz,_id)   _id	/* Always used buddy					  */

#endif	/* SM_USE_FAST_MAT */


#if	(SM_LOG_ALLOC > 1)
  #define BDY_BASE_INDEX	   6	/* (1 << 6) == 64                         */
  #define BDY_MIN_INDEX		   0	/* Minimum buddy size is   64 byte        */
  #define BDY_MAX_INDEX		  20	/* Maximum buddy size is  64M byte        */
#else
  #define BDY_BASE_INDEX	   7	/* (1 << 7) == 128, (1 << 8) = 256	      */
  #define BDY_MIN_INDEX		   0	/* Minimum buddy size is  128 byte        */
  #define BDY_MAX_INDEX		  20	/* Maximum buddy size is 128M byte.       */
#endif	/* SM_LOG_ALLOC */

#define DEF_AMASK	( (1<<10)-1 )	/* Default align mask of buddy pool.      */
#define	MAX_MSIZE	(  0x200000 )	/* 2M: Max allocation size per request    */
#define	PART_SIZE		  0x1000	/* 4k: Max size of buddy unit		      */
#define OVERHEAD	(sizeof(BuddyHead_t))


#define	SM_STATE_0			   0	/* SM System is not initialized yet.      */
#define	SM_STATE_1			   1	/* SM System is in  initializing.         */
#define	SM_STATE_2			   2	/* SM System is     initialized.	      */

#define	SM_FIND_PPID		   0	/* Find Physical Pool Id			      */
#define	SM_FIND_VPID		   1	/* Find Physical Pool Id			      */

#define	SM_PHYS_POOLS		   3	/* No. of real pools(FMAT,Buddy1,Buddy2)  */
#define	SM_VIRT_POOLS		   8	/* No. of virtual pools					  */
#define	SM_FMAT_POOL		   0	/* Pool Id for Fast Mat					  */
#define	SM_SBDY_POOL		   1	/* Pool Id for System buddy				  */
#define	SM_SDEC_POOL		   2	/* Pool Id for SDEC buddy				  */
#define	SM_VOID_POOL		 255	/* Invalid Pool Id						  */

#define SM_FLAG_FHEAD		   0	/* Memory is head of free partition		  */
#define SM_FLAG_UHEAD		   1	/* Memory is head of used partition		  */
#define SM_FLAG_CHILD		   2	/* Memory is child of partition			  */
#define SM_FLAG_BUDDY		   3	/* Memory is divided into small buddy	  */

/*-----------------------------------------------------------------------------
	매크로 함수 정의
	(Macro Definitions)
------------------------------------------------------------------------------*/
#if	   (ENDIAN_TYPE == BIG_ENDIAN)
#define BUD_ISUSED(p,x)		((gBdyUsed[p][x.s.byteNo] &   (1<<x.s.bitNo)) != 0)
#define CLEAR_USED(p,x)		( gBdyUsed[p][x.s.byteNo] &= ~(1<<x.s.bitNo))
#define SET_USED(p,x)		( gBdyUsed[p][x.s.byteNo] |=  (1<<x.s.bitNo))
#else //ENDIAN_TYPE == LITTLE_ENDIAN
#define BUD_ISUSED(p,x)		((gBdyUsed[p][x.u / 32  ] &   (1<<(x.u% 32))) != 0)
#define CLEAR_USED(p,x)		( gBdyUsed[p][x.u / 32  ] &= ~(1<<(x.u% 32)))
#define SET_USED(p,x)		( gBdyUsed[p][x.u / 32  ] |=  (1<<(x.u% 32)))
#endif	/* ENDIAN_TYPE */

#define	BUD_OFFSET(_i,_p)	((UINT32)_p - gPoolBase[_i])
#define	OFS2BOFS(p,x,y,z)	(x.u = (gBdyBlks[p][(y)] + ((z)>>((y)+BDY_BASE_INDEX))))

#define	Dptr2Hptr(_i,_p)	&gPartHdr[_i][(((UINT32)(_p)-gPoolBase[_i])/PART_SIZE)]
#define	Hptr2Dptr(_i,_p)	(gPoolBase[_i] + PART_SIZE*((_p)-gPartHdr[_i]))


#define	SHOW_STACK()		StackTrace(0x2000,     0x20,   NULL, 0, 0)
#if	(SM_LOG_ALLOC > 0)
#define	LOG_CALLER(addr)	StackTrace(0x2000, SM_NCALLS, &addr, 0, 0)
#else
#define LOG_CALLER(addr)	// Empty Macro
#endif

#undef	_BUDDY_DEBUG_
#ifdef	_BUDDY_DEBUG_
#define BDY_PRINTF(args...)	dbgprint(args)
#else
#define BDY_PRINTF(args...)
#endif


/*----------------------------------------------------------------------------
    형 정의
    (Type Definitions)
-----------------------------------------------------------------------------*/
#ifdef	SM_USE_FAST_MAT
typedef struct
{
	UINT32	sm_base;			/* Base address of pool					*/
	UINT32	sm_maxu;			/* Maximum No. of units in this pool	*/
	UINT32	sm_free;			/* Number of free units in this pool	*/
	UINT32	**sm_masks;			/* Mask data for each level				*/
	UINT08	*sm_orgSize;		/* Original size of [mc]alloc()	arg		*/
} SM_MAT_POOL_t;
#endif /* SM_USE_FAST_MAT */

/*
 * Memory Allocation Counter LOGger
 *	Index-1 Usage:
 *		mac_log[ 0] - mac_log[14] : =< 2**(2+n)      byte allocations.
 *		mac_log[15]               :  > 2**16(=65536) byte allocations.
 */
typedef struct
{
	UINT32	nAlloc;				/* Number of allocations				*/
	UINT32	nFrees;				/* Number of frees						*/
	UINT32	nMax;				/* Maximum concurrent allocations.		*/
	UINT32	naSize;				/* Allocated memory size.				*/
} mac_log_t;


typedef struct
{
	UINT32	__bd_opts :  8,
			__bd_size : 24;
	#define	bd_asize	__bd_asz.__bd_size
	#define	bd_flags	__bd_asz.__bd_opts
	#define	bd_osize	__bd_msz.__bd_size
	#define	bd_task		__bd_msz.__bd_opts
} _SZ3_t;

/*	Structure to hold essential data of buddy node						*/
typedef	struct BuddyHead
{
	struct	BuddyHead *next;	/* Next free cell						*/
	struct	BuddyHead *prev;	/* Prev free cell						*/
	_SZ3_t	__bd_asz;			/* UnitSize or Number of free cells.	*/
	_SZ3_t	__bd_msz;			/* Original Size of memory reclaim.		*/
#if	(SM_LOG_ALLOC > 0)
	UINT32	bd_mUser[SM_NCALLS];/* Address of memory requestor.			*/
	UINT32	bd_lbolt;			/* Memory reguesting time.				*/
#endif	/* SM_LOG_ALLOC */
} BuddyHead_t;

/*	Structure to present offset position from buddy base				*/
typedef struct _buddyOffsetSub
{
#if   (ENDIAN_TYPE == LITTLE_ENDIAN)
	UINT32	bitNo  :  5,
			byteNo : 27;
#else//ENDIAN_TYPE == BIG_ENDIAN
	UINT32	byteNo : 27,
			bitNo  :  5;
#endif	/* ENDIAN_TYPE */
} bOffsetSub_t;

/*	Structure to present offset position from buddy base				*/
typedef union _buddyOffset
{
	UINT32			u;
	bOffsetSub_t	s;
} bOffset_t;

typedef BuddyHead_t	BuddyList_t[BDY_MAX_INDEX+1];
typedef UINT32		BuddyBlks_t[BDY_MAX_INDEX+1];


/*-----------------------------------------------------------------------------
	외부 전역변수와 외부 함수 prototype 선언
	(Extern Variables & External Function Prototype Declarations)
------------------------------------------------------------------------------*/
extern void		SysAttachStackTracer(void (*)());
extern void		SetAllocatorRange(UINT32, UINT32);
extern int		pollPrint(char *format, ...);
extern osv_t	*findMyOsvEntry(int);
extern SID_TYPE	createArgMtxSem(void *sem_ptr, char *name, int sFlag);

/*-----------------------------------------------------------------------------
	전역 변수 정의와 전역 함수 Prototype 선언.
	(Define global variables)
------------------------------------------------------------------------------*/
void*	malloc(size_t);
void*	calloc(size_t, size_t);
void*	realloc(void *, size_t);
void	free(void *);
size_t	msize(void *buf);
size_t	asize(void *buf);
void	SM_InitResource();
void	SM_InitPool(UINT08 ppid, MEM_POOL *);
void	SM_ShowStatus(int);
void	SM_MarkRange(void);
int		SM_BuddyMerge(int ppid);
UINT32	SM_BuddyStatus(int);
void	SM_PrintBuddy(int);
void	SM_PrintRecord(void *);
int		SM_aValid(UINT32);
void*	mallocByPoolId(size_t size, UINT08 vpid, int where);
void	freeByPoolId(UINT08 vpid);
UINT08	findPoolId(void *buf, int opt);

UINT32	sms_lbolt1 = 0;		/* Start time of memory trace triggering	*/
UINT32	sms_lbolt2 = 0;		/* End   time of memory trace triggering	*/


/*-----------------------------------------------------------------------------
	Static 변수와 함수 prototype 선언
	(Static Variables & Function Prototypes Declarations)
------------------------------------------------------------------------------*/
static void	 SM_MacUpdate(size_t asize, int poolNo, int mode);
#ifdef	SM_USE_FAST_MAT
static void* SM_Fast_Alloc(size_t size, UINT08 vpid);
static void	 SM_Fast_Free(void *buf, UINT32 caller);
#else
#define		 SM_Fast_Alloc		SM_BuddyAlloc
#define		 SM_Fast_Free		SM_BuddyFree
#endif
static void* SM_BuddyAlloc(size_t size, UINT08 vpid);
static void	 SM_BuddyFree(void *buf, UINT32 caller);
static void	 SM_BuddyInit(UINT08 ppid, char *base_addr, size_t pool_size);
static void	 SM_InitPoolSize(MEM_POOL *pMem_pool);

static void	 _OSA_MEM_BEG_(void);
static void	 _OSA_MEM_END_(void);


/* Mutex Semaphore to protect internal data of SM system				*/
static ATOM_MUTEX	sm_mutex;
static ATOM_MUTEX	*SmMtx = &sm_mutex;

/* Control flag to enable/disable caller stack trace for buddy			*/
static UINT32		do_log_caller = SM_LOG_ONBOOT;
#if	(SM_FILL_MODE == 3)
static UINT32		do_fill_check = SM_FILL_MODE;
#endif

/*	Memory Allocation Counter LOGger									*/
static mac_log_t	mac_log[16];

/* Redirected malloc function. Initially								*/
/* it points to memory init function.									*/
static void* (*_sm_allocator[SM_PHYS_POOLS])(size_t, UINT08) =
{
	SM_BuddyAlloc,	/* Will be changed to SM_Fast_Alloc() after init	*/
	SM_BuddyAlloc,	/* Will not be changed at all						*/
	SM_BuddyAlloc,	/* Will not be changed at all						*/
};


/*=============================================================================
 	Variables to Maintail Fast MAT System.
==============================================================================*/
#ifdef	SM_USE_FAST_MAT

#ifdef	SM_LOG_ORG_SIZE

/* Leaf Level allocated  size*/
static UINT08	MAT_04_OSIZE[MAT_04_MAX];
static UINT08	MAT_08_OSIZE[MAT_08_MAX];
static UINT08	MAT_10_OSIZE[MAT_10_MAX];
static UINT08	MAT_20_OSIZE[MAT_20_MAX];
static UINT08	MAT_40_OSIZE[MAT_40_MAX];
#if		(SM_NUM_MATS >= 6)
static UINT08	MAT_80_OSIZE[MAT_80_MAX];
#endif /* SM_NUM_MATS >= 6*/

#else

#define	MAT_04_OSIZE		(UINT08 *)NULL
#define	MAT_08_OSIZE		(UINT08 *)NULL
#define	MAT_10_OSIZE		(UINT08 *)NULL
#define	MAT_20_OSIZE		(UINT08 *)NULL
#define	MAT_40_OSIZE		(UINT08 *)NULL
#define	MAT_80_OSIZE		(UINT08 *)NULL

#endif /* SM_LOG_ORG_SIZE */

/* Level 3 free bit mask	*/
static UINT32	MAT_04_MSK3[(MAT_04_MAX+31)/32];
static UINT32	MAT_08_MSK3[(MAT_08_MAX+31)/32];
static UINT32	MAT_10_MSK3[(MAT_10_MAX+31)/32];
static UINT32	MAT_20_MSK3[(MAT_20_MAX+31)/32];
static UINT32	MAT_40_MSK3[(MAT_40_MAX+31)/32];
#if		(SM_NUM_MATS >= 6)
static UINT32	MAT_80_MSK3[(MAT_80_MAX+31)/32];
#endif /* SM_NUM_MATS >= 6 */

/* Level 2 free bit mask	*/
static UINT32	MAT_04_MSK2[(sizeof(MAT_04_MSK3)/4+31)/32];
static UINT32	MAT_08_MSK2[(sizeof(MAT_08_MSK3)/4+31)/32];
static UINT32	MAT_10_MSK2[(sizeof(MAT_10_MSK3)/4+31)/32];
static UINT32	MAT_20_MSK2[(sizeof(MAT_20_MSK3)/4+31)/32];
static UINT32	MAT_40_MSK2[(sizeof(MAT_40_MSK3)/4+31)/32];
#if		(SM_NUM_MATS >= 6)
static UINT32	MAT_80_MSK2[(sizeof(MAT_80_MSK3)/4+31)/32];
#endif /* SM_NUM_MATS >= 6 */

/* Level 1 free bit mask	*/
static UINT32	MAT_04_MSK1[(sizeof(MAT_04_MSK2)/4+31)/32];
static UINT32	MAT_08_MSK1[(sizeof(MAT_08_MSK2)/4+31)/32];
static UINT32	MAT_10_MSK1[(sizeof(MAT_10_MSK2)/4+31)/32];
static UINT32	MAT_20_MSK1[(sizeof(MAT_20_MSK2)/4+31)/32];
static UINT32	MAT_40_MSK1[(sizeof(MAT_40_MSK2)/4+31)/32];
#if		(SM_NUM_MATS >= 6)
static UINT32	MAT_80_MSK1[(sizeof(MAT_80_MSK2)/4+31)/32];
#endif /* SM_NUM_MATS >= 6 */

/* Level 0 free bit mask	*/
static UINT32	MAT_04_MSK0[(sizeof(MAT_04_MSK1)/4+31)/32];
static UINT32	MAT_08_MSK0[(sizeof(MAT_08_MSK1)/4+31)/32];
static UINT32	MAT_10_MSK0[(sizeof(MAT_10_MSK1)/4+31)/32];
static UINT32	MAT_20_MSK0[(sizeof(MAT_20_MSK1)/4+31)/32];
static UINT32	MAT_40_MSK0[(sizeof(MAT_40_MSK1)/4+31)/32];
#if		(SM_NUM_MATS >= 6)
static UINT32	MAT_80_MSK0[(sizeof(MAT_80_MSK1)/4+31)/32];
#endif /* SM_NUM_MATS >= 6 */

/* List of free bit mask	*/
static UINT32	*MAT_04_MASKS[] = { MAT_04_MSK0, MAT_04_MSK1, MAT_04_MSK2, MAT_04_MSK3 };
static UINT32	*MAT_08_MASKS[] = { MAT_08_MSK0, MAT_08_MSK1, MAT_08_MSK2, MAT_08_MSK3 };
static UINT32	*MAT_10_MASKS[] = { MAT_10_MSK0, MAT_10_MSK1, MAT_10_MSK2, MAT_10_MSK3 };
static UINT32	*MAT_20_MASKS[] = { MAT_20_MSK0, MAT_20_MSK1, MAT_20_MSK2, MAT_20_MSK3 };
static UINT32	*MAT_40_MASKS[] = { MAT_40_MSK0, MAT_40_MSK1, MAT_40_MSK2, MAT_40_MSK3 };
#if		(SM_NUM_MATS >= 6)
static UINT32	*MAT_80_MASKS[] = { MAT_80_MSK0, MAT_80_MSK1, MAT_80_MSK2, MAT_80_MSK3 };
#endif /* SM_NUM_MATS >= 6 */

static UINT08	MAT_SIZ2ENT[SM_MAT_MAX>>2] =
{
//  ~4  ~8 ~12 ~16 ~20 ~24 ~28 ~32 ~36 ~40 ~44 ~48 ~52 ~56 ~60 ~64
	 0,  1,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
#if	 (SM_NUM_MATS >= 6)
	 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5
#endif /* SM_NUM_MATS >= 6 */
};

SM_MAT_POOL_t	SM_MAT_POOL[] =
{
  { 0, MAT_04_MAX, MAT_04_MAX, MAT_04_MASKS, MAT_04_OSIZE },
  { 0, MAT_08_MAX, MAT_08_MAX, MAT_08_MASKS, MAT_08_OSIZE },
  { 0, MAT_10_MAX, MAT_10_MAX, MAT_10_MASKS, MAT_10_OSIZE },
  { 0, MAT_20_MAX, MAT_20_MAX, MAT_20_MASKS, MAT_20_OSIZE },
  { 0, MAT_40_MAX, MAT_40_MAX, MAT_40_MASKS, MAT_40_OSIZE },
#if	 (SM_NUM_MATS >= 6)
  { 0, MAT_80_MAX, MAT_80_MAX, MAT_80_MASKS, MAT_80_OSIZE },
#endif /* SM_NUM_MATS >= 6 */
  { 0, 0,          0,          NULL,         NULL         }
};
#endif	/*	SM_USE_FAST_MAT */


/*=============================================================================
 	Variable to maintain Buddy System.
==============================================================================*/
static UINT32 __RUT[]=				// Round Up Table
{									// NO_LOG_MODE	LOG_MODE(Half size)
	1<<(BDY_BASE_INDEX+ 0),			//	   128b		64b
	1<<(BDY_BASE_INDEX+ 1),			//     256b
	1<<(BDY_BASE_INDEX+ 2),			//     512b
	1<<(BDY_BASE_INDEX+ 3),			//	     1K
	1<<(BDY_BASE_INDEX+ 4),			//	     2K
	1<<(BDY_BASE_INDEX+ 5),			//	     4K
	1<<(BDY_BASE_INDEX+ 6),			//	     8K
	1<<(BDY_BASE_INDEX+ 7),			//	    16K
	1<<(BDY_BASE_INDEX+ 8),			//	    32K
	1<<(BDY_BASE_INDEX+ 9),			//	    64K
	1<<(BDY_BASE_INDEX+10),			//     128K
	1<<(BDY_BASE_INDEX+11),			//     256K
	1<<(BDY_BASE_INDEX+12),			//     512K
	1<<(BDY_BASE_INDEX+13),			//	     1M
	1<<(BDY_BASE_INDEX+14),			//	     2M
	1<<(BDY_BASE_INDEX+15),			//	     4M
	1<<(BDY_BASE_INDEX+16),			//	     8M
	1<<(BDY_BASE_INDEX+17),			//	    16M
	1<<(BDY_BASE_INDEX+18),			//	    32M    16M
	1<<(BDY_BASE_INDEX+19),			//	    64M    32M
	1<<(BDY_BASE_INDEX+20),			//	   128M	   64M
	0xFFFFFFFF						// Upper Limit
};

/* Memory Information per virtual pool	*/
static BuddyHead_t gAllocHdr[SM_VIRT_POOLS];	// Buddy Allocation List Header

/* Memory Information per real pool		*/
static BuddyList_t gFreeHdr[SM_PHYS_POOLS];		// Buddy Free List Header
static BuddyBlks_t gBdyBlks[SM_PHYS_POOLS];		// Num of blocks in buddy
static BuddyHead_t *gPartHdr[SM_PHYS_POOLS];	// External header for partition
static UINT32	   gPartCnt[SM_PHYS_POOLS];		// Number of external partitions.

static UINT32 *gBdyUsed[SM_PHYS_POOLS] = { NULL };	// Used bit for buddy
static UINT32 gPoolBase[SM_PHYS_POOLS] = { 0 };	// Start Address of Memory Pool
static UINT32 gPoolEnd1[SM_PHYS_POOLS] = { 0 };	// End   Address of Memory Pool
static UINT32 gPoolEnd2[SM_PHYS_POOLS] = { 0 };	// End   Address of Memory Pool
static UINT32 initState[SM_PHYS_POOLS] = { 0 };		// Simple Memory is initialized
static UINT32 maxIndex[SM_PHYS_POOLS]  = { 0 };		// Detected maximum index in buddy.
static UINT32 partIndex[SM_PHYS_POOLS] = { 0 };		// Start index of partition
static UINT32 nFreePool[SM_PHYS_POOLS] = { 0 };		// Total size of freemem in Buddy

/* Memory Information per system 		*/
static UINT32 nAllocs    = 0;		/* Total number of Allocations		*/
static UINT32 nFrees     = 0;		/* Total number of Frees			*/
static UINT32 nASrchs    = 0;		/* Total allocation searches		*/
static UINT32 nFSrchs    = 0;		/* Total free searches				*/
static UINT32 nFreeTot   = 0;		/* Total size of free memory		*/
static UINT32 memASize   = 0;		/* Total size of allocated memory	*/
static UINT32 memOSize   = 0;		/* Total size of requested memory	*/



/*-----------------------------------------------------------------------------
	Static 함수와 Global 함수 구현
	(Implementation of static and global functions)
------------------------------------------------------------------------------*/


/*
 *	Begin Mark of functions in osamem.c : used right above
 */
void _OSA_MEM_BEG_(void)
{
	return;
}


/*
 *	Change control flag of do log caller stack trace
 */
void SM_DoLogCaller(int new_value)
{
	if (new_value >= 0)
	{
		do_log_caller = new_value;

		#ifdef	SM_USE_FAST_MAT
		if (initState[SM_SBDY_POOL] == SM_STATE_2)
		{
			if (new_value == 0)
				_sm_allocator[0] = SM_Fast_Alloc;
			else
				_sm_allocator[0] = SM_BuddyAlloc;
		}
		#endif	/* SM_USE_FAST_MAT */
	}
	rprint1n("Memory Caller Tracer is %s", (do_log_caller ? "on" : "off"));
	return;
}

/*
 *	Fast calculate code of get_log2
 */
UINT32 get_log2(UINT32 x)
{
  UINT32 i=0;
  while(x > __RUT[i++])
	;
  return i-1;
}

/*
 *
 */
static void waitOsamemSem(void)
{
	if (!intContext()) atomMutexGet(SmMtx, 0);
}

static void postOsamemSem(void)
{
	if (!intContext()) atomMutexPut(SmMtx);
}

/*
 * Find the pool pointer which allocated memory is included.
 */

static void
SM_MacUpdate (size_t asize, int poolNo, int mode)
{
	mac_log_t	*pMAC;		// Pointer to memory allocation log pool

	if (poolNo < 0)
	{
		poolNo = 2;
		while ((asize - 1) >> poolNo)
			poolNo++;
		poolNo -=  2;
	}

	if (poolNo > 15) poolNo = 15;

	pMAC = &mac_log[poolNo];

	if (mode == 0)			// Update allocation counter
	{
		pMAC->nAlloc++;
		if ((pMAC->nAlloc - pMAC->nFrees) > pMAC->nMax)
		 	pMAC->nMax = pMAC->nAlloc - pMAC->nFrees;
		pMAC->naSize += asize;
	}
	else					// Update free counter
	{
		pMAC->nFrees++;
		pMAC->naSize -= asize;
	}

	return;
}

/*
 *	Initialize Buddy System Parameters
 *		Enter/Exit with 'SmMtx' Locked.
 *  ==============================================================
 *  짜투리 남는 부분을 4킬로 정도로 align 해야 한다.
 *  128 바이트 로 들어와야 align 이 맞는다..
 */
static void
SM_BuddyInit(UINT08 ppid, char *base_addr, size_t pool_size)
{
	BuddyHead_t	*pBdyInit, *pBdyBase;
	UINT32		i, asize, total_blks, bIndex;
	UINT32		nMinSzBlks, nUsedBitSz;

	/* Round up Buddy Base Address by default align value	*/
	gPoolBase[ppid]	 = (((UINT32)base_addr+DEF_AMASK)&~DEF_AMASK);

	/* Decrease pool size by remainder of above round up	*/
	pool_size		-= (gPoolBase[ppid] - (UINT32)base_addr);

	/* Set last address of Buddy Pool						*/
	gPoolEnd2[ppid]  = gPoolBase[ppid] + pool_size;

	/* Set number of blocks can be created in each buddy	*/
	maxIndex[ppid]	 = get_log2(pool_size);
	partIndex[ppid]	 = get_log2(PART_SIZE);
	nMinSzBlks		 = (1 << get_log2(pool_size));
	nUsedBitSz		 = (nMinSzBlks / 8 * 2);

	/* Set pointer for gBdyUsed[] bits and decrease pool size*/
	pool_size		-= nUsedBitSz;
	gBdyUsed[ppid]	 = (UINT32 *)(gPoolBase[ppid] + pool_size);

	/* Set pointer for gPartHdr[] list						*/
	gPartCnt[ppid]	 = (pool_size / PART_SIZE) + 1;
	pool_size		-= OVERHEAD * gPartCnt[ppid];
	pool_size		 = ((pool_size+PART_SIZE-1)&~(PART_SIZE-1));
	gPartHdr[ppid]	 = (BuddyHead_t *)(gPoolBase[ppid] + pool_size);

	/* Set last address of Buddy Pool						*/
	gPoolEnd1[ppid]  = gPoolBase[ppid] + pool_size;
	nMinSzBlks		 = (1 << get_log2(pool_size));

	BDY_PRINTF("");
	BDY_PRINTF("BuddyInit_%d[0x%08x,%08x]", ppid, gPoolBase[ppid], gPoolEnd1[ppid]);

	total_blks = 0;
	for (i = BDY_MIN_INDEX; i <= maxIndex[ppid]; i++)
	{
		gBdyBlks[ppid][i]	 = total_blks;
		total_blks			+= nMinSzBlks;
		nMinSzBlks			/= 2;
		BDY_PRINTF("blks[%2d] = (%07x*%05x) [%08x]", i, __RUT[i], total_blks - gBdyBlks[ppid][i], gBdyBlks[ppid][i]);
	}

	#ifdef	_BUDDY_DEBUG_
	{
		bOffset_t	bOffset;

		BDY_PRINTF("Set Used Bits(%d) as all Buddy is allocated.", nUsedBitSz);
		BDY_PRINTF("Range  of gBdyUsed = %06x-%06x", gBdyUsed[ppid], (UINT32)gBdyUsed[ppid] + nUsedBitSz);
		OFS2BOFS(ppid, bOffset, maxIndex[ppid], pool_size);
		BDY_PRINTF("Offset of last buddy = %d", bOffset.u);
	}
	#endif	/* _BUDDY_DEBUG_ */
	memset((void *)&(gBdyUsed[ppid][0]), 0xFF, nUsedBitSz);

	/* Initialize headers in gPartHdr[] partition list		*/
	BDY_PRINTF("Initializing Partition Headers");
	for (i = 0, pBdyBase = gPartHdr[ppid]; i < gPartCnt[ppid]; i++, pBdyBase++)
	{
		LS_INIT(pBdyBase);
		pBdyBase->bd_flags = SM_FLAG_CHILD;
	}

	bIndex   = get_log2(pool_size);
	asize    = __RUT[bIndex];
	pBdyBase = &gFreeHdr[ppid][bIndex];;
	pBdyInit = Dptr2Hptr(ppid, gPoolBase[ppid]);

	BDY_PRINTF("Insert 0x%08x to buddy[%2d]: asize=%07x", pBdyInit, bIndex, pool_size);
	pBdyInit->bd_asize = pool_size;
	pBdyInit->bd_osize = pool_size;
	pBdyInit->bd_flags = SM_FLAG_FHEAD;
	LS_INS_BEFORE(pBdyBase, pBdyInit)

	nFreeTot        += pool_size;	// Increase Free Memory Size
	nFreePool[ppid] += pool_size;	// Increase Free Buddy Size

	BDY_PRINTF("Buddy Initialzation Done");

	return;
}


/*
 *	Initialize Addresses for SM System
 */
static void
SM_InitPoolSize(MEM_POOL *pMem_pool)
{
	extern char *__bss_end__[];
	char		*tmpPtr      = (char   *)__bss_end__;
	UINT32		*pSymStorage = (UINT32 *)__bss_end__;
	UINT32		tmpSize      = CPU_MEM_SIZE;

	// Copy Symbol table at the end of bss
	if (pSymStorage[0] == 0xB12791EE)
	{
		UINT32	val1 = pSymStorage[2];	/* Total data size				*/
		UINT32	val2 = pSymStorage[3];	/* Number of symbols			*/
		UINT32	val3 = pSymStorage[4];	/* Size of symbol strings table */
		UINT32	val4 = 3*sizeof(int)*val2 + val3;;

		//printf("%d :: %d %d(%d) %x\n", val1, val2, val3, val4, (UINT32)&pSymStorage[0]);

		if (val1 == val4)
		{
			extern UINT32	nTxtSyms;
			extern UINT32	*pSymTabBase;
			extern char		*pSymStrBase;
			extern UINT32	*pSymHashBase;
			extern UINT32	nDwarfLst;
			extern UINT32	*dwarfLst;
			extern char		*pDwarfData;

			nTxtSyms    = val2;
			pSymTabBase = pSymStorage + 5;
			pSymStrBase = (char *)(pSymTabBase + 3 * val2);
			if (*(UINT32 *)pSymStrBase == 0)
			{
				rprint1n("^G^16bit symbol Hash Mode");
				//pSymHashBase = (UINT16 *)((UINT32)pSymStrBase + 4);
				pSymStrBase  = (char   *)(pSymHashBase + ((nTxtSyms + 1) & ~1));
			}
			else if (*(UINT32 *)pSymStrBase == 2)
			{
				rprint1n("^G^32bit symbol Hash Mode");
				pSymHashBase = (UINT32 *)((UINT32)pSymStrBase + 4);
				pSymStrBase  = (char   *)(pSymHashBase + ((nTxtSyms + 1) & ~1));
			}
			if (*(UINT32 *)pSymStrBase == 1)
			{
				nDwarfLst	= ((UINT32 *)pSymStrBase)[1];
				val2		= ((UINT32 *)pSymStrBase)[2];
				dwarfLst	= (UINT32 *)(pSymStrBase + 12);
				pDwarfData	= (char *)(dwarfLst + 2 * nDwarfLst);
				pSymStrBase	= pDwarfData + val2;;
			}

			rprint1n("^B^nTxtSyms     = %d", nTxtSyms);
			rprint1n("^B^pSymTabBase  = [0x%06x..0x%06x)", pSymTabBase,  pSymTabBase + 3 * nTxtSyms);
			rprint1n("^B^pSymHashBase = [0x%06x..0x%06x)", pSymHashBase, pSymTabBase + nTxtSyms);
			rprint1n("^B^pSymStrBase  = [0x%06x..0x%06x)", pSymStrBase,  tmpPtr + val1 + 5 * 4);

			rprint1n("^B^nDwarfLst    = %d", nDwarfLst);
			rprint1n("^B^pDwarfLst    = [0x%06x..0x%06x)", dwarfLst, dwarfLst + 2 * nDwarfLst);
			rprint1n("^B^pDwarfData   = [0x%06x..0x%06x)", pDwarfData, val2);

			tmpPtr += val1 + 5 * 4;
		}
	}

	//	4k Align up start pointer, align down size
	tmpPtr   = (char *)(((UINT32)tmpPtr + 0xfff) & ~0xfff);
	tmpSize -= ((UINT32)tmpPtr - CPU_MEM_BASE);

	pMem_pool->mem_ptr	= tmpPtr;
	pMem_pool->mem_size	= tmpSize;
	pMem_pool->name		= "SYS";
#if 0
	printf("\nend of bss section       : 0x%08x\n", (UINT32)__bss_end__);
	printf(  "memory heap start pointer: 0x%08x\n", (UINT32)tmpPtr);
	printf(  "memory heap size         : 0x%08x\n", (UINT32)tmpSize);
#endif
	return;
}

/*
 *	Initialize SM System, including FAST_MAT and Buddy.
 */
void
SM_InitResource(void)
{
	MEM_POOL	mem_pool;
	UINT08		i, ppid;
	uint8_t		err;

	// Set ranage of memory allocator
	SetAllocatorRange((int)_OSA_MEM_BEG_, (int)_OSA_MEM_END_);

	memset(&mac_log[0], 0x00, sizeof(mac_log_t));

	// Create Semaphore for Memory Allocation System.
	err = atomMutexCreate(SmMtx);

	// Memory system initialize done.
	// Now, set real allocation function and call it.
	_sm_allocator[0] = SM_Fast_Alloc;
	_sm_allocator[1] = SM_BuddyAlloc;
	_sm_allocator[2] = SM_BuddyAlloc;

	BDY_PRINTF("Initializing Buddy Free Headers");
	for (ppid = 0; ppid < SM_PHYS_POOLS; ppid++)
	{
		for (i = BDY_MIN_INDEX; i <= BDY_MAX_INDEX; i++)
		{
			LS_INIT(&gFreeHdr[ppid][i]);
		}
	}

	SM_InitPoolSize(&mem_pool);
	SM_InitPool(SM_SBDY_POOL, &mem_pool);
}

void
SM_InitPool(UINT08 ppid, MEM_POOL *pMem_pool)
{
	void	*addr;		// Address for static allocations.
	size_t	asize;		// Aligned Memory Size
	int		i;

	// Change Internal state as on initializing
	initState[ppid] = SM_STATE_1;

	if (pMem_pool == NULL)
	{
		dbgprint("Initialize Pool : Pool info is not available");
		suspendTask(OSADAP_WAIT_FOREVER);
	}
	// Initialize Addresses and size
	addr     = pMem_pool->mem_ptr;
	asize    = pMem_pool->mem_size;

	if (asize < MAX_MSIZE/4)	/* Minimum Size is 512K */
	{
		pollPrint("Not Enough Memory For Simple Memory System,\n");
		pollPrint("asize 0x%08x < 0x%08x MAX_MSIZE", asize, MAX_MSIZE);
		suspendTask(OSADAP_WAIT_FOREVER);
	}

	#if (SM_FILL_MODE >= 2)
	/* Write Magic Patterns */
	dbgprint(">> Fill Magic Patterns for memory tracer(%d bytes)", asize);
	memset((void *)addr, SM_FILL_MAGIC_B, asize);
	#endif	/* (SM_FILL_MODE >= 2) */

	dbgprint(">> InitPool  ]]  [0x%08x-0x%08x]", addr, addr + asize);
	#ifdef	SM_USE_FAST_MAT
	if (initState[SM_FMAT_POOL] != SM_STATE_2)
	{
		int		i;

		for (i=0; i < SM_NUM_MATS; i++)
		{
			SM_MAT_POOL_t	*pMAT_Pool;
			UINT32	*pMAT_Mask, unitSize, poolSize;
			int		j, numEntry, maxEntry, matLevel;

			pMAT_Pool = &SM_MAT_POOL[i];
			maxEntry  = pMAT_Pool->sm_maxu;
			for (matLevel = MAT_NUM_LEVELS-1; matLevel >= MAT_BASE_LEVEL; matLevel--)
			{
				pMAT_Mask = pMAT_Pool->sm_masks[matLevel];

				// Set free mask bits in each level whose 32 child node are
				// all not allocated.
				for (j = 0; j < (maxEntry / 32); j++)
					pMAT_Mask[j] = 0xFFFFFFFF;

				numEntry = j* 32;
				if (numEntry < maxEntry)
				{
					pMAT_Mask[j] = 0xFFFFFFFF << (32 - (maxEntry - numEntry));
					maxEntry = j + 1;
				}
				else
				{
					maxEntry = j;
				}
			}

			unitSize = (1 << (i+2));
			poolSize = unitSize * pMAT_Pool->sm_maxu;

			pMAT_Pool->sm_base = (UINT32) addr;

			if (asize > poolSize)
			{
				addr  = (void *) ((UINT32) addr + poolSize);
				asize = asize - poolSize;
			}
			else
			{
				pMAT_Pool->sm_maxu = 0;
				pMAT_Pool->sm_free = 0;
			}

			dbgprint("SM_MAT_POOL[%2d] = 0x%08x(0x%06x)", unitSize, pMAT_Pool->sm_base, poolSize);
		}
		SM_MAT_POOL[i].sm_base = (UINT32) addr;

		gPoolBase[SM_FMAT_POOL] = SM_MAT_POOL[0].sm_base;
		gPoolEnd1[SM_FMAT_POOL] = SM_MAT_POOL[i].sm_base;
		gPoolEnd2[SM_FMAT_POOL] = SM_MAT_POOL[i].sm_base;
		nFreeTot     += (gPoolEnd1[SM_FMAT_POOL] - gPoolBase[SM_FMAT_POOL]);
		nFreePool[0] += (gPoolEnd1[SM_FMAT_POOL] - gPoolBase[SM_FMAT_POOL]);
	}
	#endif	/* SM_USE_FAST_MAT */

	initState[SM_FMAT_POOL] = SM_STATE_2;

	dbgprint(">> InitBuddy ]] Addr=0x%06x, Size=0x%06x", addr, asize);

	BDY_PRINTF("Initializing Buddy Allocation Headers");
	for (i = 0; i < SM_VIRT_POOLS; i++)
	{
		LS_INIT(&gAllocHdr[i]);
	}

	SM_BuddyInit(ppid, addr, asize);
	dbgprint(">> Total Free Heap Size = 0x%06x", nFreeTot);

	initState[ppid] = SM_STATE_2;
	nFSrchs   = 0;			// Clear free Search Counter

	return;
}

#if	(SM_FILL_MODE >= 2)
/*
 *	Magic Pattern Checker for malloc()/free()
 */
void
SM_dumpMemoryChunk(char *message, UINT32 uaddr, UINT32 asz)
{
	BuddyHead_t	*pBdy = NULL;
	UINT32		overhead = overheadSize(asz);
	UINT08		ppid;

	if (asz > 0x40) asz = 0x40;

	ppid = findPoolId((void *)uaddr, SM_FIND_PPID);
	if (ppid != SM_FMAT_POOL)
	{
		pBdy = Dptr2Hptr(ppid, uaddr);
		if (pBdy->bd_flags > SM_FLAG_UHEAD)
			pBdy = (BuddyHead_t *)uaddr - 1;
		else
			hexdump("pBdy", pBdy, OVERHEAD);
	}

	uaddr -= overhead;
	asz   += overhead;
	hexdump(message, (void *)uaddr, asz);

	#if	(SM_LOG_ALLOC > 0)
	if ((pBdy != NULL) && (pBdy->bd_mUser[0] > 0))
	{
		rprint1n(">>  Last Allocaton/Free::");
		PrintLoggedFrame(pBdy->bd_mUser, SM_NCALLS);
	}
	#endif	/* SM_LOG_ALLOC */

	rprint1n(">>  Current Requestor  ::");
	PrintStack();
//	SETBRK();

	return;
}

/*
 *	Magic Pattern Checker for malloc()/free()
 */

void
SM_checkMagic(void *vaddr, UINT32 asz, UINT32 osz, int mode)
{
	UINT32	i;
	UINT32	*lp = (UINT32 *)vaddr;
	UINT08	*cp = (UINT08 *)vaddr;

	if (initState[SM_SBDY_POOL] < SM_STATE_2)
		return;

	/*
	 *	Do not check the memory in SDEC GPB
	 *	(It may not be owerwritten after allocation
	 */
	if ((vaddr >= (void *)gPoolBase[SM_SDEC_POOL]) && (vaddr < (void *) gPoolEnd1[SM_SDEC_POOL]))
		return;

	if (mode > 0)
	{
		/*
		 * 	== Called from free()
		 *	Check magic number is remain unchanged until free()
		 *	This means that user may used uninitialized pointer.
		 */
		int		found = 0;

		if (mode < 4) mode = 2;

		#if	(SM_FILL_MODE == 3)
		if ( (asz < 512) && (do_fill_check >= SM_FILL_MODE) )
		{
			UINT16	*sp = (UINT16 *)vaddr;
			UINT32	ePos;

			/*
			 *	 Do not perform uninitializef pointer detection
			 *	for the memory pool with size greater than 512 byte.
			 *	The size of most of C code structure is less 256 bytes
			 */
			for (i = 0; i < (osz & ~(mode-1)); i += mode, lp++, sp++)
			{
				if      ((mode==2) && (*sp==SM_FILL_MAGIC_S)) ePos = (UINT32)sp;
				else if ((mode==4) && (*lp==SM_FILL_MAGIC_L)) ePos = (UINT32)lp;
				else                                          ePos = 0;

				if (ePos)
				{
					dbgprint("^r^free(%x) + 0x%04x :: Memory remain magic(sz=0x%x,0x%x)", vaddr, i, osz, asz);
					if (++found >= 2)
						break;
				}
			}
		}
		#endif	/* SM_FILL_MODE == 3 */

		for (i = osz; i < asz; i++)
		{
			if (cp[i] != SM_FILL_MAGIC_B)
			{
				dbgprint("^r^free(%x) + 0x%04x :: Buffer overrun detected(sz=0x%0x,0x%0x)", vaddr, i, osz, asz);
				if (++found >= 4)
					break;
			}
		}

		if (found)
		{
			SM_dumpMemoryChunk("checkMagic::Free", (UINT32) vaddr, asz);
		}
	}
	else
	{
		/*
		 * 	== Called from malloc()
		 *	Check magic number is has been changed after last free()
		 *	This means that user had access area not to be permitted.
		 */
		for (i = 0; i < asz/4; i++, lp++)
		{
			if (*lp != SM_FILL_MAGIC_L)
			{
				dbgprint("^r^malloc(sz=0x%x,0x%x)=0x%08x :: Memory modified after free", osz, asz, vaddr);
				hexdump("Corrupted", lp, 0x40);
				SM_dumpMemoryChunk("checkMagic::Alloc", (UINT32) vaddr, asz);
				break;
			}
		}
	}
	return;
}

#else

#define	SM_checkMagic(x, y, z, w)

#endif	/* SM_FILL_MODE >= 2 */

/*
 *	Convert caller stack list to string
 */
char *
SM_cstk2Str(UINT32 *pCallers)
{
	#if	(SM_LOG_ALLOC > 0)
	static char	outStr[12*SM_NCALLS];
	char		*cp = &outStr[0];
	int			i = 0;

	for (i = 0; i < (SM_NCALLS - 1); i++, cp += 7)
	{
		sprintf(cp, "%06x/", pCallers[i]);
	}
	sprintf(cp, "%06x", pCallers[i]);
	return(&outStr[0]);
	#else
	return((char *)"");
	#endif /* (SM_LOG_ALLOC > 0) */
}

/*
 *	Replacement of LIBC::malloc()
 */
void *
malloc(size_t osize)
{
	return(mallocByPoolId(osize, SM_SBDY_POOL, 0));
}


/*
 *	Extended malloc()
 */
void *
mallocByPoolId(size_t osize, UINT08 vpid, int where)
{
	void	*addr;
	UINT08	ppid;

	/*
	 * New buddy system do not use internal header
	 * instead, it will used free allocated external header
	if (osize > (MAX_MSIZE - OVERHEAD) )
	 */
	if (osize > MAX_MSIZE)
	{
		dbgprint(">>\tmalloc: request is too big: size=0x%x", osize);
		SHOW_STACK();
		suspendTask(1);
		return(NULL);
	}
	else if (osize == 0)
	{
		return(NULL);
	}

	if      (vpid >= SM_VIRT_POOLS) { vpid = ppid = SM_SBDY_POOL; }
	else if (vpid >= SM_PHYS_POOLS) { ppid = SM_SBDY_POOL;        }
	else if (vpid <= SM_SBDY_POOL ) { ppid = SM_SBDY_POOL;        }
	else                            { ppid = vpid;                }

	if (initState[ppid] == SM_STATE_0)
	{
		if (ppid == SM_SBDY_POOL)
		{
			MEM_POOL mem_pool;

			SM_InitPoolSize(&mem_pool);
			SM_InitPool(ppid, &mem_pool);
			return(_sm_allocator[ALLOCATOR(osize, ppid)](osize, vpid));
		}
		else
		{
			dbgprint("Pool %d(%d) is not initialized...", vpid, ppid);
			#if	0
			suspendTask(OSADAP_WAIT_FOREVER);
			#else
			vpid = SM_PHYS_POOLS;
			ppid = SM_SBDY_POOL;
			#endif
		}
	}

	(void)atomMutexGet(SmMtx, 0);

	addr = _sm_allocator[ALLOCATOR(osize, ppid)](osize, vpid);

	if (addr)
	{
		#if	(SM_FILL_MODE >= 1)
		UINT32	asz = (UINT32)asize(addr);

		SM_checkMagic(addr, asz, osize, 0);

		#if	(SM_FILL_MODE == 3)
		if (where > 0)
		{
			/*
			 * calloc()에 의한 할당인 경우, osize 까지는 0으로 채우고,
			 * 뒤의 짜투리 부분에 있는 magic value는 그대로 둔다.
			 */
			memset((char *)addr, 0, osize);
		}
		#elif (SM_FILL_MODE == 2)
		memset(addr, 0, osize);
		#else
		memset(addr, 0, asz);
		#endif	/* SM_FILL_MODE == 3 */
		#endif	/* SM_FILL_MODE >= 1 */
	}

	(void)atomMutexPut(SmMtx);

	return(addr);

}

/*
 *	Print out error in address range of parameter in free()
 *		Enter/Exit with 'SmMtx' Locked.
 */
static void
SM_printRangeError(void *vaddr)
{
	UINT08	pid;

	(void)atomMutexPut(SmMtx);
	dbgprint("^R^>>\tfree_ERROR_1: Invalid Address(0x%08x)", vaddr);

	for (pid = 0 ; pid < SM_PHYS_POOLS; pid++)
	{
		if (gPoolBase[pid] == 0)
			continue;
		dbgprint(">>\t  Pool Range[%d]: 0x%08x-0x%08x", pid, gPoolBase[pid], gPoolEnd1[pid]);
	}
	SHOW_STACK();

	(void)atomMutexGet(SmMtx, 0);
	return;
}

#ifdef	SM_USE_FAST_MAT
/*
 *	Fast Memory Allocation/Free Using Pre-allocated pool.
 *		vpid was not used.
 *		Enter/Exit with 'SmMtx' Locked.
 */
static void*
SM_Fast_Alloc(size_t osize, UINT08 vpid)
{
	SM_MAT_POOL_t	*pMAT_Pool;
	void	*addr;
	int		matIndex, matLevel;
	int		numEntry;
	UINT32	unitSize, *pMAT_Mask;
	UINT32	leafIndex, leafMask, testMask;

	matIndex  = MAT_SIZ2ENT[(osize-1)>>2];
	pMAT_Pool = &SM_MAT_POOL[matIndex];

	if ( pMAT_Pool->sm_free > 0 )
	{
		unitSize  = (1 << (matIndex+2));
		numEntry  = 0;
		leafIndex = 0;
		for (matLevel = MAT_BASE_LEVEL; matLevel < MAT_NUM_LEVELS;  matLevel++)
		{
			pMAT_Mask = pMAT_Pool->sm_masks[matLevel];

			/* Search entry having at least 1 free unit	*/
			while (pMAT_Mask[numEntry] == 0)
				numEntry++;

			leafMask = pMAT_Mask[numEntry];
			testMask = ((UINT32)1 << 31);
			for (leafIndex=0; leafIndex < 32; leafIndex++)
			{
				if (leafMask & testMask) break;
				testMask >>= 1;
			}

			numEntry  = numEntry * 32 + leafIndex;
		}

		if (numEntry > pMAT_Pool->sm_maxu)
		{
			dbgprint("FMAT broken: numEntry=%d, max=%d", numEntry, pMAT_Pool->sm_maxu);
			return NULL;
		}
		addr = ((void *)(pMAT_Pool->sm_base+numEntry*unitSize));
		if (pMAT_Pool->sm_orgSize != NULL)
			pMAT_Pool->sm_orgSize[numEntry] = osize;

		for (matLevel = MAT_NUM_LEVELS-1; matLevel >= MAT_BASE_LEVEL; matLevel--)
		{
			testMask = (1 << (31 - (numEntry % 32)));
			numEntry = numEntry / 32;

			pMAT_Mask = pMAT_Pool->sm_masks[matLevel];
			pMAT_Mask[numEntry] &= ~testMask;
			if (pMAT_Mask[numEntry] != 0)
				break;
		}

		pMAT_Pool->sm_free--;
		SM_MacUpdate(unitSize, matIndex, 0);
		nFreeTot     -= unitSize;
		nFreePool[0] -= unitSize;

		#if		0
		/* DO NOT LOG FastMat, Deallocator can not know allocator */
		pMyTSV->u.t.ts_muse += unitSize;
		#endif
	}
	else
	{
		//
		//	TODO: How to choose alternative storage on fail to allocate ?
		//		 Current implementation is to choose alternative storage from
		//		buddy system. But, it is possible to choose storage from
		//		larger FAST_MAT. If you enable this feature, It can help
		//		reducing external fragmentation problem on buddy system, but
		//		it cause shortage of another FAST_MAT.
		//		Which one is better ?
		//		God know it.
		//
		addr = SM_BuddyAlloc(osize, SM_FMAT_POOL);
	}

	return(addr);
}

static void
SM_Fast_Free(void *buf, UINT32 caller)
{
	SM_MAT_POOL_t	*pMAT_Pool;
	int		matIndex, matLevel, numEntry;
	UINT32	unitSize, *pMAT_Mask;
	UINT32	leafMask, orgSize;
	UINT32	uaddr = (UINT32)buf;

	// This search should not fail, range was checked already.
	for (matIndex=0; matIndex < SM_NUM_MATS; matIndex++)
	{
		if (uaddr<SM_MAT_POOL[matIndex+1].sm_base)
			break;
	}

	unitSize  = (1 << (matIndex+2));

	if (uaddr & (unitSize - 1))
	{
		dbgprint("^R^>>\tfree_ERROR_2: Invalid Address(0x%08x) @ 0x%08x", uaddr, caller);
		SHOW_STACK();
	}
	else
	{
		pMAT_Pool = &SM_MAT_POOL[matIndex];
		pMAT_Mask = pMAT_Pool->sm_masks[MAT_NUM_LEVELS-1];

		numEntry = (uaddr - (UINT32) pMAT_Pool->sm_base) / unitSize;
		leafMask = (1 << (31 - (numEntry % 32)));

		if (pMAT_Mask[numEntry/32] & leafMask)
		{
			dbgprint("^R^>>\tfree_ERROR[A=0x%08x]: Multiple free @ 0x%08x", uaddr, caller);
			SHOW_STACK();
		}
		else
		{
			if (pMAT_Pool->sm_orgSize != NULL)
				orgSize = pMAT_Pool->sm_orgSize[numEntry];
			else
				orgSize = unitSize;

			for (matLevel = MAT_NUM_LEVELS-1; matLevel >= MAT_BASE_LEVEL; matLevel--)
			{
				leafMask = (1 << (31 - (numEntry % 32)));
				numEntry = (numEntry / 32);

				pMAT_Mask = pMAT_Pool->sm_masks[matLevel];
				pMAT_Mask[numEntry] |= leafMask;
			}

			pMAT_Pool->sm_free++;
			SM_MacUpdate(unitSize, matIndex, 1);
			nFreeTot     += unitSize;
			nFreePool[0] += unitSize;
			#if		0
			/* DO NOT LOG FastMat, Deallocator can not know allocator */
			pMyTSV->u.t.ts_muse -= unitSize;
			#endif

			#if	(SM_FILL_MODE >= 2)
			/* Check Magic Patterns	*/
			SM_checkMagic((void *)uaddr, unitSize, orgSize, 4);
			/* Write Magic Patterns */
			memset((void *)uaddr, SM_FILL_MAGIC_B, unitSize);
			#endif	/* SM_FILL_MODE >= 2 */
		}
	}

	return;
}
#endif	/* SM_USE_FAST_MAT */

/*
 *	Fast Memory Allocation Using Buddy System.
 *		Enter/Exit with 'SmMtx' Locked.
 */
static void *
SM_BuddyAlloc(size_t osize, UINT08 vpid)
{
	BuddyHead_t	*pBdyL = NULL, *pBdyR, *pBdyT;
	BuddyHead_t	*pBdyP = NULL;
	bOffset_t	bOffset;
	UINT32		start, end, asize, exceed;
	osv_t		*pMyOsv = findMyOsvEntry(-1);
	UINT08		ppid;
	UINT32		overhead;

	if (osize == 0)
		return(NULL);

	if      (vpid >= SM_VIRT_POOLS) { vpid = ppid = SM_SBDY_POOL; }
	else if (vpid >= SM_PHYS_POOLS) { ppid = SM_SBDY_POOL;        }
	else if (vpid <= SM_SBDY_POOL ) { ppid = SM_SBDY_POOL;        }
	else                            { ppid = vpid;                }

	overhead = ((osize <= (PART_SIZE - OVERHEAD)) ? OVERHEAD : 0);
	asize	 = osize + overhead;
	start	 = get_log2(asize);

	BDY_PRINTF(" Entering: SM_BuddyAlloc(%d, %d), Start=%d", osize, vpid, start);

	if ((nAllocs % 512) == 0)
	{
		/*	정해진 횟수에 한번씩 free list를 merge 한다.	*/
		BDY_PRINTF("Call SM_BuddyMerge() by periodic");
		(void)SM_BuddyMerge(ppid);
	}

Rescan_FreeList:
	/*
	 *	크기별 Free list 에서 요구된 크기를 저장할 수 있는 최적의
	 *	크기의 메모리를 검색한다.
	 */
	nAllocs++;
	for (end = start; end <= maxIndex[ppid]; end++)
	{
		nASrchs++;
		if (!LS_ISEMPTY(&gFreeHdr[ppid][end]))
		{
			BuddyHead_t	*pBestFit = NULL;

			/*	Check pointer is valid or not		*/
			pBdyL = gFreeHdr[ppid][end].next;
			if (pBdyL->prev != &gFreeHdr[ppid][end])
			{
				dbgprint("SM_alloc_ERROR] Broken Buddy FreeList, ppid=%d, end=%d, osize=%d [%x->%x!=%x]",
								ppid, end, osize, pBdyL, pBdyL->prev, &gFreeHdr[ppid][end]);
//				hexdump("pBdy_Left", (void *)(pBdyL-2),   OVERHEAD * 4);
//				hexdump("BuddyHead", (void *)gFreeHdr[ppid], sizeof(gFreeHdr[ppid]));
				SHOW_STACK();
				LS_INIT(&gFreeHdr[ppid][end]);
				continue;
			}

			if (end >= partIndex[ppid])
			{
				/*
				 *	파티션에 의한 할당인 경우, 최적의 크기의 풀을 검색한다.
				 */
				while (pBdyL != &gFreeHdr[ppid][end])
				{
					if (pBdyL->bd_asize == asize)
					{
						/* This is right matched */
						pBestFit = pBdyL;
						break;
					}
					else if (pBdyL->bd_asize > asize)
					{
						#undef	BEST_FIT
						#ifdef	BEST_FIT
						if ((pBestFit==NULL) || (pBestFit->bd_asize>pBdyL->bd_asize))
						{
							if (pBestFit != NULL)
								dbgprint("Update best fit %d = %d", pBdyL->bd_asize, pBestFit->bd_asize);
							pBestFit = pBdyL;
						}
						#else
						pBestFit = pBdyL;
						#endif
					}
					pBdyL = pBdyL->next;
				}

				if (pBestFit == NULL)
					continue;

				pBdyL = pBestFit;
			}

			LS_REMQUE(pBdyL);
			break;
		}
	}

	if (end > maxIndex[ppid])
	{
		/*
		 * 	TODO: 검색에 실패한 경우, free list를 merge한 후 다시 시도해야 한다.
		 */
		dbgprint("^p^Call SM_BuddyMerge() by memory fragmenation");
		if (SM_BuddyMerge(-1) >= asize)
			goto Rescan_FreeList;

		(void)atomMutexPut(SmMtx);
		dbgprint("No memory available greater than (%d) byte", osize);
		SM_ShowStatus(ppid);	// Print Fast Memory summary.
		SM_BuddyStatus(2);		// Print buddy free list
		SM_PrintBuddy(9);		// Print buddy allocation list ( size >= 64K)
		(void)atomMutexGet(SmMtx, 0);
		return(NULL);
	}

	BDY_PRINTF("   %2d] Free buddy found = 0x%x(%x)", end, pBdyL, (pBdyL ? pBdyL->bd_asize : -1));
	if (end >= partIndex[ppid])
	{
		UINT32	pSize;
		UINT32	dPtr;

		/*
		 *	요구된 크기의 메모리를 할당할 수 있는 파티션이 찾아진 경우,
		 *	Free 메모리를 앞 부분을 할당하고, 뒷부분은 다시 free list에 넣는다.
		 */
		pSize = (asize + PART_SIZE - 1) & ~(PART_SIZE - 1);

		BDY_PRINTF("   %2d] Cut %d byte Part from Header %x(%d)", end, pSize, pBdyL, pBdyL->bd_asize);
		if (pBdyL->bd_asize > pSize)
		{
			dPtr = Hptr2Dptr(ppid, pBdyL) + pSize;
			pBdyR = Dptr2Hptr(ppid, dPtr);

			pBdyR->bd_flags = SM_FLAG_FHEAD;
			pBdyR->bd_asize = pBdyL->bd_asize - pSize;
			pBdyR->bd_osize = pBdyR->bd_asize;
			end = get_log2(pBdyR->bd_asize);
			BDY_PRINTF("   %2d] Add Right Part (%08x, %06x) to FreeList", end, pBdyR, pBdyR->bd_asize);
			LS_INS_BEFORE(&gFreeHdr[ppid][end], pBdyR);

			pBdyL->bd_asize = pSize;
		}
		end = get_log2(pBdyL->bd_asize);

		pBdyP = pBdyL;
		if (start < partIndex[ppid])
		{
			pBdyP->bd_flags = SM_FLAG_BUDDY;
			pBdyP->bd_asize = pSize;
			pBdyP->bd_osize = pSize;
			pBdyL = (BuddyHead_t *)Hptr2Dptr(ppid, pBdyP);
		}
		else
		{
			pBdyP->bd_flags = SM_FLAG_UHEAD;
			pBdyP->bd_asize = pSize;
			pBdyP->bd_osize = asize;
		}
		asize = pSize;
	}

	if (end > start)
	{
		/*
		 *	요구된 크기의 메모리의 크기가 파티션의 크기보다 작으면,
		 *	Buddy 의 단위 크기(128, 256, 512, 1024, 2048)로 나누어 왼쪽 것은
		 *	할당하고, 오른쪽 것은 Free buddy list에 등록한다.
		 */
		OFS2BOFS(ppid, bOffset, end, BUD_OFFSET(ppid, pBdyL));
		SET_USED(ppid, bOffset);
		BDY_PRINTF("   %2d] Mark Left buddy(%08x) as used", end, pBdyL);
		do
		{
			end--;
			asize  = __RUT[end];
			pBdyT  = &gFreeHdr[ppid][end];
			pBdyR  = (BuddyHead_t *) ((UINT32) pBdyL + asize);
			BDY_PRINTF("   %2d] Add Right buddy(%08x, %06x) to FreeList", end, pBdyR, asize);
			OFS2BOFS(ppid, bOffset, end, BUD_OFFSET(ppid, pBdyR));
			CLEAR_USED(ppid, bOffset);
			pBdyL->bd_asize = asize;
			pBdyL->bd_flags = SM_FLAG_BUDDY;
			pBdyR->bd_asize = asize;
			pBdyR->bd_flags = SM_FLAG_BUDDY;
			LS_INS_BEFORE(pBdyT, pBdyR);
		} while ( end > start);
	}

	if (start < partIndex[ppid])
	{
		asize  = __RUT[start];
		OFS2BOFS(ppid, bOffset, start, BUD_OFFSET(ppid, pBdyL));
		SET_USED(ppid, bOffset);
		pBdyL->bd_asize = asize;
		pBdyL->bd_osize = osize + overhead;
		pBdyL->bd_flags = SM_FLAG_BUDDY;
	}

	LS_INS_BEFORE(&gAllocHdr[vpid], pBdyL);

	#if	(SM_LOG_ALLOC > 0)
	pBdyL->bd_lbolt = readMsTicks();

	exceed = osize - asize/2;
	memset(&pBdyL->bd_mUser[0], 0x00, 4*SM_NCALLS);
	if ( ( do_log_caller  !=       0 ) || /* Check logging was enabled		*/
		  #ifdef	SM_USE_FAST_MAT
		 (!vpid && (osize<__RUT[0]/2)) || /* Log FastMAT full				*/
		  #endif	/* SM_USE_FAST_MAT */
		 ( osize          >  63*1024 ) || /* Log allocating more than 64K	*/
		 ((asize>0x800)&&(exceed<128)) )  /* Log having large fragment in it*/
	{
		LOG_CALLER(pBdyL->bd_mUser[0]);
	}
	#endif	/* SM_LOG_ALLOC */

	SM_MacUpdate(asize, -1, 0);
	nFreeTot        -= asize;
	nFreePool[ppid] -= asize;
	memASize		+= asize;
	memOSize		+= (osize + overhead);

	pMyOsv->u.t.ov_mUse += asize;
	pBdyL->bd_task       = (UINT32)pMyOsv->ov_pObj>>5;

	if (start < partIndex[ppid]) pBdyL++;
	else                         pBdyL = (BuddyHead_t *)Hptr2Dptr(ppid, pBdyL);

	BDY_PRINTF(" Leaving : %08x(%d)", pBdyL, osize);

	return((void *)pBdyL);
}


/*
 *	Replacement of LIBC::calloc()
 */
void *
calloc(size_t osize1, size_t osize2)
{
	void	*pointer;
	size_t	size = osize1 * osize2;

	pointer = mallocByPoolId(size, SM_SBDY_POOL, 1);
	#if	(SM_FILL_MODE == 0)
	if (pointer != NULL)
		memset(pointer, 0, size);
	#endif	/* (SM_FILL_MODE == 0) */
	return(pointer);

}


/*
 *	Replacement of LIBC::realloc()
 */
void *
realloc(void *buf, size_t size)
{
	UINT08	vpid;
	UINT32	old_asize;
	void 	*new_buf;

	old_asize = asize(buf);

	vpid = findPoolId(buf, SM_FIND_VPID);

	if (size == 0)
	{
		free(buf);
		buf = NULL;
	}
	else if (old_asize < size)
	{
		new_buf = mallocByPoolId(size, vpid, 0);
		if (new_buf != NULL)
		{
			memcpy(new_buf, buf, old_asize);
			free(buf);
		}
		buf = new_buf;
	}
	return(buf);
}


/*
 *	Replacement of LIBC::msize()
 */
size_t
msize(void *buf)
{
	UINT08	ppid  = findPoolId(buf, SM_FIND_PPID);

	#ifdef	SM_USE_FAST_MAT
	if (ppid == SM_FMAT_POOL)
	{
		int				matIndex, numEntry;
		UINT32			unitSize;
		SM_MAT_POOL_t	*pMAT_Pool;

		// This search should not fail, range was checked already.
		for (matIndex=0; matIndex < SM_NUM_MATS; matIndex++)
		{
			if ((UINT32)buf<SM_MAT_POOL[matIndex+1].sm_base)
				break;
		}

		unitSize  = (1 << (matIndex+2));
		pMAT_Pool = &SM_MAT_POOL[matIndex];

		if (pMAT_Pool->sm_orgSize != NULL)
		{
			numEntry = ((UINT32)buf - pMAT_Pool->sm_base) / unitSize;
			return((size_t)(pMAT_Pool->sm_orgSize[numEntry]));
		}
		else
		{
			return(unitSize);
		}
	}
	else
	#endif	/* SM_USE_FAST_MAT */
	if (ppid == SM_VOID_POOL)
	{
		return 0;
	}
	else
	{
		BuddyHead_t	*pBdy = Dptr2Hptr(ppid, buf);

		if		(pBdy->bd_flags == SM_FLAG_UHEAD)
			return pBdy->bd_osize;
		else if	(pBdy->bd_flags < SM_FLAG_BUDDY)
			return 0;
		else if (((BuddyHead_t *)buf-1)->bd_osize == 0xF1EE00)
			return 0;
		else
			return ((BuddyHead_t *)buf-1)->bd_osize - OVERHEAD;
	}
}


/*
 *	Return the pool size really allocated.
 */
size_t
asize(void *buf)
{
	UINT08		ppid  = findPoolId(buf, SM_FIND_PPID);

	#ifdef	SM_USE_FAST_MAT
	if (ppid == SM_FMAT_POOL)
	{
		int			i;

		for (i=0; i < SM_NUM_MATS; i++)
		{
			if ((UINT32)buf<SM_MAT_POOL[i+1].sm_base)
				return (1 << (i+2));
		}
		return(0);
	}
	else
	#endif	/* SM_USE_FAST_MAT */
	if (ppid == SM_VOID_POOL)
	{
		return 0;
	}
	else
	{
		BuddyHead_t	*pBdy = Dptr2Hptr(ppid, buf);

		if		(pBdy->bd_flags == SM_FLAG_UHEAD)
			return pBdy->bd_asize;
		else if	(pBdy->bd_flags < SM_FLAG_BUDDY)
			return 0;
		else if (((BuddyHead_t *)buf-1)->bd_osize == 0xF1EE00)
			return 0;
		else
			return ((BuddyHead_t *)buf-1)->bd_asize-OVERHEAD;
	}
}

/*
 * Return the sizeof overhead of per allocation
 */
size_t
overheadSize(size_t sz)
{
	#ifdef	SM_USE_FAST_MAT
	if ((sz <= SM_MAT_MAX) || (sz >= PART_SIZE)) return 0;
	else                                         return OVERHEAD;
	#else
	if (sz >= PART_SIZE) return 0;
	else                 return OVERHEAD;
	#endif
}

/*
 * Return the pool ID of the given buffer
 */
UINT08
findPoolId(void *buf, int opt)
{
	UINT08		ppid = SM_VOID_POOL, vpid;
	BuddyHead_t	*pBdyF, *pBdyP;

	/*	First find physical poolid	*/
	for (vpid = 0 ; vpid < SM_PHYS_POOLS; vpid++)
	{
		if ( (buf >= (void *)gPoolBase[vpid]) && (buf < (void *)gPoolEnd2[vpid]) )
		{
			ppid = vpid;
			break;
		}
	}

	if ((opt == SM_FIND_PPID) || (ppid == SM_FMAT_POOL) || (ppid == SM_VOID_POOL))
	{
		return(ppid);
	}

	#if	0
	{
		UINT08		bChk[SM_VIRT_POOLS];

		memset(bChk, 0, SM_VIRT_POOLS);
		bChk[ppid] = 1;
		if (ppid == SM_SBDY_POOL)
		{
			for (vpid = SM_PHYS_POOLS; vpid < SM_VIRT_POOLS; vpid++)
				bChk[vpid] = 1;
		}

		/*	Search allocation list to locate virtual pool id	*/
		for (vpid = SM_SBDY_POOL; vpid < SM_VIRT_POOLS; vpid++)
		{
			if (bChk[vpid] == 0)
				continue;

			pBdyF = pBdyP = (BuddyHead_t *)&gAllocHdr[vpid];

			while (pBdyF->next != pBdyP)
			{
				pBdyF = pBdy->next;
				if		((pBdyF->bd_flags==SM_FLAG_UHEAD) && (Hptr2Dptr(ppid, pBdyF)==(UINT32)buf) )
					return vpid;
				else if ((pBdyF->bd_flags==SM_FLAG_BUDDY) && ((void *)(pBdyF+1)==buf) )
					return vpid;
			}
		}
	}
	#else

	pBdyP = Dptr2Hptr(ppid, buf);
	if (pBdyP->bd_flags <= SM_FLAG_UHEAD) pBdyF = pBdyP;
	else                                  pBdyF = pBdyP = (BuddyHead_t *)buf - 1;

	while (pBdyF->next != pBdyP)
	{
		pBdyF = pBdyF->next;
		if ((pBdyF >= &gAllocHdr[0]) && (pBdyF <= &gAllocHdr[SM_VIRT_POOLS]))
		{
//			dbgprint("pBdy = %x, &gAllocHdr[0] = %x", pBdyF, &gAllocHdr[0]);
			return(pBdyF - &gAllocHdr[0]);
		}
	}
	#endif

	return SM_VOID_POOL;
}


/*
 *	Return true if valid address and allocated.
 */
int
SM_aliveAddr(void *buf)
{
	UINT08		vpid  = findPoolId(buf, SM_FIND_VPID);

	#ifdef	SM_USE_FAST_MAT
	if (vpid == SM_FMAT_POOL)
	{
		int		matIndex, numEntry;
		UINT32	unitSize, leafMask;
		UINT32	uaddr = (UINT32) buf;

		// This search should not fail, range was checked already.
		for (matIndex=0; matIndex < SM_NUM_MATS; matIndex++)
		{
			if (uaddr<SM_MAT_POOL[matIndex+1].sm_base)
				break;
		}

		unitSize  = (1 << (matIndex+2));

		if (!(uaddr & (unitSize - 1)))
		{
			SM_MAT_POOL_t	*pMAT_Pool = &SM_MAT_POOL[matIndex];
			UINT32			*pMAT_Mask = pMAT_Pool->sm_masks[MAT_NUM_LEVELS-1];

			numEntry = (uaddr - (UINT32) pMAT_Pool->sm_base) / unitSize;
			leafMask = (1 << (31 - (numEntry % 32)));

			if (pMAT_Mask[numEntry/32] & leafMask) return(FALSE);	/* Dead */
			else                                   return(TRUE);	/* Live */
		}
		else
		{
			return(FALSE);	/* Dead */
		}
	}
	else
	#endif	/* SM_USE_FAST_MAT */
	if (vpid != SM_VOID_POOL)
	{
		return(TRUE);		/* Live */
	}
	else
	{
		return(FALSE);		/* Dead */
	}
}


/*
 *	Return TRUE if address is in valid buddy or fast_mat
 */
int
SM_aValid(UINT32 uaddr)
{
	return(findPoolId((void *)uaddr, SM_FIND_PPID) != SM_VOID_POOL);
}


/*
 *	Replacement of LIBC::free()
 */
void
free(void *vaddr)
{
	UINT08	ppid;
	UINT32	uaddr = (UINT32) vaddr;
	UINT32	caller;

	str_lr(&caller);		/* Get Link Register */

	if      (uaddr == 0xFFFFFFFF)
	{
		dbgprint("^R^>>\tfree_ERROR: free(-1) @ 0x%08x : may be t_delete()", caller);
		return;
	}
	else if (uaddr == 0x00000000)
	{
		return;				// Just ignore Freeing Nulls
	}

	(void)atomMutexGet(SmMtx, 0);

	ppid = findPoolId(vaddr, SM_FIND_PPID);

	#ifdef	SM_USE_FAST_MAT
	if (ppid == SM_FMAT_POOL)
	{
		SM_Fast_Free(vaddr, caller);
	}
	else
	#endif	/* SM_USE_FAST_MAT */
	if (ppid != SM_VOID_POOL)
	{
		SM_BuddyFree(vaddr, caller);
	}
	else
	{
		SM_printRangeError(vaddr);
	}

	(void)atomMutexPut(SmMtx);
	return;
}


/*
 *	Free all memories having given pool Id
 */
void
freePoolById(UINT08 vpid)
{
	BuddyHead_t	*pBdy, *pBdyBase;

	if ((vpid < SM_PHYS_POOLS) || (vpid >= SM_VIRT_POOLS))
		return;

	waitOsamemSem();

	pBdy = pBdyBase = (BuddyHead_t *)&gAllocHdr[vpid];

	while (pBdy->next != pBdyBase)
	{
		pBdy = pBdy->next;
		free(pBdy+1);

		pBdy = pBdyBase;
	}

	postOsamemSem();

	return;
}


/*
 *	Free functon for memory allocated with SM_BuddyAlloc()
 *		Enter/Exit with 'SmMtx' Locked.
 */
static void
SM_BuddyFree(void *vaddr, UINT32 caller)
{
	BuddyHead_t	*pBdyF;		// Current free pointer;
	BuddyHead_t	*pBdyS;		// Sibling free pointer;
	BuddyHead_t	*pBdyP;		// Current partition pointer
	BuddyHead_t	*pBdyBase;
	bOffset_t	bOffsetF, bOffsetS;
	UINT32		offsetF, offsetS, bIndex, asize, osize, eAddr;
	UINT32		isUsed, asMask, overhead;
	UINT08		ppid, isPart;

	if ((ppid = findPoolId(vaddr, SM_FIND_PPID)) == SM_VOID_POOL)
	{
		SM_printRangeError(vaddr);
		return;
	}

	pBdyP	= Dptr2Hptr(ppid, vaddr);
	isPart	= (pBdyP->bd_flags <= SM_FLAG_UHEAD);
	pBdyF	= (isPart ? pBdyP : (BuddyHead_t *)vaddr - 1);
	eAddr   = (UINT32) pBdyF + pBdyF->bd_asize;
	bIndex  = get_log2(pBdyF->bd_asize);
	asize   = (isPart ? pBdyP->bd_asize : __RUT[bIndex]);
	osize   = pBdyF->bd_osize;

	if (isPart)
	{
		offsetF = offsetS = (UINT32)vaddr - gPoolBase[ppid];
		OFS2BOFS(ppid, bOffsetF, bIndex, offsetF);
		isUsed	= (pBdyP->bd_flags == SM_FLAG_UHEAD);
		asMask	= PART_SIZE - 1;
		overhead= 0;
	}
	else
	{
		offsetF = offsetS = BUD_OFFSET(ppid, pBdyF);
		OFS2BOFS(ppid, bOffsetF, bIndex, offsetF);
		isUsed	= BUD_ISUSED(ppid, bOffsetF);
		asMask	= asize - 1;
		overhead= OVERHEAD;
	}

	if ( (isUsed           ==     0 ) ||	// Used bit must be set.
		#ifndef	SM_LOG_ALLOC
		 !LS_ISEMPTY(pBdyF)		      ||	// Has valid allocated chain ?
		#endif	/* SM_LOG_ALLOC */
		 (offsetS          &  asMask) ||	// Address must be 'asize' aligned
		 (asize            <  osize ) ||	// osize can not greater than asize
		 (pBdyF->bd_asize  != asize )  )		// Is asize field correct ?
	{
		(void)atomMutexPut(SmMtx);
		dbgprint("^R^>>\tfree_ERROR:A=%x(%06x),U=%d,S=%04x/%04x/%04x",
					pBdyF, offsetF, isUsed, osize, asize, pBdyF->bd_asize);
		dbgprint("^R^>>\tfree_ERROR:P=%x,N=%x", pBdyF->prev, pBdyF->next);
		#if	(SM_LOG_ALLOC > 0)
		dbgprint(">>\t  %s at %s", (isUsed ? "Allocated" : "Deallocated"),
					SM_cstk2Str(&pBdyF->bd_mUser[0]));
		#endif	/* SM_LOG_ALLOC */
		if (isPart)
		{
			hexdump("Parts Error, pBdyP ", pBdyP, OVERHEAD);
			hexdump("Parts Error, pData", vaddr, 0x40);
		}
		else
		{
			hexdump("Buddy Error", (void *)((UINT32)pBdyF - 0x20), 0x80);
		}
		SHOW_STACK();
		(void)atomMutexGet(SmMtx, 0);
		return;
	}

	#if	(SM_LOG_ALLOC > 0)
	LS_REMQUE(pBdyF);
	pBdyF->bd_lbolt = readMsTicks();
	#endif	/* SM_LOG_ALLOC */

	nFreeTot        += asize;
	nFreePool[ppid] += asize;

	findMyOsvEntry(pBdyF->bd_task)->u.t.ov_mUse -= asize;

	if (initState[ppid] == SM_STATE_2)
	{
		SM_MacUpdate(asize, -1, 1);
		nFrees   += 1;
		memASize -= asize;
		memOSize -= osize;
	}

	#if	(SM_FILL_MODE >= 2)
	/* Check Magic Patterns	*/
	SM_checkMagic(vaddr, pBdyF->bd_asize-overhead, pBdyF->bd_osize-overhead, 4);
	#endif	/* SM_FILL_MODE == 3 */

	#if	(SM_LOG_ALLOC > 0)
	if (do_log_caller) LOG_CALLER(pBdyF->bd_mUser[0]);
	#endif	/* SM_LOG_ALLOC */

	pBdyF->bd_osize = 0xF1EE00;
	while (bIndex < partIndex[ppid])
	{
		nFSrchs++;				// Increment Free Search Counter
		offsetF = BUD_OFFSET(ppid, pBdyF);
		offsetS = (offsetF ^ __RUT[bIndex]);
		OFS2BOFS(ppid, bOffsetF, bIndex, offsetF);
		OFS2BOFS(ppid, bOffsetS, bIndex, offsetS);
		pBdyS   = (BuddyHead_t *)(gPoolBase[ppid] + offsetS);
		pBdyBase= &gFreeHdr[ppid][bIndex];

		if (((UINT32)pBdyS <  gPoolEnd1[ppid]) &&
			(BUD_ISUSED(ppid, bOffsetS) == 0) &&
			(bIndex < maxIndex[ppid]        ) )
		{
			SET_USED(ppid, bOffsetS);
			LS_REMQUE(pBdyS);
			if (pBdyS < pBdyF)
			{
				pBdyF = pBdyS;
				pBdyF->bd_osize = 0xF1EE00;
			}
			bIndex++;
			pBdyF->bd_asize = __RUT[bIndex];
			BDY_PRINTF("   %2d] Merging   Buddy(%08x, %06x)", bIndex, pBdyF, pBdyF->bd_asize);
			continue;
		}
		else
		{
			CLEAR_USED(ppid, bOffsetF);
			BDY_PRINTF("   %2d] Add Freed Buddy(%08x, %06x)", bIndex, pBdyF, pBdyF->bd_asize);
			LS_INS_BEFORE(pBdyBase, pBdyF)
			pBdyF->bd_flags = SM_FLAG_BUDDY;
			#if (SM_FILL_MODE >= 2)
			memset((void *) (pBdyF+1), SM_FILL_MAGIC_B, pBdyF->bd_asize-OVERHEAD);
			#endif	/* (SM_FILL_MODE >= 2) */
			break;
		}
	}

	if (bIndex >= partIndex[ppid])
	{
		BDY_PRINTF("   %2d] Add Freed Parts(%08x, %06x)", bIndex, pBdyP, pBdyP->bd_asize);
		pBdyBase= &gFreeHdr[ppid][bIndex];
		LS_INS_BEFORE(pBdyBase, pBdyP)
		pBdyP->bd_flags = SM_FLAG_FHEAD;
		#if (SM_FILL_MODE >= 2)
		memset((void *)Hptr2Dptr(ppid, pBdyP), SM_FILL_MAGIC_B, pBdyP->bd_asize);
		#endif	/* (SM_FILL_MODE >= 2) */
	}
}

/*
 *	Merge buddy free lists
 */
int
SM_BuddyMerge(int ppid)
{
	int			index, bIndex, maxMerged = 0;
	UINT08		sppid, eppid;
	BuddyHead_t	*pBdy, *pBdyBase, *pBdy1, *pBdy2;

	if ( (ppid < 0) || (ppid >= SM_PHYS_POOLS) )
	{
		sppid = SM_FMAT_POOL;
		eppid = SM_SDEC_POOL;
	}
	else
	{
		sppid = eppid = ppid;
	}

//	dbgprint("SM_BuddyMerge(%d)", ppid);

	(void)atomMutexGet(SmMtx, 0);

	for (ppid = sppid; ppid <= eppid; ppid++)
	{
		for (index = partIndex[ppid]; index < maxIndex[ppid]; index++)
		{
			pBdyBase = (BuddyHead_t *)&gFreeHdr[ppid][index];
			if (LS_ISEMPTY(pBdyBase))
				continue;

			pBdy = pBdyBase;

			while (pBdy->next != pBdyBase)
			{
				pBdy1 = pBdy->next;
				pBdy2 = pBdy1 + (pBdy1->bd_asize / PART_SIZE);

				if (pBdy2->bd_flags == SM_FLAG_FHEAD)
				{
					#if 0
					dbgprint("^p^ == Merging %x(%x+%06x) and %x(%x+%06x), idx %d->%d",
							pBdy1, Hptr2Dptr(ppid, pBdy1), pBdy1->bd_asize,
							pBdy2, Hptr2Dptr(ppid, pBdy2), pBdy2->bd_asize,
							index, get_log2(pBdy1->bd_asize + pBdy2->bd_asize));
					#endif

					LS_REMQUE(pBdy2);

					pBdy1->bd_asize	+= pBdy2->bd_asize;
					pBdy1->bd_osize	 = pBdy1->bd_asize;

					pBdy2->bd_flags	 = SM_FLAG_CHILD;

					if (pBdy1->bd_asize > maxMerged)
						maxMerged = pBdy1->bd_asize;

					bIndex = get_log2(pBdy1->bd_asize);
					if (bIndex != index)
					{
						LS_REMQUE(pBdy1);
						LS_INS_BEFORE(&gFreeHdr[ppid][bIndex], pBdy1);
					}
					pBdy = pBdyBase;
				}
				else
				{
					pBdy = pBdy->next;
				}
			}
		}
	}

	(void)atomMutexPut(SmMtx);

//	dbgprint("SM_BuddyMerge(%d), max %d byte part merged", ppid, maxMerged);

	return(maxMerged);
}

UINT32
SM_BuddyStatus(int printOn)
{
	UINT08		ppid;
	int			i, count, tSize1, tSize2, isUsed;
	BuddyHead_t	*pBdy, *pBdyBase;
	bOffset_t	bOffset;

	printOn = (printOn < 0) ? 0 : printOn;

	waitOsamemSem();
	for (ppid = SM_SBDY_POOL ; ppid < SM_PHYS_POOLS ; ppid++)
	{
		if (printOn)
			dbgprint("PRINTING Buddy Free List(pool %d)", ppid);

		tSize1 = 0;
		if (gFreeHdr[ppid] == NULL)
			continue;
		for (i = BDY_MIN_INDEX; i <= maxIndex[ppid]; i++)
		{
			pBdyBase = (BuddyHead_t *)&gFreeHdr[ppid][i];
			if (LS_ISEMPTY(pBdyBase))
				continue;
			tSize2 = 0;
			count  = 0;
			pBdy   = pBdyBase;
			if (printOn)
			{
				if (__RUT[i] < 1024)
					tprint0n("\t[Buddy_%02d:%5dB]=>", i, __RUT[i]);
				else
					tprint0n("\t[Buddy_%02d:%5dK]=>", i, __RUT[i]/1024);
			}
			while (pBdy->next != pBdyBase)
			{
				pBdy   = pBdy->next;
				if (pBdy->bd_flags <= SM_FLAG_UHEAD)
				{
					isUsed = (pBdy->bd_flags == SM_FLAG_UHEAD);
				}
				else
				{
					OFS2BOFS(ppid, bOffset, i, BUD_OFFSET(ppid, pBdy));
					isUsed = BUD_ISUSED(ppid, bOffset);
				}
				if ((pBdy->bd_asize <= __RUT[i]/2) || (pBdy->bd_asize > __RUT[i]) || isUsed)
					rprint0n("ERR:%d,", isUsed);
				if (printOn > 1)
					rprint0n("%08x=>", pBdy);
				tSize1 += pBdy->bd_asize;
				tSize2 += pBdy->bd_asize;
				count++;
			}
			if (printOn)
				rprint1n("Size = %x/%x, count=%d", tSize1, tSize2, count);
		}
	}
	postOsamemSem();
	return(tSize1);
}

/*
 *	Print a internal structure in buddy system.
 */
void
SM_PrintRecord(void *vaddr)
{
	UINT08		ppid = findPoolId(vaddr, SM_FIND_PPID);
	BuddyHead_t	*pBdy;

	if ( (ppid != SM_FMAT_POOL) && (ppid != SM_VOID_POOL) )
	{
		UINT32		bIndex;
		bOffset_t	bOffset;

		pBdy = (BuddyHead_t *)vaddr - 1;
		bIndex = get_log2(pBdy->bd_asize);
		OFS2BOFS(ppid, bOffset, bIndex, BUD_OFFSET(ppid, pBdy));

		hexdump("a Buddy", pBdy, 0x80);

		dbgprint("[0x%08x] <==> [0x%08x] <==> [0x%08x]", pBdy->prev, pBdy, pBdy->next);
		dbgprint("    aSize = %d, oSize = %d, used = %d",
					pBdy->bd_asize, pBdy->bd_osize, BUD_ISUSED(ppid, bOffset));

		#if	(SM_LOG_ALLOC > 0)
		PrintLoggedFrame(pBdy->bd_mUser, SM_NCALLS);
		#endif	/* SM_LOG_ALLOC */
	}
	else
	{
		dbgprint("[0x%06x] not is in buddy memory", vaddr);
	}
}

/*
 *	Print allcation list of buddy
 *		0  : Just return total free size of buddy
 *		1  : + print number of freelist and size
 *		0  : + entire free list
 */
void
SM_PrintBuddy(int index)
{
	UINT08		vpid, ppid;
	int			count, printSize;
	BuddyHead_t	*pBdy, *pBdyBase;

	rprint1n("PRINTING Buddy Allocation List");

	waitOsamemSem();

	count     = 0;
	printSize = ((index < 0) ? 0 : __RUT[index]);

	for (vpid = SM_SBDY_POOL; vpid < SM_VIRT_POOLS; vpid++)
	{
		pBdy = pBdyBase = (BuddyHead_t *)&gAllocHdr[vpid];

		while (pBdy->next != pBdyBase)
		{
			ppid = findPoolId(pBdy->next, SM_FIND_PPID);
			if ((pBdy->next < (BuddyHead_t *)gPoolBase[ppid]) || (pBdy->next >= (gPartHdr[ppid]+gPartCnt[ppid])))
			{
				hexdump("pBdy", pBdy-2, OVERHEAD * 4);
				break;
			}

			pBdy   = pBdy->next;
			if ((printSize == 0) || (pBdy->bd_asize >= printSize))
			{
				UINT32	pData = (UINT32)((pBdy->bd_flags == SM_FLAG_UHEAD) ? Hptr2Dptr(ppid, pBdy) : (UINT32)(pBdy+1));

				rprint1n("%3d] %07x(%07x):%7d(%7d), %d, %s", vpid, pBdy, pData,
						pBdy->bd_asize, pBdy->bd_osize, pBdy->bd_flags, SM_cstk2Str(pBdy->bd_mUser));
				count++;
			}
		}
	}

	rprint1n("Total Allocations = %d", count-1);

	postOsamemSem();

	return;
}

/*
 *	Print Summary of Simple Memory allocations.
 */
void
SM_ShowStatus(int opt)
{
	mac_log_t	*pMAC;		// Pointer to memory allocation log pool
	int			i, j;
	int			memSz, totSz = 0;
	char		szStr1[16], szStr2[16];

	dbgprint(">>  ======================================================");
	dbgprint(">>  SM] Total Free  = %06x(%8d)",  nFreeTot, nFreeTot);
	dbgprint(">>  SM] SM_FAST_FAT = %06x(%8d), [%07x .. %07x)", nFreePool[0], nFreePool[0], gPoolBase[0], gPoolEnd1[0]);
	dbgprint(">>  SM] SystemBuddy = %06x(%8d), [%07x .. %07x)", nFreePool[1], nFreePool[1], gPoolBase[1], gPoolEnd1[1]);
	dbgprint(">>  SM] SDEC  Buddy = %06x(%8d), [%07x .. %07x)", nFreePool[2], nFreePool[2], gPoolBase[2], gPoolEnd1[2]);
	dbgprint(">>  SM] Allocations = %5d/%5d(=%5.2f)",
				  nASrchs, nAllocs, (nAllocs ? (0.0+nASrchs)/nAllocs: 0.0));
	dbgprint(">>  SM] Frees       = %5d/%5d(=%5.2f)",
				  nFSrchs, nFrees,  (nFrees  ? (0.0+nFSrchs)/nFrees : 0.0));

	dbgprint(">>  MemSize nAlloc  nFree  nA-nF mxUnit maxReq totalSz");
	for (i = 0; i < 16; i++)
	{
		pMAC = &mac_log[i];
		if (i < 15)
		{
			memSz = (1 << (i+2));
			if (memSz < 1024) sprintf(szStr1, "<=%3dB", memSz);
			else              sprintf(szStr1, "<=%3dK", memSz/1024);
		}
		else
		{
			memSz = 1;
			sprintf(szStr1, ">= 64K");
		}

		totSz += pMAC->naSize;
		#ifdef	SM_USE_FAST_MAT
		if ((memSz == 1) || (memSz == SM_MAT_MAX) )
		#else
		if (memSz == 1)
		#endif
			sprintf(szStr2, "%8d", totSz), totSz = 0;
		else
			szStr2[0] = '\0';

		#ifdef	SM_USE_FAST_MAT
		/* Currently active allocations	in Fast Memory Structure	*/
		// j = ( (i < SM_NUM_MATS)	? (SM_MAT_TPTR[i][0]-SM_MAT_TPTR[i][1]) : 0);
		/* Maximum number of units in Fase Memory Structure			*/
		j = ( (i < SM_NUM_MATS) ? SM_MAT_POOL[i].sm_maxu : 0);
		#else
		j = 0;
		#endif	/* SM_USE_FAST_MAT */

		dbgprint(">>   %s %6d %6d %6d(%5d) %6d %7d%s",
				szStr1, pMAC->nAlloc , pMAC->nFrees, pMAC->nAlloc-pMAC->nFrees,
				j, pMAC->nMax, pMAC->naSize, szStr2);
	}

	// SM_BuddyStatus(1);

	#if	(SM_LOG_ALLOC > 0)
	if ((sms_lbolt1 != 0) && (sms_lbolt2 != 0))
	{
		BuddyHead_t	*pBdy, *pBdyBase;
		UINT32		i, vpid, statIdx, nCallers;
		UINT32		mUsers[SM_NTRACES][SM_NCALLS];
		UINT32		mStats[SM_NTRACES][3];
		UINT32		LostMem;

		waitOsamemSem();

		statIdx = nCallers = LostMem = 0;
		memset(&mUsers[0][0], 0x00, sizeof(mUsers));
		memset(&mStats[0][0], 0x00, sizeof(mStats));

		for (vpid = SM_SBDY_POOL; vpid < SM_VIRT_POOLS; vpid++)
		{
			pBdy = pBdyBase = &gAllocHdr[vpid];
			while (pBdy->next != pBdyBase)
			{
				pBdy = pBdy->next;
				if ((pBdy->bd_lbolt>=sms_lbolt1) && (pBdy->bd_lbolt<sms_lbolt2))
				{
					LostMem += pBdy->bd_asize;
					for (i = 0; i < statIdx; i++)
					{
						// Check bd_mUser whether have been registered.
						if (memcmp(&mUsers[i],&pBdy->bd_mUser[0],4*SM_NCALLS)==0)
						{
							mStats[i][1] += 1;
							mStats[i][2] += pBdy->bd_osize;
							break;
						}
					}

					if     ( i < statIdx )		// Found in previous trace slot.
					{
						// pBdy->bd_lbolt = prev_lbolt;	// Mark as notified.
					}
					else if (statIdx < SM_NTRACES)	// New trace slot
					{
						// pBdy->bd_lbolt = prev_lbolt;	// Mark as notified.
						memcpy(&mUsers[statIdx], &pBdy->bd_mUser[0], 4*SM_NCALLS);
						mStats[statIdx][0] = pBdy->bd_asize;
						mStats[statIdx][1] = 1;
						mStats[statIdx][2] = pBdy->bd_osize;
						statIdx++;
						nCallers++;
					}
					else
					{
						nCallers++;
					}
				}
			}
		}

		if (statIdx)
			dbgprint(">>    SMLOG   ]       aSize/Cnt/oSize]");

		for (i = 0; i < statIdx; i++)
		{
			dbgprint(">>    SMLOG_%2d]Calls=0x%04x,%3d,%5d] <- %s", i,
					mStats[i][0], mStats[i][1], mStats[i][2],
					SM_cstk2Str(mUsers[i]));
		}

		postOsamemSem();
		dbgprint(">>  SM] LostMemory = %06x (%d Callers)", LostMem, nCallers);
	}
	#endif	/* SM_LOG_ALLOC */
	dbgprint(">>  SM] Asize/Osize= %06d/%06d", memASize, memOSize);
}


/*
 *	Print Summary of Simple Memory allocations.
 */
void
SM_MarkRange(void)
{
	static int	sms_toggle = 0;

	if      (sms_toggle == 0) sms_lbolt1 = readMsTicks();
	else if (sms_toggle == 1) sms_lbolt2 = readMsTicks();
	else if (sms_toggle == 2) sms_lbolt1 = sms_lbolt2 = 0;
	sms_toggle = ((sms_toggle + 1) % 3);

	dbgprint("Setting Marker[%6d-%6d]", sms_lbolt1, sms_lbolt2);
	return;
}


/*
 *	End Mark of functions in osamem.c : used right above
 */
void _OSA_MEM_END_(void)
{
	return;
}


/*
 *	Test functions in this module
 */

void
SM_TestFuncs(void)
{
	int		i;
	size_t	sizeList[] = { 10, 250, 4100, 20, 130, 9000, 0x81000, 0 }, sz;
	char	*x;

	dbgprint("==========================================================");
	dbgprint("Test malloc[1]: Address(text area, SM_TestFuncs)");
	free((void *)SM_TestFuncs);

	dbgprint("Test malloc[2]: Address(Invalid area, 0xffff0000)");
	free((void *)0xffff0000);

	dbgprint("==========================================================");
	for (i = 0; i < 3; i++)
	{
		sz = sizeList[i];

		dbgprint("Test malloc[%4d]: Uninitialized pointer in C STRUCT", sz);
		x = (char *)malloc(sz);
		free(x);
		dbgprint("Test malloc[%4d]: Buffer overrun", sz);
		x = (char *)malloc(sz);
		memset(x, 0, sz+1);
		free(x);
	}

	dbgprint("==========================================================");
	dbgprint("Test modified after free");
	x = malloc(128); free(x);
	x[10] = 0;
	x = malloc(128); free(x);

	dbgprint("==========================================================");
	dbgprint(" pointer     size oh   osize   asize valid alive ppid vpid");
	dbgprint("----------------------------------------------------------");
	for (i = 0; sizeList[i] != 0; i++)
	{
		static char	fmt[] = " %08x %7d %2d %7d %7d %5d %5d %4d %4d";
		if (i == 0)
		{
			x = (char *)0x9999;
			dbgprint(fmt,
						x, 1, overheadSize(1), msize(x), asize(x),
						SM_aValid((UINT32)x),
						SM_aliveAddr(x),
						findPoolId((void *)x, SM_FIND_PPID),
						findPoolId((void *)x, SM_FIND_VPID));
		}
		sz = sizeList[i];
		x  = mallocByPoolId(sz, i, 0);

		dbgprint(fmt,
					x, sz, overheadSize(sz), msize(x), asize(x),
					SM_aValid((UINT32)x),
					SM_aliveAddr(x),
					findPoolId((void *)x, SM_FIND_PPID),
					findPoolId((void *)x, SM_FIND_VPID));
		memset(x, 0, sz);
		free(x);
		dbgprint(fmt,
					x, sz, overheadSize(sz), msize(x), asize(x),
					SM_aValid((UINT32)x),
					SM_aliveAddr(x),
					findPoolId((void *)x, SM_FIND_PPID),
					findPoolId((void *)x, SM_FIND_VPID));
	}

	dbgprint("==========================================================");

	return;
}
