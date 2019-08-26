/*
 *	This program is to filter warning message from arm compiler.
 *	Stupid ARM compiler generate too many warnings and no way
 *	to disable each messages.
 *
 *	So, we should do it manually.
 *
 *	Use it following way.
 *		armcc [options] file.c 2>&1 | sift-warn
 *
 *
 */

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<libgen.h>
#include	<ctype.h>

#define	MAX_SIFT_ITEMS	128
#define	OUTPUT_FILE		stdout

typedef struct
{
	int		sw_type;
	int		sw_len[3];
	char	sw_str[3][40];
} SIFT_WARN_T;

SIFT_WARN_T warn_fmts[MAX_SIFT_ITEMS] =
{
	{ -1, { 0, 0, 0 }, { "Fatal error: ",	"Too many errors",	"" } },
	{ -2, { 0, 0, 0 }, { "Serious error: ",	"Serious",			"" } },
	{ -2, { 0, 0, 0 }, { "Error: ",			"",					"" } },
	{ -2, { 0, 0, 0 }, { "error: ",			"",					"" } },
	{  0, { 0, 0, 0 }, { "",				"",					"" } },
};

static char	warn_buf[512];		/* Input warning string from compilier		*/
static char	in_str1[256];		/* Read string buffer temporary used		*/
static char	in_str2[256];		/* Read string buffer temporary used		*/
static char	in_str3[256];		/* Read string buffer temporary used		*/
static char	in_str4[256];		/* Read string buffer temporary used		*/
static char	in_str5[256];		/* Read string buffer temporary used		*/
static char	in_str6[256];		/* Read string buffer temporary used		*/
static char path_pfx[512];		/* Prefix of relative path					*/
static int	nSupItems = 0;		/* Number of items in supress list			*/
static int	nTotWarns = 0;		/* Number of Total     warnings				*/
static int	nSupWarns = 0;		/* Number of Supressed warnings				*/
static int	nTotError = 0;		/* Number of Total   errors					*/
static int	nSerError = 0;		/* Number of Serious errors					*/
static int	nHiddenEr = 0;		/* Number of Hidden  errors 				*/
static int	bHasError = 0;		/* Error message 가 있었는지 여부			*/

       int	bPathConv = 0;		/* Dos style path를 Unix style Path 로 변경	*/
       int	bSilent   = 0;		/* Quiet   mode or not						*/
       int	nResolvePath=1;		/* Resolve heading pathname in warning msg	*/
       int	nVerbose  = 0;		/* Verbose mode or not						*/
       int	nMaxWarns = 10;		/* Maximum number of warns allowed			*/
	   int  bIsatty   = 0;		/* Whether connected to terminal or not		*/
       int	bCppHack  = 0;		/* Enable cpp processing hack function		*/

extern char gSource[8][PATH_MAX];/* Name of source file                     */
extern int	gNumWarnFiles;		/* Number of C Sources should be checked	*/

/*
 *	Dos path에 사용되는 back-slash 를 Unix 용 slash로 변경한다.
 */
char *
bslash2slash(char *instr)
{
	int		i, bQuote = 0;

	for (i = 0; i < strlen(instr); i++)
	{
		if (instr[i] == '"')
		{
			if (bQuote) break;
			else        bQuote = 1;
		}
		if (instr[i] == '\\')
			instr[i] = '/';
	}
	return(instr);
}

int resolve_path(char* warnStr, int doSaveSrc)
{
	int		n;
	int		ofs = 0;
	char	*cp1, *cp2;
	char const *cp0 = &in_str5[0];

	cp1 = strchr(warnStr, ' ');
	if ((nResolvePath > 0) && (cp1 != NULL))
	{
		struct stat sb;

		/* in_str1: Get first word from warning message	*/
		n = cp1 - warnStr;
		sprintf(in_str1, "%*.*s", n, n, warnStr);

		/* in_str2: Extract filename from in_str1		*/
		/*	   cp1: Filename, cp2: line number			*/
		strcpy(in_str2, in_str1);
		cp1 = strtok(in_str2, ":");
		cp2 = strtok(NULL,    ":");

		/* in_str3: Archive name from in_str2			*/
		strcpy(in_str4, in_str2);
		strtok(in_str4, "(");
		// fprintf(stderr, "In_str4 = %s\n", in_str4);

		/* Check whether source is not found, so should be re-resolved */
		if ( (strcmp(in_str4, "collect2") != 0) && (stat(in_str4, &sb) < 0) )
		{
			char *envList[] = { "DIST_LIB_MODULES", "DIST_SRC_MODULES" };
			char *envp, *ecp1, *ecp2;
			int	index, found = 0;

			for (index = 0; index < sizeof(envList)/sizeof(envList[0]); index++)
			{
				envp = getenv(envList[index]);
				if (NULL == envp)
					continue;
				envp = strdup(envp);
				// fprintf(stderr, "modules %s\n", envp);

				ecp1 = strtok(envp, " 	");
				while (ecp1 != NULL)
				{
					ecp2 = dirname(ecp1);
					sprintf(in_str5, "%s/%s", ecp2, in_str4);
					// fprintf(stderr, "checking file : %s\n", in_str5);
					if (stat(in_str5, &sb) >= 0)
					{
						ofs = strlen(in_str2) - strlen(in_str5);
						strcpy(in_str2, in_str5);
						// fprintf(stderr, "<<<< %s has been found\n", in_str5);
						found = 1;
					}
					ecp1 = strtok(NULL, " 	");
				}
				free(envp);

				if (found != 0)
					break;
			}
		}

		/* Save first occurrence of source file name */
		if ( (doSaveSrc != 0) && (gNumWarnFiles < 8) && (cp2 != NULL) && (*(cp2-2) != ')'))
		{
			for (n = 0; n < gNumWarnFiles; n++)
			{
				if (strcmp(gSource[n], in_str2) == 0)
					break;
			}
			if (n == gNumWarnFiles)
			{
				strcpy(gSource[gNumWarnFiles++], in_str2);
			}
		}

		#if 0
		if (cp2 != NULL)
		{
			// printf("===== in_str1=%s, %s, %d\n", in_str1, cp1, atoi(cp2));
			fprintf(OUTPUT_FILE, "%s", path_pfx);
		}
		#else
		if ((warnStr[0] != '/') && (cp1 != NULL) && strchr(basename(in_str2), '.'))
		{
			strcpy((char *)cp0, path_pfx);
			cp2 = (char *)cp0+strlen((char *)cp0);

			while (*cp1 != '\0')
			{
				// fprintf(stderr, "cp0 = [%s], cp1 = [%s]\n", cp0, cp1);
				/* 1. Remove "../" at the head of cp1		*/
				/* 2. Remove "XXXXX/" at the tail of cp0	*/
				if ( (cp1[0] == '.') && (cp1[1] == '.') && (cp1[2] == '/'))
				{
					char	*cp3;
					int		nSlash = 0;

					for (cp3 = cp2 - 1; cp3 >= cp0; cp3--)
					{
						if (*cp3 == '/')
						{
							if (++nSlash == 2)
							{
								cp3++;
								break;
							}
						}
					}

					if ( cp3 < cp0) cp3 = (char *)cp0;

					if ( (cp3 >= cp2) || (strcmp(cp3, "../") == 0) )
					{
						*cp2++ = '.';
						*cp2++ = '.';
						*cp2++ = '/';
					}
					else
					{
						cp2 = cp3;
					}

					ofs  += 3;
					cp1  += 3;
					*cp2  = '\0';
				}
				/* 1. Remove "./" at the head of cp1		*/
				else if ( (cp1[0] == '.') && (cp1[1] == '/'))
				{
					ofs += 2;
					cp1 += 2;
				}
				/* 1. Move "../" at the head of cp1 to the tail of cp0 */
				else
				{
					do
					{
						*cp2++ = *cp1;

						if (*cp1 == '\0')
							break;
						ofs++;
						if (*cp1++ == '/')
						{
							*cp2 = '\0';
							break;
						}
					} while (1);
				}
			}

			fprintf(OUTPUT_FILE, "%s", cp0);
		}
		#endif
	}
	return (ofs);
}

/*
 *	Warning message 가 Warning format list에 등록되어 있는지 검색하여,
 *	등록된 내용이면, skip하거나 강조하여 출력한다.
 */
int search_warn(char *warnStr)
{
	static int	nSupressed = 0;
	static int	nHeaderStack = 0;
	static char	sHeaderStack[32][512];
	char		*w_str1, *w_str2, *w_str3;
	char		*f_str1, *f_str2, *f_str3;
	int			hlColor = 0, type = 0;
	int			i, n, rc = 0;

	if (bPathConv)
	{
		(void)bslash2slash(warnStr);
	}

	for (i = 0; i < nSupItems; i++)
	{
		w_str1 = warn_fmts[i].sw_str[0];
		w_str2 = warn_fmts[i].sw_str[1];
		w_str3 = warn_fmts[i].sw_str[2];

		f_str1 = strstr(warnStr, w_str1);
		f_str2 = strstr(warnStr, w_str2);

		if ( (f_str1 != NULL) && (f_str2 != NULL) )
		{
			if (warn_fmts[i].sw_len[2] > 0)
			{
				f_str3 = strstr(f_str2 + warn_fmts[i].sw_len[1], w_str3);
				if (f_str3 == NULL)
					continue;
			}

			type = warn_fmts[i].sw_type;

			if (type == 0)
			{
				nSupressed++;
				nHeaderStack = 0;
				return(i+1);
			}
			else if (type == -1)
			{
				fprintf(OUTPUT_FILE, warn_buf);
				fprintf(OUTPUT_FILE, "Compilation aborted\n");
				fflush(OUTPUT_FILE);
				exit(1);
			}
			else if (type < -1 )
			{
				bHasError = 1;
				nHiddenEr+= ( (type < -2) ? 1 : 0);
				hlColor = 1;
				rc = -1;
			}
			else if ((type >= 1) && (type <= 7))
			{
				hlColor = warn_fmts[i].sw_type;
				if (hlColor == 6) nMaxWarns++;
				rc = 1;
			}
			break;
		}
	}

	if (rc == 0)
	{
		/*
		 *	Detect file name in inclusion tree
		 *  0         1         2         3
		 *	0123456789012345678901234567890
		 *	In file included from ../../mw/include/mfs_api.h:41,
		 *	                 from e2fsprogs/mfsfsck.c:57:
		*/
		if ( (bSilent == 0) && (strncmp(" from ", &warnStr[16], 6) == 0) )
		{
			if (nHeaderStack < 32)
				sprintf(sHeaderStack[nHeaderStack++], "%s ", warnStr+22);
			return(0);
		}

		n = sscanf(warnStr,	"%s %d %s %s %d %s %d %s %d serious %s",
							in_str1,					// cm/cm_scan.c:
							&nTotWarns, in_str2, in_str3,	// 1 warnings (+
							&nSupWarns, in_str4,			// 1 suppressed),
							&nTotError, in_str5,			// 1 errors,
							&nSerError, in_str6				// 0 serious errors
						);

		if (n == 10)
		{
			nTotWarns -= nSupressed;
			nSupWarns += nSupressed;
		}
		else
		{
			n = sscanf(warnStr,	"%s %d %s %d %s %d serious %s",
								in_str1,
								&nTotWarns, in_str2,
								&nTotError, in_str5,
								&nSerError, in_str6
						);
			if (n == 7)
			{
				nTotWarns -= nSupressed;
				nSupWarns  = nSupressed;
			}
		}

		if ( (n == 7) || (n == 10) )
		{
			nTotError += nHiddenEr;

			if (nTotWarns || nTotError || nSerError)
			{
			  if (nSupWarns > 0)
			  {
				fprintf(OUTPUT_FILE, "%s %d warning%s (+ %d supressed), %d error%s, %d serious error%s\n",
								in_str1,
								nTotWarns, (nTotWarns > 1 ? "s" : ""),
								nSupWarns,
								nTotError, (nTotError > 1 ? "s" : ""),
								nSerError, (nSerError > 1 ? "s" : "")
							);
			  }
			  else
			  {
				fprintf(OUTPUT_FILE, "%s %d warning%s, %d error%s, %d serious error%s\n",
								in_str1,
								nTotWarns, (nTotWarns > 1 ? "s" : ""),
								nTotError, (nTotError > 1 ? "s" : ""),
								nSerError, (nSerError > 1 ? "s" : "")
							);
			  }
			  fflush(OUTPUT_FILE);
			}
			return(0);
		}
	}

	if (strstr(warnStr, "warning:") != NULL)
	{
		nTotWarns++;
	}

	if (bSilent == 0)
	{
		int	pos0 =  resolve_path(warnStr, 1);
		int line;

//		if (hlColor && bIsatty)
		if (hlColor)
		{
			int		pos1, pos2, len1, len2;

			pos1 = strstr (warnStr, w_str1) - &warnStr[0];
			pos2 = strrchr(warnStr, '\n'  ) - &warnStr[0];

			len1 = pos1 - pos0;

			if ( (type <= -1) && (strncmp(&warnStr[pos1], "warning", 7) == 0) )
			{
				pos1 += 2;
				warnStr[pos1+0] = 'e';
				warnStr[pos1+1] = 'r';
				warnStr[pos1+2] = 'r';
				warnStr[pos1+3] = 'o';
				warnStr[pos1+4] = 'r';
			}
			else if (type == -9)
			{
				len2 = sprintf(&warnStr[pos1], "ERROR: Invalid file inclusion, check inclusion path below");
				pos2 = pos1 + len2;
			}

			len2 = pos2 - pos1;

			fprintf(OUTPUT_FILE, "%*.*s\x1b[1;%dm%*.*s\x1b[0m\n",
								len1, len1, &warnStr[pos0], hlColor+30,
								len2, len2, &warnStr[pos1]);

		}
		else
		{
			fprintf(OUTPUT_FILE, "%s", &warnStr[pos0]);
		}

		for (n = 0; n < nHeaderStack; n++)
		{
			strcpy(in_str6, in_str5);
			warnStr = sHeaderStack[n];
			line = atoi(warnStr+resolve_path(warnStr,1)+1);
			if (bCppHack)
				line--;
			fprintf(OUTPUT_FILE, ":%d: info: this file has included \"%s\"\n", line, in_str6);
		}

		nHeaderStack = 0;

		fflush(OUTPUT_FILE);
	}

	return -1;
}

/*
 *	Warning list 를 화일로 부터 읽어들인다.
 */
int import_list(char *filename)
{
	FILE	*fp;
	char	*cp0, *cp1, *cp2, *cpp[3];
	int		i, n, sw_type;
	int		len1, len2;
	int		nQuotes, nItems;
	int		bCheckForcedError = 1;

	/*	Build filter configuration strings		*/
	nItems = 0;
	while (warn_fmts[nItems].sw_type < 0)
	{
		nItems++;
		for (i = 0; i < 3; i++)
		{
	    	warn_fmts[nItems].sw_len[i] = strlen(warn_fmts[nItems].sw_str[i]);
		}
	}

	if (nVerbose > 1)
	{
		fprintf(OUTPUT_FILE, "Data File  = %s\n", filename);
		fprintf(OUTPUT_FILE, "nSupItems1 = %d\n", nItems);
	}

	if ( (fp = fopen(filename, "r")) != NULL)
	{
		(void) fgets(warn_buf, 512, fp);

		while (!feof(fp))
		{
			nQuotes = 0;
			sw_type = 1;
			n = sscanf(warn_buf, "%d", &sw_type);

			if (n == 1)
			{
				cp0 = strchr(warn_buf, '"');
			}
			else
			{
				sw_type = 0;
				cp0     = &warn_buf[0];
				while (isspace(*cp0))
				{
					cp0++;
				}
			}

			if ((cp0 != NULL) && (*cp0 == '"'))
			{
				cpp[0] = cpp[1] = cpp[2] = (char *)"";

				for (nQuotes = 0; nQuotes < 3; nQuotes++)
				{
					cp1 = (cp0 ? strchr(cp0  , '"') : NULL);
					if (cp1 == NULL) break;
					cp2 = (cp1 ? strchr(cp1+1, '"') : NULL);
					if (cp2 == NULL) break;

					cpp[nQuotes] = cp1+1;
					*cp2 = '\0';
					cp0 = cp2 + 1;
				}

				if ( (cp2 != NULL) && (nQuotes >= 2) )
				{
					int insertPos;

					if (nVerbose > 1)
					{
						fprintf(OUTPUT_FILE, "%d:[%s][%s][%s]\n", sw_type, cpp[0], cpp[1], cpp[2]);
					}

					if ((bCheckForcedError > 0) && (sw_type == -9))
					{
						warn_fmts[nItems - 0] = warn_fmts[nItems - 1];
						warn_fmts[nItems - 1] = warn_fmts[nItems - 2];
						insertPos = nItems - 2;
					}
					else
					{
						bCheckForcedError = 1;
						insertPos = nItems;
					}

		        	warn_fmts[insertPos].sw_type = sw_type;
					for (i = 0; i < 3; i++)
					{
		        		snprintf(warn_fmts[insertPos].sw_str[i], 40, "%s", cpp[i]);
	    				warn_fmts[insertPos].sw_len[i] = strlen(warn_fmts[insertPos].sw_str[i]);
					}
					nItems++;

				}
			}

			(void) fgets(warn_buf, 512, fp);
		}

		fclose(fp);
	}

	if (nVerbose > 1)
	{
		fprintf(OUTPUT_FILE, "nSupItems2 = %d\n", nItems);
	}

	return(nItems);
}


/*
 *	Parse command line arguements, and do ....
 */
int
siftWarnMain(char *cmdName, char *buf)
{
	char *cp, *cp1, *cp2;
	int	len;

	/*	Make data filename from command name		*/
	sprintf(in_str1, "%s.pattern", cmdName);

	/*	Call warning list builder					*/
	nSupItems = import_list(in_str1);

	/*	Calculate relative path prefix from make	*/
	cp1 = getenv("PWD_LEVEL0");		// Can be NULL
	cp2 = getcwd(in_str2, 256);		// Can not be NULL
	cp1 = strcpy(in_str1, (cp1 == NULL) ? cp2 : cp1);

	/*	Now both cp1 and cp2 is not null			*/
	/*	Skip different point						*/
	if (nResolvePath == 1)
	{
		char *pLastTop1 = cp1, *pLastTop2 = cp2;

		// fprintf(stderr, "+++ i: pwd0 = '%s', pwd1 = '%s'\n", cp1, cp2);
		for ( ; *cp1 && *cp2 && (*cp1 == *cp2); cp1++, cp2++)
		{
			if (*cp1 == '/') pLastTop1 = cp1+1;
			if (*cp2 == '/') pLastTop2 = cp2+1;
		}

		// fprintf(stderr, "+++ 0: pwd0 = '%s', pwd1 = '%s'\n", cp1, cp2);
		if ( ((*cp1 == '/') || (*cp1 == '\0')) && ((*cp2 == '/') || (*cp2 == '\0')) )
		{
			if (*cp1 == '/') cp1++;
			if (*cp2 == '/') cp2++;
		}
		else
		{
			cp1 = pLastTop1;
			cp2 = pLastTop2;
		}

		// fprintf(stderr, "+++ 1: pwd0 = '%s', pwd1 = '%s'\n", cp1, cp2);

		if (*cp1)
		{
			path_pfx[0] = '\0';
			cp1 = strtok(cp1, "/");
			while (cp1 != NULL)
			{
				sprintf(path_pfx, "%s../", path_pfx);
				cp1 = strtok(NULL, "/");
			}
		}
		if (cp2 && *cp2)
			sprintf(path_pfx, "%s%s/", path_pfx, cp2);

		// fprintf(stderr, "+++ 2: path_pfx = '%s'\n", path_pfx);
	}
	else if (nResolvePath == 2)
	{
		sprintf(path_pfx, "%s/", in_str2);
	}

	/* Check output is terminal or file				*/
	bIsatty = isatty(1);

	/*	Now read sift waring messages by filter DB	*/
	while ((cp = strchr(buf, '\n')) != NULL)
	{
		len = cp - buf + 1;
		snprintf(warn_buf, sizeof(warn_buf), "%*.*s", len, len, buf);
		buf += len;
		(void) search_warn(&warn_buf[0]);
	}

	if ( (bHasError == 0) && (nTotError == 0) && (nSerError == 0) && (nTotWarns <= nMaxWarns) )
		return(0);
	else
		return(1);
}
