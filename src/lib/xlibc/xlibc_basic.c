#include <common.h>
#include <osa/osadap.h>
#include <stdarg.h>
#include <serial.h>

#define CFG_PBSIZE		(512)
extern int vsprintf(char *buf, const char *fmt, va_list args);

int getc(FILE *fp)
{
	return serial_getc();
}

int fflush(FILE *fp)
{
	return 0;
}

char *fgets(char *c, int n, FILE *fp)
{
	int i;

	for (i = 0 ; i < n ; i++)
	{
		c[i] = getc(fp);
		if ((c[i] == ' ') || (c[i] == '\n') || (c[i] == '\r'))
			break;
	}
	return c;
}

void fputs(char *c, FILE *fp)
{
	serial_puts(c);
}

void puts(char *c)
{
	serial_puts(c);
}

void putchar(UINT08 digit)
{
	serial_putc(digit);
}


void memShow(int in)
{
}

float atof(char *in)
{
	return 0;
}

int printf(const char *fmt, ...)
{
	va_list			args;
	unsigned int	count;
	char			outStr[CFG_PBSIZE];

	va_start (args, fmt);

	/*
	 *	For this to work, outStr should be large enough to hold message
	 */
	count = vsnprintf (outStr, CFG_PBSIZE, fmt, args);
	va_end (args);

	/*
	 *	If not in interrupt context, *pPuts() should block and unlock
	 *	interrupt while printing.
	 */
	serial_puts(outStr);

	return count;
}

/* output string to stand output port, fp will be ignored */
int fprintf(FILE *fp, char *fmt, ...)
{
	va_list			args;
	unsigned int	count;
	char			outStr[CFG_PBSIZE];

	va_start (args, fmt);

	/*
	 *	For this to work, outStr should be large enough to hold message
	 */
	count = vsnprintf (outStr, CFG_PBSIZE, fmt, args);
	va_end (args);

	/*
	 *	If not in interrupt context, *pPuts() should block and unlock
	 *	interrupt while printing.
	 */
	serial_puts(outStr);

	return count;
}

int pollPrint(char *fmt, ...)
{
	va_list			args;
	unsigned int	count;
	char			outStr[CFG_PBSIZE];

	va_start (args, fmt);

	/*
	 *	For this to work, outStr should be large enough to hold message
	 */
	count = vsnprintf (outStr, CFG_PBSIZE, fmt, args);
	va_end (args);

	/*
	 *	If interrupt context, flush io buffer upto newline
	 */
	serial_flush_line();
	serial_puts_unbuffered(outStr);

	return count;
}

int max(int a, int b)
{
	if (a < b)
		return b;
	else
		return a;
}

int min(int a, int b)
{
	if (a > b)
		return b;
	else
		return a;
}
int abs(int a)
{
	if (a > 0)
		return a;
	else
		return a * -1;
}

FILE *stdin = NULL, *stdout = NULL, *stderr = NULL;
