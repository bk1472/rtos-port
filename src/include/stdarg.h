//------------------------------------------------------------------------------
// �� �� �� : stdarg.h
// ������Ʈ : ezboot
// ��    �� : ezboot���� ����ϴ� C ���α׷��� �������ڿ� ���� ���ǿ� ���õ� ����
// �� �� �� : ����â
// �� �� �� : 2001�� 11�� 3��
// �� �� �� : 
// ��    �� : �� ��� ȭ���� ��κ��� ������ 
//            blob�� serial.h���� ���� �Դ�. 
//            ���� ���� �Դ�. 
//------------------------------------------------------------------------------

#ifndef _STDARG_HEADER_
#define _STDARG_HEADER_

typedef char *va_list;

#  define va_start(ap, p)	(ap = (char *) (&(p)+1))
#  define va_arg(ap, type)	((type *) (ap += sizeof(type)))[-1]
#  define va_end(ap)		((void)0)

#endif // _STDARG_HEADER_

