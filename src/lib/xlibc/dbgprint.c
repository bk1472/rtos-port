/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 1998-2003. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : dbgprint.c
    VERSION     : 2.0
    AUTHOR      : 이재길(jackee@lge.com)
    DATE        : 2003.01.22
    DESCRIPTION : Customized print, crc and stacktrace  utilities.

*******************************************************************************/

/*-----------------------------------------------------------------------------
	제어 상수
	(Control Constants)
------------------------------------------------------------------------------*/
#define	PRINT_DIGIT_TIME_MODE	1

/*-----------------------------------------------------------------------------
    #include 파일들
    (File Inclusions)
------------------------------------------------------------------------------*/
#include <common.h>
#include <osa/osadap.h>
#include <osa/dbgprint.h>
#include <bsp_arch.h>
#include <serial.h>

/*-----------------------------------------------------------------------------
	상수 정의
	(Constant Definitions)
------------------------------------------------------------------------------*/

#define ISR_BUFFER_SIZE		4096
#define	DBG_BUFSZ			 256
#define	GHD_BUFSZ			  80

#define	CRNL_MASK			0x01
#define	HEAD_MASK			0x02
#define	TIME_MASK			0x04

#define	DBG_CRNL			"\n"

#define	DBG_FMT_R0N			(                                0)
#define	DBG_FMT_R1N			(                        CRNL_MASK)
#define	DBG_FMT_H0N			(            HEAD_MASK            )
#define	DBG_FMT_H1N			(            HEAD_MASK | CRNL_MASK)
#define	DBG_FMT_T0N			(TIME_MASK                        )
#define	DBG_FMT_T1N			(TIME_MASK             | CRNL_MASK)
#define	DBG_FMT_TH0N		(TIME_MASK | HEAD_MASK            )
#define	DBG_FMT_TH1N		(TIME_MASK | HEAD_MASK | CRNL_MASK)

#define	DBG_FMT_DEF0N		DBG_FMT_TH0N
#define	DBG_FMT_DEF1N		DBG_FMT_TH1N

#if	  (PRINT_DIGIT_TIME_MODE == 1)
#  define LL_UDIV_MS		   1
#  define LL_UDIV_SEC		1000
#elif (PRINT_DIGIT_TIME_MODE == 2)
#  define LL_UDIV_MS		   1
#  define LL_UDIV_SEC		1000000
#else
#  define LL_UDIV_MS		  10
#  define LL_UDIV_SEC		 100
#endif /* PRINT_DIGIT_TIME_MODE */


/*-----------------------------------------------------------------------------
	매크로 함수 정의
	(Macro Definitions)
------------------------------------------------------------------------------*/
#define	isprint(c)			( ((c) >= 32) && ((c) < 127) )


/*----------------------------------------------------------------------------
    형 정의
    (Type Definitions)
-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
	Extern 전역변수와 함수 prototype 선언
	(Extern Variables & Function Prototype Declarations)
------------------------------------------------------------------------------*/
extern int		printf (__const char *__restrict __format, ...);
extern int		pollPrint(char *format, ...);

/*-----------------------------------------------------------------------------
	전역 변수 정의
	(Define global variables)
------------------------------------------------------------------------------*/
int		gDebugEnable = TRUE;
UINT32	*gpDelayIsrMsg = (UINT32 *)&gpDelayIsrMsg;
char	gDebugCtrl[16] = { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1};

void 	printIsrBuffer();

/*-----------------------------------------------------------------------------
	Static 변수와 함수 prototype 선언
	(Static Variables & Function Prototypes Declarations)
------------------------------------------------------------------------------*/
static void (*pmPrintf)() = (void (*)(void))pollPrint;
static void (*imPrintf)() = (void (*)(void))rprint0n;

static char *dbg_format[10] = {
			"%s%s%s",								//	DBG_FMT_R0N
			"%s%s%s"					DBG_CRNL,	//	DBG_FMT_R1N
			"%-8.8s] %s%s%s",						//	DBG_FMT_H0N
			"%-8.8s] %s%s%s"			DBG_CRNL,	//	DBG_FMT_H1N
#if	  (PRINT_DIGIT_TIME_MODE == 1)
			"%03d.%03d] %s%s%s",					//	DBG_FMT_T0N
			"%03d.%03d] %s%s%s"		 	DBG_CRNL,	//	DBG_FMT_T1N
			"%03d.%03d:%-8.8s] %s%s%s",				//	DBG_FMT_TH0N
			"%03d.%03d:%-8.8s] %s%s%s"	DBG_CRNL,	//	DBG_FMT_TH1N
#elif (PRINT_DIGIT_TIME_MODE == 2)
			"%06d.%06d] %s%s%s",					//	DBG_FMT_T0N
			"%06d.%06d] %s%s%s"		 	DBG_CRNL,	//	DBG_FMT_T1N
			"%06d.%06d:%-8.8s] %s%s%s",				//	DBG_FMT_TH0N
			"%06d.%06d:%-8.8s] %s%s%s"	DBG_CRNL,	//	DBG_FMT_TH1N
#else
			"%02d.%02d] %s%s%s",					//	DBG_FMT_T0N
			"%02d.%02d] %s%s%s"		 	DBG_CRNL,	//	DBG_FMT_T1N
			"%02d.%02d:%-8.8s] %s%s%s",				//	DBG_FMT_TH0N
			"%02d.%02d:%-8.8s] %s%s%s"	DBG_CRNL,	//	DBG_FMT_TH1N
#endif /* PRINT_DIGIT_TIME_MODE */
			"%s%s%s\n",								//	DBG_FMT_R0N
			"%s%s%s\n"					DBG_CRNL	//	DBG_FMT_R1N
};

static char	*clrStrings[] = {   /* color    : index: Value      */
			"\x1b[30m",         /* blac[k]  :    0 : 30         */
			"\x1b[31m",         /* [r]ed    :    1 : 31         */
			"\x1b[32m",         /* [g]reen  :    2 : 32         */
			"\x1b[33m\x1b[40m", /* [y]ellow :    3 : 33         */
			"\x1b[34m",         /* [b]lue   :    4 : 34         */
			"\x1b[35m",         /* [p]urple :    5 : 35         */
			"\x1b[36m",         /* [c]yan   :    6 : 36         */
			"\x1b[37m",         /* gr[a]y   :    7 : 37==0 -> x */
			"\x1b[37m\x1b[40m", /* blac[K]  :    0 : 40         */
			"\x1b[37m\x1b[41m", /* [R]ed    :    1 : 41         */
			"\x1b[30m\x1b[42m", /* [G]reen  :    2 : 42         */
			"\x1b[30m\x1b[43m", /* [Y]ellow :    3 : 43         */
			"\x1b[37m\x1b[44m", /* [B]lue   :    4 : 44         */
			"\x1b[37m\x1b[45m", /* [P]urple :    5 : 45         */
			"\x1b[30m\x1b[46m", /* [C]yan   :    6 : 46         */
			"\x1b[37m\x1b[40m", /* gr[A]y   :    7 : 47==0 -> x */
			"\x1b[0m"           /* Reset  : Reset fg coloring */
			};

static char	isrBuffer[ISR_BUFFER_SIZE] = {0};
static int	isrBufferRead	= 0;
static int	isrBufferWrite	= 0;
static int	dumpIsrBuffer(char *format, ...);

/*-----------------------------------------------------------------------------
	Static 함수와 Global 함수 구현
	(Implementation of static and global functions)
------------------------------------------------------------------------------*/
/*
 *	Dump raw data in hexadecimal format and charater format
 */
static void GenHexDump(char *name, void *vcp, int size, void (*pFunc)())
{
	static   UCHAR	n2h[] = "0123456789abcdef";
	volatile ULONG	*lp = NULL, word_buff[4];
	volatile USHORT	*sp = NULL, *swrd_buff = (volatile USHORT *)word_buff;
	int		i, j, hpos, cpos;
	int		word_mode = 0;
	char	buf[GHD_BUFSZ+1];
	UCHAR	*cp, uc;
	ULONG	crc_val = 0;

	if (gDebugEnable == FALSE)
		return;

	/* Heron Secific Memory Map Adaptation Begin		*/
	if		(((ULONG) vcp >= 0xfff00000) && ((ULONG) vcp <= 0xffffffff))
	{
		/*	Address space for special function registers		*/
		word_mode = 2;
		vcp  = (void  *)((ULONG) vcp & ~3);
		size = (size & ~3);
		lp   = (ULONG *)vcp;
		cp   = (UCHAR *)&word_buff[0];
		lp	-=  4;			/* Compensate initial offset move	*/
		cp  += 16;			/* Compensate initial offset move	*/
	}
	else if (((ULONG) vcp >= 0x42000000) && ((ULONG) vcp <= 0x42ffffff))
	{
		/*	Address space for Ethernet chip in 16 bit mode		*/
		word_mode = 1;
		vcp  = (void  *)((ULONG) vcp & ~1);
		size = (size & ~1);
		sp   = (USHORT *)vcp;
		cp   = (UCHAR *)&word_buff[0];
		sp	-=  8;			/* Compensate initial offset move	*/
		cp  += 16;			/* Compensate initial offset move	*/
	}
	else
	{
		cp   = (UCHAR *) vcp;
	}
	/* Heron Secific Memory Map Adaptation End		*/

	if (word_mode < 1)
		crc_val = calccrc32(cp, size);
	snprintf(buf, GHD_BUFSZ, "      %s(Size=0x%x, CRC32=0x%08x)", name, size, crc_val);
	(*pFunc)(dbg_format[1], "", buf, "");

	if (size == 0) return;

	memset(buf, ' ', GHD_BUFSZ);

	hpos = cpos = 0;
	for (i=0; i < size; )
	{
		if ((i % 16) == 0)
		{
			if (word_mode == 2)
			{
				lp +=  4;	/* Advance   Long source pointer */
				cp -= 16;	/* Roll back char buffer pointer */
				for (j = 0; j < 4; j++) word_buff[j] = lp[j];
			}
			else if (word_mode == 1)
			{
				sp +=  8;	/* Advance   Long source pointer */
				cp -= 16;	/* Roll back char buffer pointer */
				for (j = 0; j < 8; j++) swrd_buff[j] = sp[j];
			}
			snprintf(buf, 80, "\t0x%08x(%04x):", (int)vcp+i, i);
			hpos = 18;
		}

		if ((i % 4)  == 0) buf[hpos++] = ' ';

		if ((i+0)<size) {uc=cp[i+0]; buf[hpos++]=n2h[uc>>4];buf[hpos++]=n2h[uc&15];}
		if ((i+1)<size) {uc=cp[i+1]; buf[hpos++]=n2h[uc>>4];buf[hpos++]=n2h[uc&15];}
		if ((i+2)<size) {uc=cp[i+2]; buf[hpos++]=n2h[uc>>4];buf[hpos++]=n2h[uc&15];}
		if ((i+3)<size) {uc=cp[i+3]; buf[hpos++]=n2h[uc>>4];buf[hpos++]=n2h[uc&15];}
		//buf[hpos] = ' ';

		cpos = (i%16) + 56;

		if (i<size) {buf[cpos++] = (isprint(cp[i]) ? cp[i] : '.'); i++;}
		if (i<size) {buf[cpos++] = (isprint(cp[i]) ? cp[i] : '.'); i++;}
		if (i<size) {buf[cpos++] = (isprint(cp[i]) ? cp[i] : '.'); i++;}
		if (i<size) {buf[cpos++] = (isprint(cp[i]) ? cp[i] : '.'); i++;}

		if ((i%16) == 0)
		{
			buf[cpos] = 0x00;
			(*pFunc)(dbg_format[1], "", buf, "");
		}
	}
	buf[cpos] = 0x00;
	if ((i%16) != 0)
	{
		for ( ; hpos < 56; hpos++)
			buf[hpos] = ' ';
		(*pFunc)(dbg_format[1], "", buf, "");
	}

	return;
}

void HEXDUMP(char *name, void *vcp, int	size)
{
	GenHexDump(name, vcp, size, pmPrintf);
	return;
}

void hexdump(char *name, void *vcp, int	size)
{
	GenHexDump(name, vcp, size, imPrintf);
	return;
}


//
//	Named Debug Print Utility
//		Prepend the name of calling task by using t_ident system call
//		It may be slower than normal.
//		But, it's O.K, we are to debug almost all problems.
//
//	Note:	Increase the stack size of idle task.
//
static int __DBGvsprint(void *head, const char *format , va_list args)
{
	extern char *getTaskName(void);
	char	dbgbuf[DBG_BUFSZ];
	char    tnameBuf[20];
	char	*tname  = (char *)"";
	char	*color1 = (char *)"";
	char	*color2 = (char *)"";
    SINT32	count   = 0;
	SINT32	fromISR = FALSE;
	UINT32	type, sec, msec, caller;
	UINT64	tick;
	void	(*pFunc)() = (void (*)(void))printf;

	str_ra(&caller);

	if (gDebugEnable == FALSE)
		return 0;

	if (*format == '!')
		return 0;

	if (intContext() == TRUE)
	{
		extern char *getIntName(void);
		char *name = getIntName();

		fromISR = TRUE;
		pFunc	= (void (*)(void))(*gpDelayIsrMsg ? (int)dumpIsrBuffer : (int)pmPrintf);
		if(name == NULL) sprintf(tnameBuf, "-Isr");
		else             sprintf(tnameBuf, "Isr.%s", name);

		tname = tnameBuf;
	}
	else
	{
		tname = getTaskName();
	}


	/* get color value and skip color code in debug print */
	if ( (format[0]=='^') && (format[2]=='^') )
	{
		int	colorIndex = -1;
		switch (format[1])
		{
			case 'k': case 'K': colorIndex = 0; break;
			case 'r': case 'R': colorIndex = 1; break;
			case 'g': case 'G': colorIndex = 2; break;
			case 'y': case 'Y': colorIndex = 3; break;
			case 'b': case 'B': colorIndex = 4; break;
			case 'p': case 'P': colorIndex = 5; break;
			case 'c': case 'C': colorIndex = 6; break;
			case 'a': case 'A': colorIndex = 7; break;
		}

		if (colorIndex >= 0)
		{
			if (!(format[1] & 0x20)) colorIndex += 8;
			format += 3;
			color1  = clrStrings[colorIndex];
			color2  = clrStrings[16];
		}
	}

   	count = vsnprintf( &dbgbuf[0], DBG_BUFSZ-14, format, args );
	type  = (ULONG) head;

	if (type > DBG_FMT_TH1N)
	{
		type  = DBG_FMT_DEF1N;
		tname = (char *) head;
	}

	if (type & TIME_MASK)
	{
		#if	(LL_UDIV_MS == 1) && (PRINT_DIGIT_TIME_MODE == 2)
		tick = readUsTicks();
		#else
		tick = readMsTicks();
		#endif

		#if	(LL_UDIV_MS == 1)
		#if (PRINT_DIGIT_TIME_MODE == 2)
		msec = (UINT32)((tick%1000000L)               );
		sec  = (UINT32)((tick/1000000L) % LL_UDIV_SEC );
		#else
		msec = (UINT32)((tick%1000L)                  );
		sec  = (UINT32)((tick/1000L) % LL_UDIV_SEC    );
		#endif
		#else
		msec = (UINT32)((tick%1000L) / LL_UDIV_MS     );
		sec  = (UINT32)((tick/1000L) % LL_UDIV_SEC    );
		#endif
	}
	else
	{
		tick = 0;
		msec = 0;
		sec  = 0;
	}

	if ( (intContext() == FALSE) && (*gpDelayIsrMsg != 0) )
	{
		printIsrBuffer();
	}

	lockUartPrint();

	switch (type)
	{
	  case DBG_FMT_R0N:
	  case DBG_FMT_R1N:
		(*pFunc)(dbg_format[type], color1, dbgbuf, color2);
		break;

	  case DBG_FMT_H0N:
	  case DBG_FMT_H1N:
		(*pFunc)(dbg_format[type], tname, color1, dbgbuf, color2);
		break;

	  case DBG_FMT_T0N:
	  case DBG_FMT_T1N:
		(*pFunc)(dbg_format[type], sec, msec, color1, dbgbuf, color2);
		break;

	  case DBG_FMT_TH0N:
	  case DBG_FMT_TH1N:
		(*pFunc)(dbg_format[type], sec, msec, tname, color1, dbgbuf, color2);
		break;
    }

	unlockUartPrint();

    return count;
}

#if		(NULL_PRINT==_FGLOBAL_)
#undef	dbgprintNull
//	Null debug print utility
int	dbgprintNull(const char *format , ... )
{
	return(0);
}
#endif	/* NULL_PRINT == _FGLOBAL_ */


//	Named debug print with ending newline
int dbgprint(const char *format , ... )
{
    int		count;
    va_list	ap;

    va_start( ap, format);
    count = __DBGvsprint((void *)DBG_FMT_DEF1N, format, ap);
    va_end(ap);
    return count;
}


//	Named debug print without ending newline
int tprint0n(const char *format , ... )
{
    int		count;
    va_list	ap;

    va_start( ap, format);
    count = __DBGvsprint((void *)DBG_FMT_DEF0N, format, ap);
    va_end(ap);
    return count;
}


//	Named debug print with ending newline
int tprint1n(const char *format , ... )
{
    int		count;
    va_list	ap;

    va_start( ap, format);
    count = __DBGvsprint((void *)DBG_FMT_DEF1N, format, ap);
    va_end(ap);
    return count;
}


//	Raw(Non-named) debug print with ending newline : for old compatibility
int	DBGprint(const char *format , ... )
{
    int		count;
    va_list	ap;

    va_start( ap, format);
    count = __DBGvsprint((void *)DBG_FMT_R1N, format, ap);
    va_end(ap);
    return count;
}


//	Raw(non-named) debug print without ending newline
int	rprint0n(const char *format , ... )
{
    int		count;
    va_list	ap;

    va_start( ap, format);
    count = __DBGvsprint((void *)DBG_FMT_R0N, format, ap);
    va_end(ap);
    return count;
}


//	Raw(non-named) debug print with ending newline
int	rprint1n(const char *format , ... )
{
    int		count;
    va_list	ap;

    va_start( ap, format);
    count = __DBGvsprint((void *)DBG_FMT_R1N, format, ap);
    va_end(ap);
    return count;
}

/******************************************************************************
**		Utility for calculating CRC32
******************************************************************************/
/*
 * Build auxiliary table for parallel byte-at-a-time CRC-32.
 */
static void init_crc32(unsigned long *crc_tbl)
{
	#define CRC32_POLY 0x04c11db7     /* AUTODIN II, Ethernet, & FDDI */
	int				i, j;
	unsigned long	c;

	for (i = 0; i < 256; ++i)
	{
		for (c = i << 24, j = 8; j > 0; --j)
			c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
			crc_tbl[i] = c;
	}
}

unsigned long calccrc32(unsigned char *buf, int len)
{
	static unsigned long	crc32_table[256];
	unsigned char			*p;
	unsigned long			crc;

	if (!crc32_table[1])						/* if not already done, */
		init_crc32(&crc32_table[0]);			/* build table			*/

	crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */
	for (p = buf; len > 0; ++p, --len)
		crc = (crc << 8) ^ crc32_table[(crc >> 24) ^ *p];
	return(crc);
	// return ~crc;            /* transmit complement, per CRC-32 spec */
}

/******************************************************************************
**		Utility functions for delayed print out from ISR
******************************************************************************/
void registerDelayIsrMsgControl(UINT32 *pVar)
{
	gpDelayIsrMsg = pVar;
	return;
}

int dumpIsrBuffer(char *format, ...)
{
	char	buffer[DBG_BUFSZ];
    int		count, nBytes;
    va_list	arg_pt;

    va_start(arg_pt, format);

	count = vsnprintf(buffer, DBG_BUFSZ, format, arg_pt);
	nBytes = (ISR_BUFFER_SIZE + isrBufferRead - isrBufferWrite - 1) % ISR_BUFFER_SIZE;
	count = min(count, nBytes);

    va_end(arg_pt);

	if (count <= 0)
	{
		return(count);
	}

	if ( ISR_BUFFER_SIZE < isrBufferWrite + count )
	{
		nBytes = ISR_BUFFER_SIZE - isrBufferWrite;
		memcpy(&isrBuffer[isrBufferWrite], buffer,        nBytes        );
		memcpy(&isrBuffer[             0], buffer+nBytes, count - nBytes);
	}
	else
	{
		memcpy(&isrBuffer[isrBufferWrite], buffer, count);
	}
	isrBufferWrite = (isrBufferWrite + count) % ISR_BUFFER_SIZE;

    return(count);
}

void printIsrBuffer(void)
{
	int		n;

	lockUartPrint();

	while ( isrBufferRead != isrBufferWrite)
	{
		if (isrBufferRead < isrBufferWrite) n = isrBufferWrite  - isrBufferRead;
		else                                n = ISR_BUFFER_SIZE - isrBufferRead;

		printf("%s", &isrBuffer[isrBufferRead]);
		fflush(stdout);
		isrBufferRead = (isrBufferRead + n) % ISR_BUFFER_SIZE;
	}

	unlockUartPrint();

	return;
}
