/*
 *	This file will be used in
 *		1) mkbiz command
 *			Build .debug_line index table and shrink the .debug_line section
 *		2) Stack tracer in target board
 *			Change the pc to filename:line_number pair to ease to debug
 */
#ifdef	MKSYM

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "mytype.h"

#define	tprint0n		printf

#define	CHECK_READELF_OUTPUT
#undef	PRINT

#ifdef	CHECK_READELF_OUTPUT
extern unsigned long	verbose;
#define	PRINT(x...)		if (verbose && (aux_opts & AUX_OPT_VERBOSE)) { printf(x); fflush(stdout); }
#else
#define	PRINT(x...)
#endif

#define	CHECK_ENDIAN
#ifdef	CHECK_ENDIAN
extern int need_swap;
#endif

extern char *find_comp_dir(unsigned long);

#else

#include <osa/osadap.h>
#include <string.h>

#undef	CHECK_READELF_OUTPUT
#undef	PRINT

#ifdef	CHECK_READELF_OUTPUT
#define	PRINT(x...)		rprint0n(x);
#else
#define	PRINT(x...)
#endif

#endif	/* MKSYM */

#include "dwarf2.h"


/*
 *	CHECK_READELF_OUTPUT
 *		- define it to compare the search result with readelf command as followed.
 *		  # readelf -wl -e ucos > out.readelf
 *		  # mkbiz       -a ucos > out.mkbiz
 *		  # diff out.readelf out.mkbiz
 */

#define	DEBUG	0

#ifdef	MKSYM
unsigned int	dwarfTbl[3*MAX_DWARF_PKT];	/* Sort table for Dwarf				*/
unsigned int	dwarfLst[2*MAX_DWARF_PKT];	/* Pointer to dwarf packet			*/
unsigned int	nDwarfLst = 0;				/* Number of dwarf packets			*/
#else
unsigned int	*dwarfLst = 0;				/* Pointer to dwarf packet			*/
unsigned int	nDwarfLst = 0;				/* Number of dwarf packets			*/
#endif	/* MKSYM */
unsigned char 	*pDwarfData = 0;
unsigned int	bFullPath = 1;

#define	NUM_FILES		64
#define	NUM_DIRS		64

#ifndef	MKSYM
static
#endif /* MKSYM */
short getShort(char *pSrc)
{
	short data;
	char *pDst = (char *)&data;

	#ifdef	CHECK_ENDIAN
	if (need_swap)
	{
		pDst[0] = pSrc[1];
		pDst[1] = pSrc[0];
	}
	else
	{
	#endif
		pDst[0] = pSrc[0];
		pDst[1] = pSrc[1];
	#ifdef	CHECK_ENDIAN
	}
	#endif
	return(data);
}

#ifndef	MKSYM
static
#endif /* MKSYM */
int getLong(char *pSrc)
{
	int	data;
	char *pDst = (char *)&data;

	#ifdef	CHECK_ENDIAN
	if (need_swap)
	{
		pDst[0] = pSrc[3];
		pDst[1] = pSrc[2];
		pDst[2] = pSrc[1];
		pDst[3] = pSrc[0];
	}
	else
	{
	#endif
		pDst[0] = pSrc[0];
		pDst[1] = pSrc[1];
		pDst[2] = pSrc[2];
		pDst[3] = pSrc[3];
	#ifdef	CHECK_ENDIAN
	}
	#endif
	return(data);
}

static unsigned int decodeULEB128 (char *cpp[])
{
	unsigned int  result;
	int           shift;
	unsigned char byte;

	result   = 0;
	shift    = 0;

	do
    {
		byte = *cpp[0]; cpp[0] += 1;
		result |= ((byte & 0x7f) << shift);
		shift += 7;
    }
	while (byte & 0x80);

	return result;
}

static int decodeSLEB128(char *cpp[])
{
	int           result;
	int           shift;
	unsigned char byte;

	result = 0;
	shift = 0;

	do
	{
		byte = *cpp[0]; cpp[0] += 1;
		result |= ((byte & 0x7f) << shift);
		shift += 7;
	}
	while (byte & 0x80);

	if ((shift < 32) && (byte & 0x40))
		result |= -(1 << shift);

	return result;
}

static char * _basename (const char *name)
{
  const char *base;

  if (bFullPath)
    return (char *)name;

  for (base = name; *name; name++)
    {
      if ((*name == '/') || (*name == '\\'))
	{
	  base = name + 1;
	}
    }
  return (char *) base;
}

#ifdef	MKSYM
int compare_line_info(const void *a, const void *b)
{
	int *ia = (int *)a, *ib = (int *)b;

	return( ( (ia[0] > ib[0]) ? 1 : ((ia[0] == ib[0]) ? 0 : -1) ) );
}

/* Remove relative referencings in pathname */
static char * demangle_pathname(char *pathname)
{
  char *pFirstDot, *pLastSlash;
  int len;

  pLastSlash = pathname;
  while (*pLastSlash)
  {
    if (*pLastSlash == '\\') *pLastSlash = '/';
	pLastSlash++;
  }

  pLastSlash = pathname;
  while ((pFirstDot = strchr(pLastSlash, '.')) != NULL)
  {
    if (pFirstDot[1] == '.') len = 2;
    else                     len = 1;

    if ((pFirstDot[len] != '/') && (pFirstDot[len] != '\\'))
    {
      pLastSlash += len + 1;
      continue;
    }

    pLastSlash = pFirstDot - len;

    while (pLastSlash >= pathname)
    {
      if ((*pLastSlash == '/') || (*pLastSlash == '\\'))
      {
        strcpy(pLastSlash+1, pFirstDot+len+1);
        pFirstDot = pLastSlash + 1;
        break;
      }
      pLastSlash--;
    }
  }
  return(pathname);
}

/* Extract a fully qualified filename from a line info table.
   The returned string has been malloc'ed and it is the caller's
   responsibility to free it.  */

static char * concat_filename (char *comp_dir, char *dirList[], int dir, char *file_name)
{
  char *retFile = file_name;

  if ((*retFile != '/') && (*retFile != '\\'))
    {
      char *dirname = (dir ? dirList[dir-1] : NULL);

      /* Not all tools set DW_AT_comp_dir, so dirname may be unknown.
         The best we can do is return the retFile part.  */
      if (dirname != NULL)
        {
          unsigned int len = strlen (dirname) + strlen (retFile) + 2;
	      unsigned int dir_len = strlen(dirname);
          char * name;

	      if ((dirname[dir_len-1] == '\\') || (dirname[dir_len-1] == '/'))
            dirname[dir_len-1] = '\0';
          name = malloc (len);
          if (name)
            sprintf (name, "%s/%s", dirname, retFile);
          retFile = name;
        }
      else
        {
          retFile = strdup(retFile);
        }

      if ((*retFile != '/') && (*retFile != '\\') && (comp_dir != NULL))
        {
          unsigned int len = strlen (comp_dir) + strlen (retFile) + 2;
	      unsigned int dir_len = strlen(comp_dir);
          char * name;

	      if ((comp_dir[dir_len-1] == '\\') || (comp_dir[dir_len-1] == '/'))
            comp_dir[dir_len-1] = '\0';
          name = malloc (len);
          if (name)
            sprintf (name, "%s/%s", comp_dir, retFile);
          free(retFile);
          return  demangle_pathname(name);
        }
      return demangle_pathname(retFile);
    }

  return demangle_pathname(strdup(retFile));
}
#endif /* MKSYM */

int searchLineInfo(char **ppDebugLine, size_t *pSize, unsigned int srchAddr, char **ppFileName)
{
	unsigned long	length;				/* Copy of Dwarf line info header */
	unsigned short	version;			/* Copy of Dwarf line info header */
	unsigned int	prologue_length;	/* Copy of Dwarf line info header */
	unsigned char	insn_min;			/* Copy of Dwarf line info header */
	unsigned char	default_is_stmt;	/* Copy of Dwarf line info header */
	int				line_base;			/* Copy of Dwarf line info header */
	unsigned char	line_range;			/* Copy of Dwarf line info header */
	unsigned char	opcode_base;		/* Copy of Dwarf line info header */
	unsigned int	ptr_size;			/* Size of pointer, fixed to 4 */

	int				i;
	char			*cp;
	char			*dwarfStart;		/* Start of current input dwarf packet */
	char			*pLineEnd;			/* End of current input dwarf packet */
	char			*secEndPtr;			/* End of debug_line section */
	char			*pEncStream;		/* Start of encoded opcode in current input packet */
	#ifdef	MKSYM
	char			*pNewDwarfData;		/* Start of new dwarf section to be stored */
	char			*pLastPkt;			/* Last write pointer in output dwarf packet */
	char			*comp_dir;			/* Base directory of compilation */
	char			*opcodeLen; 		/* Standard opcode length */
	#endif
	unsigned char	opcode;				/* current opcode */
	unsigned int	address = 0;		/* Current address */
	unsigned int	low_pc;				/* lowest address in current packet */
	unsigned int	high_pc;			/* lowest address in current packet */
	unsigned long	curr_offset = 0;	/* Current offset in input dwarf packet */
	unsigned int	lineNo, prevNo = 1;	/* Line number and previous line number */
	unsigned int	fileNo, newFno;		/* File number and new file number */
	int				bEos = 0, nAddedPkt = 0, bNewPc = 0, bAdded = 0;
	int				column, is_stmt, basic_block = 1;
	int				numFiles = 0;
	char			*fileList[NUM_FILES];
	#ifdef	MKSYM
	int				numDirs = 0;
	char			*dirList[NUM_DIRS];
	char			fileUsed[NUM_FILES], dirIndex[NUM_FILES];
	size_t			sTotal = 0, sEmpty = 0, sValid = 0;
	size_t			nTotal = 0, nEmpty = 0, nValid = 0;
	#endif /* MKSYM */


	#if (DEBUG > 0)
	tprint0n("searchLineInfo(0x%x, %d, 0x%x, 0x%x)\n", *ppDebugLine, *pSize, srchAddr, ppFileName);
	#endif

	#ifdef MKSYM
	if ((pDwarfData == NULL) && (srchAddr == -1) && (ppFileName == NULL))
	{
		pLastPkt = pNewDwarfData = malloc(*pSize+0x1000); /* Spare 4k for directory expansion */
		pDwarfData = pLastPkt;
	}
	#endif

	PRINT("\n");
	PRINT("Dump of debug contents of section .debug_line:\n");
	PRINT("\n");

	cp = *ppDebugLine;
	secEndPtr = *ppDebugLine + *pSize;

	/*
	 *	Support 32bit format only
	 */
	while (cp < secEndPtr)
	{
		dwarfStart		= cp;
		ptr_size		= 4;
		length			= getLong(cp);  cp += 4;
		pLineEnd		= cp + length;
		version			= getShort(cp); cp += 2;
		prologue_length	= getLong(cp);  cp += 4;
		insn_min		= (unsigned char)(*cp++);
		default_is_stmt	= (unsigned char)(*cp++);
		line_base		= (signed   char)(*cp++);
		line_range		= (unsigned char)(*cp++);
		opcode_base		= (unsigned char)(*cp++);

		#if	0
		hexdump("DwarfPacket", dwarfStart, 0x100);
		hexdump("DwarfPacket", pLineEnd,   0x100);
		#endif

		PRINT("  Length:                      %u\n", length);
		PRINT("  DWARF Version:               %u\n", version);
		PRINT("  Prologue Length:             %u\n", prologue_length);
		PRINT("  Minimum Instruction Length:  %u\n", insn_min);
		PRINT("  Initial value of 'is_stmt':  %u\n", default_is_stmt);
		PRINT("  Line Base:                   %d\n", line_base);
		PRINT("  Line Range:                  %u\n", line_range);
		PRINT("  Opcode Base:                 %u\n", opcode_base);
		PRINT("  (Pointer size:               %u)\n", ptr_size);

		#ifdef	MKSYM
		for (i = 0; i < NUM_FILES; i++)
		{
			fileUsed[i] = 0;
			dirIndex[i] = 0;
		}

		if (srchAddr == -1)
		{
			opcodeLen = cp - 1;
			PRINT("\n");
			PRINT(" Opcodes:\n");
			for (i = 1; i < opcode_base; i++)
				PRINT("  Opcode %d has %d args\n", i, opcodeLen[i]);

			cp = cp + opcode_base - 1;
			PRINT("\n");

			comp_dir = find_comp_dir(curr_offset);
			if (comp_dir)
				PRINT(" comp_dir = %s\n\n", comp_dir);

			numDirs = 0;
			if (cp[0] == 0x00)
			{
				PRINT(" The Directory Table is empty.\n");
			}
			else
			{
				PRINT(" The Directory Table:\n");
				do
				{
					PRINT("  %s\n", cp);
					if (numDirs < NUM_DIRS)
					{
						dirList[numDirs++] = cp;
					}
					cp += strlen(cp) + 1;
				} while (*cp != 0x00);
			}
			cp++;
		}
		#endif /* MKSYM */

		PRINT("\n");
		i = numFiles = 0;
		if (cp[0] == 0x00)
		{
			PRINT(" The File Name Table is empty.\n");
		}
		else
		{
			PRINT(" The File Name Table:\n");
			PRINT("  Entry	Dir	Time	Size	Name\n");
			while (cp[0] != 0x00)
			{
				unsigned int ch1 = 0, ch2 = 0, ch3 = 0;
				char *name;

				i++;
				name = cp;
				cp  += strlen(name) + 1;
				if (srchAddr == -1)				/* Removed in packed debug_line */
				{
					ch1  = decodeULEB128(&cp);	/* Dir */
					ch2  = decodeULEB128(&cp);	/* Time */
					ch3  = decodeULEB128(&cp);	/* Size */
				}
				PRINT("  %d	%d	%d	%d	%s\n", i, ch1, ch2, ch3, name);
				if (numFiles < NUM_FILES)
				{
					#ifdef	MKSYM
					dirIndex[numFiles  ] = ch1;
					#endif /* MKSYM */
					fileList[numFiles++] = name;
				}
			}
		}
		cp++;

		PRINT("\n");
		PRINT(" Line Number Statements:\n");

		if (cp >= (dwarfStart + length + 4))
		{
			if (cp > (dwarfStart + length + 4))
				PRINT("overrun %x :: %xn\n", cp, dwarfStart + length + 4);
			PRINT("\n");
		}

		nAddedPkt = 0;			/* Number of packets added */
		pEncStream = cp;		/* Start pointer of encoded actual line info stream */

		while (cp < pLineEnd)
		{
			opcode	= 0;
			lineNo	= 1;
			prevNo	= 1;
			fileNo	= 1;
			newFno	= 1;
			column	= 0;
			address	= 0;
			bEos	= 0;
			low_pc  = 0;
			bNewPc	= 1;
			bAdded 	= 0;
			high_pc = 0;
			is_stmt = default_is_stmt;
			basic_block = 1;

			for (i = 0; bEos == 0; i++)
			{
				opcode = *cp++;

				if (opcode >= opcode_base)
				{
					int	addrInc, lineInc;

					/* Mark valid line_info has been added */
					bAdded = 1;

					if (bNewPc || (address < low_pc)) { bNewPc = 0; low_pc = address; }

					/* Line and Address increment */
					opcode	-= opcode_base;

					lineInc  = line_base + (opcode % line_range);
					addrInc	 = ((opcode - lineInc + line_base) / line_range) * insn_min;
					lineNo	+= lineInc;
					address	+= addrInc;
					PRINT("  Special opcode %d: advance Address by %d to 0x%x and Line by %d to %d\n",
								opcode, addrInc, address, lineInc, lineNo);

					if (srchAddr < address)
					{
						#if (DEBUG > 0)
						tprint0n("Found at line case[1] %d, addr = 0x%x\n", prevNo, address);
						#endif
						*ppFileName = _basename(fileList[fileNo-1]);
						return(prevNo);
					}
					fileNo = newFno;

					#ifdef	MKSYM
					fileUsed[fileNo-1] = TRUE;
					#endif /* MKSYM */

					prevNo	 = lineNo;
					if (address > high_pc) high_pc = address;
				}
				else
				{
					switch (opcode)
					{
					  case DW_LNS_extended_op : 	 // 0,
					  {
						opcode = *cp++; /* Ignore length */
//						PRINT("  Length = %x\n", opcode);
						opcode = *cp++;

						switch (opcode)
						{
						  case DW_LNE_end_sequence : // 1,
						  {
							int  nStuff = 0;

							PRINT("  Extended opcode %d: End of Sequence\n", opcode);
							/*
							 *		Work arround for ARM axf file.
							 *	Norcroft ARM compiler add stuffing bytes to make 4 byte
							 *	align for the next packet.
							 *	It seems that there's a stuffing byte at the end of
							 *	Dwarf packet. Just skip it.
							 */
							if (cp >= pLineEnd)
							{
								for (nStuff = 0; (nStuff < 16) && (cp < secEndPtr); nStuff++, cp++)
								{
									if ((getLong(cp) != 0) && (getShort(cp+4) == 2))
										break;
									#if	0
									PRINT("  %08x %08x\n", getLong(cp), getLong(cp+4));
									#endif
								}
								#if	0
								if (nStuff)
								{
									PRINT("  nStuff=%d, cp =0x%x, pLineEnd=0x%x\n", nStuff, cp, pLineEnd);
								}
								#endif
							}
							else if (cp < pLineEnd)
							{
								PRINT("  More Sequence Presents\n");
							}
							bEos = 1;
							if (bNewPc || (address < low_pc)) { bNewPc = 0; low_pc = address; }
							if (address > high_pc) high_pc = address;
							if (srchAddr < address)
							{
								#if (DEBUG > 0)
								tprint0n("Found at line case[2] %d, addr = 0x%x\n", prevNo, address);
								#endif
								*ppFileName = _basename(fileList[fileNo-1]);
								return(prevNo);
							}
							break;
						  }
						  case DW_LNE_set_address :	 // 2,
						  {
							address = getLong(cp); cp += 4;
							if (bNewPc || (address < low_pc)) { bNewPc = 0; low_pc = address; }
							if (address > high_pc) high_pc = address;
					  		PRINT("  Extended opcode %d: set Address to 0x%x\n", opcode, address);
							break;
						  }
						  case DW_LNE_define_file : // 3
						  {
							unsigned int ch1, ch2, ch3;
							char *name;

							name = cp;
							cp  += strlen(name) + 1;
							ch1  = decodeULEB128(&cp);	/* Dir */
							ch2  = decodeULEB128(&cp);	/* Time */
							ch3  = decodeULEB128(&cp);	/* Size */
							PRINT("  Define file :  %d	%d	%d	%d	%s\n", numFiles, ch1, ch2, ch3, name);
							if (numFiles < NUM_FILES)
								fileList[numFiles] = name;
							break;
						  }
						  default :
						  {
							PRINT("Unknown extended opcode(%x)\n", opcode);
							break;
						  }
						}
						break;
					  }
					  case DW_LNS_copy : 				// 1,
					  {
						/* Mark valid line_info has been added */
						bAdded = 1;

						prevNo = lineNo;
						PRINT("  Copy\n");
						break;
					  }
					  case DW_LNS_advance_pc : 			// 2,
					  {
						int	addrInc;

						addrInc	 = decodeULEB128(&cp) * insn_min;
						address	+= addrInc;
						PRINT("  Advance PC by %d to %x\n", addrInc, address);
						if (srchAddr < address)
						{
							#if (DEBUG > 0)
							tprint0n("Found at line case[3] %d, addr = 0x%x\n", prevNo, address);
							#endif
							*ppFileName = _basename(fileList[fileNo-1]);
							return(prevNo);
						}
						fileNo = newFno;

						#ifdef	MKSYM
						fileUsed[fileNo-1] = TRUE;
						#endif /* MKSYM */

						break;
					  }
					  case DW_LNS_advance_line : 		// 3,
					  {
						int	lineInc;

						lineInc	 = decodeSLEB128(&cp);
						lineNo	+= lineInc;
						PRINT("  Advance Line by %d to %d\n", lineInc, lineNo);
						break;
					  }
					  case DW_LNS_set_file : 			// 4,
					  {
						newFno = decodeULEB128(&cp);
						PRINT("  Set File Name to entry %d in the File Name Table\n", newFno);
						break;
					  }
					  case DW_LNS_set_column : 			// 5,
					  {
						column = decodeULEB128(&cp);
						PRINT("  Set column to %u\n", column);
						break;
					  }
					  case DW_LNS_negate_stmt : 		// 6,
					  {
						is_stmt = !is_stmt;
						PRINT("  Set is_stmt to %d\n", is_stmt);
						break;
					  }
					  case DW_LNS_set_basic_block :	 	// 7,
					  {
						basic_block = 1;
						PRINT("  Set basic to 1\n");
						break;
					  }
					  case DW_LNS_const_add_pc : 		// 8,
					  {
						int	addrInc;

						addrInc	 = insn_min * ((255-opcode_base)/line_range);
						address	+= addrInc;
						PRINT("  Advance PC by constant %d to 0x%x\n", addrInc, address);
						break;
					  }
					  case DW_LNS_fixed_advance_pc :	// 9,
					  {
						int	addrInc;

						addrInc	 = (unsigned)getShort(cp); cp += 2;
						address	+= addrInc;
						PRINT("  Command Fixed Advance PC\n");
						break;
					  }
					}
				}
			}

			#ifdef	MKSYM
			if (srchAddr == -1)
			{
				if (bAdded)
				{
					/* Mark start/end of current packet */
					dwarfTbl[3*nDwarfLst+0] = low_pc;
					dwarfTbl[3*nDwarfLst+1] = high_pc;
					dwarfTbl[3*nDwarfLst+2] = dwarfStart - *ppDebugLine;

					#if	0
					PRINT("Adding[%d] %d bytes, low = %x, high = %x\n",
							nDwarfLst, getLong(dwarfStart) + 4, low_pc, high_pc);
					#endif

					nDwarfLst++;
					nAddedPkt++;
				}
			}
			#endif /* MKSYM */
		}

		if (srchAddr != -1)
		{
			*ppFileName = (char *)"??";
			return(0);
			/* If search address given, decode just 1 dwarf packet */
		}
		#ifdef	MKSYM
		else
		{
			int	pktSize = getLong(dwarfStart) + 4;	/* Size of original packet */
			int	packedSize = 0, tmp;				/* Size of packeed packet */

			if (nAddedPkt)
			{
				char *newFileName, *pNewPkt;
				size_t len;

				/*
				 *	Trim data which will not be used in target board
				 *		1. Opdoce length array
				 *		2. File table entry not referrenced
				 *		3. Merge directory names ane filename to pathname
				 *		4. Remove opcode length field
				 *	or Remove packet which has no effecitive debug_line data
				 */
				#if (DEBUG > 1)
				for (i = 0; i < numFiles; i++)
				{
					if (fileUsed[i])
					{
						newFileName = concat_filename(comp_dir, dirList, dirIndex[i], fileList[i]);
						printf("[File_%d] %s\n", i, newFileName);
						free(newFileName);
					}
				}
				#endif

				/* Copy headers including length, version .. etc */
				pNewPkt = pLastPkt;
				memcpy(pLastPkt, dwarfStart, 15);	pLastPkt += 15;

				/* Do not encode Directory Table */

				/* Encode File Table */
				for (i = 0; i < numFiles; i++)
				{
					if (fileUsed[i])
					{
						newFileName = concat_filename(comp_dir, dirList, dirIndex[i], fileList[i]);
						len = strlen(newFileName);
						memcpy(pLastPkt, newFileName, len+1); pLastPkt += len + 1;
						free(newFileName);
					}
				}
				*pLastPkt++ = '\0';		/* Terminating List */

				/* Update prologue length */
				tmp = (pLastPkt - (pNewPkt + 4 + 2 + 4));
				tmp = getLong((char *)&tmp);
				memcpy(pNewPkt + 4 + 2, &tmp, 4);

				/* Copy actual statement of programms */
				memcpy(pLastPkt, pEncStream, pktSize - (pEncStream - dwarfStart));
				pLastPkt += pktSize - (pEncStream - dwarfStart);

				/* Update packet length at the start of packet */
				packedSize = pLastPkt - pNewPkt;
				tmp = packedSize - 4;
				tmp = getLong((char *)&tmp);
				memcpy(pNewPkt, &tmp, 4);

				/*
				printf("Packed : %d -> %d\n", pktSize, packedSize);
				printf("%x || %x\n", packedSize, tmp);
				*/

				/* Update offset information */
				if (dwarfStart != pLastPkt)
				{
					for (i = nDwarfLst - nAddedPkt; i < nDwarfLst; i++)
					{
						dwarfTbl[3*i+2] = pNewPkt - pNewDwarfData;
					}
				}

				/* Add size of valid packet */
				sValid += packedSize;
				sEmpty += pktSize - packedSize;
				nValid++;
			}
			else
			{
				/* Add size of useless packet */
				sEmpty += pktSize;
				nEmpty++;

				#if	0
				PRINT("Removing Packet\n");
				#endif
			}

			sTotal += pktSize;
			nTotal++;
		}
		#endif /* MKSYM */

		curr_offset += cp - dwarfStart;

		#ifdef	MKSYM
		if (nAddedPkt)
			PRINT("\n\n");
		#endif /* MKSYM */
	}

	#ifdef	MKSYM
	if (srchAddr == -1)
	{
		qsort(&dwarfTbl[0], nDwarfLst, 3 * sizeof(int), compare_line_info);

		#if (DEBUG > 1)
		tprint0n("Table Before pack\n");
		for (i = 0; i < nDwarfLst; i++)
			tprint0n("== [%3d] vAddr 0x%08x..0x%08x : fOffset 0x%06x\n",
							i, dwarfTbl[3*i+0], dwarfTbl[3*i+1], dwarfTbl[3*i+2]);
		#endif

		for (i = 0; i < nDwarfLst; i++)
		{
			dwarfLst[2*i+0] = dwarfTbl[3*i+0];
			dwarfLst[2*i+1] = dwarfTbl[3*i+2];
			dwarfLst[2*i+2] = dwarfTbl[3*i+1];
		}
		nDwarfLst++;

		#if (DEBUG > 1)
		tprint0n("Table After pack\n");
		for (i = 0; i < nDwarfLst; i++)
			tprint0n("== [%3d] vAddr 0x%08x : fOffset 0x%06x\n", i, dwarfLst[2*i+0], dwarfLst[2*i+1]);
		#endif

		PRINT("Total %d (Empty %d + Valid %d) bytes\n", sTotal, sEmpty, sValid);
		PRINT("Total %d (Empty %d + Valid %d) packets\n", nTotal, nEmpty, nValid);

		/* Free input debug line */
		free(*ppDebugLine);

		/* Update size and debug line data */
		*pSize = pLastPkt - pNewDwarfData;
		*ppDebugLine = pNewDwarfData;
	}
	#endif /* MKSYM */

	return 0;
}

int addr2line(unsigned int addr, char **ppFileName)
{
	int		x, l = 0, r = nDwarfLst-1, matched = 0;
	int		lineNo = 0;

	if (dwarfLst == NULL)
		return 0;

	do
	{
		x = (l + r) / 2;
		if      (addr < dwarfLst[2*x+0]) { matched = 0; r = x - 1; }
		else if (addr < dwarfLst[2*x+2]) { matched = 1; l = x + 1; }
		else                             { matched = 0; l = x + 1; }

	} while ((l <= r) && (matched == 0));

	if (matched)
	{
		char	*pDwarf = (char*)pDwarfData + dwarfLst[2*x+1];
		size_t	size	= 4 + getLong(pDwarf);

		#if (DEBUG > 0)
		tprint0n("Address is in dwarf packet %d, [%x .. %x]\n", x, pDwarf, pDwarf+size);
		#endif
		lineNo = searchLineInfo(&pDwarf, &size, addr, ppFileName);
		#if (DEBUG > 0)
		tprint0n("%08x ==> %s:%d\n", addr, *ppFileName, lineNo);
		#endif
	}
	else
	{
		*ppFileName = (char *)"Not Found";
	}
	return lineNo;
}

#ifdef	MKSYM
void testAddr2Line(int loadAddr)
{
	#if (DEBUG > 0)
	int		addr;
	int		lineNo;
	char	*pFileName;

//	for (addr = loadAddr; addr < (loadAddr + 0x80000); addr += 0x20)
	for (addr = loadAddr; addr < (loadAddr + 0x80000); addr += 0x4)
	{
		lineNo = addr2line(addr, &pFileName);
	}
	#endif
}
#endif /* MKSYM */
