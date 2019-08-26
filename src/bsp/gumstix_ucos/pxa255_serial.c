/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 2000. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : pxa255_serial.c
    VERSION     :
    AUTHOR      : bk1472 (baekwon.choi@lge.com)
    DATE        : 2010/01/28
    DESCRIPTION :

*******************************************************************************/

/*---------------------------------------------------------
    전역 제어 상수 정의
    (Global Control Constant Definitions)
---------------------------------------------------------*/

/*---------------------------------------------------------
    #include 파일들
    (File Inclusions)
---------------------------------------------------------*/
#include <common.h>
#include <board.h>
#include <osadap.h>
#include "pxa255_serial.h"

/*---------------------------------------------------------
    상수 정의
    (Constant Definitions)
---------------------------------------------------------*/
#define USE_BUFFERED_MODE		0
#define USE_READ_ISR			1

/*---------------------------------------------------------
    형 정의
    (Type Definitions)
---------------------------------------------------------*/
typedef  void (*TERMCALLBACK)(char);

/*---------------------------------------------------------
    Extern 전역변수와 함수 prototype 선언
    (Extern Variables & Function Prototype Declarations)
---------------------------------------------------------*/
extern int		intLock		(void);
extern void		intUnlock	(int);
extern int		intContext	(void);

/*---------------------------------------------------------
    Static 변수와 함수 prototype 선언
    (Static Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
static OS_SEM		os_rtrmMtx;
static OS_SEM		os_rbtrmMtx;
static OS_SEM		os_wtrmMtx;
static OS_SEM		os_wbtrmMtx;
static OS_SEM		*osvRTermMutex = &os_rtrmMtx;
static OS_SEM		*osvRBTermSem  = &os_rbtrmMtx;
static OS_SEM		*osvWTermMutex = &os_wtrmMtx;
static OS_SEM		*osvWBTermSem  = &os_wbtrmMtx;

static int	wait_term		(void);
static int	give_term		(void);
static void	write_term		(char readbyte);
static int	read_term		(void);
static int	give_wbuf		(void);
static int	wait_wbuf		(void);
#if (USE_BUFFERED_MODE > 0)
static int	give_wterm		(void);
static int	wait_wterm		(void);
#endif

static int	pxa255_serial_putc_unbuffered(char c);
static int	pxa255_serial_putc_buffered(char c);
static int	pxa255_serial_getc(void);
static int	pxa255_serial_tstc(void);
static void uartIsr (void);

#ifdef IN_GUMSTIX
static volatile UART_STR_T *pUart = (volatile UART_STR_T *)FFUART_BASE;
#else
static volatile UART_STR_T *pUart = (volatile UART_STR_T *)STUART_BASE;
#endif

/*---------------------------------------------------------
    전역 변수와 함수 prototype 선언
    (Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
int  serial_putc_unbuffered (const char c);
int  serial_putc_buffered   (const char c);
void serial_puts_unbuffered (const char *s);
void serial_puts_buffered   (const char *s);

volatile int	uartOFlags = ONLCR;
volatile int	uartLFlags = 0;
volatile int	bSentLF    = 1;

TERMCALLBACK	TerminalCallBack = NULL;

void setTerminalMode (int flag)
{
	uartLFlags = flag;
}

void setTerminalExtISR(void (*func)(UINT08))
{
	int		flag;
    flag = intLock();
    TerminalCallBack = (TERMCALLBACK)func;
    intUnlock(flag);
}

/*------------------------------------------------------------------------------
 * external variables & function pre-declare
 *-----------------------------------------------------------------------------*/
int serial_init(int baudrate)
{
	UINT32			quot = 0;
	OS_ERR			err;
	volatile UINT32 tmp;

	switch(baudrate)
	{
		case   1200: quot = 768;break;
		case   9600: quot =  96;break;
		case  19200: quot =  48;break;
		case  38400: quot =  24;break;
		case  57600: quot =  16;break;
		case 115200: quot =   8;break;
		default    : quot =   8;
	}

	CKEN |= CKEN_FFUART;

	pUart->u2.ier  = 0;
	pUart->u3.fcr  = 0;
	pUart->lcr  = LCR_WLEN8 | LCR_DLAB;
	pUart->u1.dll = quot & 0xff;
	pUart->u2.dlh = quot >> 8;;
	pUart->lcr  = LCR_WLEN8;

	pUart->mcr  = 0;
	intConnect (INT_NUM_FFUART, (PFNCT)uartIsr, 0, "uart");

	pUart->u3.fcr  = FCR_TRFIFOE; //FIFO enable
	pUart->u3.fcr  = FCR_TRFIFOE | FCR_RESETRF | FCR_RESETTF;

	pUart->u2.ier  = IER_RAVIE | IER_RLSE | IER_RTIOE | IER_UUE;

	(void)OSSemCreate(osvRTermMutex, "RTERMSEM", 1, &err);
	(void)OSSemCreate(osvRBTermSem, "RBUFSEM", 1, &err);
	(void)OSSemCreate(osvWTermMutex, "WTERMSEM", 1, &err);
	(void)OSSemCreate(osvWBTermSem, "WBUFSEM", 1, &err);

	/*interrupt register clear*/
	tmp = pUart->lsr;
	tmp = pUart->u1.rbr;
	tmp = pUart->u3.iir;
	tmp = tmp;
	#if (USE_READ_ISR > 0)
	intEnable(INT_NUM_FFUART);
	#endif
	return 0;
}

#define WR_BUF_SIZE			10240
#define WR_BUF_THRESHOLD	100
static char  wr_buf[WR_BUF_SIZE];
static int 	 wr_read	= 0;
static int 	 wr_write	= 0;
int serial_putc_buffered (const char c)
{
	if ( (c == '\n') && (uartOFlags & ONLCR) )
		pxa255_serial_putc_buffered ('\r');

	return pxa255_serial_putc_buffered (c);
}

int serial_putc_unbuffered (const char c)
{
	if ( (c == '\n') && (uartOFlags & ONLCR) )
		pxa255_serial_putc_unbuffered ('\r');

	return pxa255_serial_putc_unbuffered (c);
}

int serial_putc (const char c)
{
#if	(USE_BUFFERED_MODE > 0)
	if ((intContext() == 0) && (wait_wterm() >= 0))
	{
		serial_putc_buffered(c);
		give_wterm();
	}
	else
#endif /* USE_BUFFERED_MODE */
	{
		serial_putc_unbuffered(c);
	}
	return 0;
}

void serial_puts_buffered (const char *s)
{
	while (*s)
	{
		serial_putc_buffered(*s);
		s++;
	}
	return;
}

void serial_puts_unbuffered (const char *s)
{
	while (*s)
	{
		serial_putc_unbuffered(*s);
		s++;
	}
	return;
}

void serial_puts (const char *s)
{
#if	(USE_BUFFERED_MODE > 0)
	if ((intContext() == 0) && (wait_wterm() >= 0))
	{
		while (*s)
		{
			serial_putc_buffered(*s);
			s++;
		}
		give_wterm();
	}
	else
#endif /* USE_BUFFERED_MODE */
	{
		while (*s)
		{
			serial_putc_unbuffered(*s);
			s++;
		}
	}
	return;
}

void serial_puts_nomask (const char *s)
{
#if	(USE_BUFFERED_MODE > 0)
	if ((intContext() == 0) && (wait_wterm() >= 0))
	{
		while (*s)
		{
			serial_putc_buffered(*s);
			s++;
		}
		give_wterm();
	}
	else
#endif /* USE_BUFFERED_MODE */
	{
		while (*s)
		{
			serial_putc_unbuffered(*s);
			s++;
		}
	}
	return;
}

int serial_getc (void)
{
	int data;

	#if (USE_READ_ISR > 0)
	if(intContext())
	{
		while(!pxa255_serial_tstc())
			;
		data = pUart->u1.rbr;
		if (data == 0x0d) data = 0x0a;  /* Convert CR to LF */
		if (uartLFlags & ECHO) serial_putc(data); /* Echo character */
		return(data);

	}
	#else
	while(!pxa255_serial_tstc())
	{
		OS_ERR	err;
		OSTimeDly((OS_TICK)1, OS_OPT_TIME_DLY, &err);;
	}
	data = pUart->u1.rbr;

	if (TerminalCallBack != NULL)
	{
		int i = 0, cnt;
		if (data == 0x1b) cnt = 5;
		else              cnt = 1;
		for(i = 0; i < cnt; i++)
			TerminalCallBack(data);
	}

	if (data == 0x0d)
		data = 0x0a;  /* Convert CR to LF */
	if (uartLFlags & ECHO)
		putchar(data); /* Echo character */
	write_term(data);
	give_term();
	#endif
	data = pxa255_serial_getc ();
	return data;
}

int serial_tstc (void)
{
	return pxa255_serial_tstc ();
}

void serial_setbrg (void)
{
}

static int pxa255_serial_putc_unbuffered (char c)
{
	while (((volatile UINT32)pUart->lsr & LSR_TDRQ) == 0)
		;
	pUart->u1.thr = c;
	bSentLF = (c == '\n' ? 1 : 0);
	return 1;
}


static int pxa255_serial_putc_buffered (char c)
{
	int		ret = 0;
	char	toPut = '\0';
	UINT32	intStatus = 0;

	/* 0. wait if there is no buffer */
	while ((c != '\0') && (((wr_write + 2) % WR_BUF_SIZE) == wr_read))
	{
		if (intContext())
		{
			/* If in ISR and no buffer left, just return */
			return 0;
		}
		else
		{
			/* Wait until buffer is not full */
			if (wait_wbuf() < 0)
				break;
		}
	}

	intStatus = intLock();

	/* 1. data exist in buffer : ready first data in buffer */
	if (wr_read != wr_write)
	{
		toPut = wr_buf[wr_read];
	}
	/* 2. no data in buffer, new data exist : ready new data */
	else if (c != '\0')
	{
		toPut = c;
	}
	/* 3. data exist and buffer is not full : put data in buffer */
	if ((c != '\0') && (((wr_write + 2) % WR_BUF_SIZE) != wr_read))
	{
		wr_buf[wr_write] = c;
		wr_write = (wr_write + 1) % WR_BUF_SIZE;
	}
	/* 4. Wait until there is space in the FIFO */
	if ((toPut != '\0') && ((volatile UINT32)pUart->lsr & LSR_TDRQ))
	{
		/* Send the character */
		pUart->u1.thr = toPut;
		bSentLF = (toPut == '\n' ? 1 : 0);
		wr_read = (wr_read + 1) % WR_BUF_SIZE;
		ret = 1;
		if ((c == '\0') && (((wr_write + WR_BUF_SIZE - wr_read)%WR_BUF_SIZE) > WR_BUF_THRESHOLD))
			give_wbuf();
	}

	intUnlock(intStatus);

	return ret;

}

static int pxa255_serial_getc(void)
{
	int data;

	if ((data = read_term()) >= 0)
		return data;

	if (wait_term() < 0)
	{
		/* Wait until there is data in the FIFO */
		while ((pUart->lsr & LSR_DR) == 0)
			;
		data = pUart->u1.rbr;
	}
	else
	{
		data = read_term();
	}

	#if 0
	/* Check for an error flag */
	if (data & 0xFFFFFF00) {
		/* Clear the error */
		pLgeUart->status = 0xFFFFFFFF; /* Error clear */
		return 0;
	}
	#endif
	return (int) data;
}

static int pxa255_serial_tstc(void)
{
	return (pUart->lsr & LSR_DR);
}

int serial_get_buf_status(void)
{
	if (wr_read == wr_write)
	{
		return(0);
	}
	else
	{
		return((wr_write + WR_BUF_SIZE - wr_read) % WR_BUF_SIZE);
	}
}

void serial_flush_line(void)
{
	if (intContext())
	{
		while ((wr_write != wr_read) && (bSentLF == 0))
		{
			pxa255_serial_putc_unbuffered(wr_buf[wr_read]);
			wr_read = (wr_read + 1) % WR_BUF_SIZE;
		}
	}
	return;
}

void serial_flush_buffer(void)
{
	while (wr_write != wr_read)
	{
		pxa255_serial_putc_unbuffered(wr_buf[wr_read]);
		wr_read = (wr_read + 1) % WR_BUF_SIZE;
	}
	return;
}

static void uartIsr (void)
{
	UINT32	iir, uartIntStatus;

	iir = pUart->u3.iir;

	if (iir & IIR_IP)
		return;

	uartIntStatus = pUart->lsr;

	if (uartIntStatus & LSR_DR)
	{
		int	max_count = 256;

		do
		{
			volatile UINT08 readbyte = (volatile UINT32)pUart->u1.rbr & 0xFF;

			if (TerminalCallBack != NULL)
			{
				TerminalCallBack(readbyte);
			}
			if (readbyte == 0x0d)
				readbyte = 0x0a;
			if (uartLFlags & ECHO)
				putchar(readbyte);
			write_term(readbyte);
		} while (((volatile UINT32)pUart->lsr & LSR_DR) && (max_count-- > 0));
		give_term();
	}

	if (uartIntStatus & LSR_TDRQ)
	{
		while (((volatile UINT32)pUart->lsr & LSR_TDRQ))
		{
			if (serial_putc_buffered('\0') == 0)
				break;
		}
	}
}

static int wait_term(void)
{
  	OS_ERR	err;

	if (osvRTermMutex != NULL)
	{
		OSSemPend(osvRTermMutex, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

		return 0;
	}
	else
	{
		return -1;
	}
}

static int give_term(void)
{
	OS_ERR	err;

	if (osvRTermMutex != NULL)
		OSSemPost(osvRTermMutex, OS_OPT_POST_1, &err);

	return 0;
}

static int wait_wbuf(void)
{
  	OS_ERR	err;

	if (osvWBTermSem != NULL)
	{
	    OSSemPend(osvWBTermSem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

		return 0;
	}
	else
	{
		return -1;
	}
}

static int give_wbuf(void)
{
  	OS_ERR	err;

	if (osvWBTermSem != NULL)
		OSSemPost(osvWBTermSem, OS_OPT_POST_1, &err);

	return 0;
}

static int wait_rbuf(void)
{
  	OS_ERR	err;

	if (osvRBTermSem != NULL)
	{
	    OSSemPend(osvRBTermSem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

		return 0;
	}
	else
	{
		return -1;
	}
}

static int give_rbuf(void)
{
  	OS_ERR	err;

	if (osvRBTermSem != NULL)
		OSSemPost(osvRBTermSem, OS_OPT_POST_1, &err);

	return 0;
}

#define TERM_BUFF_SIZE 500
char term_buffer[TERM_BUFF_SIZE];
volatile int  read_count = 0;
volatile int  write_count = 0;
static void write_term(char readbyte)
{
	int	do_give_rbuf = (read_count == write_count);

	if (((write_count + 1) % TERM_BUFF_SIZE) != read_count)
	{
		term_buffer[write_count] = readbyte;
		write_count = (write_count + 1) % TERM_BUFF_SIZE;
	}
	if (do_give_rbuf) give_rbuf();
}

static int read_term(void)
{
	int retVal;

	while (read_count == write_count)
		wait_rbuf();

	retVal = term_buffer[read_count];
	read_count = (read_count + 1) % TERM_BUFF_SIZE;

	return retVal;
}

#if (USE_BUFFERED_MODE > 0)
static int give_wterm(void)
{
	OS_ERR	err;

	if (osvRTermMutex != NULL)
		OSSemPost(osvRTermMutex, OS_OPT_POST_1, &err);

	return 0;
}

static int wait_wterm(void)
{
  	OS_ERR	err;

	if (osvWTermMutex != NULL)
	{
	    OSSemPend(osvWTermMutex, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

		return 0;
	}
	else
	{
		return -1;
	}
}
#endif
