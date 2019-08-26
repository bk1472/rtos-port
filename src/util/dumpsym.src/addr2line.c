/******************************************************************************
 *	 ROBOT LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 *	 Copyright(c) 2018 by LG Electronics Inc.
 *
 *	 All rights reserved. No part of this work may be reproduced, stored in a
 *	 retrieval system, or transmitted by any means without prior written
 *	 permission of LG Electronics Inc.
 *****************************************************************************/

/** addr2line.c
 *
 *	symbol address line libraries
 *
 *	@author		Baekwon Choi (baekwon.choi@lge.com)
 *	@version    1.0
 *	@date       2018. 3. 29
 *	@note
 *	@see
 */

/*-----------------------------------------------------------------------------
 *	Control Constants
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *	#include Files
------------------------------------------------------------------------------*/
#ifdef	MKSYM
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdint.h>
#include	"symtypes.h"
#include	"util.h"
#include	"dwarf.h"
#include	"def.h"

#else	/*!MKSYM*/

#include	"symbol.h"

#endif	/*MKSYM*/

/*-----------------------------------------------------------------------------
 *	Constant Definitions
------------------------------------------------------------------------------*/
#define	NUM_FILES		1024
#define	NUM_DIRS		1024

/*-----------------------------------------------------------------------------
 *	Macro Definitions
------------------------------------------------------------------------------*/
#ifdef	MKSYM
#define tprint0n		printf
#define	CHECK_READELF_OUTPUT
#undef	PRINT
#ifdef	CHECK_READELF_OUTPUT
extern unsigned int		verbose;
#define	PRINT(x...)		if (verbose == 2) { printf(x); fflush(stdout); }
#else
#define	PRINT(x...)
#endif
#define	CHECK_ENDIAN
#ifdef	CHECK_ENDIAN
extern int need_swap;
#endif
extern char *find_comp_dir(unsigned long);
#else	/*!MKSYM*/
#undef	CHECK_READELF_OUTPUT
#undef	PRINT

#ifdef	CHECK_READELF_OUTPUT
#define	PRINT(x...)		rprint0n(x);
#else
#define	PRINT(x...)
#endif
#endif	/*MKSYM*/



/*-----------------------------------------------------------------------------
 *	Type Definitions
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *	External Variables & Function prototypes declarations
------------------------------------------------------------------------------*/

/*
 *	CHECK_READELF_OUTPUT
 *		- define it to compare the search result with readelf command as followed.
 *		  # readelf -wl -e ucos > out.readelf
 *		  # mkbiz       -a ucos > out.mkbiz
 *		  # diff out.readelf out.mkbiz
 */

#ifdef	MKSYM
 #if (DEBUG > 0)
 extern int debug_lvl_ad;
 #endif
#endif	/*MKSYM*/

/*-----------------------------------------------------------------------------
 *	Global Variables declarations
------------------------------------------------------------------------------*/
#ifdef	MKSYM
uint64_t		dwarfTbl[3*MAX_DWARF_PKT];	/* Sort table for Dwarf	          */
uint64_t		dwarfLst[2*MAX_DWARF_PKT];	/* Pointer to dwarf packet        */
#else	/*!MKSYM*/
uint64_t		*dwarfLst = NULL;			/* Pointer to dwarf packet        */
#endif	/*MKSYM*/
uint64_t		nDwarfLst = 0;				/* Number of dwarf packets        */
unsigned char 	*pDwarfData = 0;
unsigned int	bFullPath = 1;


/*-----------------------------------------------------------------------------
 *	Static Variables & Function prototypes declarations
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *	Static Function Definition
------------------------------------------------------------------------------*/
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

#ifndef	MKSYM
static
#endif /* MKSYM */
int64_t getLLong(char *pSrc)
{
	int64_t	data;
	char *pDst = (char *)&data;

	#ifdef	CHECK_ENDIAN
	if (need_swap)
	{
		pDst[0] = pSrc[7];
		pDst[1] = pSrc[6];
		pDst[2] = pSrc[5];
		pDst[3] = pSrc[4];
		pDst[4] = pSrc[3];
		pDst[5] = pSrc[2];
		pDst[6] = pSrc[1];
		pDst[7] = pSrc[0];
	}
	else
	{
	#endif
		pDst[0] = pSrc[0];
		pDst[1] = pSrc[1];
		pDst[2] = pSrc[2];
		pDst[3] = pSrc[3];
		pDst[4] = pSrc[4];
		pDst[5] = pSrc[5];
		pDst[6] = pSrc[6];
		pDst[7] = pSrc[7];
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

/*-----------------------------------------------------------------------------
 *	Global Function Definition
------------------------------------------------------------------------------*/
#ifdef	MKSYM
int compare_line_info(const void *a, const void *b)
{
	uint64_t *ia = (uint64_t *)a, *ib = (uint64_t *)b;

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

/*-----------------------------------------------------------------------------
 *	API Function Definition
------------------------------------------------------------------------------*/
#define LineInfo_t DWARF2_Internal_LineInfo
int searchLineInfo(char **ppDebugLine, uint64_t *pSize, uint64_t srchAddr, char **ppFileName)
{
	LineInfo_t		Linfo;

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
	uint64_t		address = 0;		/* Current address */
	uint64_t		low_pc;				/* lowest address in current packet */
	uint64_t		high_pc;			/* lowest address in current packet */
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
	if(debug_lvl_ad > 0)
		tprint0n("searchLineInfo(0x%x, %d, 0x%lx, 0x%x)\n", *ppDebugLine, *pSize, srchAddr, ppFileName);
	#endif

	#ifdef MKSYM
	if ((pDwarfData == NULL) && (srchAddr == -1) && (ppFileName == NULL))
	{
		pLastPkt = pNewDwarfData = malloc(*pSize+0x1000); /* Spare 4k for directory expansion */
		pDwarfData = (unsigned char *)pLastPkt;
	}
	#endif

	PRINT("\n");
	PRINT("Dump of debug contents of section .debug_line:\n");
	PRINT("\n");

	cp        = *ppDebugLine;
	secEndPtr = *ppDebugLine + *pSize;

	while (cp < secEndPtr)
	{
		unsigned int offs_size;
		unsigned int addr_size = sizeof(long);
		dwarfStart					= cp;

		Linfo.li_length				= getLong(cp); cp += 4;
		offs_size                   = 4;
		if(Linfo.li_length == 0xffffffff)
		{
			Linfo.li_length	= getLLong(cp); cp += 8;
			offs_size       = 8;
		}
		else if (Linfo.li_length == 0 && addr_size == 8)
		{
			Linfo.li_length	= getLong(cp); cp += 4;
			offs_size       = 8;
		}
		pLineEnd					= cp + Linfo.li_length;

		Linfo.li_version			= getShort(cp);
		if(Linfo.li_version < 2 || Linfo.li_version > 4)
			break;
		cp += 2;

		if(offs_size == 4)
		{
			Linfo.li_prologue_length	= getLong(cp);
		}
		else
		{
			Linfo.li_prologue_length	= getLLong(cp);
		}
		cp += offs_size;

		if (Linfo.li_version >= 4)
		{
			Linfo.li_max_ops_per_insn = (unsigned char)(*cp++);
		}
		else
		{
			Linfo.li_max_ops_per_insn = 1;
		}
		if(Linfo.li_prologue_length == 0)
			break;

		Linfo.li_min_insn_length	= (unsigned char)(*cp++);
		Linfo.li_default_is_stmt	= (unsigned char)(*cp++);
		Linfo.li_line_base      	= (signed   char)(*cp++);
		Linfo.li_line_range     	= (unsigned char)(*cp++);
		Linfo.li_opcode_base    	= (unsigned char)(*cp++);

		#if	0
		hexdump("DwarfPacket", dwarfStart, 0x100);
		hexdump("DwarfPacket", pLineEnd,   0x100);
		#endif

		PRINT("  Length:                      %u\n",Linfo.li_length);
		PRINT("  DWARF Version:               %u\n",Linfo.li_version);
		PRINT("  Prologue Length:             %u\n",Linfo.li_prologue_length);
		PRINT("  Minimum Instruction Length:  %u\n",Linfo.li_min_insn_length);
		PRINT("  Initial value of 'is_stmt':  %u\n",Linfo.li_default_is_stmt);
		PRINT("  Line Base:                   %d\n",Linfo.li_line_base);
		PRINT("  Line Range:                  %u\n",Linfo.li_line_range);
		PRINT("  Opcode Base:                 %u\n",Linfo.li_opcode_base);

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
			for (i = 1; i < Linfo.li_opcode_base; i++)
				PRINT("  Opcode %d has %d args\n", i, opcodeLen[i]);

			cp = cp + Linfo.li_opcode_base - 1;
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

		if (cp >= (dwarfStart + Linfo.li_length + 4))
		{
			if (cp > (dwarfStart + Linfo.li_length + 4))
				PRINT("overrun %x :: %xn\n", cp, dwarfStart + Linfo.li_length + 4);
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
			is_stmt = Linfo.li_default_is_stmt;
			basic_block = 1;

			for (i = 0; bEos == 0; i++)
			{
				opcode = *cp++;

				if (opcode >= Linfo.li_opcode_base)
				{
					unsigned int	addrInc, lineInc;

					/* Mark valid line_info has been added */
					bAdded = 1;

					if (bNewPc || (address < low_pc)) { bNewPc = 0; low_pc = address; }

					/* Line and Address increment */
					opcode	-= Linfo.li_opcode_base;

					lineInc  = Linfo.li_line_base + (opcode % Linfo.li_line_range);
					addrInc	 = ((opcode - lineInc + Linfo.li_line_base) / Linfo.li_line_range) * Linfo.li_min_insn_length;
					lineNo	+= lineInc;
					address	+= addrInc;
					PRINT("  Special opcode %d: advance Address by %d to 0x%x and Line by %d to %d\n",
								opcode, addrInc, address, lineInc, lineNo);

					if (srchAddr < address)
					{
						#if (DEBUG > 0)
						if(debug_lvl_ad > 0)
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
						//PRINT("  Length = %x\n", opcode);
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
								if(debug_lvl_ad > 0)
									tprint0n("Found at line case[2] %d, addr = 0x%x\n", prevNo, address);
								#endif
								*ppFileName = _basename(fileList[fileNo-1]);
								return(prevNo);
							}
							break;
						  }
						  case DW_LNE_set_address :	 // 2,
						  {
							address = getLLong(cp); cp += 8;
							if (bNewPc || (address < low_pc)) { bNewPc = 0; low_pc = address; }
							if (address > high_pc) high_pc = address;
					  		PRINT("  Extended opcode %d: set Address to 0x%lx\n", opcode, address);
							break;
						  }
						  case DW_LNE_define_file : // 3
						  {
							unsigned int ch1, ch2, ch3;
							char *name;

							name = cp;
							if (*cp == '\0')
								cp += 1;
							else
								cp  += strlen(name) + 1;
							ch1  = decodeULEB128(&cp);	/* Dir */
							ch2  = decodeULEB128(&cp);	/* Time */
							ch3  = decodeULEB128(&cp);	/* Size */
							PRINT("  Define file :  %d	%d	%d	%d	%s\n", numFiles, ch1, ch2, ch3, name);
							if (numFiles < NUM_FILES)
								fileList[numFiles] = name;

							#if 0
							hexdump("DW_LNE_define_file", name, 0x80);
							#endif
							break;
						  }

						 case DW_LNE_set_discriminator:
						 {
						 	PRINT("DW_LNE_set_discriminator(%x)\n", opcode);
						  	decodeULEB128(&cp);
							break;
						 }
						default:
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
						unsigned int	addrInc;

						addrInc	 = decodeULEB128(&cp) * Linfo.li_min_insn_length;
						address	+= addrInc;
						PRINT("  Advance PC by %d to %x\n", addrInc, address);
						if (srchAddr < address)
						{
							#if (DEBUG > 0)
							if(debug_lvl_ad > 0)
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
						unsigned int	addrInc;

						addrInc	 = Linfo.li_min_insn_length * ((255-Linfo.li_opcode_base)/Linfo.li_line_range);
						address	+= addrInc;
						PRINT("  Advance PC by constant %d to 0x%x\n", addrInc, address);
						break;
					  }
					  case DW_LNS_fixed_advance_pc :	// 9,
					  {
						unsigned int	addrInc;

						addrInc	 = (unsigned)getShort(cp); cp += 2;
						address	+= addrInc;
						PRINT("  Command Fixed Advance PC\n");
						break;
					  }
					  case DW_LNS_set_prologue_end:
						 break;

					  case DW_LNS_set_epilogue_begin:
						 break;

					  case DW_LNS_set_isa:
					  {
							decodeULEB128(&cp);
					   		break;
					  }
					  default:
					  {
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
		}
		#ifdef	MKSYM
		else
		{
			/* If search address given, decode just 1 dwarf packet */
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
				#if (DEBUG > 0)
				if(debug_lvl_ad > 1)
				{
					for (i = 0; i < numFiles; i++)
					{
						if (fileUsed[i])
						{
							newFileName = concat_filename(comp_dir, dirList, dirIndex[i], fileList[i]);
							printf("[File_%d] %s\n", i, newFileName);
							free(newFileName);
						}
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
		qsort(&dwarfTbl[0], nDwarfLst, 3 * sizeof(uint64_t), compare_line_info);

		#if (DEBUG > 0)
		if(debug_lvl_ad > 1)
		{
			tprint0n("Table Before pack\n");
			for (i = 0; i < nDwarfLst; i++)
				tprint0n("== [%3d] vAddr 0x%016lx..0x%016lx : fOffset 0x%06lx\n",
								i, dwarfTbl[3*i+0], dwarfTbl[3*i+1], dwarfTbl[3*i+2]);
		}
		#endif

		for (i = 0; i < nDwarfLst; i++)
		{
			dwarfLst[2*i+0] = dwarfTbl[3*i+0];
			dwarfLst[2*i+1] = dwarfTbl[3*i+2];
			dwarfLst[2*i+2] = dwarfTbl[3*i+1];
		}
		nDwarfLst++;

		#if (DEBUG > 0)
		if(debug_lvl_ad > 1)
		{
			tprint0n("Table After pack\n");
			for (i = 0; i < nDwarfLst; i++)
				tprint0n("== [%3d] vAddr 0x%016lx : fOffset 0x%06x\n", i, dwarfLst[2*i+0], dwarfLst[2*i+1]);
		}
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
}	/*end of searchLineInfo*/

int addr2line(uint64_t addr, char **ppFileName)
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
		char		*pDwarf = (char *)pDwarfData + dwarfLst[2*x+1];
		uint64_t	size	= 8 + getLLong(pDwarf);

		#if (DEBUG > 0)
		if(debug_lvl_ad > 0)
			tprint0n("Address is in dwarf packet %d, [%x .. %x]\n", x, pDwarf, pDwarf+size);
		#endif
		lineNo = searchLineInfo(&pDwarf, &size, addr, ppFileName);
		#if (DEBUG > 0)
		if(debug_lvl_ad > 0)
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

	if(debug_lvl_ad <= 0)
		return;

//	for (addr = loadAddr; addr < (loadAddr + 0x80000); addr += 0x20)
	for (addr = loadAddr; addr < (loadAddr + 0x80000); addr += 0x4)
	{
		lineNo = addr2line(addr, &pFileName);
	}
	#endif
}
#endif /* MKSYM */
