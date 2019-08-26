#include	<stdlib.h>
#include	<string.h>
#include	<ar.h>
#include	<unistd.h>
#include	<sys/mman.h>

#include	"elf_func.h"
#include	"def.h"
#include	"dwarf.h"
#include	"process.h"

extern ulong_t		process32		(FILE *ifp, FILE *ofp);
extern ulong_t		process64		(FILE *ifp, FILE *ofp);

static int			process_bit_size   = 0;

void set_bit_size(char id)
{
	if      (id == 1) //32bit
		process_bit_size = 32;
	else if (id == 2) //64bit
		process_bit_size = 64;
	else
		process_bit_size = 0;
}

int get_bit_size(void)
{
	return process_bit_size;
}

ulong_t process (FILE *ifp, FILE *ofp)
{
	if(get_bit_size() == 32)
		return process32(ifp, ofp);

	if(get_bit_size() == 64)
		return process64(ifp, ofp);

	return 0;
}

