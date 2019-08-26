#include	<stdlib.h>
#include	<string.h>
#include	<ar.h>
#include	<unistd.h>
#include	<sys/mman.h>

#include	"elf_func.h"
#include	"def.h"
#include	"dwarf.h"
#include	"process.h"

/**
 * Global Control Constant
 */
#if (DEBUG > 0)
extern int debug_lvl_pc;
#endif

#define SYM_MAGIC					0xB12791EEEE19721B


static uchar_t	*pBinBuff          = NULL;

static uint64_t	sym_hash			[MAX_SYM_COUNT];
static char		pSym_buff			[MAX_SYM_BUFF];
static uint64_t	sym_tabs			[MAX_SYM_COUNT][3];

static int		nTxtSyms			= 0;
static int		nSbSize 			= 0;
static uint64_t	nBssSz				= 0;

static void process_phdr (Elf64_Phdr *pPhdr, Elf64_Half num)
{
	#if (DEBUG > 0)
	Elf64_Phdr	*p = pPhdr;
	int			i;

	if(debug_lvl_pc <= 0)
		return;

	fprintf(stderr, ">>Program Header<<\n");
	fprintf(stderr, "Type             Offset            vAddr            pAddr           FileSz            MemSz    flag         Align\n");
	fflush(stderr);

	for (i = 0; i < (int)num; i++, ++p)
	{
		if (p->p_type == PT_LOAD) fprintf(stderr, "  LOAD ");
		else					  fprintf(stderr, "       ");

		fprintf(stderr, "%16lx ", p->p_offset);
		fprintf(stderr, "%16lx ", p->p_vaddr);
		fprintf(stderr, "%16lx ", p->p_paddr);
		fprintf(stderr, "%16lx ", p->p_filesz);
		fprintf(stderr, "%16lx ", p->p_memsz);
		fprintf(stderr, "%4x ",   p->p_flags);
		fprintf(stderr, "%16lx\n",p->p_align);
	} /* end for statement */
	fflush(stderr);
	#endif	/* DEBUG */
}

static int compare_sym(const void *a, const void *b)
{
	uint64_t *ia = (uint64_t *)a, *ib = (uint64_t *)b;

	return( ( (ia[0] > ib[0]) ? 1 : ((ia[0] == ib[0]) ? 0 : -1) ) );
}

static int compare_str(const void *a, const void *b)
{
	int res = strcmp(&pSym_buff[sym_tabs[*(uint64_t *)a][2]], &pSym_buff[sym_tabs[*(uint64_t *)b][2]]);

	return( ( (res > 0) ? 1 : ((res ==  0) ? 0 : -1) ) );
}

static void *fseek_and_read(FILE *fp, unsigned long int offset, unsigned int size)
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

ulong_t process64 (FILE *ifp, FILE *ofp)
{
	Elf64_Ehdr	*pEhdr         = NULL;
	Elf64_Phdr	*pPhdr         = NULL;
	Elf64_Shdr	*pShdr         = NULL;
	Elf64_Shdr	*pSec          = NULL;

	Elf64_Shdr	*pSymtabHdr    = NULL;
	Elf64_Shdr	*pStrtabHdr    = NULL;
	Elf64_Shdr	*pRodataHdr    = NULL;
	Elf64_Shdr	*pDebugLineHdr = NULL;
	Elf64_Shdr	*pDebugAbbrHdr = NULL;
	Elf64_Shdr	*pDebugInfoHdr = NULL;
	Elf64_Shdr	*pDebugStrHdr  = NULL;
	Elf64_Shdr	*pEhFrameHdr   = NULL;
	Elf64_Shdr	*pEhFrame      = NULL;
	char		*pSymp, *pBuf;
	int			i, notfirst    = 0;
	uint64_t	va_addr        = 0xffffffffffffffff;
	uint64_t	va_base        = 0xffffffffffffffff;
	uint64_t	sh_flags       = 0;
	uint64_t	start_rodata   = 0;
	uint64_t	end_rodata     = 0;
	uint64_t	mv_src         = 0;
	uint64_t	mv_dst         = 0;
	uint64_t	data_sz        = 0;
	uint64_t	tmp;


	versionBuf[0] = '\0';
    tmStampBuf[0] = '\0';

	if ((pEhdr = Elf64_getehdr(ifp)) == NULL)
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

	if ((pPhdr = Elf64_getphdr(ifp, pEhdr)) == NULL)
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
	pShdr = Elf64_getshdr(ifp, pEhdr);
	pSymp = Elf64_getstrtab(ifp, pEhdr, pShdr[i].sh_offset, pShdr[i].sh_size);

	#if (DEBUG > 0)
	if(debug_lvl_pc > 0)
		fprintf(stderr, "shstrndx = %d\n", i);
	#endif

	#if (DEBUG > 0)
	if(debug_lvl_pc > 0)
	{
		fprintf(stderr, "\nSection Header\n");
		fprintf(stderr, "No  Name                     Type           sh_flags            Addr           Offset           Size               Aln\n");
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
			fprintf(stderr, "%-20s",  pSymp + pSec->sh_name);
			fprintf(stderr, "%8x ",   pSec->sh_type);
			fprintf(stderr, "%16lx ", pSec->sh_flags);
			fprintf(stderr, "%16lx ", pSec->sh_addr);
			fprintf(stderr, "%16lx ", pSec->sh_offset);
			fprintf(stderr, "%16lx ", pSec->sh_size);
			fprintf(stderr, "%16lx ", pSec->sh_addralign);
			fprintf(stderr, "  %10lx(%05lx)\n", va_addr, va_addr-va_base);
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
		else if (strcmp(pSymp+pSec->sh_name, ".eh_frame_hdr") == 0)
		{
			pEhFrameHdr = pSec;
		}
		else if (strcmp(pSymp+pSec->sh_name, ".eh_frame") == 0)
		{
			pEhFrame = pSec;
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

		if (va_base == 0xffffffffffffffff)
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
	if (pRodataHdr    != NULL) { PRINT("pRodataHdr    = %lx\n", pRodataHdr);    }
	if (pDebugLineHdr != NULL) { PRINT("pDebugLineHdr = %lx\n", pDebugLineHdr); }
	if (pDebugInfoHdr != NULL) { PRINT("pDebugInfoHdr = %lx\n", pDebugInfoHdr); }
	if (pDebugAbbrHdr != NULL) { PRINT("pDebugAbbrHdr = %lx\n", pDebugAbbrHdr); }
	if (pDebugStrHdr  != NULL) { PRINT("pDebugStrHdr  = %lx\n", pDebugStrHdr);  }
	if (pEhFrameHdr   != NULL) { PRINT("pEhFrameHdr   = %lx\n", pEhFrameHdr);   }
	if (pEhFrame      != NULL) { PRINT("pEhFrame      = %lx\n", pEhFrame);      }

	if ( (pBuf != NULL) && (mv_src != 0) )
	{
		fprintf(stderr, "Data in [0x%lx..0x%lx] are moved to 0x%x\n",
						 mv_src, mv_src + (va_addr - mv_dst), mv_dst);
	}

	if ((pSymtabHdr != NULL) && (pStrtabHdr != NULL))
	{
		int64_t			n;
		int64_t			nEntries;
		Elf64_Sym		*pSymtab, *pSym;
		char			*pStrtab;
		char			*pDebugLine = NULL;
		uint64_t		tableSize;
		uint64_t		sortTableSize;
		uint64_t		debugLineSize;
		uint64_t		dwarfTableSize;

		nEntries = pSymtabHdr->sh_size / sizeof(Elf64_Sym);
		pSymtab	 = fseek_and_read(ifp, pSymtabHdr->sh_offset, pSymtabHdr->sh_size);
		pStrtab	 = fseek_and_read(ifp, pStrtabHdr->sh_offset, pStrtabHdr->sh_size);

		for (n = 0, pSym = pSymtab; n < nEntries; n++, pSym++)
		{
			uint64_t		myAddr, mySize, myOffset;
			uint64_t		begAddr, endAddr;
			char			*pContent = NULL;
			char			*pSymName;
			int				sType, symLen;

			swap_stab64(pSym);
			pSymName = pStrtab+pSym->st_name;

			if ( ( ((ELF64_ST_TYPE(pSym->st_info) == STT_FUNC  )) ||
				   ((ELF64_ST_TYPE(pSym->st_info) == STT_OBJECT)) ) &&
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
						fprintf(stderr, "0x%016lx 0x%016lx %02x %s\n", pSym->st_value, pSym->st_size, pSym->st_info, pSymName);
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

			myAddr = *(uint64_t *)pContent;
			swap_llong(&myAddr);
			//TODO: BK1472 Check****
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
		qsort(&sym_tabs[0][0], nTxtSyms, 3 * sizeof(uint64_t), compare_sym);

		#if	(DEBUG > 0)
		if(debug_lvl_pc > 1)
		{
			fprintf(stderr, "++++ Dumping text symbols\n");
			for (n = 0; n < nTxtSyms; n++)
			{
				int	*pTab = &sym_tabs[n][0];

				fprintf(stderr, "%016lx %016lx T %s\n", pTab[0], pTab[1], &pSym_buff[pTab[2]]);
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

		qsort(&sym_hash[0], nTxtSyms, sizeof(uint64_t), compare_str);

		#if	(DEBUG > 0)
		if(debug_lvl_pc > 1)
		{
			fprintf(stderr, "++++ Dumping other symbols\n");
			for (n = 0; n < nTxtSyms; n++)
			{
				uint64_t	v    = sym_hash[n];
				uint64_t*	pTab = &sym_tabs[v][0];

				fprintf(stderr, "%4d:: symbol %04x %06x %016lx %016lx %s\n", n, v,
								pTab[0], pTab[1], pTab[2], &pSym_buff[pTab[2]]);
			}
			fprintf(stderr, "done\n");
			fflush(stderr);
		}
		#endif

		if (pDebugLineHdr)
		{
			extern int		searchLineInfo(char **, uint64_t *, uint64_t , char **);
			extern char		*pDebugStr, *pDebugInfo, *pDebugAbbr;
			extern size_t	debugStrSize, debugInfoSize, debugAbbrSize;

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

			PRINT("%5ld dwarf  packets use %ld+%ld(=%ld) bytes\n",
					nDwarfLst, nDwarfLst*2*sizeof(uint64_t), debugLineSize, nDwarfLst*8 + debugLineSize);
		}
		#if 0
		if(pEhFrameHdr)
		{
			extern uint64_t decode_frame(unsigned char*, unsigned char*, uint64_t, uint64_t);
			uint64_t		eh_frame_hdr_size;
			uint64_t		eh_frame_size;
			unsigned char*	pFrame_hdr = NULL;
			unsigned char*	pFrame     = NULL;

			if(pEhFrameHdr)
			{
				eh_frame_hdr_size = pEhFrameHdr->sh_size;
				pFrame_hdr = fseek_and_read(ifp, pEhFrameHdr->sh_offset,  eh_frame_hdr_size);
			}
			if(pEhFrame)
			{
				eh_frame_size = pEhFrame->sh_size;
				pFrame = fseek_and_read(ifp, pEhFrame->sh_offset,  eh_frame_size);
			}
			decode_frame(pFrame_hdr, pFrame, eh_frame_hdr_size, eh_frame_size);
		}
		#endif
		sortTableSize	= sizeof(n) + sizeof(uint64_t) * ((nTxtSyms + 1) & ~1);
		dwarfTableSize	= (nDwarfLst ? (3*sizeof(uint64_t) + 2*sizeof(uint64_t) * nDwarfLst + debugLineSize) : 0);
		tableSize       = sortTableSize + dwarfTableSize;

		for (n = 0; n < nTxtSyms; n++)
			swap_llong(&sym_hash[n]);

		/* Check symbol buffer overflow */
		if ((nSbSize + tableSize) >= MAX_SYM_BUFF)
		{
			fprintf(stderr, "Symbol list buffer overflow(case 2), %ld+%d >= %d\n", tableSize, nSbSize, MAX_SYM_BUFF);
			fflush(stderr);
			return(0);
		}

		/* Move original symbol strings */
		PRINT("Moving symbols from %p..%p to %p..%p\n", 0, nSbSize, tableSize, tableSize + nSbSize);
		memmove(&pSym_buff[tableSize], &pSym_buff[0], nSbSize);

		/* Insert alphabetic sort table */
		/* Bit size mark code 16 bit:0, 32 bit: 2, 64 bit: 4  */
		n = 4;	swap_llong(&n);	/* Mark this is sorted hash table */
		memcpy (&pSym_buff[0], &n,           sizeof(n));
		memcpy (&pSym_buff[sizeof(n)], &sym_hash[0], sortTableSize - sizeof(n));

		/* Insert dwarf index table & encoded string */
		if (nDwarfLst)
		{
			char *pSymBuff = &pSym_buff[sortTableSize];

			for (n = 0; n < 2 * nDwarfLst; n++)
				swap_llong(&dwarfLst[n]);

			n = 1;				swap_llong(&n);	/* Mark this is dwarf table */
			memcpy (pSymBuff, &n, sizeof(n));				pSymBuff += sizeof(n);
			n = nDwarfLst;		swap_llong(&n);
			memcpy (pSymBuff, &n, sizeof(n));				pSymBuff += sizeof(n);
			n = debugLineSize;	swap_llong(&n);
			memcpy (pSymBuff, &n, sizeof(n));				pSymBuff += sizeof(n);
			memcpy (pSymBuff, dwarfLst,   2*sizeof(uint64_t)*nDwarfLst);	pSymBuff += (2*sizeof(uint64_t)*nDwarfLst);
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
				uint64_t	v = sym_hash[n];
				uint64_t*	pTab;

				swap_llong(&v);
				pTab = &sym_tabs[v][0];
				fprintf(stderr, "symbol %08lx %08lx %08lx %08lx %s\n", v,
								pTab[0], pTab[1], pTab[2], &pSym_buff[pTab[2] + tableSize]);
			}
		}
		#endif

		PRINT("%5d object symbols use %d+%d(=%d) bytes\n", nTxtSyms, nTxtSyms*12, nSbSize, nTxtSyms*12 + nSbSize);
	}

	data_sz = (sizeof(uint64_t)*(5 + 3 * nTxtSyms) + nSbSize);
	PRINT("Exporting %08lu(0x%08lx) byte symbol table\n", data_sz, data_sz);
	if (ofp)
	{
		tmp = SYM_MAGIC;                  swap_llong(&tmp); fwrite(&tmp, 1, sizeof(uint64_t), ofp);
		tmp = nBssSz;                     swap_llong(&tmp); fwrite(&tmp, 1, sizeof(uint64_t), ofp);
		tmp = data_sz-sizeof(uint64_t)*5; swap_llong(&tmp); fwrite(&tmp, 1, sizeof(uint64_t), ofp);
		tmp = nTxtSyms;                   swap_llong(&tmp); fwrite(&tmp, 1, sizeof(uint64_t), ofp);
		tmp = nSbSize;                    swap_llong(&tmp); fwrite(&tmp, 1, sizeof(uint64_t), ofp);

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
			swap_llong(&sym_tabs[tmp][0]);
			swap_llong(&sym_tabs[tmp][1]);
			swap_llong(&sym_tabs[tmp][2]);
		}

		fwrite(&sym_tabs[0][0], sizeof(uint64_t), 3 * nTxtSyms, ofp);

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
