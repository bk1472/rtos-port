//------------------------------------------------------------------------------
// 파 일 명 : stdio.h
// 프로젝트 : ezboot
// 설    명 : ezboot에서 사용하는 표준 입출력을 위한 헤더 화일
// 작 성 자 : 유영창
// 작 성 일 : 2001년 11월 3일
// 수 정 일 :
// 주    의 : 이 헤더 화일의 대부분의 내용은 리눅스에서 가져왔다.
//------------------------------------------------------------------------------

#ifndef _STDIO__HEADER_
#define _STDIO__HEADER_

#include <osa/osa_types.h>
#include <ctype.h>
#include <stdarg.h>

typedef unsigned short		__kernel_mode_t;
typedef long		        __kernel_time_t;

#define        NUL        0x00
#define        SOH        0x01
#define        STX        0x02
#define        ETX        0x03
#define        EOT        0x04
#define        ENQ        0x05
#define        ACK        0x06
#define        BEL        0x07
#define        BS         0x08
#define        HT         0x09
#define        LF         0x0a
#define        VT         0x0b
#define        FF         0x0c
#define        CR         0x0d
#define        SO         0x0e
#define        SI         0x0f
#define        DLE        0x10
#define        DC1        0x11
#define        DC2        0x12
#define        DC3        0x13
#define        DC4        0x14
#define        NAK        0x15
#define        SYN        0x16
#define        ETB        0x17
#define        CAN        0x18
#define        EM         0x19
#define        SUB        0x1a
#define        ESC        0x1b
#define        FS         0x1c
#define        GS         0x1d
#define        RS         0x1e
#define        US         0x1f
#define        DEL        0x7f


#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define SPACEP	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define LARGE	64		/* use 'ABCDEF' instead of 'abcdef' */

#define do_div(n,base)									\
({														\
	int __res;											\
	__res = ((unsigned long)n) % (unsigned int)base;	\
	n = ((unsigned long)n) / (unsigned int)base;		\
	__res;												\
})

#endif //_STDIO_HEADER_
