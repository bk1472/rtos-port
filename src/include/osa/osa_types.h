#ifndef __OSA_TYPES_HEADER__
#define __OSA_TYPES_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#undef	FALSE
#undef	TRUE

#define NULL_POINTER	((void*)0)

#ifndef NULL
#define NULL			NULL_POINTER
#endif

#define MAX(_a, _b)		(((_a) > (_b)) ? (_a) : (_b))
#define MIN(_a, _b)		(((_a) < (_b)) ? (_a) : (_b))
#define ABS(_a)			(((_a) >  0  ) ? (_a) : (-(_a)))
#define SGN(_a)			(((_a)>0)?(1) : (((_a)<0)?(-1) : (0)))

#define DIV(_a,_b)		(((_a)+(_b)-1)/(_b))
#define ALIGN_UP(_a,_u)	( DIV((_a),(_u)) * (_u) )
#define ALIGN_DN(_a,_u)	( ((_a) / (_u) ) * (_u) )

#ifndef EOS
#define EOS				'\0'
#endif/*EOS*/

#ifndef TAB
#define TAB				'\t'
#endif/*TAB*/

/**
 * type definitions of character (8 bits width)
 */
#ifndef __SCHAR
#define __SCHAR				1
typedef signed char		SCHAR;
#endif/*__SCHAR*/

#ifndef __UCHAR
#define __UCHAR				1
typedef unsigned char	UCHAR;
#endif/*__UCHAR*/

#ifndef __SINT08
#define __SINT08			1
typedef signed char		SINT08;
#endif/*__SINT08*/

#ifndef __UINT08
#define __UINT08			1
typedef unsigned char	UINT08;
#endif/*__UINT08*/

/**
 * type definitions of short (16 bits width)
 */
#ifndef __SSHORT
#define __SSHORT			1
typedef signed short	SSHORT;
#endif/*__SSHORT*/

#ifndef __USHORT
#define __USHORT			1
typedef unsigned short	USHORT;
#endif/*__USHORT*/

#ifndef __SINT16
#define __SINT16			1
typedef signed short	SINT16;
#endif/*__SINT16*/

#ifndef __UINT16
#define __UINT16			1
typedef unsigned short	UINT16;
#endif/*__UINT16*/

/**
 * type definitions of integer (32 bits width)
 */
#ifndef __SINT
#define __SINT				1
typedef signed int		SINT;
#endif/*__SINT*/

#ifndef __UINT
#define __UINT				1
typedef unsigned int	UINT;
#endif/*__UINT*/

#ifndef __SINT32
#define __SINT32			1
typedef signed int		SINT32;
#endif/*__SINT32*/

#ifndef __UINT32
#define __UINT32			1
typedef unsigned int	UINT32;
#endif/*__UINT32*/

/**
 * type definitions of long integer (32 bits width)
 */
#ifndef __SLONG
#define __SLONG				1
typedef signed long		SLONG;
#endif/*__SLONG*/

#ifndef __ULONG
#define __ULONG				1
typedef unsigned long	ULONG;
#endif/*__ULONG*/

/**
 * type definitions of long long integer (64 bits width)
 */
#ifndef __SLLONG
#define __SLLONG				1
typedef signed long long	SLLONG;
#endif/*__SLLONG*/

#ifndef __ULLONG
#define __ULLONG				1
typedef unsigned long long	ULLONG;
#endif/*__ULLONG*/

#ifndef __SINT64
#define __SINT64				1
typedef signed long long	SINT64;
#endif/*__SINT64*/

#ifndef __UINT64
#define __UINT64				1
typedef unsigned long long	UINT64;
#endif/*__UINT64*/

/**
 * type definitions of misc
 */
#ifndef	__byte
#define	__byte				1
typedef unsigned char 	byte;
#endif	/* __byte	*/
#ifndef	__word
#define	__word				1
typedef unsigned short 	word;
#endif	/* __word	*/
#ifndef	__dword
#define	__dword				1
typedef unsigned long 	dword;
#endif	/* __dword	*/
#ifndef	__ASCII
#define	__ASCII				1
typedef unsigned char	ASCII;
#endif	/* __ASCII	*/
#ifndef	__LONG
#define	__LONG				1
typedef signed   long   LONG;
#endif	/* __LONG	*/
#ifndef	__BYTE
#define	__BYTE				1
typedef unsigned char	BYTE;
#endif	/* __BYTE	*/
#ifndef	__BYTE
#define	__BYTE				1
typedef signed   char	SBYTE;
#endif	/* __BYTE	*/
#ifndef	__UBYTE
#define	__UBYTE				1
typedef unsigned char	UBYTE;
#endif	/* __UBYTE	*/

#ifndef	__BIT
#define	__BIT				1
typedef unsigned char	BIT;
#endif	/* __BIT	*/
#ifndef	__bool
#define	__bool				1
typedef unsigned char	bool;
#endif	/* __bool	*/
#ifndef	__BOOLEAN
#define	__BOOLEAN			1
typedef unsigned char	BOOLEAN;
typedef unsigned char	BOOL;
#endif	/* __BOOLEAN	*/
#ifndef	__bool32
#define	__bool32			1
typedef	unsigned int	bool32;
#endif	/* __bool32	*/

#ifndef INT8_MIN
/** The smallest value an 8 bit signed integer can hold @stable ICU 2.0 */
#   define INT8_MIN        ((int8_t)(-128))
#endif
#ifndef INT16_MIN
/** The smallest value a 16 bit signed integer can hold @stable ICU 2.0 */
#   define INT16_MIN       ((int16_t)(-32767-1))
#endif
#ifndef INT32_MIN
/** The smallest value a 32 bit signed integer can hold @stable ICU 2.0 */
#   define INT32_MIN       ((int32_t)(-2147483647-1))
#endif

#ifndef INT8_MAX
/** The largest value an 8 bit signed integer can hold @stable ICU 2.0 */
#   define INT8_MAX        ((int8_t)(127))
#endif
#ifndef INT16_MAX
/** The largest value a 16 bit signed integer can hold @stable ICU 2.0 */
#   define INT16_MAX       ((int16_t)(32767))
#endif
#ifndef INT32_MAX
/** The largest value a 32 bit signed integer can hold @stable ICU 2.0 */
#   define INT32_MAX       ((int32_t)(2147483647))
#endif

#ifndef UINT8_MAX
/** The largest value an 8 bit unsigned integer can hold @stable ICU 2.0 */
#   define UINT8_MAX       ((uint8_t)(255U))
#endif
#ifndef UINT16_MAX
/** The largest value a 16 bit unsigned integer can hold @stable ICU 2.0 */
#   define UINT16_MAX      ((uint16_t)(65535U))
#endif
#ifndef UINT32_MAX
/** The largest value a 32 bit unsigned integer can hold @stable ICU 2.0 */
#   define UINT32_MAX      ((uint32_t)(4294967295U))
#endif


typedef enum	{ FALSE     = 0, TRUE     = 1 } BOOLEAN_T;
typedef enum	{ DISABLE   = 0, ENABLE   = 1 } DISABLE_ENABLE;
typedef enum	{ ONOFF_OFF = 0, ONOFF_ON = 1 } ONOFF;

typedef unsigned int	size_t;
typedef int				ssize_t;
typedef unsigned long	INSTR;

#ifdef __cplusplus
#define	VARG_PROTO	...
#else
#define	VARG_PROTO
#endif	/* __cplusplus */

typedef int 	(*INT_FUNCPTR) (VARG_PROTO); /* ptr to function returning int   */
typedef void 	(*VOIDFUNCPTR) (VARG_PROTO); /* ptr to function returning void  */
typedef double 	(*DBL_FUNCPTR) (VARG_PROTO); /* ptr to function returning double*/
typedef float 	(*FLT_FUNCPTR) (VARG_PROTO); /* ptr to function returning float */

typedef int		(*pFUNCPTR)       (void);
typedef int		(*pINTFUNCPTR)    (void);
typedef void	(*pVOIDFUNCPTR)   (void);
typedef void	(*PFNCT)          (int);
#ifdef __cplusplus
}
#endif

#endif/*__OSA_TYPES_HEADER__*/
