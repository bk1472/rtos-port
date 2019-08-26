#include	<stdio.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<string.h>
#include	<errno.h>
#include	<ar.h>
#include	<time.h>
#include	<unistd.h>

#include	"symtypes.h"
#include	"elf_func.h"
#include	"def.h"
#include	"util.h"
#include	"process.h"

#define OPTSTR		"h?o:v:d:a:"
#define OPTMSG		"[h?ovda]"

static char		*tool_name   = NULL;
ulong_t			verbose      = 0;
ulong_t			debug_level  = 0;
ulong_t			debug_lvl_pc = 0;
ulong_t			debug_lvl_ad = 0;
ulong_t			analyze      = 0;

static void		usage			(void);

int main (int argc, char **argv)
{
	extern int	optind;
	FILE		*ifp      = NULL;
	FILE		*ofp      = NULL;
	char		*dst_name = NULL;
	char		*src_name = NULL;
	char		*sym_name = NULL;
	char		*cp;
	int			c, len;
	ulong_t		img_size  = 0;
	int			arg_list  = 0;

	if ( argc < 2 )
		exit(-1);

	if      (cp = strrchr(argv[0], '/'))
		tool_name = cp + 1;
	else if (cp = strrchr(argv[0], '\\'))
		tool_name = cp + 1;
	else
		tool_name = argv[0];

	while ((c = getopt(argc, argv, OPTSTR)) != EOF)
	{
		switch(c)
		{
			case 'v':
				sscanf(optarg, "%d", &verbose);
				break;
			case 'd':
				sscanf(optarg, "%x", &debug_level);
				debug_lvl_pc = debug_level >> 4;
				debug_lvl_ad = debug_level & 0xf;
				break;
			case 'a':
				analyze = 1;
				break;
			case 'o':
				dst_name = strdup(optarg);
				break;
			case '?':
			case 'h':
				usage();
				exit(0);
			dafault :
				break;
		}
	}
	arg_list = argc - optind;
	if (arg_list != 1)
	{
		usage();
		exit(1);
	}

	if (dst_name == NULL)
		dst_name = strdup(argv[optind]);

	len = strlen(dst_name);

	if ((cp  = strrchr(dst_name, '.')) != NULL)
	{
		if (
			  !strcasecmp(cp, ".bin")
			||!strcasecmp(cp, ".elf")
			||!strcasecmp(cp, ".biz")
			||!strcasecmp(cp, ".axf")
			||!strcasecmp(cp, ".dat")
		   )
		{
			*cp++ = '\0';
			len = strlen(dst_name);
		}
	}

	if (0 == analyze)
	{
		sym_name = (char *)malloc(len + 64);
		sprintf(sym_name, "%s.sym", dst_name);
		unlink(sym_name);
	}

	src_name = argv[optind];

	if ((ifp = fopen(src_name, "rb")) == (FILE *) NULL)
	{
		err_print(src_name, "Cannot open");
		errcase();
		goto flush_and_exit;
	}

	if (ELF_K_ELF != Elf32_gettype(ifp))
	{
		(void)fprintf(stderr, "%s: %s:  File is not ELF format\n", tool_name, src_name);
		errcase();
		goto flush_and_exit;
	}

	if (NULL != sym_name)
	{
		if ((ofp = fopen (sym_name, "wb")) == NULL)
			ofp = stdout;
	}
	else
		ofp = NULL;

	PRINT("=======================================================================\n");
	img_size = process(ifp, ofp);
	PRINT("=======================================================================\n");

	if(img_size == 0)
	{
		(void)fprintf(stderr, "%s: %s(%s): Parsing image has been failed\n", tool_name, sym_name, src_name);
		errcase();
		goto flush_and_exit;
	}
	fclose(ifp); ifp = NULL;


flush_and_exit:
	if (sym_name)             free(sym_name);
	if (dst_name)             free(dst_name);
	if (ifp     )             fclose(ifp);
	if (ofp && ofp != stdout) fclose(ofp);

	if (exitcode())	exit(FATAL);
	else            exit(0);

	return 0;
}

static void usage (void)
{
	extern char *RevHistStr;
	extern char version[];

	(void) fprintf(stdout, ">>%s : [Get ELF Type Excution File Symbol Informations],%s %s<<\n", tool_name, &version[0], __DATE__ " "__TIME__);
	(void) fprintf(stdout, "Usage: \n   %s [-%s] application ...\n\n", tool_name, OPTMSG);
	(void) fprintf(stdout,
"   [-?h]      Print this help message\n"
"   [-o name]  Set output file name\n"
"   [-v]       Verbose display - 0 : no display 1 : process area 2 : addr2line area\n"
"   [-d]       debug   display - 0x10 : process level (1)  0x20 : process level (2)\n"
"                                0x01 : addr2line level(1) 0x02 : addr2line level (2)\n"
"   [-a]       Just Analyze ELF file (do not make symbol file at any conditions\n"
"Note:\n"
"   - Revision History -\n"
"%s",
	RevHistStr		);
}
