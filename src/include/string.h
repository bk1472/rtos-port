//------------------------------------------------------------------------------
// �� �� �� : string.h
// ������Ʈ : ezboot
// ��    �� : ezboot���� ����ϴ� ���ڿ� ó�� ���� ���
// �� �� �� : ����â
// �� �� �� : 2001�� 11�� 3��
// �� �� �� :
// ��    �� : �� ��� ȭ���� ��κ��� ������  ������ string.h����
//            ���� ���� �Դ�.
//------------------------------------------------------------------------------

#ifndef _STRING_HEADER_
#define _STRING_HEADER_
#include <osa/osa_types.h>

extern char * ___strtok;
extern char * strpbrk(const char *,const char *);
extern char * strtok(char *,const char *);
extern char * strsep(char **,const char *);
extern size_t strspn(const char *,const char *);
extern char * strcpy(char *,const char *);
extern char * strncpy(char *,const char *, size_t);
extern char * strcat(char *, const char *);
extern char * strncat(char *, const char *, size_t);
extern int strcmp(const char *,const char *);
extern int strncmp(const char *,const char *,size_t);
extern int strnicmp(const char *, const char *, size_t);
extern char * strchr(const char *,int);
extern char * strrchr(const char *,int);
extern char * strstr(const char *,const char *);
extern size_t strlen(const char *);
extern size_t strnlen(const char *,size_t);
extern void * memset(void *,int,size_t);
extern void * memcpy(void *,const void *,size_t);
extern void * memmove(void *,const void *,size_t);
extern void * memscan(void *,int,size_t);
extern int memcmp(const void *,const void *,size_t);
extern void * memchr(const void *,int,size_t);

extern void UpperStr( char *Str );
extern void LowerStr( char *Str );

#endif // _STRING_HEADER_

