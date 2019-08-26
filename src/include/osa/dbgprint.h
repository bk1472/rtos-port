#ifndef __DBG_PRINT_HEADER__
#define __DBG_PRINT_HEADER__

#include <string.h>
#include <stdarg.h>
#include <osa/osa_types.h>

extern int	gDebugEnable;
extern char	gDebugCtrl[16];

#ifdef __cplusplus
extern "C" {
#endif

#define	_FDEFINE_		(0)
#define	_FINLINE_		(1)
#define	_FGLOBAL_		(2)

#define	NULL_PRINT		_FDEFINE_
		/*				_FDEFINE_		Remove by defining as NULL		*/
		/*				_FINLINE_		Use inline null print function	*/
		/*				_FGLOBAL_		Use global null print function	*/

/*		Global Usage						*/
void	dbgRegisterName(ULONG, char*, UCHAR);
void*	dbgFindTaskBySP(ULONG);
void	hexdump(char*, void *, int);
void	HEXDUMP(char*, void *, int);

/*		Tagged(Named) print via t_ident()			*/
int     dbgprint(const char*, ... );
int     tprint0n(const char*, ... );
int     tprint1n(const char*, ... );

#if		(NULL_PRINT==_FDEFINE_)
#define	dbgprintNull
#elif	(NULL_PRINT==_FGLOBAL_)
int                dbgprintNull(const char*, ... );
#else	/* NULL_PRINT == _FINLINE_			*/
inline static void dbgprintNull(const char*, ... ) {};
#endif

/*		Raw(Non-Named) print 					*/
int		DBGprint(const char *, ...);
int		rprint0n(const char *, ...);
int		rprint1n(const char *, ...);

/*		Trace function calls  					*/
void	StackTrace(UINT32 size, UINT32 count, UINT32 *res, UINT32 *pStack, int taskId);
void	PrintStack(void);
void	PrintLoggedFrame(UINT32 *pFrame, UINT32 count);
unsigned long calccrc32(unsigned char *buf, int len);

#ifdef __cplusplus
}
#endif

#endif/*__DBG_PRINT_HEADER__*/
