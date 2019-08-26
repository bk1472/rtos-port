#ifndef __EXCEPTION_HEADER__
#define __EXCEPTION_HEADER__

#ifdef __cplusplus
extern "C" {
#endif
#include <bsp_arch.h>

#define NUM_EXC_VECS				8
#define EXC_VEC_TABLE_BASE			0x20

#define EXC_OFF_RESET				0x00	/* reset */
#define EXC_OFF_UNDEF				0x04	/* undefined instruction */
#define EXC_OFF_SWI					0x08	/* software interrupt */
#define EXC_OFF_PREFETCH			0x0c	/* prefetch abort */
#define EXC_OFF_DATA				0x10	/* data abort */
#define EXC_OFF_RSVD				0x14	/* reserved */
#define EXC_OFF_IRQ					0x18	/* interrupt */
#define EXC_OFF_FIQ					0x1c	/* fast interrupt */

/* Exception Vector Base Address */
#define ARM_EXC_VEC_BASE			0x00

#define EXC_RESET_CODE				0xE7FDDEFE
#define EXC_BRKPNT_CODE				0xE7FDDEDD

#define NUM_EXC_VECS				8
#define NUM_CHANGABLE_EXC_VECS		5
#define EXC_VEC_TABLE_BASE			0x20
/* exception valid bit mask */
#define EXC_INFO_VECADDR			0x01	/* vector is valid   */
#define EXC_INFO_PC					0x02	/* PC is valid       */
#define EXC_INFO_CPSR				0x04	/* ARM CPSR is valid */

#define EXC_ACEPT_MAX_COUNT			3

#ifndef __ASSEMBLY__

#include <common.h>

typedef struct excTbl
{
	UINT32			vecAddr;
	pVOIDFUNCPTR	fn;
} EXC_TBL;

typedef struct
{
	INSTR	*pc;			/* program counter                     */
	UINT32	cpsr;			/* current PSR                         */
	UINT32	vecAddr;		/* address of exception vector => type */
} ESF;

typedef struct
{
	UINT32	r[GREG_NUM];	/* general purpose register r0 - r14   */
	INSTR	*pc;			/* program counter                     */
	UINT32	cpsr;           /* current PSR                         */
} REG_SET;

typedef struct
{
	UINT32	valid;			/* indicators that following fields are valid */
	UINT32	vecAddr;        /* exception vector address                   */
	INSTR	*pc;            /* program counter                            */
	UINT32	cpsr;           /* current PSR                                */
} EXC_INFO;


typedef int		(*PUCOS_EXC_HANDLE)			(int vect, void *sp, REG_SET *pRegs, EXC_INFO *pExcInfo);
typedef void	(*PUCOS_EXC_RESET)			(void);

/* External Function Description */
extern void		armInitExceptionModes		(void);
extern int		exception_vector_init		(void);
extern void		excEnterReset				(void);
extern void		excEnterSwi					(void);
extern void		excEnterUndef				(void);
extern void		excEnterPrefetchAbort		(void);
extern void		excEnterDataAbort			(void);
extern void		excExcContinue				(ESF *pEsf, REG_SET *pRegs);

extern volatile PUCOS_EXC_HANDLE			atomExcHandle;
extern volatile PUCOS_EXC_RESET				atomResetHandle;

#endif/*__ASSEMBLY__*/

#ifdef __cplusplus
}
#endif

#endif/*__EXCEPTION_HEADER__*/
