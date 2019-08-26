#include	<stdlib.h>
#include	<string.h>
#include	<ar.h>
#include	<unistd.h>
#include	<sys/mman.h>

#include	"elf_func.h"
#include	"def.h"
#include	"dwarf.h"
#include	"process.h"


#if 0
typedef struct
{
	uint32_t magic;
	uint32_t _dummy;
	uint32_t total_size;
	uint32_t sym_num;
	uint32_t sym_str_tbl_size;
	uint32_t *pSymTabBase;
} symStorage_t;

typedef struct
{
	uint32_t bit_sz;  // bit_sz: 0 :16 bit, 2: 32 bit, 4: 64 bit
	uint32_t *pSymHashBase;
} symHash_t;

		total_size  <= 3*sizeof(uint32_t)*sym_num + sym_str_tbl_size;
char* 	pSymStrBase <= (char*)(pSymTabBase + 3 * sym_num)
	    --> system¿ bit_width¿ ¿¿¿¿ Logic
#endif

/**
 * Global Control Constant
 */
#if (DEBUG > 0)
extern int debug_lvl_pc;
#endif

#define SYM_MAGIC					0xB12791EE


char			versionBuf			[MAX_VER_LEN];
char			tmStampBuf			[MAX_VER_LEN];
static uchar_t	*pBinBuff          = NULL;

static ulong_t	sym_hash			[MAX_SYM_COUNT];
static char		pSym_buff			[MAX_SYM_BUFF];
static uint32_t	sym_tabs			[MAX_SYM_COUNT][3];

static int		nTxtSyms			= 0;
static int		nSbSize 			= 0;
static ulong_t	nBssSz				= 0;

static void process_phdr (Elf32_Phdr *pPhdr, Elf32_Half num)
{
	#if (DEBUG > 0)
	Elf32_Phdr	*p = pPhdr;
	int			i;

	if(debug_lvl_pc <= 0)
		return;

	fprintf(stderr, ">>Program Header<<\n");
	fprintf(stderr, "Type Offset  vAddr  pAddr FileSz  MemSz flag    Align\n");
	fflush(stderr);

	for (i = 0; i < (int)num; i++, ++p)
	{
		if (p->p_type == PT_LOAD) fprintf(stderr, "  LOAD ");
		else					  fprintf(stderr, "       ");

		fprintf(stderr, "%6x ", p->p_offset);
		fprintf(stderr, "%6x ", p->p_vaddr);
		fprintf(stderr, "%6x ", p->p_paddr);
		fprintf(stderr, "%6x ", p->p_filesz);
		fprintf(stderr, "%6x ", p->p_memsz);
		fprintf(stderr, "%4x ", p->p_flags);
		fprintf(stderr, "%8x\n", p->p_align);
	} /* end for statement */
	fflush(stderr);
	#endif	/* DEBUG */
}

static int compare_sym(const void *a, const void *b)
{
	unsigned int *ia = (unsigned int *)a, *ib = (unsigned int *)b;

	return( ( (ia[0] > ib[0]) ? 1 : ((ia[0] == ib[0]) ? 0 : -1) ) );
}

static int compare_str(const void *a, const void *b)
{
	int res = strcmp(&pSym_buff[sym_tabs[*(ulong_t *)a][2]], &pSym_buff[sym_tabs[*(ulong_t *)b][2]]);

	return( ( (res > 0) ? 1 : ((res ==  0) ? 0 : -1) ) );
}

static void *fseek_and_read(FILE *fp, unsigned int offset, unsigned int size)
{
	char *pData = NULL;

	if (size)
	{
		pData = (char *) malloc(size);
		fseek(fp, offset, SEEK_SET);
		fread(pData, 1, size, fp);
	}
	return(pData);
}


ulong_t process32 (FILE *ifp, FILE *ofp)
{
	Elf32_Ehdr	*pEhdr         = NULL;
	Elf32_Phdr	*pPhdr         = NULL;
	Elf32_Shdr	*pShdr         = NULL;
	Elf32_Shdr	*pSec          = NULL;

	Elf32_Shdr	*pSymtabHdr    = NULL;
	Elf32_Shdr	*pStrtabHdr    = NULL;
	Elf32_Shdr	*pRodataHdr    = NULL;
	Elf32_Shdr	*pDebugLineHdr = NULL;
	Elf32_Shdr	*pDebugAbbrHdr = NULL;
	Elf32_Shdr	*pDebugInfoHdr = NULL;
	Elf32_Shdr	*pDebugStrHdr  = NULL;
	char		*pSymp, *pBuf;
	int			i, notfirst    = 0;
	ulong_t		va_addr        = 0xffffffff;
	ulong_t		va_base        = 0xffffffff;
	ulong_t		sh_flags       = 0;
	ulong_t		start_rodata   = 0;
	ulong_t		end_rodata     = 0;
	ulong_t		mv_src         = 0;
	ulong_t		mv_dst         = 0;
	ulong_t		data_sz        = 0;
	int			tmp;


	versionBuf[0] = '\0';
    tmStampBuf[0] = '\0';

	if ((pEhdr = Elf32_getehdr(ifp)) == NULL)
	{
		fprintf(stderr, "Can't read Ehdr\n");
		return 0;
	}

	#if (DEBUG > 0)
	if(debug_lvl_pc > 0)
	{
		fprintf(stderr, "Start address = %08x\n", pEhdr->e_entry);
		fprintf(stderr, "Machine Type  = %08x\n", pEhdr->e_machine);
		fflush(stderr);
	}
	#endif

	if ((pPhdr = Elf32_getphdr(ifp, pEhdr)) == NULL)
	{
		fprintf(stderr, "Can't read Phdr\n");
		return 0;
	}

	process_phdr(pPhdr, pEhdr->e_phnum);

	if (pEhdr->e_shnum == 0)
	{
		fprintf(stderr, "no section data!\n");
		fflush(stderr);
	}

	pBinBuff = (char *)mmap((void *) NULL, MAX_BIN_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	pBuf  = (char *)&pBinBuff[0];
	i     = pEhdr->e_shstrndx;
	pShdr = Elf32_getshdr(ifp, pEhdr);
	pSymp = Elf32_getstrtab(ifp, pEhdr, pShdr[i].sh_offset, pShdr[i].sh_size);

	#if (DEBUG > 0)
	if(debug_lvl_pc > 0)
		fprintf(stderr, "shstrndx = %d\n", i);
	#endif

	#if (DEBUG > 0)
	if(debug_lvl_pc > 0)
	{
		fprintf(stderr, "\nSection Header\n");
		fprintf(stderr, "No  Name                     Type   sh_flags   Addr    Offset Size  Aln\n");
		fflush(stderr);
	}
	#endif	/* DEBUG */

	for (i = 0, pSec = pShdr; i < pEhdr->e_shnum; i++, pSec++)
	{
		sh_flags = (pSec->sh_flags & ~SHF_MASKPROC);

		#if (DEBUG > 0)
		if(debug_lvl_pc > 0)
		{
			fprintf(stderr, "%03d ",  i);
			fprintf(stderr, "%-20s", pSymp + pSec->sh_name);
			fprintf(stderr, "%8x ",  pSec->sh_type);
			fprintf(stderr, "%8x ",  pSec->sh_flags);
			fprintf(stderr, "%8x ",  pSec->sh_addr);
			fprintf(stderr, "%8x ",  pSec->sh_offset);
			fprintf(stderr, "%5x ",  pSec->sh_size);
			fprintf(stderr, "%3x ",  pSec->sh_addralign);
			fprintf(stderr, "%8x(%06x)\n", va_addr, va_addr-va_base);
			fflush(stderr);
		}
		#endif	/* DEBUG */

		if      (pSec->sh_type == SHT_SYMTAB)
		{
			pSymtabHdr = pSec;
		}
		else if ( (pSec->sh_type == SHT_STRTAB) && (strcmp(pSymp+pSec->sh_name, ".strtab") == 0) && (pStrtabHdr == NULL) )
		{
			pStrtabHdr = pSec;
		}
		else if (strcmp(pSymp+pSec->sh_name, ".rodata") == 0)
		{
			pRodataHdr   = pSec;
			start_rodata = pSec->sh_addr,
			end_rodata   = start_rodata + pSec->sh_size;
		}
		else if (strcmp(pSymp+pSec->sh_name, ".debug_line") == 0)
		{
			pDebugLineHdr = pSec;
		}
		else if (strcmp(pSymp+pSec->sh_name, ".debug_info") == 0)
		{
			pDebugInfoHdr = pSec;
		}
		else if (strcmp(pSymp+pSec->sh_name, ".debug_abbrev") == 0)
		{
			pDebugAbbrHdr = pSec;
		}
		else if (strcmp(pSymp+pSec->sh_name, ".debug_str") == 0)
		{
			pDebugStrHdr = pSec;
		}

		if (pSec->sh_type == SHT_NOBITS)	/* This is bss	*/
		{
			if (pSec->sh_addr >= va_addr)
			{
				nBssSz = pSec->sh_size;
				PRINT("s_bss - e_data is %d, bss_size = %d(0x%x)\n", pSec->sh_addr - va_addr, nBssSz, nBssSz);
				PRINT("Symbol table will be loaded 0x%x\n", va_addr + nBssSz);
			}
			continue;
		}

		if (pSec->sh_type != SHT_PROGBITS)
			continue;

		if (va_base == 0xffffffff)
		{
			va_base = va_addr = pSec->sh_addr;
		}
		else if (va_addr == pSec->sh_addr)
		{
			; 	// Nothing to do
		}
		else if (va_addr > pSec->sh_addr)
		{
			if (mv_src == 0)
			{
				mv_src = pSec->sh_addr;
				mv_dst = va_addr;
			}
		}
		else if ((pSec->sh_size != 0) && ((pSec->sh_addr - va_addr) < 256))
		{
			va_addr = pSec->sh_addr;
		}
		else
		{
			;
		}

		if (pSec->sh_type == SHT_PROGBITS)
		{
			PRINT("Adding section %s, type 0x%x, flags=0x%x, size=0x%x\n",
							pSymp + pSec->sh_name, pSec->sh_type, sh_flags, pSec->sh_size);
			fseek(ifp, pSec->sh_offset, SEEK_SET);
			if (fread(pBuf + va_addr-va_base, 1, pSec->sh_size, ifp) != pSec->sh_size)
			{
				fprintf(stderr, "cannot read section\n");
			}

			va_addr += pSec->sh_size;
		}

	}

	/*
	 *	Header for debugging
	 */
	if (pRodataHdr    != NULL) { PRINT("pRodataHdr    = %x\n", pRodataHdr);    }
	if (pDebugLineHdr != NULL) { PRINT("pDebugLineHdr = %x\n", pDebugLineHdr); }
	if (pDebugInfoHdr != NULL) { PRINT("pDebugInfoHdr = %x\n", pDebugInfoHdr); }
	if (pDebugAbbrHdr != NULL) { PRINT("pDebugAbbrHdr = %x\n", pDebugAbbrHdr); }
	if (pDebugStrHdr  != NULL) { PRINT("pDebugStrHdr  = %x\n", pDebugStrHdr);  }

	if ( (pBuf != NULL) && (mv_src != 0) )
	{
		fprintf(stderr, "Data in [0x%x..0x%x] are moved to 0x%x\n",
						 mv_src, mv_src + (va_addr - mv_dst), mv_dst);
	}

	if ((pSymtabHdr != NULL) && (pStrtabHdr != NULL))
	{
		int				n, nEntries;
		Elf32_Sym		*pSymtab, *pSym;
		char			*pStrtab;
		char			*pDebugLine = NULL;
		uint64_t		tableSize;
		uint64_t		sortTableSize;
		uint64_t		debugLineSize;
		uint64_t		dwarfTableSize;

		nEntries = pSymtabHdr->sh_size / sizeof(Elf32_Sym);
		pSymtab	 = fseek_and_read(ifp, pSymtabHdr->sh_offset, pSymtabHdr->sh_size);
		pStrtab	 = fseek_and_read(ifp, pStrtabHdr->sh_offset, pStrtabHdr->sh_size);

		for (n = 0, pSym = pSymtab; n < nEntries; n++, pSym++)
		{
			unsigned int	myAddr, mySize, myOffset;
			unsigned int	begAddr, endAddr;
			char			*pContent = NULL;
			char			*pSymName;
			int				sType, symLen;

			swap_stab(pSym);
			pSymName = pStrtab+pSym->st_name;

			if ( ( ((ELF32_ST_TYPE(pSym->st_info) == STT_FUNC  )) ||
				   ((ELF32_ST_TYPE(pSym->st_info) == STT_OBJECT)) ) &&
				 (pSymName[0] != '$') && (pSymName[1] != '$') )
			{
				symLen = strlen(pSymName);
				if (nTxtSyms >= MAX_SYM_COUNT)
				{
					fprintf(stderr, "Number of functions(%d) are exceed %d\n", nTxtSyms, MAX_SYM_COUNT);
				}
				else if ((nSbSize + symLen) >= MAX_SYM_BUFF)
				{
					fprintf(stderr, "Symbol list buffer overflow(case 1), %d >= %d\n", nSbSize, MAX_SYM_BUFF);
				}
				else
				{
					#if (DEBUG > 2)
					if(debug_lvl_pc > 0)
						fprintf(stderr, "0x%08x 0x%04x %02x %s\n", pSym->st_value, pSym->st_size, pSym->st_info, pSymName);
					#endif
					sym_tabs[nTxtSyms][0] = pSym->st_value;
					sym_tabs[nTxtSyms][1] = pSym->st_size;
					sym_tabs[nTxtSyms][2] = nSbSize;

					nSbSize += snprintf(&pSym_buff[nSbSize], MAX_SYM_BUFF, "%s", pSymName) + 1;
					nTxtSyms++;
				}
			}

			if      (strcmp(pSymName, "versionString" ) == 0) sType = 0;
			else if (strcmp(pSymName, "VerNumber"     ) == 0) sType = 0;
			else if (strcmp(pSymName, "buildVers"     ) == 0) sType = 0;
			else if (strcmp(pSymName, "creationDate"  ) == 0) sType = 1;
			else if (strcmp(pSymName, "BuiltDate"     ) == 0) sType = 1;
			else if (strcmp(pSymName, "BuiltTime"     ) == 0) sType = 2;
			else                                              continue;

			pSec = pShdr + pSym->st_shndx;
			begAddr = pSec->sh_addr;
			endAddr = pSec->sh_addr + pSec->sh_size;

			myAddr  = pSym->st_value;
			mySize  = pSym->st_size;
			myOffset= pSec->sh_offset+(myAddr-begAddr);

			/* Workarround for SDT Arm Compiler	*/
			if (pSym->st_size == 0) mySize = 512;

			pContent = fseek_and_read(ifp, myOffset, mySize + 1);

			myAddr = *(int *)pContent;
			swap_long(&myAddr);
			if ((mySize == 4) && ((myAddr >= start_rodata) && (myAddr <= end_rodata)))
			{
				/*
				 *		Case for CIPDP with vxworks and D2A with uC/OS-II
				 *	Since the variable has been declared like
				 *	  char * buildVers = "blahblah";
				 *	We need to go through the pointer
				 */
				free(pContent);

				mySize  = 512;
				myOffset= pRodataHdr->sh_offset + (myAddr - pRodataHdr->sh_addr);

				pContent = fseek_and_read(ifp, myOffset, mySize);
			}

			#if (DEBUG > 0)
			if(debug_lvl_pc > 0)
			{
				fprintf(stderr, "++++ Symbol %s is %s, with 0x%x(%d)\n", pSymName, pContent, myOffset, mySize);
				fflush(stderr);
			}
			#endif

			switch (sType)
			{
				case 0: snprintf(versionBuf, 128, pContent); break;
				case 1: snprintf(tmStampBuf, 128, pContent); break;
				case 2: snprintf(tmStampBuf, 128, "%s %s", tmStampBuf, pContent); break;
			}

			free(pContent);
		}

		free(pSymtab);
		free(pStrtab);

		#if (DEBUG > 0)
		if(debug_lvl_pc > 0)
		{
			fprintf(stderr, "++++ %d symbol has been imported(%d bytes)\n", nTxtSyms, nSbSize);
			fflush(stderr);
		}
		#endif

		/*
		 *	Now build tables to store debug informations
		 */
		qsort(&sym_tabs[0][0], nTxtSyms, 3 * sizeof(int), compare_sym);

		#if	(DEBUG > 0)
		if(debug_lvl_pc > 1)
		{
			fprintf(stderr, "++++ Dumping text symbols\n");
			for (n = 0; n < nTxtSyms; n++)
			{
				int	*pTab = &sym_tabs[n][0];

				fprintf(stderr, "%08x %08x T %s\n", pTab[0], pTab[1], &pSym_buff[pTab[2]]);
			}
			fflush(stderr);
		}
		#endif

		#if	(DEBUG > 0)
		if(debug_lvl_pc > 1)
		{
			fprintf(stderr, "++++ Sorting symbol table by name\n");
			fflush(stderr);
		}
		#endif

		for (n = 0; n < nTxtSyms; n++)
			sym_hash[n] = n;

		qsort(&sym_hash[0], nTxtSyms, sizeof(int), compare_str);

		#if	(DEBUG > 0)
		if(debug_lvl_pc > 1)
		{
			fprintf(stderr, "++++ Dumping other symbols\n");
			for (n = 0; n < nTxtSyms; n++)
			{
				ulong_t v = sym_hash[n];
				int  *pTab = &sym_tabs[v][0];

				fprintf(stderr, "%d:: symbol %04x %04x %04x %04x %s\n", n, v,
								pTab[0], pTab[1], pTab[2], &pSym_buff[pTab[2]]);
			}
			fprintf(stderr, "done\n");
			fflush(stderr);
		}
		#endif

		if (pDebugLineHdr)
		{
			extern int searchLineInfo(char **, uint64_t *, uint64_t , char **);
			extern char *pDebugStr, *pDebugInfo, *pDebugAbbr;
			extern size_t debugStrSize, debugInfoSize, debugAbbrSize;

			/* Read Dwarf2 debug section data */
			if (pDebugLineHdr)
			{
				debugLineSize = pDebugLineHdr->sh_size;
				pDebugLine = fseek_and_read(ifp, pDebugLineHdr->sh_offset, debugLineSize);
			}
			if (pDebugInfoHdr)
			{
				debugInfoSize = pDebugInfoHdr->sh_size;
				pDebugInfo = fseek_and_read(ifp, pDebugInfoHdr->sh_offset, debugInfoSize);
			}
			if (pDebugAbbrHdr)
			{
				debugAbbrSize = pDebugAbbrHdr->sh_size;
				pDebugAbbr = fseek_and_read(ifp, pDebugAbbrHdr->sh_offset, debugAbbrSize);
			}
			if (pDebugStrHdr)
			{
				debugStrSize  = pDebugStrHdr->sh_size;
				pDebugStr  = fseek_and_read(ifp, pDebugStrHdr->sh_offset,  debugStrSize);
			}

			#if	(DEBUG > 0)
			if(debug_lvl_pc > 1)
			{
				fprintf(stderr, "++++ Parsing all comp units\n");
				fflush(stderr);
			}
			#endif

			/* Build debug_info table */
			parse_all_comp_units();

			/* Build debug_line table */
			searchLineInfo(&pDebugLine, (uint64_t*)&debugLineSize, (uint64_t)-1, NULL);
			//testAddr2Line(ehdr->e_entry);

			/* Release debug_info table */
			release_all_comp_units();

			free(pDebugInfo);
			free(pDebugAbbr);
			free(pDebugStr);

			PRINT("%5d dwarf  packets use %d+%d(=%d) bytes\n",
					nDwarfLst, nDwarfLst*8, debugLineSize, nDwarfLst*8 + debugLineSize);
		}

		/* hash table ¿ 32bit ¿ ¿¿¿ size¿ 2¿¿ 4¿ ¿¿.*/
		sortTableSize	= 4 + sizeof(int) * ((nTxtSyms + 1) & ~1);
		dwarfTableSize	= (nDwarfLst ? (12 + 8 * nDwarfLst + debugLineSize) : 0);
		tableSize = sortTableSize + dwarfTableSize;

		for (n = 0; n < nTxtSyms; n++)
			swap_long(&sym_hash[n]);

		/* Check symbol buffer overflow */
		if ((nSbSize + tableSize) >= MAX_SYM_BUFF)
		{
			fprintf(stderr, "Symbol list buffer overflow(case 2), %d+%d >= %d\n", tableSize, nSbSize, MAX_SYM_BUFF);
			fflush(stderr);
			return(0);
		}

		/* Move original symbol strings */
		PRINT("Moving symbols from %p..%p to %p..%p\n", 0, nSbSize, tableSize, tableSize + nSbSize);
		memmove(&pSym_buff[tableSize], &pSym_buff[0], nSbSize);

		/* Insert alphabetic sort table */
		/* ¿¿ 16 bit ¿¿ n ¿¿ 0¿¿ ¿¿¿ 32bit¿¿ n ¿¿ 2¿ ¿. */
		n = 2;	swap_long(&n);	/* Mark this is sorted hash table */
		memcpy (&pSym_buff[0], &n,           4);
		memcpy (&pSym_buff[4], &sym_hash[0], sortTableSize - 4);

		/* Insert dwarf index table & encoded string */
		if (nDwarfLst)
		{
			char *pSymBuff = &pSym_buff[sortTableSize];

			for (n = 0; n < 2 * nDwarfLst; n++)
				swap_long(&dwarfLst[n]);

			n = 1;				swap_long(&n);	/* Mark this is dwarf table */
			memcpy (pSymBuff, &n, 4);						pSymBuff += 4;
			n = nDwarfLst;		swap_long(&n);
			memcpy (pSymBuff, &n, 4);						pSymBuff += 4;
			n = debugLineSize;	swap_long(&n);
			memcpy (pSymBuff, &n, 4);						pSymBuff += 4;
			memcpy (pSymBuff, dwarfLst,   8 * nDwarfLst);	pSymBuff += 8 * nDwarfLst;
			memcpy (pSymBuff, pDebugLine, debugLineSize);	pSymBuff += debugLineSize;
		}


		/* Advance size of symbol buffer size */
		nSbSize += tableSize;

		if (pDebugLine)
			free(pDebugLine);

		#if	(DEBUG > 0)
		if(debug_lvl_pc > 1)
		{
			for (n = 0; n < nTxtSyms; n++)
			{
				ulong_t v = sym_hash[n];
				int *pTab;

				swap_long(&v);
				pTab = &sym_tabs[v][0];
				fprintf(stderr, "symbol %04x %04x %04x %04x %s\n", v,
								pTab[0], pTab[1], pTab[2], &pSym_buff[pTab[2] + tableSize]);
			}
		}
		#endif

		PRINT("%5d object symbols use %d+%d(=%d) bytes\n", nTxtSyms, nTxtSyms*12, nSbSize, nTxtSyms*12 + nSbSize);
	}

	data_sz = (sizeof(int)*(5 + 3 * nTxtSyms) + nSbSize);
	PRINT("Exporting %d(0x%06x) byte symbol table\n", data_sz, data_sz);
	if (ofp)
	{
		tmp = SYM_MAGIC;  swap_long(&tmp); fwrite(&tmp, 1, sizeof(int), ofp);
		tmp = nBssSz;     swap_long(&tmp); fwrite(&tmp, 1, sizeof(int), ofp);
		tmp = data_sz-20; swap_long(&tmp); fwrite(&tmp, 1, sizeof(int), ofp);
		tmp = nTxtSyms;   swap_long(&tmp); fwrite(&tmp, 1, sizeof(int), ofp);
		tmp = nSbSize;    swap_long(&tmp); fwrite(&tmp, 1, sizeof(int), ofp);

		/* Write index table with addresses and ptr to name */
		for (tmp = 0; tmp < nTxtSyms; tmp++)
		{
			if(sym_tabs[tmp][1] == 0)
			{
				if(tmp <(nTxtSyms-1))
					sym_tabs[tmp][1] = sym_tabs[tmp+1][0];
			}
			else
			{
				sym_tabs[tmp][1] += sym_tabs[tmp][0];
			}
			swap_long(&sym_tabs[tmp][0]);
			swap_long(&sym_tabs[tmp][1]);
			swap_long(&sym_tabs[tmp][2]);
		}

		fwrite(&sym_tabs[0][0], sizeof(int), 3 * nTxtSyms, ofp);

		/* Write symbol strings */
		fwrite(&pSym_buff[0], 1, nSbSize, ofp);
	}
_finalize_resource:
	munmap(pBinBuff, MAX_BIN_SIZE);
	free(pPhdr);
	free(pShdr);
	free(pSymp);

	return (va_addr - va_base);
}
