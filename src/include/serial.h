//------------------------------------------------------------------------------
// �� �� �� : serial.h
// ������Ʈ : ezboot
// ��    �� : ezboot���� ����ϴ� serial�� ���õ� ����
// �� �� �� : ����â
// �� �� �� : 2001�� 11�� 3��
// �� �� �� :
// ��    �� : �� ��� ȭ���� ��κ��� ������
//            blob�� serial.h���� ���� �Դ�.
//            ���� ���� �Դ�.
//------------------------------------------------------------------------------

#ifndef _SERIAL_HEADER_
#define _SERIAL_HEADER_

// PXA250�� ���� Ŭ���� �̿��Ҷ��� �� ��������Ʈ�� �ش��ϴ� ��

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
