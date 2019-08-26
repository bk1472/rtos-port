#include <common.h>
#include <string.h>
#include <ctype.h>
#include <osa/osa_utils.h>

/*
**	Implement of extended strdup() not supported in pSOS
*/
char *
strdup2(const char *src, size_t *dstLen)
{
	size_t	len;
	char 	*dst;

	len = strlen(src);
	if (dstLen != NULL) *dstLen = len;
	dst = (char *)malloc(len+1);
	memcpy(dst, src, len);
	dst[len] = '\0';
	return ((char *) dst);
}

/*
**	Implement of libc extended strdup() not supported in pSOS
*/
char *
octsdup(const char *src, size_t len)
{
	char *dst;

	dst = (char *)malloc(len+1);
	memcpy(dst, src, len);
	dst[len] = '\0';
	return ((char *) dst);
}

/*
**	Change all upper case characters in string to lower case.
*/
char *
strlwr( const char *s_str )
{
	char *s = (char *) s_str;

	if (s == NULL) return(s);
	while (*s)
	{
		if (isupper(*s)) *s += 0x20;		//	To Lower Case
		s++;
	}
	return ((char *) s_str);
}

/*
**	Change all lower case characters in string to upper case.
*/
char *
strupr( const char *s_str )
{
	char *s = (char *) s_str;

	if (s == NULL) return(s);
	while (*s)
	{
		if (islower(*s)) *s -= 0x20;		//	To Upper Case
		s++;
	}
	return ((char *) s_str);
}

#if 0
/*
**	Implement of libc strncasecmp() not supported in pSOS
*/
int
strncasecmp(char *a, char *b, size_t len)
{
	size_t	n = 0;

	if ((a == NULL) && (b == NULL)) return  0;
	if ((a == NULL) && (b != NULL)) return -1;
	if ((a != NULL) && (b == NULL)) return  1;

	while ((*a) && (n < len))
	{
		if (tolower(*a) != tolower(*b))
			return((int)(*a - *b));
		a++;
		b++;
		n++;
	}
	if ((n == len) || (*b == '\0'))
		return 0;
	return(-(int)(*b));
}
#endif

#if 0
/*
**	Implement of libc strncasecmp() not supported in pSOS
*/
int
strcasecmp(char *a, char *b)
{
	return(strncasecmp(a, b, (size_t)-1));
}
#endif

/*
**	An Extended strtoul() to support binary input
*/
unsigned long
strtoul2(char *s, char **ptr, int base)
{
	unsigned long res;
	char *t;

	if (s == NULL)
	{
		if (ptr != NULL) *ptr = NULL;
		return 0;
	}

	/*	Remove heading spaces	*/
	while ((*s == '\t') || (*s == ' '))
	{
		if (*s == '\0') break;
		s++;
	}

	if ((base == 0) && (*s == 'b'))
	{
		base = 2;
		s++;
	}

	if (base == 2)
	{
		for (t=s; (*t == '0') || (*t == '1'); t++)
			;
	}
	else
	{
		for (t=s; isxdigit(*t) || (*t == 'x') || (*t == 'X'); t++)
			;
	}

	res = strtoul(s, ptr, base);

    switch (tolower(*t))
	{
		case 'r' : res = (res << 13); break;
		case 'k' : res = (res << 10); break;
		case 'm' : res = (res << 20); break;
		case 'g' : res = (res << 30); break;
	}

	return(res);
}

/*
**	An Extended strtok() to support binary input
*/
char
*strtok2(char *srcStr, char *del, char **ptr)
{
	char	*resStr;
	size_t	srcLen, resLen;

	if (del == NULL)
	{
		del = (char *)"\t ";
	}

	/* Normal Type Of strtok	*/
	if (ptr == NULL)
	{
		return(strtok(srcStr, del));
	}

	if (srcStr && *srcStr)
	{
		srcLen = strlen(srcStr);
		resStr = strtok(srcStr, del);
		resLen = strlen(resStr);
	}
	else
	{
		srcStr = NULL;
		srcLen = 0;
		resStr = NULL;
		resLen = 0;
	}

	if (resStr && ((srcStr+srcLen) > (resStr+resLen)))
		*ptr = (resStr + resLen + 1);
	else
		*ptr = NULL;
	return(resStr);
}

/*
**	An Extended function to trim heading spaces
*/
char *
strtrim(char *src)
{
	if (src == NULL)
		return src;

	while ((*src == '\t') || (*src == ' '))
	{
		if (*src == '\0') break;
		src++;
	}

	return(src);
}

/*
**	An externed function to build argv list from string.
**	It need one room to NULL terminate argv list.
*/
int
str2argv(char *line, int limit, char **argv)
{
	register int	argc = 0;

	if (limit < 0) limit = 0;

    while (*line)
	{
		/* Skip Heading Spaces characters	*/
        while (*line && isspace(*line))
            line++;

		/* Check end of string				*/
        if (*line == '\0') break;

		/* Check end of argv list storage 	*/
		if ( (argc+1) >= limit) break;

		argc++;
        *argv++ = line;

		/* Find Next Space characters		*/
        while ( *line && !isspace(*line))
            line++;

		/* Check end of string				*/
        if (*line == '\0') break;

		/* Null Terminate argument			*/
        *line++ = '\0';
    }

	if (argc < limit)
		*argv = NULL;

	return(argc);
}

/*
 *	Get enumerated index of command 'cmd' from option list 'opts'
 *	Return 0  if 'cmd' is found, and index is stored at *output
 *	      -1  if 'cmd' is not in 'opts' list
 */
int str2indexInOpts(char **argv, char *opts, int *output)
{
	char	*incp, *cp1, *cp2;
	int		res, len, min, max, opt_flag = FALSE;
	int		i;

	#define	END_DELIM	"^] \t"
	#define	OPT_DELIM	",|"
	#define	ALL_DELIM	OPT_DELIM	END_DELIM


	if ((opts == NULL) || (output == NULL))
		return -1;		/* Indicate Error */

	cp1 = opts;
	cp2 = strchr(cp1, '^');

	if (cp2) len = cp2 - cp1;
	else     len = strlen(cp1);

	*output  = -1;		/* Initialize output index as not found	*/

	if (sscanf(cp1, "[%d..%d]", &min, &max) == 2)
	{
		opt_flag = TRUE;
		if (argv != NULL)
		{
			*output = (int) strtoul2(*argv, NULL, 0);
			if ((*output < min) || (*output > max))
			{
				*output = -1;
				return -1;
			}
		}
	}
	else if (sscanf(cp1, "%d..%d", &min, &max) == 2)
	{
		if (argv == NULL)
			return -1;

		*output = (int) strtoul2(*argv, NULL, 0);
		if ((*output < min) || (*output > max))
		{
			*output = -1;
			return -1;
		}
	}
	else if (sscanf(cp1, "[0x%x..0x%x]", &min, &max) == 2)
	{
		opt_flag = TRUE;
		if (argv != NULL)
		{
			*output = (int) strtoul2(*argv, NULL, 16);
			if ((*output < (unsigned)min) || (*output > (unsigned)max))
			{
				*output = -1;
				return -1;
			}
		}
	}
	else if (sscanf(cp1, "0x%x..0x%x", &min, &max) == 2)
	{
		if (argv == NULL)
			return -1;

		*output = (int) strtoul2(*argv, NULL, 16);
		if ((*output < (unsigned)min) || (*output > (unsigned)max))
		{
			*output = -1;
			return -1;
		}
	}
	else
	{
		if ((cp1[0] == '[') && (cp1[len-1] == ']'))
		{
			opt_flag = TRUE;
			cp1 = cp1 + 1;
			len = len - 2;
		}

		if (argv == NULL)
		{
			/* Cancel search operation */
			*output = -1;
			if (!opt_flag) return -1;
			else           return  0;
		}

		res  = 0;
		incp = *argv;

		while (len > 0)
		{
			int		tRes = 0;
			void	*fmt = (void *)(((cp1[0]=='0') && (cp1[1]=='x')) ? (char *)"%x" : (char *)"%d");

			if (cp1[0] != '_')
				if (sscanf(cp1, (const char *)fmt, &tRes) == 1) res = tRes;

			for (i=0; (cp1 && incp[i]); i++)
			{
				if (cp1[i] != incp[i])
					break;
			}

			if ( !incp[i] && (!cp1[i] || strchr(ALL_DELIM, cp1[i])))
			{
				/* Option Mached */
				break;
			}

			for (cp2=cp1; cp2[0] != '\0'; cp2++)
			{
				if (strchr(ALL_DELIM, cp2[0]))
					break;
			}

			if ( (cp1[0] == '\0') || (cp2[0] == '\0') || strchr(END_DELIM, cp2[0]) )
			{
				/* Option is not in option list*/
				return -1;
			}

			if (cp2[0] != ',')
				res++;

			len -= (cp2 - cp1) + 1;
			cp1  = cp2 + 1;

		}
		*output = res;
	}

	return 0;
}

/*
**	Implementation of bitmask operators
*/
void *
bitmask_alloc(size_t last_pos)
{
	size_t			num_words = ((last_pos + 7) >> 3 ) + 4;
	unsigned char	*bitmask;

	bitmask  = (unsigned char *)malloc(num_words);
	memset(bitmask, 0, num_words);
	*(size_t *)bitmask = last_pos;

	bitmask += sizeof(size_t);
	return((void *)bitmask);
}

void
bitmask_free(void *arg)
{
	unsigned char	*bitmask = (unsigned char *)arg;

	bitmask -= sizeof(size_t);
	free((void *)bitmask);
}

void
bitmask_set(void *arg, unsigned char set_pos)
{
	unsigned char	*bitmask = (unsigned char *)arg;
	size_t			last_pos = *((size_t *)bitmask - 1);

	if (set_pos > last_pos)
	{
		dbgprint("Bitmask Overflow: last_pos=%d, set_pos=%d", last_pos, set_pos);
		return;
	}

	bitmask[set_pos/8] |= (1 << (set_pos % 8));
	return;
}

void
bitmask_clear(void *arg, unsigned char set_pos)
{
	unsigned char	*bitmask = (unsigned char *)arg;
	size_t			last_pos = *((size_t *)bitmask - 1);

	if (set_pos > last_pos)
	{
		dbgprint("Bitmask Overflow: last_pos=%d, set_pos=%d", last_pos, set_pos);
		return;
	}

	bitmask[set_pos/8] &= ~(1 << (set_pos % 8));
	return;
}

int
bitmask_isset(void *arg, unsigned char test_pos)
{
	unsigned char	*bitmask = (unsigned char *)arg;
	size_t			last_pos = *((size_t *)bitmask - 1);

	if (test_pos > last_pos)
	{
		dbgprint("Bitmask Overflow: last_pos=%d, test_pos=%d", last_pos, test_pos);
		return 0;
	}

	if (bitmask[test_pos/8] & (1 << (test_pos % 8)))
		return 1;
	else
		return 0;
}
