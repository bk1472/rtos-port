/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 1998-2007. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : cmd_debug.c
    VERSION     : 1.0
    AUTHOR      : Jackee, Lee(이재길, jackee@lge.com)
    DATE        : 2007/03/07
	DESCRIPTION	: Global command line debugging commands

*******************************************************************************/

/*-----------------------------------------------------------------------------
	전역 제어 상수
	(Global Control Constants)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	#include 파일들
	(File Inclusions)
------------------------------------------------------------------------------*/
#include <common.h>
#include <board.h>
#include <osa/osa_utils.h>
#include "bsp_arch.h"
#include "dbgprint.h"
#include "cmd_debug.h"
#include "cmd_io.h"

/*-----------------------------------------------------------------------------
	상수 정의
	(Constant Definitions)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	매크로 함수 정의
	(Macro Definitions)
------------------------------------------------------------------------------*/
#define	PL_UNIT_SIZE	4

/*-----------------------------------------------------------------------------
	형 정의
	(Type Definitions)
------------------------------------------------------------------------------*/
typedef struct {
	UINT08		ei_numEntry;
	UINT08		ei_maxEntry;
	UINT08		ei_reserved[2];
} ExtItem_t;

/*-----------------------------------------------------------------------------
	외부 전역변수와 외부 함수 prototype 선언
	(Extern Variables & External Function Prototype Declarations)
------------------------------------------------------------------------------*/
#if		defined(_EMBEDDED_)
extern UINT32		_start_code[];
#define	__START		(UINT32)_start_code
#elif	defined(_LINUX_)
extern UINT32		__start[];
#define	__START		(UINT32)__start
#endif

extern void		dbgPrintTask1(char *);
extern void		dbgPrintSemas(void);
extern void		dbgPrintMsgQs(void);

/*------------------------------------------------------------------------------
	Static 변수와 함수 prototype 선언
	(Static Variables & Function Prototypes Declarations)
------------------------------------------------------------------------------*/
//static unsigned int saved_UartOffStatus;

static char         *getDescription(DebugMenu_t*);

/*-----------------------------------------------------------------------------
	전역 변수 정의와 전역 함수 Prototype 선언.
	(Define global variables)
------------------------------------------------------------------------------*/
SINT32		gnDebugLevel      = 0;
SINT32		gnEditMemory      = 0;
DebugMenu_t	*gpCurrMenu       = NULL;
char		*gpCurrCmd        = NULL;

DebugMenu_t	SDM_Global[];			/* Global Menu items					*/
DebugMenu_t	SDM_Main[];				/* Top level Main menu items			*/

void		debug_SendRebootCmd(char **argv);
void		debug_TestBoard(char **argv);


static char		*getDescription(DebugMenu_t*);


/*====================================================================
Function Name   :   debug_HelpMenu
Version         :   0.0
Create Date     :   2002, 08, 21
Author          :   Jackee
Description     :
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
void debug_HelpMenu(DebugMenu_t *pMenu, int help_mode)
{
	int			len;
	char		*cp1, *cp2;
	char		typeChar = ' ';
	DebugMenu_t	*pCurrMenu;

	if (help_mode == 0)
	{
		if (pMenu)
			pCurrMenu = pMenu;
		else
			pCurrMenu = gpCurrMenu;

		if ((pCurrMenu->dm_help == NULL) || (pCurrMenu->dm_type == DMT_LINK))
		{
			if (pCurrMenu->dm_type == DMT_LINK) typeChar = '>';
			else                                typeChar = ' ';

			rprint1n("  %c %-8.8s %s", typeChar, gpCurrCmd,  getDescription(pCurrMenu));
		}
		else
		{
			rprint1n("    %s", pCurrMenu->dm_desc);
			cp1 = pCurrMenu->dm_help;
			cp2 = strchr(cp1, DMT_HELP_TAG[0]);
			if (cp2) len = cp2 - cp1;
			else     len = strlen(cp1);

			rprint1n("    Usage: %s %*.*s", gpCurrCmd, len, len, cp1);
			cp1 += (len+1);

			while (*cp1 && (cp2))
			{
				cp2 = strchr(cp1, DMT_HELP_TAG[0]);
				if (cp2) len = cp2 - cp1;
				else     len = strlen(cp1);

				rprint1n("\t %*.*s", len, len, cp1);

				cp1 += (len+1);
			}
		}
	}
	else if ( (help_mode == 1) || (help_mode == 2) )
	{
		int			i;
		DebugMenu_t *pMenuList[2], *pParMenu;

		rprint1n("    %-8.8s Print this help message", "help,?");
		rprint1n("    ===========================================");

		pMenuList[0] = &SDM_Global[1];
		pMenuList[1] = pMenu+1;

		for (i = 0; i < 2; i++)
		{
			if (i > 0)
			{
				pCurrMenu = pMenuList[i] - 1;
				rprint1n("    ------ %-12s -----------------------", pCurrMenu->dm_desc);
			}

			for (pCurrMenu = pMenuList[i]; pCurrMenu->dm_name; pCurrMenu++)
			{
				int		dm_type  = pCurrMenu->dm_type;
				char	*dm_name = pCurrMenu->dm_name;

				if      (dm_type == DMT_LINK) typeChar = '>';
				else if (dm_type == DMT_MENU) typeChar = '*';
				else                          typeChar = ' ';

				rprint1n("  %c %-8.8s %s", typeChar, dm_name, getDescription(pCurrMenu));
			}
		}

		if (help_mode == 2)
		{
			for (pParMenu = pMenuList[1]; pParMenu->dm_name; pParMenu++)
			{
				if (pParMenu->dm_type != DMT_MENU)
					continue;

				rprint1n("    ----- [SubMenu]::%-12s -------------", pParMenu->dm_name);

				pCurrMenu = (DebugMenu_t *)pParMenu->dm_next + 1;
				for (; pCurrMenu->dm_name; pCurrMenu++)
				{
					int 	dm_type  = pCurrMenu->dm_type;
					char	*dm_name = pCurrMenu->dm_name;

					if      (dm_type == DMT_LINK) typeChar = '>';
					else if (dm_type == DMT_MENU) typeChar = '*';
					else                          typeChar = ' ';

					rprint1n("  %c %-8.8s %s", typeChar, dm_name, getDescription(pCurrMenu));
				}
			}
		}

		rprint1n("    ===========================================");
		rprint1n("    %-8.8s Exit from %s menu\n", "exit", pMenu->dm_desc);
	}
	return;
}

static char *getDescription(DebugMenu_t *pCurrMenu)
{
	DebugMenu_t	*pLinkMenu;
	static char	descStr[128];

	if (pCurrMenu->dm_type == DMT_LINK)
	{
		pLinkMenu = pCurrMenu->dm_next;
		snprintf(descStr, 80, "%-24s ---> %s@%s", pCurrMenu->dm_desc, pCurrMenu->dm_help, pLinkMenu->dm_desc);
	}
	else
	{
		snprintf(descStr, 80, "%s", pCurrMenu->dm_desc);
	}

	return(&descStr[0]);
}


/*====================================================================
Function Name   :   debug_RegisterCmd(), debugRegisterCmdStruct()
Version         :   0.
Create Date     :   2007,03,06
Author          :   Jackee
Description     : 	Register a commander(or commanders) into existing menu.
Input           :   Parent and child menu
Output          :   NONE
Remarks         :	New command can be added to parent menus(Main menu by default)
					In case of appending new menu, the menu structure should
					be resident in data section instead of rodata, text or heap
====================================================================*/
int debug_RegisterCmdStruct(DebugMenu_t *pParent, DebugMenu_t *pCmd)
{
	DebugMenu_t	*pCurrent;

	/*	Register command to Top Level Debug Menu by default		*/
	if (pParent == NULL)
		pParent = &SDM_Main[0];

	/*	Register new command if parent is menu type				*/
	if (pParent->dm_name && (pParent->dm_type == DMT_DESC))
	{
		ExtItem_t	*pExtItem;
		DebugMenu_t	*pExtMenu, *pNewMenu;

//		dbgprint("Adding command %s to menu %s(dm_opts=%x)", pCmd->dm_desc, pParent->dm_desc, pParent->dm_opts);

		/*	Search terminal node at the end of list				*/
		for (pCurrent = pParent; (pCurrent->dm_type != DMT_TERM) && (pCurrent->dm_type != DMT_EXTM); pCurrent++)
			;

		pExtMenu = (DebugMenu_t *)pCurrent->dm_next;
		pExtItem = (ExtItem_t   *)&pCurrent->dm_opts;
		pNewMenu = pExtMenu;
		if (pCurrent->dm_type == DMT_TERM)
		{
			/*	This is terminal node							*/
			pExtItem->ei_numEntry = 0;
			pExtItem->ei_maxEntry = 2 * PL_UNIT_SIZE - 1;

			pCurrent->dm_name = (char *)"extm";
			pCurrent->dm_type = DMT_EXTM;
			pNewMenu = malloc((pExtItem->ei_maxEntry + 1) * sizeof(DebugMenu_t));
//			dbgprint("Allocating new extended menu buffer");
		}
		else if (pExtItem->ei_numEntry == pExtItem->ei_maxEntry)
		{
			/*	Extended menu item, but rack of menory			*/
//			dbgprint("Reallocating current extended menu buffer");
			pExtItem->ei_maxEntry += PL_UNIT_SIZE;
			pNewMenu = realloc(pExtMenu, (pExtItem->ei_maxEntry + 1) * sizeof(DebugMenu_t));
		}

		if (pNewMenu != pExtMenu)
		{
			/*	Update pointer to extended menu					*/
			pCurrent->dm_next = pNewMenu;
			pExtMenu          = pNewMenu;
		}

		/*	Copy content of menu item							*/
		pNewMenu = pExtMenu + pExtItem->ei_numEntry;
		memcpy(pNewMenu, pCmd, sizeof(DebugMenu_t));
		pExtItem->ei_numEntry++;

		/*	NULL terminate last menu item in extended menu		*/
		pNewMenu++;
		pNewMenu->dm_name = NULL;
		pNewMenu->dm_next = NULL;
		pNewMenu->dm_type = DMT_TERM;
		pNewMenu->dm_opts = 0;
		pNewMenu->dm_desc = 0;
		pNewMenu->dm_help = 0;
		return(1);
	}
	return(0);
}

int debug_RegisterCmd(DebugMenu_t *pParent, char *name, void *next, int opts, int type, char *desc, char *help)
{
	DebugMenu_t	command;

	command.dm_name = strdup(name);
	command.dm_next = next;
	command.dm_opts = opts;
	command.dm_type = type;
	command.dm_desc = strdup(desc);
	command.dm_help = strdup(help);

	return(debug_RegisterCmdStruct(pParent, &command));
}

/*====================================================================
Function Name   :   SeparateLongVal
Version         :   0.0
Create Date     :   2002,02.15
Author          :   KUN-IL LEE
Description     :   VERSION NUMBER와 VERSION STRING간의 변환함수.
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
void  SeparateLongVal( UINT32 longVal, UINT32 *pVal0, UINT32 * pVal1, UINT32 * pVal2  )
{
	*pVal0  = ((longVal >> 16) & 0xFFFF);
	*pVal1  = ((longVal >>  8) & 0xFF);
	*pVal2  = ( longVal        & 0xFF);
}

/*====================================================================
Function Name   :   debug_GetIndexFromOpts
Version         :   0.0
Create Date     :   2002, 08, 21
Author          :   Jackee
Description     :
Input           :   NONE
Output          :   NONE
Remarks         :	Moved to osa_utils.c
====================================================================*/
int debug_GetIndexFromOpts(char **argv, char *opts, int *output)
{
	return(str2indexInOpts(argv, opts, output));
}

/*====================================================================
Function Name   :   debug_NotCodedYet
Version         :   0.0
Create Date     :   2002, 08, 21
Author          :   Jackee
Description     :
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
void debug_NotCodedYet()
{
	rprint1n("Not implemented yet, Sorry");
	rprint1n("If you really want this menu, do it yourself");
	return;
}

int debug_GetAddress(char *arg)
{
	extern UINT32 findSymByName(char *symName);
	char	*remain;
	int		sign = -1;
	UINT32	addr;

	if (1             )	{ strtok2(arg, "+", &remain); sign =  1; }
	if (remain == NULL)	{ strtok2(arg, "-", &remain); sign = -1; }

	addr = strtoul2(arg, NULL, 0);

	if (addr == 0)
	{
		addr = findSymByName(arg);
	}

	if      (sign ==  1) addr += strtoul2(remain, NULL, 0);
	else if (sign == -1) addr -= strtoul2(remain, NULL, 0);

	if (addr < CPU_MEM_BASE)
	{
		rprint1n("Address[0x%08x] is invalid, set to default", addr);
		addr = (UINT32)_start_code;
	}

	return(addr);
}

char* debug_GetSymbolName(UINT32 addr, char *retName, size_t size, char *defName, int precision, int bRef)
{
	extern UINT32 findSymByAddr(UINT32 addr, char **pSymName);
	char	*symName = defName;
	UINT32	sym_addr;

	sym_addr = findSymByAddr(addr, &symName);
	if (sym_addr != 0)
	{
		if ( bRef || (addr == sym_addr) )
			snprintf(retName, size, "%*s+0x%04x", precision, symName, addr - sym_addr);
		else
			snprintf(retName, size, "%*s+0x%04x", precision, "", addr - sym_addr);
		symName = retName;
	}
	else
	{
		symName = defName;
	}
	return(symName);
}

/*====================================================================
Function Name   :	debug_GetMeoryMode
					debug_MemoryModifyByte
					debug_MemoryModifyShort
					debug_MemoryModifyWord
					debug_MemoryDump
					debug_MemoryModify
                    debug_MemoryFill
                    debug_PrintNumber
Version         :   0.0
Create Date     :   2002, 08, 21
Author          :   Jackee
Description     :
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
int debug_GetMemoryMode(char ***pArgv)
{
	char	c;
	char	n;

	c = pArgv[0][0][0];
	n = pArgv[0][0][1];
	if(n != '\0')
		return (4);

	if		(c == 'b') { (*pArgv)++; return(1); }
	else if (c == 's') { (*pArgv)++; return(2); }
	else if (c == 'l') { (*pArgv)++; return(4); }
	else               {             return(4); }
}

void debug_MemoryDump(char **argv)
{
	static UINT32	addr = CPU_MEM_BASE, size = 0x80;
	char			*symName, symNameBuf[64];

	if (argv)
	{
		if (*argv) addr = debug_GetAddress(*argv++);
		if (*argv) size = strtoul2(*argv++, NULL, 0);
		addr &= ~0x3;
	}

	symName = debug_GetSymbolName(addr, symNameBuf, 64, "MemoryDump", 0, 0);
	hexdump(symName, (void *)addr, size);

	addr += size;
	return;
}

void debug_MemoryModifyByte(UINT08 *data, UINT32 count)
{
	UINT32	index;
	char	inputStr[12];
	char	*cmd;

	if (count == 0)
	{
		count = (UINT32)0x7ffffff;
		rprint1n("\tType '.' for exit");
	}

	for (index = 0; (SINT32)index < (SINT32)count; index++, data++)
	{
		ReadCmdString(" => ", &inputStr[0], 12);

		cmd = (char *)strtrim(&inputStr[0]);

		if (*cmd == '.') {                         break;    }
		if (*cmd == 'k') { index -= 2; data  -= 2; continue; }
		if (*cmd == 'j') {                         continue; }

		if (*cmd)
			*data  = strtoul2(&inputStr[0], NULL, 0);
	}

	return;
}

void debug_MemoryModifyShort(UINT16 *data, UINT32 count)
{
	UINT32	index;
	char	inputStr[12];
	char	*cmd;

	if (count == 0)
	{
		count = (UINT32)0x7fffffff;
		rprint1n("\tType '.' for exit");
	}

	for (index = 0; (SINT32)index < (SINT32)count; index++, data++)
	{
		ReadCmdString(" => ", &inputStr[0], 12);

		cmd = (char *)strtrim(&inputStr[0]);

		if (*cmd == '.') {                         break;    }
		if (*cmd == 'k') { index -= 2; data  -= 2; continue; }
		if (*cmd == 'j') {                         continue; }

		if (*cmd)
			*data  = strtoul2(&inputStr[0], NULL, 0);
	}

	return;
}

void debug_MemoryModifyWord(UINT32 *data, UINT32 count)
{
	UINT32	index;
	char	inputStr[16];
	char	*cmd;

	if (count == 0)
	{
		count = (UINT32)0x7fffffff;
		rprint1n("\tType '.' for exit");
	}

	for (index = 0; (SINT32)index < (SINT32)count; index++, data++)
	{
		ReadCmdString(" => ", &inputStr[0], 16);

		cmd = (char *)strtrim(&inputStr[0]);

		if (*cmd == '.') {                         break;    }
		if (*cmd == 'k') { index -= 2; data  -= 2; continue; }
		if (*cmd == 'j') {                         continue; }

		if (*cmd)
			*data  = strtoul2(&inputStr[0], NULL, 0);
	}

	return;
}

void debug_MemoryModify(char **argv)
{
	UINT32	addr = 0, count = 0;
	int		mode;

	TAG_USAGE_LABEL(!argv);

	mode = debug_GetMemoryMode(&argv);

	if (*argv) addr = debug_GetAddress(*argv++);
	else	   GOTO_USAGE_LABEL();
	addr &= ~(mode-1);

	if (*argv) count = strtoul2(*argv++, NULL, 0);

	gnEditMemory = 1;

	if		(mode == 1) { debug_MemoryModifyByte ((UINT08 *)addr, count); }
	else if (mode == 2) { debug_MemoryModifyShort((UINT16 *)addr, count); }
	else                { debug_MemoryModifyWord ((UINT32 *)addr, count); }

	gnEditMemory = 0;

	return;
}

void debug_MemoryFill(char **argv)
{
	UINT32	addr = 0, value, count = 1;
	int		i, mode;
	UINT08	*ucp;
	UINT16	*usp;
	UINT32	*ulp;

	TAG_USAGE_LABEL(!argv);

	mode = debug_GetMemoryMode(&argv);

	if (!*argv) GOTO_USAGE_LABEL();

	addr  = strtoul2(*argv++, NULL, 0);
	value = strtoul2(*argv++, NULL, 0);
	if (*argv)
		count = strtoul2(*argv,NULL, 0);

	if (mode == 1)
	{
		ucp = (UINT08 *)addr;
		for (i = 0; i < count; i++)
			*ucp++ = (UINT08) value;
	}
	else if (mode == 2)
	{
		usp = (UINT16 *)addr;
		for (i = 0; i < count; i++)
			*usp++ = (UINT16) value;
	}
	else
	{
		ulp = (UINT32 *)addr;
		for (i = 0; i < count; i++)
			*ulp++ = (UINT32) value;
	}

	return;
}

void debug_PrintNumber(char *arg)
{
/*
 *
 *	다음의 형식으로 출력된다.
 *	      12345678901     12345678901       12345678901234567890123456789012345
 *	UINT:  4294967295                        [ 3G + 1023M + 1023K + 1023 byte ]
 *	SINT:          -1                       10987654 32109876 54321098 76543210
 *	Hex: 0xffffffff  Oct: 37777777777  Bin: 11111111 11111111 11111111 11111111
 */
	char	binStr[40], canStr[40], octStr[20];
	char	unitStr[4] = " KMG";
	int		i;
	UINT32	data1, data2, data3, num;

	data1 = (UINT32)strtoul2(arg, NULL, 0);

	strcpy(canStr, "[ ");
	data2 = data1;
	for ( i = 3; i > 0; i--)
	{
		num = (1 << (i * 10));
		data3 = data2 / num;
		data2 = data2 % num;
		if (data3)
		{
			snprintf(octStr, 20, "%d%c + ", data3, unitStr[i]);
			strcat(canStr, octStr);
		}
	}

	snprintf(octStr, 20, "%d byte ]", data2);
	strcat(canStr, octStr);

	memset(binStr, 0x20, sizeof(binStr)-1);
	binStr[sizeof(binStr)-1] = '\0';

	for (i = 31; i >= 0; i--)
	{
		num = (1 << i);
		if (data1 & num) binStr[34 - i - (i/8)] = '1';
		else             binStr[34 - i - (i/8)] = '0';
	}

	data2 = (data1 >> 30);
	data3 = (data1 & 0x3fffffff);

	rprint1n("UINT: %11u %57s", data1, canStr);
	rprint1n("SINT: %11d %57s", data1, "10987654 32109876 54321098 76543210");
	rprint1n("Hex: 0x%08x  Oct: %d%010o  Bin: %35s", data1, data2, data3, binStr);

	return;
}

/*====================================================================
Function Name   :   debug_Disassemble
Version         :   0.0
Create Date     :   2002, 08, 21
Author          :   Jackee
Description     :
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
extern UINT32 _start_code[];
#define __start_text	_start_code

static UINT32 dsmaddr[3] = {(UINT32)__start_text};
static __inline void putaddr(UINT32 addr)
{
	int i;
	for(i = 2; i >= 0; i--)
	{
		if(i == 0)
			dsmaddr[i] = addr;
		else
			dsmaddr[i] = dsmaddr[i-1];
	}
}

static __inline UINT32 getaddr(int index)
{
	return dsmaddr[index];
}

#include "ctype.h"
#include "exception.h"
void debug_Disassemble(char **argv)
{
	extern int dsmInst
    (
    FAST INSTR 		*binInst,	/* Pointer to the instruction */
    UINT32			address,	/* Address prepended to instruction */
    pVOIDFUNCPTR	prtAddress	/* Address printing function */
    );
	extern int regPrintDsmMode(UINT32 print_func);
	static UINT32	count = 9;
	UINT32          addr;
	int				i;

	addr = getaddr(0);

	if (argv)
	{
		if (*argv)
		{
			char *arg0 = *argv++;

			if (strcmp(arg0, "..") == 0)
			{
				addr = getaddr(1);
			}
			else
			{
				addr = debug_GetAddress(arg0);
			}
		}
		if (*argv) count = strtoul2(*argv++, NULL, 0);
		addr &= ~0x3;
	}

	regPrintDsmMode((UINT32)rprint0n);
	rprint0n("\n\t[    Symbol Name    | Address | Hexcode |       Assembly code       ]\n");

	for (i = 0; i < count; i++, addr += 4)
	{
		char	symNameBuf[32];
		rprint0n("\t%-19.19s] ", debug_GetSymbolName(addr, symNameBuf, 32, "      None", -12, (i == 0)));

		dsmInst((void *)addr, (UINT32)addr, NULL);
	}

	putaddr(addr);
	return;
}

/*====================================================================
Function Name   :   cmdDebugMain
Version         :   1.0
Create Date     :   2007/03/06
Author          :   이재길(jackee@lge.com)
Description     :
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
void cmdDebugMain(void)
{
	int debug_MainMenu(DebugMenu_t *pMenu, char **argv);
	debug_MainMenu(NULL, NULL);
	return;
}

/*====================================================================
Function Name   :   debug_MainMenu
Version         :   1.0
Create Date     :   2007/03/06
Author          :   이재길(jackee@lge.com)
Description     :
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
int debug_MainMenu(DebugMenu_t *pMenu, char **argv)
{
	static DebugMenu_t	*ppMenuStack[64] = { NULL, };
	DebugMenu_t	*pCurrMenu;
	static int	verbose = FALSE;
	static int	curr_hist = 9;
	static int	last_hist = 9;
	static char	cmd_hist[10][80];
	int			argc, value, error_flag;
	int			do_loop = TRUE, cmd_type = 0;
	int			newMenuLevel = -1, popLevel = 0;
	char		cmdStr[80];
	char		*cmd, **new_argv, *argList[16];

	/*	Enter Top Level Debug Menu by default	*/
	if (pMenu  == NULL)
		pMenu  = &SDM_Main[0];

	argc = 0;
	/*	Check Arguement Counter					*/
	if ((argv != NULL) && (*argv != NULL) )
	{
		do_loop = FALSE;
		for (argc = 0; argv[argc] != 0; argc++)
			;
	}
	if (gnDebugLevel == 0)
	{
		/* Old debug menu 진입시에만 F9에 의한	*/
		/* message 출력 방지를 해제한다.		*/
		if (pMenu->dm_opts & 1)
			gDebugEnable = 1;
	}

	/* Push the pointer to current menu			*/
	ppMenuStack[gnDebugLevel] = pMenu;

	/* Increase Menu Level						*/
	gnDebugLevel++;


	/*	Loop to read command and excute it		*/
	do
	{
		char		*dm_name;
		char		*dm_help;
		int			dm_type;
		int			dm_opts;
		int			cHistory;
		void		(*dm_func)();
		DebugMenu_t	*dm_menu;

		/* PRINTF 만 출력되게 함				*/
		cHistory = 0;
		newMenuLevel = gnDebugLevel - 1;

		if (do_loop)
		{
			/*	F10으로 mainMenu까지 종료함.	*/
			if (gnDebugLevel == 0)
				goto exit_menu;

			if (pMenu->dm_opts & 1)
				debug_HelpMenu(pMenu, 1);

			/*	Read command strings from uart	*/
			{
				char *mode_name = NULL;
				mode_name = "";
				rprint0n("\n%d:%s%s", gnDebugLevel, pMenu->dm_desc, mode_name);
			}
			(void) ReadCmdString(" $ ", &cmdStr[0], 80);

			/*	Save command string for history	*/
			if ( cmdStr[0] != '!')
				strcpy(cmd_hist[0], cmdStr);

ParseHistory:
			/*	Skip heading white spaces		*/
			cmd = &cmdStr[0];
			while ((*cmd == ' ') || (*cmd == '\t'))
				cmd++;
			/*	Check if referencing from root	*/
			if (*cmd == '/')
				newMenuLevel = 0;
			/*	Convert '/' to space			*/
			while ((*cmd != ' ') && (*cmd != '\t') && (*cmd != '\0'))
			{
				if (*cmd == '/') *cmd = ' ';
				cmd++;
			}

			/*	Tokenize as command and args	*/
			argv = argList;
			memset(argList, 0, sizeof(argList));
			argc = str2argv(&cmdStr[0], 8, argv);
			if (verbose)
			{
				rprint1n("+%d:: argc = %d, argv = %x/%x/%x",
					gnDebugLevel, argc, argv[0], argv[1], argv[2]);
			}
		}

		while ((argc > 0) && (strcmp("..", argv[0]) == 0))
		{
			argc--;
			argv++;
			if (newMenuLevel > 0) newMenuLevel--;
		}

		if (argc == 0)
		{
			if ((argc == 0) && (gnDebugLevel > 0) && (pMenu->dm_opts & 2))
			{
				popLevel = 1;
				break;
			}
			if (gnDebugLevel > (newMenuLevel + 1))
				popLevel = (gnDebugLevel - (newMenuLevel + 1));
			continue;
		}

		cmd = argv[0];
		if (cmd[0] == '!')
		{
			if ( (cmd[1] >= '1') && (cmd[1] <= '9') )
				last_hist = cmd[1] - '0';

			cHistory = last_hist;
			strcpy(cmdStr, cmd_hist[cHistory]);
			if (verbose)
			{
				rprint1n("+%d:: cHist = %d, cmd = '%16.16s'",
						gnDebugLevel, cHistory, cmd_hist[cHistory]);
			}
			goto ParseHistory;
		}

		/*               cmd_type     0         1            2    3           */
		(void) str2indexInOpts(&cmd, "help,?,ls|exit,x,,ff|hist|verbose,verb", &cmd_type);

		/*	Check type of command				*/
		if		(cmd_type == 0) { cmd = argv[1];		/* Help				*/	}
		else if (cmd_type == 1) { popLevel = 1; break;	/* Return to parent */	}
		else if (cmd_type == 2)
		{
			int	i;

			for (i = 1; i < 10; i++)
			{
				rprint1n("  %d: %s", i, cmd_hist[i]);
			}
			continue;
		}
		else if (cmd_type == 3)
		{
			verbose = !verbose;
			rprint1n("Turn '%s' verbose flag", (verbose ? "on " : "off"));
			continue;
		}
		else if (cHistory == 0 && cmdStr[0] != '!')
		{
			curr_hist = (curr_hist % 9) + 1;
			last_hist = curr_hist;
			strcpy(cmd_hist[curr_hist], cmd_hist[0]);
			if (verbose)
			{
				rprint1n("+%d:: Hist last %d curr %d %s %s",
						gnDebugLevel, last_hist, curr_hist, cmd_hist[curr_hist], cmd_hist[0]);
			}
		}

		pCurrMenu = &SDM_Global[1];
		dm_name	  = (char *)"None";

		if (cmd_type != 1)		/* Not ! cmd	*/
		{
			int			i, depth = 0;
			DebugMenu_t *pMenuList[2];

			pMenuList[0] = &SDM_Global[1];
			pMenuList[1] = ppMenuStack[newMenuLevel]+1;

			for (i = 0; i < 2; i++)
			{
				for (pCurrMenu = pMenuList[i]; (dm_name = pCurrMenu->dm_name) != NULL; pCurrMenu++)
				{
					if (strcasecmp(dm_name, cmd) == 0)
					{
						i = 99;	/* Force to exit outer for loop */
						break;
					}
				}
			}

			gpCurrCmd = cmd;	/* Save Real Command Name	*/

			while  ( (pCurrMenu->dm_type == DMT_LINK) && (pCurrMenu->dm_help != NULL))
			{
				cmd       = pCurrMenu->dm_help;
				pCurrMenu = (DebugMenu_t *)pCurrMenu->dm_next + 1;
				for (; (dm_name = pCurrMenu->dm_name) != NULL; pCurrMenu++)
				{
					if (strcasecmp(dm_name, cmd) == 0)
						break;
				}

				/* Prevent from Recursive Linking	*/
				if (++depth > 5) break;
			}
		}

		dm_type = pCurrMenu->dm_type;
		dm_opts = pCurrMenu->dm_opts;
		dm_help = pCurrMenu->dm_help;
		dm_func = (VFP_t       *)pCurrMenu->dm_next;
		dm_menu = (DebugMenu_t *)pCurrMenu->dm_next;

		if (cmd_type == 0)		/* Help		*/
		{
			if (argc <= 1)
			{
				debug_HelpMenu(ppMenuStack[newMenuLevel], 1);		/* overall help message		*/
			}
			else if (dm_type < DMT_MENU)
			{
				if (dm_name != NULL)
				{
					gpCurrMenu = pCurrMenu;
					debug_HelpMenu(pCurrMenu, 0);	/* Detail help message		*/
				}
				else if (strcmp(cmd, "*") == 0)
				{
					debug_HelpMenu(ppMenuStack[newMenuLevel], 2);
				}
				else
				{
					rprint1n("'%s' is not in command list", cmd);
				}
			}
			else
			{
				/*
				**	Cascade to next menu level
				**	Command line was entered as "help video", so we must
				**	send "help" as initial command of video menu.
				*/
				argv[1] = argv[0];
				debug_MainMenu(dm_menu, argv+1);
			}
		}
		else
		{
			/*	No matched command found		*/
			if (dm_name == NULL)
			{
				rprint1n("Command not found '%s'", cmd);
				continue;
			}

			gpCurrMenu = pCurrMenu;
			error_flag = TRUE;
			new_argv   = ((argc > 1) ? (argv+1) : NULL);

			switch (dm_type)
			{
			  case DMT_NONE:
				if (verbose)
					rprint1n("+%d:: Type %d cmd : %s()", gnDebugLevel, dm_type, dm_name);
				dm_func();
				error_flag = FALSE;
				break;
			  case DMT_ARG1:
				if (argc == 2)
				{
					if (verbose)
						rprint1n("+%d:: Type %d cmd : %s(%s)", gnDebugLevel, dm_type, dm_name, argv[1]);
					dm_func(argv[1]);
					error_flag = FALSE;
				}
				break;
			  case DMT_ARG2:
				if (argc == 3)
				{
					if (verbose)
					{
						rprint1n("+%d:: Type %d cmd : %s(%s,%s)",
							gnDebugLevel, dm_type, dm_name, argv[1], argv[2]);
					}
					dm_func(argv[1], argv[2]);
				}
				break;
			  case DMT_OPTS:
				if (verbose)
					rprint1n("+%d:: Type %d cmd : %s(%d)", gnDebugLevel, dm_type, dm_name, dm_opts);
				dm_func(dm_opts);
				error_flag = FALSE;
				break;
			  case DMT_INDX:
				if (str2indexInOpts(new_argv, dm_help, &value) >= 0)
				{
					if (verbose)
						rprint1n("+%d:: Type %d cmd : %s(%d)", gnDebugLevel, dm_type, dm_name, value);
					dm_func(value);
					error_flag = FALSE;
				}
				break;
			  case DMT_ARGV:
				if (verbose)
					rprint1n("+%d:: Type %d cmd : %s(0x%x)", gnDebugLevel, dm_type, dm_name, new_argv);
				dm_func(new_argv);
				error_flag = FALSE;
				break;
			  case DMT_MENU:
				if (verbose)
					rprint1n("+%d:: Enter Menu : %s(0x%x)", gnDebugLevel, dm_name, new_argv);
				popLevel = debug_MainMenu(dm_menu, new_argv);
				error_flag = FALSE;
				break;
			  default:
				/* Just Ignore Unknown type of command */
				break;
			}

			if (error_flag)
			{
				debug_HelpMenu(NULL, 0);
			}
		}
	} while (do_loop && (popLevel == 0));

	/* Decrease Menu Level						*/
	gnDebugLevel--;

	if (do_loop && (popLevel == 0))
		rprint1n("");

exit_menu:
	if (gnDebugLevel == 0)
	{
	}

	return ( (popLevel>0) ? popLevel-1 : 0 );
}




/*====================================================================
Function Name   :   debug_SysVersion
Version         :   0.1
Create Date     :   2002,02,22
Author          :   DREAMER
Description     :   FLASH에 저장된 VERSION INFO를 출력한다.
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
void debug_SysVersion(void)
{
#if 0
	extern SystemHeader_t	SystemHeader;
	SystemHeader_t	*pSysHdrAppl = &SystemHeader;
	SystemHeader_t	*pSysHdrBrom = (SystemHeader_t *)(FLASH_BASE);

	rprint1n("\n");
	rprint1n("--------------------------------------------------------------------------------");
	rprint1n("  Package  Version  DateOfCompilation     builder@computer:/directory");
	rprint1n("  Bootrom  V%-7s %s %s:%s",
						pSysHdrBrom->sh_ver_string,
						pSysHdrBrom->sh_build_date,
						pSysHdrBrom->sh_image_maker,
						pSysHdrBrom->sh_compile_path);
	rprint1n("  UserApp  V%-7s %s %s:%s",
						pSysHdrAppl->sh_ver_string,
						pSysHdrAppl->sh_build_date,
						pSysHdrAppl->sh_image_maker,
						pSysHdrAppl->sh_compile_path);
	rprint1n("--------------------------------------------------------------------------------");
	rprint1n("\n");
#endif
	return;
}

/*====================================================================
Function Name   :   debug_ControlSerial
Version         :   0.0
Create Date     :   2002, 08, 21
Author          :   Jackee
Description     :
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
void debug_ControlSerial(char **argv)
{
}

/*====================================================================
Function Name   :   debug_PrintUptime
Version         :   0.0
Create Date     :   2006,03.07
Author          :   JACKEE
Description     :   Booting 후 경과한 시간을 출력하는 함
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
void debug_PrintUptime()
{
	rprint1n("Uptime = %d Seconds Elapsed", (UINT32)(readMsTicks()/1000));
}

/*====================================================================
Function Name   :   do_system_reset()
Version         :   0.
Create Date     :   2006,03,14
Author          :   Jackee
Description     :   Call a function with 0/1 numeric arguement
Input           :   Arguement list of function address and number
Output          :   NONE
Remarks         :
====================================================================*/
#if 0
RebootLine_t *getRebootLine(int mode)
{
	return ((RebootLine_t *)(CPU_MEM_BASE+RL_BOOT_OFFSET));
}
#include "cache.h"
#endif
static void do_system_reset(char *arg)
{
	#if 0
	extern void		system_reset(void);
	extern void		cacheFlush(int mode,  int addr, int size);
	RebootLine_t	*pRebootLine = getRebootLine(0);

	if (arg == NULL )
	{
		sprintf(pRebootLine->rl_bootCmds, "");
	}
	else
	{
		sprintf(pRebootLine->rl_bootCmds, "boot %s", arg);
	}
	pRebootLine->rl_bootMagic = RL_BOOT_MAGIC;

    /*
	 * Lock interrupts before jumping out of boot image. The interrupts
     * enabled during system initialization in sysHwInit()
     */
    intLock();

	/*
	 * Flush Cache to write back bootline to ddr
	 */
	Cache_Disable();

	/*
	 * Make goto reset vector by watch dog timer
	 */
   	system_reset();

	while (1)
		;

	/* Never Reached Here	*/
	#endif
}

/*====================================================================
Function Name   :   debug_SystemReset()
Version         :   0.
Create Date     :   2006,03,14
Author          :   Jackee
Description     :   Call a function with 0/1 numeric arguement
Input           :   Arguement list of function address and number
Output          :   NONE
Remarks         :
====================================================================*/
void debug_SystemReset(char **argv)
{
	do_system_reset(NULL);

	/* Never Reached Here	*/
}

/*====================================================================
Function Name   :   debug_SendRebootCmd()
Version         :   0.
Create Date     :   2002,08,28
Author          :   Jackee
Description     :   Send new boot command to bootrom
Input           :   Arguement list of boot command
Output          :   NONE
Remarks         :
====================================================================*/
void debug_SendRebootCmd(char **argv)
{
	char		arg[128];

	sprintf(arg, " ");
	while (argv && *argv)
	{
		strcat(arg, *argv++);
		strcat(arg, " ");
	}

	do_system_reset(arg);

	/* Never Reached Here	*/
}

/*====================================================================
Function Name   :   debug_CallFunction()
Version         :   0.
Create Date     :   2006,03,14
Author          :   Jackee
Description     :   Call a function with 0/1 numeric arguement
Input           :   Arguement list of function address and number
Output          :   NONE
Remarks         :
====================================================================*/
void debug_CallFunction(char **argv)
{
	UINT32	addr, arg;

	TAG_USAGE_LABEL(!argv);

	if (*argv) addr = debug_GetAddress(*argv++);
	else	   GOTO_USAGE_LABEL();
	addr &= ~3;

	if (*argv)
	{
		arg = strtoul2(*argv++, NULL, 0);
		dbgprint("Calling 0x%08x(0x%08x)\n", addr, arg);
		((void (*)(int))addr)(arg);
	}
	else
	{
		dbgprint("Calling 0x%08x()\n", addr);
		((void (*)(void))addr)();
	}
	return;
}

/*====================================================================
Function Name   :   debug_TestBoard
Version         :   0.
Create Date     :   2002,03.06
Author          :   KUN-IL LEE
Description     :   TARGET BOARD의 상황을 TEST 하기위한 함수
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
static void testStackTrace(void)
{
	(void)PrintStack();
}

static void readInvalidAddr(void)
{
	volatile int *ip1 = (volatile int *)((UINT32)_start_code+0x3d3b);
	volatile int *ip2 = (volatile int *)((UINT32)_start_code+0x3d3d);
	*ip1 = *ip2;
}

static void returnToBrokenLr(int depth)
{
	/*
	 *	Saved return address may differ according to compile option
	 */
	unsigned long	sp, lr;

	if (depth > 0)
	{
		returnToBrokenLr(depth-1);
	}
	else
	{
		str_sp(&sp);
		str_lr(&lr);

		rprint1n("Now in function returnToBrokenLr(), lr=0x%x", lr);
		PrintStack();

		hexdump("Before Change", (void *)sp, 0x40);
		do
		{
			sp += 4;
		} while (*(long *)sp != lr);
		*(long *)sp = 0x0c777777;

		rprint1n("Return address at 0x%x has been destroied", sp);
	}
	return;
}

/*====================================================================
Function Name   :   debug_TestBoard
Version         :   0.
Create Date     :   2002,03.06
Author          :   KUN-IL LEE
Description     :   TARGET BOARD의 상황을 TEST 하기위한 함수
Input           :   NONE
Output          :   NONE
Remarks         :
====================================================================*/
static void testStackTrace(void);
static void readInvalidAddr(void);
static void returnToBrokenLr(int);
extern int	UartGetKeyPressed(void);

void debug_TestBoard( char **argv )
{
	int	index;

	if (argv == NULL)
	{
    	// EEROM TEST

	    // 기타 다른 부분은 추후 확인한다.
	}
	else if (!strcmp(*argv, "crash"))
	{
		void (*pFunc)(void);

		(void) str2indexInOpts(argv+1, "[trace|strlen|atoi|addr|align|lr|call]", &index);
		switch (index)
		{
		  case 0:
			rprint1n("\nTesting:: StackTrace()");
			pFunc = (void (*)(void))testStackTrace;
	    	((void (*)(void))pFunc)();
			break;
		  case 1:
			rprint1n("\nCrashing:: strlen()");
			rprint1n("%s", ((UINT32)_start_code+0x3d43));
			break;
		  case 2:
			rprint1n("\nCrashing:: atoi()");
			index = atoi((char *)((UINT32)_start_code+0x3d45));
			rprint1n("result = %d", index);
			break;
		  case 3:
			rprint1n("\nCrashing:: By reading invalid address");
			readInvalidAddr();
			break;
		  case 4:
			rprint1n("\nCrashing:: By reading non-aligned address");
			rprint1n("Data is %d", *(UINT32 *)((UINT32)_start_code+0x3d3d));
			break;
		  case 5:
			rprint1n("\nCrashing:: By return to broken link register");
			returnToBrokenLr(16);
			break;
		  case 6:
		    rprint1n("\nCrashing:: By call function with Invalid address");
			pFunc = (void (*)(void))((UINT32)_start_code+0x3d3d);
	    	((void (*)(void))pFunc)();
			break;
		  default:
			rprint1n("Options available %s", "[trace|strlen|atoi|addr|align|lr|call]");
			break;
		}
	}
}

/*------------------------------------------------------------------------------
	Debug Menu 를 위한 구조체 선언.
------------------------------------------------------------------------------*/
DebugMenu_t SDM_SysInfo[] = {
//  Name,     Next,                          OPTS,  TYPE,     Description,                   Help
  { "none",	  (void *)NULL,                     0,  DMT_NONE, "systemInfo",                  NULL },

  /* OS Adaptation Layer Information Viwer	*/
  { "task",	  (void *)dbgPrintTask1,            0,  DMT_ARG1, "Show OSA Task status",
	"taskId|taskName|-1"							DMT_HELP_TAG
	": Use -1 to display all task information"		DMT_HELP_TAG
  },
  { "sema",	  (void *)dbgPrintSemas,            0,  DMT_NONE, "Show OSA Sema4 status",       NULL },
  { "msg",	  (void *)dbgPrintMsgQs,            0,  DMT_NONE, "Show OSA MsgQ status",        NULL },

  {  NULL,	  (void *)NULL,                     0,  DMT_NONE,  NULL,                         NULL }
};

DebugMenu_t SDM_System[] = {
//  Name,     Next,                          OPTS,  TYPE,     Description,                   Help
  { "none",	  (void *)NULL,                     0,  DMT_NONE, "debugSystem",                 NULL },

#if (USE_OSAMEM > 0)
  { "smtr",   (void *)SM_DoLogCaller,           0,  DMT_INDX, "Change SM Trace flag",  "[off|on]" },
  { "smpb",   (void *)SM_PrintBuddy,            0,  DMT_INDX, "Print Buddy Alloc List", "[0..15]" },
  { "smbm",   (void *)SM_BuddyMerge,            0,  DMT_INDX, "Merge Buddy free list",   "[0..2]" },
  { "smbs",   (void *)SM_BuddyStatus,           0,  DMT_INDX, "Show Buddy Memory Status","[1..2]" },
  { "smfs",   (void *)SM_ShowStatus,            0,  DMT_NONE, "Show Fast/Buddy Statistics0", NULL },
  { "smt",    (void *)SM_TestFuncs,             0,  DMT_NONE, "Test functions in SM",        NULL },
#endif	/* USE_OSAMEM > 0 */

  {  NULL,	  (void *)NULL,                     0,  DMT_NONE,  NULL,                         NULL }
};

/*------------------------------------------------------------------------------
	Main Debug Menu and Global Command Menus
------------------------------------------------------------------------------*/
DebugMenu_t	SDM_Global[] = {
  { "none",	  (void *)NULL,                     0,  DMT_NONE, "globalCmd",                   NULL },
  { "show",	  (void *)&SDM_SysInfo,             0,  DMT_MENU, "Show Various status",         NULL },
  { "ver",	  (void *)debug_SysVersion,         0,  DMT_NONE, "Show VERsion data",           NULL },
  { "md",	  (void *)debug_MemoryDump,         0,  DMT_ARGV, "Memory dump",
	"[address [size]]"								DMT_HELP_TAG
	"- Dump range is [addres, address+size)"		DMT_HELP_TAG
	"- Default dump size is 64"						DMT_HELP_TAG
  },
  { "mm",	  (void *)debug_MemoryModify,       0,  DMT_ARGV, "Modify memory",
	"[mode] address [count]"						DMT_HELP_TAG
	"- Modify memory from 'address' count units"	DMT_HELP_TAG
	"- mode : One of b,s,l(default)"				DMT_HELP_TAG
	"         b for  8 bit access"					DMT_HELP_TAG
	"         s for 16 bit access"					DMT_HELP_TAG
	"         l for 32 bit access"					DMT_HELP_TAG
  },
  { "mf",	  (void *)debug_MemoryFill,         0,  DMT_ARGV, "Memory fill",
    "[mode] address value count"					DMT_HELP_TAG
	"- mode : One of b,s,l. Same with 'mm' command"	DMT_HELP_TAG
  },
  { "dsm",	  (void *)debug_Disassemble,        0,  DMT_ARGV, "Disassemble memory",
    "address [count]"								DMT_HELP_TAG
	"- address : Hex/Dec address     => Use address   "					DMT_HELP_TAG
	"            SymbolName[+offset] => Use address of symbol"			DMT_HELP_TAG
	"            .                   => Use previous address in stack"	DMT_HELP_TAG
  },
  { "uptime", (void *)debug_PrintUptime,     	0,  DMT_NONE, "Print system up time",        NULL },
  { "call",   (void *)debug_CallFunction,       0,  DMT_ARGV, "Call a function",
    "addr|symbol [number]"							DMT_HELP_TAG
    "- excute function call symbol(number)"			DMT_HELP_TAG
  },
  { "reset",  (void *)debug_SystemReset,		0,  DMT_ARGV, "Reset system",                NULL },
  { "boot",   (void *)debug_SendRebootCmd,      0,  DMT_ARGV, "Reboot new application",     "url" },
  {  NULL,	  (void *)NULL,                     0,  DMT_NONE,  NULL,                         NULL }
};

DebugMenu_t	SDM_Main[] = {
  { "none",	  (void *)NULL,                     0,  DMT_NONE, "debugMain",                   NULL },
  { "sys",	  (void *)&SDM_System,              0,  DMT_MENU, "System debug",    			 NULL },

  { "ser",	  (void *)debug_ControlSerial,      0,  DMT_ARGV, "Toggle Serial Output Control",
	"[1..12]"										DMT_HELP_TAG
	"- Toggle debug output from Task group"			DMT_HELP_TAG
  },

  { "test",   (void *)debug_TestBoard,          0,  DMT_ARGV, "Do some tests",               NULL },

  {  NULL,	  (void *)NULL,                     0,  DMT_NONE,  NULL,                         NULL }
};
