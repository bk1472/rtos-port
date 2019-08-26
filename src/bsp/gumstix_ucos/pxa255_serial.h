#ifndef __PXA255_SERIAL_HEADER__
#define __PXA255_SERIAL_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	union {
		UINT32	rbr;
		UINT32	thr;
		UINT32	dll;
	} u1;
	union {
		UINT32	ier;
		UINT32	dlh;
	} u2;
	union {
		UINT32	fcr;
		UINT32	iir;
	} u3;
	UINT32	lcr;		/* line control      */
	UINT32	mcr;		/* modem control     */
	UINT32	lsr;		/* line status       */
	UINT32	msr;		/* model status      */
	UINT32	spr;		/*                   */
	UINT32	isr;		/* IR port selection */
} UART_STR_T;

#ifdef __cplusplus
}
#endif

#endif/*__PXA255_SERIAL_HEADER__*/
