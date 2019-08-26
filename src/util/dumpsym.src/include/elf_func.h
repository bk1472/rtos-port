#ifndef __ELF_FUNC_H__
#define __ELF_FUNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ar.h>

#include	"libelf.h"
#include	"symtypes.h"

/*elf.c*/
extern int				Elf32_gettype			(FILE *fp);
extern Elf32_Ehdr*		Elf32_getehdr			(FILE *fp);
extern Elf32_Phdr*		Elf32_getphdr			(FILE *fp, Elf32_Ehdr *ehdr);
extern Elf32_Shdr*		Elf32_getshdr			(FILE *fp, Elf32_Ehdr *ehdr);
extern char*			Elf32_getstrtab			(FILE *fp, Elf32_Ehdr *ehdr, int off, int size);

extern Elf64_Ehdr*		Elf64_getehdr			(FILE *fp);
extern Elf64_Phdr*		Elf64_getphdr			(FILE *fp, Elf64_Ehdr *ehdr);
extern Elf64_Shdr*		Elf64_getshdr			(FILE *fp, Elf64_Ehdr *ehdr);
extern char*			Elf64_getstrtab			(FILE *fp, Elf64_Ehdr *ehdr, int off, int size);

extern void				swap_half				(void *vp);
extern void				swap_long				(void *vp);
extern void				swap_llong				(void *vp);

extern void				swap_ehdr				(Elf32_Ehdr *ehdr);
extern void				swap_phdr				(Elf32_Phdr *phdr);
extern void				swap_shdr				(Elf32_Shdr *shdr);
extern void				swap_stab				(Elf32_Sym  *sym );

extern void				swap_ehdr64				(Elf64_Ehdr *ehdr);
extern void				swap_phdr64				(Elf64_Phdr *phdr);
extern void				swap_shdr64				(Elf64_Shdr *shdr);
extern void				swap_stab64				(Elf64_Sym  *sym );

/*dwarf.c*/
extern void				parse_all_comp_units	(void);
extern void				release_all_comp_units	(void);

/*addr2line.c*/
extern int				searchLineInfo			(char **ppDebugLine, uint64_t *pSize, uint64_t srchAddr, char **ppFileName);
extern short			getShort				(char *pSrc);
extern int				getLong					(char *pSrc);
extern int64_t			getLLong				(char *pSrc);
#ifdef __cplusplus
}
#endif

#endif/*__ELF_FUNC_H__*/
