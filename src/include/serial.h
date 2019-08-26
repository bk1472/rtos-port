//------------------------------------------------------------------------------
// 파 일 명 : serial.h
// 프로젝트 : ezboot
// 설    명 : ezboot에서 사용하는 serial에 관련된 내용
// 작 성 자 : 유영창
// 작 성 일 : 2001년 11월 3일
// 수 정 일 :
// 주    의 : 이 헤더 화일의 대부분의 내용은
//            blob의 serial.h에서 가져 왔다.
//            에서 가져 왔다.
//------------------------------------------------------------------------------

#ifndef _SERIAL_HEADER_
#define _SERIAL_HEADER_

// PXA250의 내부 클럭을 이용할때의 각 보오레이트에 해당하는 값

typedef enum
{
	BAUD_4800    = 192 ,
	BAUD_9600    =  96 ,
	BAUD_19200   =  48 ,
	BAUD_38400   =  24 ,
	BAUD_57600   =  16 ,
	BAUD_115200  =   8 ,
	BAUD_230400  =   4 ,
	BAUD_307200  =   3 ,
	BAUD_460800  =   2 ,
	BAUD_921600  =   1
} eBauds;

extern int	serial_init (void);
extern int	serial_putc (const char c);
extern void	serial_puts (const char *s);
extern void serial_puts_nomask (const char *s);
extern int	serial_putc_buffered (const char c);
extern void	serial_puts_buffered (const char *s);
extern int	serial_putc_unbuffered (const char c);
extern void	serial_puts_unbuffered (const char *s);

extern int	serial_getc (void);
extern int	serial_tstc (void);
extern void	serial_setbrg (void);

extern int	serial_get_buf_status(void);
extern void	serial_flush_line(void);
extern void	serial_flush_buffer(void);


extern unsigned int gMainUartPort;
extern unsigned int gSubUartPort;
extern unsigned int gMainUartIRQ;
extern unsigned int gSubUartIRQ;

#endif //_SERIAL_HEADER_
