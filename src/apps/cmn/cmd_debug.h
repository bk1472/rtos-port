#ifndef	_CMD_DEBUG_H_
#define	_CMD_DEBUG_H_

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */


/*------------------------------------------------------------------------------
	��ũ�� �Լ� ����
	(Macro Definitions)
------------------------------------------------------------------------------*/
#define	TAG_USAGE_LABEL(expr)						\
	if (expr)										\
	{												\
PrintHelpAndReturn:									\
		debug_HelpMenu(NULL, 0);					\
		return;										\
	}

#define	GOTO_USAGE_LABEL()	goto PrintHelpAndReturn

#define DMT_HELP_TAG		"^"
#define DMT_VOPT_END		"\0"

#define	GET_EXTM(_p)		((DebugMenu_t *)_p->dm_next - 1)

/*------------------------------------------------------------------------------
	�� ����
	(Type Definitions)
------------------------------------------------------------------------------*/

typedef	void	VFP_t(void);
typedef void	UFP_t(ULONG);

/*
**	Debug Menu �� ���� ����ü �ʱⰪ
**
**	  - ù��° Entry�� Prompt�� ���� Description �� ����Ѵ�.
**	  - ������ Entry�� Termination �� ���� dm_type �� �ݵ��
**			DMT_TERM(Terminal node �� ���) �Ǵ�
**			DMT_EXTM(Extended menu �� �ִ� ���)
**		�̾�� �Ѵ�. DMT_TERM�� ���� dm_name �� ���� NULL�̾�� �Ѵ�.
**
**	  - dm_type ���� ���.
**		DMT_NONE: �ƹ��� Arguement�� �������� �ʴ´�.
**			f(void)
**
**		DMT_ARG1: ����࿡�� �Է��� argument�� ù��°�͸� String���� ����
**			f(char *arg1)
**
**		DMT_ARG2: ����࿡�� �Է��� argument�� ó�� �ΰ��� String���� ����.
**			f(char *arg1, char *arg2) ������ �Լ�
**
**		DMT_OPTS: dm_opts �� ���ǵ� ���� argument�� �����Ѵ�.
**			f(int)
**
**		DMT_INDX: dm_help�� ���ǵ� option string�� �Էµ� arguement�� ���Ͽ�
**                ��ġ�ϴ� option�� ��ȣ�� arguement �� �����Ѵ�.
**			f(int)
**			[1..15]		 : 1���� 15������ ����. �Է����� ������ -1
**			1..15		 : 1���� 15������ ����. �Է����� ������ ����
**			[off|on|str] : "on" = 0, "off" = 1, "str" = 2, �Է����� ������ -1
**			off|on|str	 : "on" = 0, "off" = 1, "str" = 2, �Է����� ������ ����
**          1,kor|2,eng  : "kor" �Ǵ� "1" = 1, "eng" �Ǵ� "2" = 2, �Է����� ������ ����
**
**		DMT_ARGV: ����࿡�� �Է��� argument�� argvlist �� ���� �����Ѵ�.
**			f(char **argv)
**
**		DMT_ARGCV: ����࿡�� �Է��� argument�� argvlist �� ���� �����Ѵ�.
**			f(int argc, char **argv)
**
**		DMT_LINK: �ٸ� �޴� ����ü�� ����� �����Ѵ�. ������ �����
**			      dm_help �� ����Ѵ�.
**
**	 	DMT_MENU: dm_args �� ���ǵ� Menu ����ü�� argvlist�� �Բ� �����Ѵ�.
**			f(DebugMenu_t *, char **argv)
**
**		DMT_DESC: Menu list�� ���� ó�� ������ Menu�� ����� ���� ������ ������.
**
**		DMT_EXTM: Menu list�� ���� ������ ������ dm_next�� ����Ű�� ���� Ȯ��
**			menu(����ڰ� �߰� ����� �͵�)�� ������ �˸���.
**
**		DMT_TERM: Menu list�� ���� ������ ������ list�� ���� �˸���.
**			dm_name�� ���� NULL�̿��� �Ѵ�.
*/

typedef struct {
	char		*dm_name;		/* Command Name of debug menu			*/
	void		*dm_next;		/* Call Context for Next Level			*/
								/*  1. Function pointer to be called	*/
								/*	2. Menu Structure to be passed		*/
	int			dm_opts;		/* Optional argument to be passed		*/
	int			dm_type;		/* Call type of sub function or Menu	*/
	char		*dm_desc;		/* Description of debug menu			*/
	char		*dm_help;		/* Detail help message of usage 		*/
								/*  must start with option strings, it	*/
								/*  will be parsed while command line	*/
								/*  interpreting 						*/
} DebugMenu_t;

#define	DMT_NONE		 0		/* Pass no arguement       				*/
#define	DMT_ARG1		11		/* Pass only first one arguement as str */
#define	DMT_ARG2		12		/* Pass only first two arguement as str */
#define	DMT_ARGV		18		/* Pass argc and argv list  			*/
#define	DMT_ARGCV		19		/* Pass argv list        				*/
#define	DMT_OPTS		21		/* Pass optional arguement 				*/
#define DMT_INDX		22		/* Pass Index of matched options string	*/
#define DMT_LINK		23		/* link to command in other menu 		*/
#define	DMT_DESC		80		/* Item describing current menu			*/
#define	DMT_EXTM		81		/* Link to user added command menus		*/
#define	DMT_TERM		82		/* End mark of list of menu items		*/
#define	DMT_MENU		90		/* Pass Menu Struct & Argv List 		*/
								/* This should be last item				*/

#define	DM_SHOW_HELP	 1		/* Show help on entering new menu		*/
#define	DM_EXIT_ON_CR	 2		/* Exit on enter command				*/
#define DM_BUF_ALLOC	 4		/* DebugMenu_t stroage is allocated on	*/

/*------------------------------------------------------------------------------
	Extern ���������� �Լ� prototype ����
	(Extern Variables & Function Prototype Declarations)
------------------------------------------------------------------------------*/
/* Top level Main Menu and Global Menus						*/
extern DebugMenu_t	SDM_Main[];
extern DebugMenu_t	SDM_Global[];

/* Function to add new command into menu, in cmd_debug.c	*/
extern int	  debug_RegisterCmdStruct(DebugMenu_t*, DebugMenu_t*);
extern int	  debug_RegisterCmd(DebugMenu_t*, char *, void *, int, int, char *, char*);

/* Character input function in key_board.c	*/
extern int    UartTimedGetChar(FILE *fp, int waitMs);
extern int    UartGetChar(FILE *fp);
extern int    UartGetCharEcho(FILE *fp);
extern char*  UartGetString(char *buf, int n, FILE *fp);

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/*	_CMD_DEBUG_H_ */
