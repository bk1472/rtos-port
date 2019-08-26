//------------------------------------------------------------------------------
// �� �� �� : time.h
// ������Ʈ : ezboot
// ��    �� : ezboot���� ����ϴ� time�� ���õ� ��� 
// �� �� �� : ����â
// �� �� �� : 2001�� 11�� 3��
// �� �� �� : 
//------------------------------------------------------------------------------

#ifndef _TIME_HEADER_
#define _TIME_HEADER_

#define TICKS_PER_SECOND 3686400
#define TICKS_PER_TICK   36864


void		SetWatchdog( int msec );
void  		TimerInit(void);             	// Ÿ�̸� �ʱ�ȭ 
unsigned int  	TimerGetTime(void);       		// 1/TICKS_PER_SECOND �� �ð� ���� ��ȯ�Ѵ�. 
int   		TimerDetectOverflow(void);
void  		TimerClearOverflow(void);
void  		msleep(unsigned int msec);   	// �и������� �����ð����� ��� �Ѵ�. 
void        sleep(unsigned int sec);

extern void 	ReloadTimer( unsigned char bTimer, unsigned int msec);
extern int  	TimeOverflow( unsigned char bTimer );
extern void 	FreeTimer( unsigned char bTimer );

#endif //_TIME_HEADER_

