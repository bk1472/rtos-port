/* ccdv.c */
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>

#define SETCOLOR_SUCCESS	(gANSIEscapes ? "\033\1331;32m" : "")
#define SETCOLOR_FAILURE	(gANSIEscapes ? "\033\1331;31m" : "")
#define SETCOLOR_WARNING	(gANSIEscapes ? "\033\1331;33m" : "")
#define SETCOLOR_NORMAL		(gANSIEscapes ? "\033\1330;39m" : "")

#define TEXT_BLOCK_SIZE		8192
#define INDENT				2

#define	TOOL_NAME			"ccdv"

extern int siftWarnMain(char *cmdName, char *buf);
extern int bPathConv;		/* Dos style path를 Unix style Path 로 변경	*/
extern int bSilent;			/* Quiet   mode or not						*/
extern int nResolvePath;	/* Resolve heading pathname in warning msg	*/
extern int nVerbose;		/* Verbose mode or not						*/
extern int nMaxWarns;		/* Maximum number of warns allowed			*/
extern int bCppHack;		/* Enable cpp processing hack function		*/

size_t gNBufUsed = 0, gNBufAllocated = 0;
char *gBuf = NULL;
int gCCPID;
char gAction[256] = "";
char gTarget[256] = "";
char gOutput[256] = "";
char gSource[8][PATH_MAX] = { "", };
int gNumWarnFiles = 0;
char gAr[32] = "";
char gArLibraryTarget[256] = "";
int gDumpCmdArgs = 0;
size_t gArgsBufUsed = 0, gArgsBufSize = 0;
char *gArgsStr = NULL;
int gColumns = 80;
int gANSIEscapes = 0;
int gExitStatus = 95;
int gCcCmd = 0;
int gCcOpt = 0;
int gNewCcdv = 1;
char *cmdName;
char gPathname[256];

static void DumpFormattedOutput(void)
{
	char *cp;
	char spaces[8 + 1] = "        ";
	char *saved;
	int curcol;
	int i;
	size_t len = strlen(gArgsStr);

	if ( (gCcCmd > 0) || (gNewCcdv > 0) )
	{
		int	rc;

		if ((gDumpCmdArgs > 0) || (nVerbose > 0))
		{
			cp = gBuf;
			/*
			 * Change ':' in command line options to ';',
			 * Since vi treat ':' as a delimeter to finding the position of filename,
			 */
			while ((cp = strchr(cp, ':')) != NULL)
			{
				if (isdigit(cp[-1]) && isdigit(cp[1]))
					*cp = '.';
				cp++;
			}
			printf("%*.*s", len, len, gBuf);
		}
		if (siftWarnMain(cmdName, gBuf+strlen(gArgsStr)) != 0)
			gExitStatus = 90;
	}
	else
	{
		curcol = 0;
		saved = NULL;
		for(cp = gBuf + ((gDumpCmdArgs == 0) ? len : 0);; cp++)
		{
			if(*cp == '\0')
			{
				if(saved != NULL)
				{
					cp = saved;
					saved = NULL;
				}
				else
					break;
			}
			if(*cp == '\r')
				continue;
			if(*cp == '\t')
			{
				saved = cp + 1;
				cp = spaces + 8 - (8 - ((curcol - INDENT - 1) % 8));
			}
			if(curcol == 0)
			{
				for(i = INDENT; --i >= 0;)
					putchar(' ');
				curcol = INDENT;
			}
			putchar(*cp);
			if(++curcol == (gColumns - 1))
			{
				putchar('\n');
				curcol = 0;
			}
			else if(*cp == '\n')
				curcol = 0;
		}
	}
	free(gBuf);

	if ( (gExitStatus == 90) && (gPathname[0] != '\0') )
		unlink(gPathname);

	return;
}	/* DumpFormattedOutput */

/* Difftime(), only for timeval structures.  */
static void TimeValSubtract(struct timeval *tdiff, struct timeval *t1,
							struct timeval *t0)
{
	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if(tdiff->tv_usec < 0)
	{
		tdiff->tv_sec--;
		tdiff->tv_usec += 1000000;
	}
}	/* TimeValSubtract */

static void Wait(void)
{
	int pid2, status;

	do
	{
		status = 0;
		pid2 = (int) waitpid(gCCPID, &status, 0);
	}
	while(((pid2 >= 0) && (!WIFEXITED(status)))
		  || ((pid2 < 0) && (errno == EINTR)));
	if(WIFEXITED(status))
		gExitStatus = WEXITSTATUS(status);
}	/* Wait */

static void SlurpTitle(char *s1, size_t len, char *tail)
{
    if ((gCcCmd == 0) || (gTarget[0] == 0))
	{
		snprintf(s1, len, "%s%s%s%s", gAction, gTarget[0] ? " " : "", gTarget, tail);
	}
	else
	{
		if (gOutput[0] == 0)
		{
			size_t len;
			char *cp;

			len = strlen(gTarget);
			cp = strrchr(gTarget, '.');
			if (cp == NULL) cp = gTarget + len;
			else            len = cp - gTarget;

			if      (gCcOpt == 1) snprintf(gOutput, sizeof(gOutput), "%*.*s.o", len, len, gTarget);
			else if (gCcOpt == 2) snprintf(gOutput, sizeof(gOutput), "%*.*s.i", len, len, gTarget);
			else if (gCcOpt == 3) snprintf(gOutput, sizeof(gOutput), "%*.*s.s", len, len, gTarget);
			else                  snprintf(gOutput, sizeof(gOutput), "%*.*s",   len, len, gTarget);

			cp = strrchr(gPathname, '.');
			if (cp == NULL) cp = &gPathname[strlen(gPathname)];
			if      (gCcOpt == 1) { cp[0] = '.'; cp[1] = 'o'; cp[2] = '\0'; }
			else if (gCcOpt == 2) { cp[0] = '.'; cp[1] = 'i'; cp[2] = '\0'; }
			else if (gCcOpt == 3) { cp[0] = '.'; cp[1] = 's'; cp[2] = '\0'; }
			else                  {                           cp[0] = '\0'; }
		}
		snprintf(s1, len, "%s %-32s from %s%s", gAction, gOutput, gTarget, tail);
	}
	return;
}

static int SlurpProgress(int fd)
{
	char s1[256];
	char *newbuf;
	int nready;
	size_t ntoread;
	ssize_t nread;
	struct timeval now, tnext, tleft;
	fd_set ss;
	fd_set ss2;
	const char *trail = "/-\\|", *trailcp;

	trailcp = trail;
	SlurpTitle(s1, sizeof(s1), " ...");
	printf("\r%-89s%-9s", s1, "");
	fflush(stdout);

	gettimeofday(&now, NULL);
	tnext = now;
	tnext.tv_sec++;
	tleft.tv_sec = 1;
	tleft.tv_usec = 0;
	FD_ZERO(&ss2);
	FD_SET(fd, &ss2);
	for(;;)
	{
		if(gNBufUsed == (gNBufAllocated - 1))
		{
			if((newbuf = (char *) realloc(gBuf, gNBufAllocated + TEXT_BLOCK_SIZE)) == NULL)
			{
				perror(TOOL_NAME ": realloc");
				return (-1);
			}
			gNBufAllocated += TEXT_BLOCK_SIZE;
			gBuf = newbuf;
		}
		for(;;)
		{
			ss = ss2;
			nready = select(fd + 1, &ss, NULL, NULL, &tleft);
			if(nready == 1)
				break;
			if(nready < 0)
			{
				if(errno != EINTR)
				{
					perror(TOOL_NAME ": select");
					return (-1);
				}
				continue;
			}
			gettimeofday(&now, NULL);
			if((now.tv_sec > tnext.tv_sec)
			   || ((now.tv_sec == tnext.tv_sec)
				   && (now.tv_usec >= tnext.tv_usec)))
			{
				tnext = now;
				tnext.tv_sec++;
				tleft.tv_sec = 1;
				tleft.tv_usec = 0;
				printf("\r%-89s%c%-7s", s1, *trailcp, "");
				fflush(stdout);
				if(*++trailcp == '\0')
					trailcp = trail;
			}
			else
			{
				TimeValSubtract(&tleft, &tnext, &now);
			}
		}
		ntoread = (gNBufAllocated - gNBufUsed - 1);
		nread = read(fd, gBuf + gNBufUsed, ntoread);
		if(nread < 0)
		{
			if(errno == EINTR)
				continue;
			perror(TOOL_NAME ": read");
			return (-1);
		}
		else if(nread == 0)
		{
			break;
		}
		gNBufUsed += nread;
		gBuf[gNBufUsed] = '\0';
	}
	SlurpTitle(s1, sizeof(s1), "");
	Wait();

	if(gExitStatus == 0)
	{
		printf("\r%-89s", s1);
		if((gNBufUsed - strlen(gArgsStr)) < 4)
			printf("[%s%s%s]", SETCOLOR_SUCCESS, "OK", SETCOLOR_NORMAL);
		else
			printf("[%s%s%s]", SETCOLOR_WARNING, "WARN", SETCOLOR_NORMAL);

		printf("%-5s\n", " ");
	}
	else
	{
		printf("\r%-89s", s1);
		printf("[%s%s%s]", SETCOLOR_FAILURE, "ERROR", SETCOLOR_NORMAL);
		printf("%-2s\n", " ");
		gDumpCmdArgs = 1;	/* print cmd when there are errors */
	}
	fflush(stdout);
	return (0);
}	/* SlurpProgress */

static int SlurpAll(int fd)
{
	char s1[256];
	char *newbuf;
	size_t ntoread;
	ssize_t nread;

	SlurpTitle(s1, sizeof(s1), "\n");
	printf(s1);
	fflush(stdout);

	for(;;)
	{
		if(gNBufUsed == (gNBufAllocated - 1))
		{
			if((newbuf = (char *) realloc(gBuf, gNBufAllocated + TEXT_BLOCK_SIZE)) == NULL)
			{
				perror(TOOL_NAME ": realloc");
				return (-1);
			}
			gNBufAllocated += TEXT_BLOCK_SIZE;
			gBuf = newbuf;
		}
		ntoread = (gNBufAllocated - gNBufUsed - 1);
		nread = read(fd, gBuf + gNBufUsed, ntoread);
		if(nread < 0)
		{
			if(errno == EINTR)
				continue;
			perror(TOOL_NAME ": read");
			return (-1);
		}
		else if(nread == 0)
		{
			break;
		}
		gNBufUsed += nread;
		gBuf[gNBufUsed] = '\0';
	}
	Wait();
	gDumpCmdArgs = (gExitStatus != 0);	/* print cmd when there are errors */
	return (0);
}	/* SlurpAll */

static const char *Basename(const char *path)
{
	const char *cp;
	cp = strrchr(path, '/');
	if(cp == NULL)
		return (path);
	return (cp + 1);
}	/* Basename */

static const char *Extension(const char *path)
{
	const char *cp = path;
	cp = strrchr(path, '.');
	if(cp == NULL)
		return ("");
	return (cp);
}	/* Extension */

static void Usage(void)
{
	char *sUsage =
			"Usage: ccdv /path/to/cc CFLAGS...\n"
			"\n"
			"I wrote this to reduce the deluge Make output to make finding actual problems\n"
			"easier.  It is intended to be invoked from Makefiles, like this.  Instead of:\n"
			"It will filter warning/error messages and highligt important one. To specify\n"
			"these messages to be colored, edit 'ccdv.error.lst'. See the help message in\n"
			"the sample error list file\n"
			"\n"
			"	.c.o:\n"
			"		$(CC) $(CFLAGS) $(DEFS) $(CPPFLAGS) $< -c\n"
			"\n"
			"Rewrite your rule so it looks like:\n"
			"\n"
			"	.c.o:\n"
			"		@ccdv $(CC) $(CFLAGS) $(DEFS) $(CPPFLAGS) $< -c\n"
			"	.cpp.o:\n"
			"		@ccdv $(CXX) $(CFLAGS) $(DEFS) $(CPPFLAGS) $< -c\n"
			"\n"
			"ccdv 1.2.0 is Free under the GNU Public License.  Enjoy!\n"
			"  -- Mike Gleason, NcFTP Software <http://www.ncftp.com>\n"
			"  -- John F Meinel Jr, <http://ccdv.sourceforge.net>\n"
			"  -- Baekwon, Choi, baekwon.choi@lge.com(build 2007/11/30)"
			"  ";
	fprintf(stderr, sUsage);
	exit(96);
}	/* Usage */

int main(int argc, char **argv)
{
	int pipe1[2];
	int devnull;
	char emerg[256];
	int fd;
	int nread;
	int i;
	int end_of_opt = 0;
	int cmd_index = 1;
	const char *quote;

	if(argc < 2)
		Usage();

	cmdName = argv[0];

	gArgsStr = calloc(1, TEXT_BLOCK_SIZE);
	gArgsBufSize = TEXT_BLOCK_SIZE;
	for(i = 1; i < argc; i++)
	{
		char	*basename;
		char	*extension;
		char	*crossBase;
		char	*optChars = &"/+-"[gCcOpt ? 1 : 0];
		char	*newbuf;
		size_t	currArgSize;

		if (end_of_opt == 0)
		{
			if (argv[i][0] == '-')
			{
				int		inc = 0, ofs = 0, val;

				if (argv[i][2] ==  0) inc = 1;
				else                  ofs = 2;
				val = atoi(argv[i+inc]+ofs);

				if      (argv[i][1] == 'n') {           gNewCcdv  	 = 1;   }
				else if (argv[i][1] == 'p') {           bPathConv 	 = 1;   }
				else if (argv[i][1] == 's') {           bSilent      = 1;   }
				else if (argv[i][1] == 'r') { i += inc; nResolvePath = val; }
				else if (argv[i][1] == 'v') { i += inc; nVerbose     = val; }
				else if (argv[i][1] == 'x') { i += inc; nMaxWarns    = val; }
				continue;
			}
			end_of_opt++;
			cmd_index = i;
		}
		else if (end_of_opt == 1)
		{
			end_of_opt++;
			snprintf(gAction, sizeof(gAction), "Running %s", Basename(argv[cmd_index]));
		}

		quote = (strchr(argv[i], ' ') != NULL) ? "\"" : "";
		currArgSize = strlen(argv[i]) + (*quote == '\0' ? 1 : 3); /* 2 quotes + 1 space */
		if ((gArgsBufSize - gArgsBufUsed) < (currArgSize+1))
		{
			if ((newbuf = (char *) realloc(gArgsStr, gArgsBufSize + TEXT_BLOCK_SIZE)) == NULL)
			{
				perror(TOOL_NAME ": realloc");
				return (-1);
			}
			gArgsBufSize += TEXT_BLOCK_SIZE;
			gArgsStr = newbuf;
		}
		gArgsBufUsed += snprintf(gArgsStr + gArgsBufUsed,
				 gArgsBufSize - gArgsBufUsed - 1, "%s%s%s%s",
				 quote, argv[i], quote,
				 (i == (argc - 1)) ? "\n" : " ");

		basename  = (char *)Basename(argv[i]);
		extension = (char *)Extension(argv[i]);

		crossBase = strrchr(basename, '-');
		if (crossBase != NULL) crossBase++;
		else                   crossBase = basename;

		if((strcmp(argv[i], "-o") == 0) && ((i + 1) < argc))
		{
			i++;
			quote = (strchr(argv[i], ' ') != NULL) ? "\"" : "";
			currArgSize = strlen(argv[i]) + (*quote == '\0' ? 1 : 3); /* 2 quotes + 1 space */
			if ((gArgsBufSize - gArgsBufUsed) < (currArgSize+1))
			{
				if ((newbuf = (char *) realloc(gArgsStr, gArgsBufSize + TEXT_BLOCK_SIZE)) == NULL)
				{
					perror(TOOL_NAME ": realloc");
					return (-1);
				}
				gArgsBufSize += TEXT_BLOCK_SIZE;
				gArgsStr = newbuf;
			}
			gArgsBufUsed += snprintf(gArgsStr + gArgsBufUsed,
					 gArgsBufSize - gArgsBufUsed - 1, "%s%s%s%s",
					 quote, argv[i], quote,
					 (i == (argc - 1)) ? "\n" : " ");
			basename  = (char *)Basename(argv[i]);
			extension = (char *)Extension(argv[i]);
			if ((extension[0] == '\0') ||
				((strcasecmp(extension, ".o") != 0)&&
			     (strcasecmp(extension, ".s") != 0)&&
			     (strcasecmp(extension, ".i") != 0)))
			{
				strcpy(gAction, "Linking");
				snprintf(gTarget, sizeof(gTarget), "%s", basename);
			}
			else if ((strstr(argv[cmd_index], "ld") != NULL) && (strcasecmp(extension, ".o") == 0))
			{
				strcpy(gAction, "Linking");
				snprintf(gTarget, sizeof(gTarget), "%s", argv[i]);
			}
			else
			{
				snprintf(gPathname, sizeof(gPathname), "%s", argv[i]);
				snprintf(gOutput, sizeof(gOutput), "%s", basename);
			}
		}
		else if(strcmp(argv[i], "-c") == 0)
		{
			gCcOpt = 1;
		}
		else if(strcmp(argv[i], "-E") == 0)
		{
			gCcOpt = 2;
		}
		else if(strcmp(argv[i], "-S") == 0)
		{
			gCcOpt = 3;
		}
		else if(strchr(optChars, (int) argv[i][0]) != NULL)
		{
			continue;
		}
		else if((strncasecmp(extension, ".c",   2) == 0) ||
                (strncasecmp(extension, ".s",   2) == 0) ||
                (strncasecmp(extension, ".i",   2) == 0) ||
                (strncasecmp(extension, ".cpp", 4) == 0) )
		{
			gCcCmd++;
			snprintf(gTarget, sizeof(gTarget), "%s", basename);
			if (gPathname[0] == '\0')
				snprintf(gPathname, sizeof(gPathname), "%s", argv[i]);
            if (strncasecmp(extension, ".i",   2) == 0)
				bCppHack = 1;
		}
		else if (i == cmd_index)
		{
			if      (strcmp(crossBase, "ar" ) == 0) strcpy(gAr, crossBase);
			else if (strcmp(crossBase, "ld" ) == 0) strcpy(gAction, "Linking");
			else if (strcmp(crossBase, "cc" ) == 0) strcpy(gAction, "Compiling");
			else if (strcmp(crossBase, "gcc") == 0) strcpy(gAction, "Compiling");
			else if (strcmp(crossBase, "g++") == 0) strcpy(gAction, "Compiling");
		}
		else if((gArLibraryTarget[0] == '\0') && (strcasecmp(extension, ".a") == 0))
		{
			snprintf(gArLibraryTarget, sizeof(gArLibraryTarget), "%s", basename);
		}
	}
	if((gAr[0] != '\0'))//&& (gArLibraryTarget[0] != '\0'))
	{
		strcpy(gAction, "Creating library");
		snprintf(gTarget, sizeof(gTarget), "%s", gArLibraryTarget);
	}
	else if (gCcCmd > 0)
	{
		strcpy(gAction, "Compiling");
	}

	if(pipe(pipe1) < 0)
	{
		perror(TOOL_NAME ": pipe");
		exit(97);
	}

	(void) close(0);
	devnull = open("/dev/null", O_RDWR, 00666);
	if((devnull != 0) && (dup2(devnull, 0) == 0))
		close(devnull);

	gCCPID = (int) fork();
	if(gCCPID < 0)
	{
		(void) close(pipe1[0]);
		(void) close(pipe1[1]);
		perror(TOOL_NAME ": fork");
		exit(98);
	}
	else if(gCCPID == 0)
	{
		/* Child */
		(void) close(pipe1[0]);	/* close read end */
		if(pipe1[1] != 1)
		{	/* use write end on stdout */
			(void) dup2(pipe1[1], 1);
			(void) close(pipe1[1]);
		}
		(void) dup2(1, 2);	/* use write end on stderr */
		execvp(argv[cmd_index], argv + cmd_index);
		perror(argv[cmd_index]);
		exit(99);
	}

	/* parent */
	(void) close(pipe1[1]);	/* close write end */
	fd = pipe1[0];	/* use read end */

	gColumns = (getenv("COLUMNS") != NULL) ? atoi(getenv("COLUMNS")) : 80;
	gANSIEscapes = (getenv("TERM") != NULL)
		&&
		(strstr
		 ("vt100:vt102:vt220:vt320:xterm:ansi:linux:scoterm:scoansi:dtterm:cons25:cygwin",
		  getenv("TERM")) != NULL);
	gBuf = (char *) malloc(gArgsBufSize);
	if(gBuf == NULL)
		goto panic;
	strcpy(gBuf, gArgsStr);
	gNBufUsed = gArgsBufUsed;
	gNBufAllocated = gArgsBufSize;

	if(isatty(1))
	{
		if(SlurpProgress(fd) < 0)
			goto panic;
	}
	else
	{
		if(SlurpAll(fd) < 0)
			goto panic;
	}
	DumpFormattedOutput();
	if ( (gNumWarnFiles > 0) && (gExitStatus == 90) )
	{
		char	excCmd[256+8*PATH_MAX];
		char	cmdRoot[256];
		char	*errMsgBuf, *scp, *dcp;
		int		count;

		strcpy(cmdRoot,cmdName);
		dirname(cmdRoot);

		errMsgBuf = malloc(2048);
		scp = gBuf + strlen(gArgsStr);
		for (dcp = errMsgBuf; (*scp != '\0') && ((dcp - errMsgBuf) < 2040); scp++, dcp++)
		{
			if      (*scp == '\n') { *dcp =  '+'; continue; }
			else if (*scp == '`' ) { *dcp = 0x27; continue; }
			else if (*scp == '\\') { *dcp = '\\'; dcp++;    }
			else if (*scp == '\"') { *dcp = '\\'; dcp++;    }
			else if (*scp == '|' ) { *dcp = '\\'; dcp++;    }
			else if (*scp == '*' ) { *dcp = '\\'; dcp++;    }
			else if (*scp == '&' ) { *dcp = '\\'; dcp++;    }
			else if (*scp == '$' ) { *dcp = '\\'; dcp++;    }
//			else if (*scp == '(' ) { *dcp = '\\'; dcp++;    }
//			else if (*scp == ')' ) { *dcp = '\\'; dcp++;    }
//			else if (*scp == '<' ) { *dcp = '\\'; dcp++;    }
//			else if (*scp == '>' ) { *dcp = '\\'; dcp++;    }
//			else if (*scp == '{' ) { *dcp = '\\'; dcp++;    }
//			else if (*scp == '}' ) { *dcp = '\\'; dcp++;    }
			*dcp = *scp;
		}
		*dcp = '\0';
		setenv("_XCMD_ERR_MSG", errMsgBuf, 1);
		if (strcmp(gAction, "Linking") == 0)
		{
			#if 0
			for (count = 0; count < gNumWarnFiles; count++)
			{
				sprintf(excCmd, "%s/last_author.sh %s", cmdRoot, gSource[count]);
				system(excCmd);
			}
			#endif
		}
		else if (strcmp(gAction, "Compiling") == 0)
		{
			#if 0
			int	cfile_index = -1;
			int	length;
			char *cp;

			for (count = 0; count < gNumWarnFiles; count++)
			{
				length = strlen(gSource[count]);
				cp = &gSource[count][length];
				if ( (strcmp(cp-2, ".c"  ) == 0) ||
					 (strcmp(cp-2, ".S"  ) == 0) ||
					 (strcmp(cp-4, ".cpp") == 0) )
				{
					cfile_index = count;
					break;
				}
			}

			sprintf(excCmd, "%s/last_author.sh %s", cmdRoot, gSource[cfile_index]);
			if (gNumWarnFiles > 1)
				strcat(excCmd, " -h");

			for (count = 0; count < gNumWarnFiles; count++)
			{
				if (count == cfile_index)
					continue;
				strcat(excCmd, " ");
				strcat(excCmd, gSource[count]);
			}
			system(excCmd);
			#endif
		}
	}
	exit(gExitStatus);

  panic:
	gDumpCmdArgs = 1;	/* print cmd when there are errors */
	DumpFormattedOutput();
	while((nread = read(fd, emerg, (size_t) sizeof(emerg))) > 0)
		(void) write(2, emerg, (size_t) nread);
	Wait();
	exit(gExitStatus);
}	/* main */

/* eof ccdv.c */
