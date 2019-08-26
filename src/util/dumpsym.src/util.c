#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "symtypes.h"
#include "util.h"

#define	GHD_BUFSZ			80

static	int	err_code = 0;

#define BIT_64	64
#define BIT_32	32

static void hexdump_fp(FILE *fp, const char *name, void *vcp, int size)
{
	#if    (BIT_WIDTH == BIT_64)
	 #define HPOS_INIT		26
	 #define CPOS_OFFS		64
	 #define PRNTMSG		"\t0x%016lx(%04x):"
	#elif  (BIT_WIDTH == BIT_32)
	 #define HPOS_INIT		18
	 #define CPOS_OFFS		56
	 #define PRNTMSG		"\t0x%08x(%04x):"
	#endif

	static uint8_t n2h[] = "0123456789abcdef";
	int		i, hpos, cpos;
	char	buf[GHD_BUFSZ+1] = {0};
	uint8_t	*cp, uc;

	snprintf(buf, GHD_BUFSZ, "%s(Size=0x%x)", name, size);
	buf[GHD_BUFSZ] = 0;
	fprintf(fp,"%s\n",buf);

	if (size == 0) return;

	memset(buf, ' ', GHD_BUFSZ);

	cp = (uint8_t*)vcp;
	hpos = cpos = 0;
	for (i=0; i < size; ) {

		if ((i % 16) == 0) {
			snprintf(buf, GHD_BUFSZ, PRNTMSG, (long)vcp+i, i);
			hpos = HPOS_INIT;
		}

		if ((i % 4)  == 0) buf[hpos++] = ' ';

		if ((i+0)<size) {uc=cp[i+0]; buf[hpos++]=n2h[(uc&0xF0)>>4];buf[hpos++]=n2h[uc&15];}
		if ((i+1)<size) {uc=cp[i+1]; buf[hpos++]=n2h[(uc&0xF0)>>4];buf[hpos++]=n2h[uc&15];}
		if ((i+2)<size) {uc=cp[i+2]; buf[hpos++]=n2h[(uc&0xF0)>>4];buf[hpos++]=n2h[uc&15];}
		if ((i+3)<size) {uc=cp[i+3]; buf[hpos++]=n2h[(uc&0xF0)>>4];buf[hpos++]=n2h[uc&15];}

		cpos = (i%16) + CPOS_OFFS;

		if (i<size) {buf[cpos++] = (isprint(cp[i]) ? cp[i] : '.'); i++;}
		if (i<size) {buf[cpos++] = (isprint(cp[i]) ? cp[i] : '.'); i++;}
		if (i<size) {buf[cpos++] = (isprint(cp[i]) ? cp[i] : '.'); i++;}
		if (i<size) {buf[cpos++] = (isprint(cp[i]) ? cp[i] : '.'); i++;}

		if ((i%16) == 0) {
			buf[cpos] = 0x00;
			fprintf(fp,"%s\n", buf);
		}
	}
	buf[cpos] = 0x00;
	if ((i%16) != 0) {
		for ( ; hpos < CPOS_OFFS; hpos++)
			buf[hpos] = ' ';
		fprintf(fp,"%s\n", buf);
	}
}

void hexdump(const char *name, void *vcp, int size)
{
	hexdump_fp(stdout, name, vcp, size);
}


void err_print(char *file, char *string)
{
    (void) fflush(stdout);
   	(void) fprintf(stderr, "getsym: %s: %s\n", file, string);
	errcase();
    return;
}

void errcase(void)
{
	err_code++;
}

int exitcode(void)
{
	return err_code;
}
