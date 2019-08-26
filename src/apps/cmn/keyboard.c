/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 1998. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : keyboard.c
    VERSION     :
    AUTHOR      : baekwon choi (baekwon.choi@lge.com).
    DATE        : 2010/02/09
    DESCRIPTION :

*******************************************************************************/

/*-----------------------------------------------------------------------------
	전역 제어 상수
	(Control Constants)
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    #include 파일들
    (File Inclusions)
------------------------------------------------------------------------------*/
#include <common.h>
#include <pc_key.h>
#include "resource_mgr.h"

extern SINT32		gnDebugLevel;
extern SINT32		gnEditMemory;

static volatile int	keyPressed = 0;

static char			*EnableMsgs[2]			= { "Disabling", "Enabling " };

int			gbBrkPressed    = FALSE;

void		keyboardTask(void);
int			CheckSpecialFunctions(int key);

/*====================================================================
Function Name   :   CheckSpecialFunctions()
Version         :   0.1
Create Date     :   2002,09,03
Author          :   Jackee
Description     :   Execute special function mapped at key code.
Input           :   key code sequence
Output          :	NONE
Remarks         :
====================================================================*/
int CheckSpecialFunctions(int key)
{
	int	processed;

	/*
	 * Functions valid only in top-level debug menu
	 *	i.e., not in command line debug mode
	 */
	processed = TRUE;
	if (gnDebugLevel == 0)
	{
		switch (key)
		{
#if (USE_OSAMEM > 0)
		  	case '`':
		  	{
				SM_MarkRange();
				break;
	  		}

		  	case '~':
			{
				SM_ShowStatus(0);
				break;
			}
#else
			/*
			 * No Memory Allocation System profiler.
			 * Should be implemented Here
			 */
#endif	/* USE_OSAMEM */

			default:
				processed = FALSE;
				break;
		}

		if (processed != FALSE)
			return(processed);
	}

	/*
	 * Functions valid in all level of debug
	 */
	processed = TRUE;
	switch (key)
	{
		case PC_KEY_F01:
		{
			break;
		}

		case PC_KEY_F02:
		{
			break;
		}

		case PC_KEY_F03:
		{
			break;
		}

		case PC_KEY_F04:
		{
			break;
		}

		case PC_KEY_F05:
		{
			break;
		}

		case PC_KEY_F06:
		{
			dbgPrintTasks();
			break;
		}

		case PC_KEY_F07:
		{
			dbgPrintSemas();
			break;
		}

		case PC_KEY_F08:
		{
			dbgPrintMsgQs();
			break;
		}

		case PC_KEY_F09:
		{
//			if(!gUartOffStatus)
			{
				gDebugEnable = !gDebugEnable;
				printf("%s display debug message \n", EnableMsgs[gDebugEnable]);
			}
			break;
		}

		case PC_KEY_F10:
		{
			if (gnDebugLevel == 0)
			{
				extern void cmdDebugMain(void);
				setTerminalMode( ECHO);
				cmdDebugMain();
				setTerminalMode(~ECHO);
			}
			break;
		}

		case PC_KEY_F11 :
		{
			break;
		}

		case PC_KEY_F12:
		{
			break;
		}
		case PC_KEY_EXT:
		{
			break;
		}

		case '<':
		case '>':
		{
			/*	Language Changer will be implemented here	*/

			break;
		}

		default:
		{
			processed = FALSE;
			break;
		}
	}
	return(processed);
}

/*====================================================================
Function Name   :   UartSetKeyPressed
Version         :   0.0
Create Date     :   2002,08,22
Author          :   Jackee
Description     :   Increase key pressed counter.
Input           :   Character entered
Output          :
Remarks         :
====================================================================*/
void UartSetKeyPressed(UINT08 inChar)
{
	keyPressed++;
	if (inChar == 0x03)			/* Ctrl-C */
		gbBrkPressed = TRUE;
	return;
}

int UartGetKeyPressed(void)
{
	return keyPressed;
}

/*====================================================================
Function Name   :   ProcessSerialInput
Version         :   0.1
Create Date     :   2002,09,03
Author          :   Jackee
Description     :   Filter escape sequence from keyboard input.
Input           :   Character entered
Output          :
Remarks         :
====================================================================*/
static int ProcessSerialInput( UINT08 inChar )
{
	static UINT32	inCode = 0;
	static SINT32	index  = 0;
	unsigned long	key    = 0;

	index++;

	if (inChar == 0x1B) inCode = 0;
	else                inCode = (inCode << 8) | inChar;

	if      ((index == 1) && (inChar != 0x1B)) key = inChar;
	else if ((index == 2) && (inChar != 0x5B)) key = inChar;
	else if ((index == 3) && (inChar  > 0x40)) key = inCode;
	else if ((index == 4) && (inChar == 0x7E)) key = inCode;
	else if ((index == 5) && (inChar == 0x7E)) key = inCode;
	else if ( index >  5)           index = inCode = 0;

	if (key)
	{
		index  = 0;
		inCode = 0;
	}
	return(key);
}

/*====================================================================
Function Name   :   UartGetChar()
Version         :   0.1
Create Date     :   2002,09,03
Author          :   Jackee
Description     :   Get char or escape sequence from stdin.
Input           :   NONE
Output          :	32 bit key code
Remarks         :
====================================================================*/
int UartGetChar(FILE *fp)
{
	int	i, inKey, prevKeyPressed;

	updateWaitStatus(-1, -1);
	prevKeyPressed = keyPressed + 1;

	inKey = getc(fp);

	if (inKey == PC_KEY_ESC)
	{
		/*	Check for escape sequnce by waiting	20ms	*/
		/* next characters to be entered				*/
		for (i = 0; i < 24; i++)
		{
			if (prevKeyPressed != keyPressed)
				break;
			delayMicroSecond(1000);
		}

		if (prevKeyPressed != keyPressed)
		{
			int	inKey2 = inKey;

			/* Escape sequence entered, like page up	*/
			while (TRUE)
			{
				inKey = ProcessSerialInput(inKey2);
				if (inKey != 0)
					break;
				inKey2 = getc(fp);
			}
		}
	}
	else
	{
		inKey = ProcessSerialInput((UINT08)inKey);
	}
	updateWaitStatus(-1, -1);

	return(inKey);
}

int UartGetCharEcho(FILE *fp)
{
	int ch = UartGetChar(fp);

	putchar(ch);
	return(ch);
}

/*====================================================================
Function Name   :   UartGetString()
Version         :   0.1
Create Date     :   2002,09,03
Author          :   Jackee
Description     :   Get string from file pointer.
Input           :
Output          :
Remarks         :	Extension of fgets with key code filter.
====================================================================*/
char *UartGetString(char *buf, int n, FILE *fp)
{
	int		key, kpc, i, j;
	int		oldLevel;

	if ((buf == NULL) || (n < 1))
		return NULL;

	/*	Flush all input buffer	*/
	fflush(fp);

	/*	Clear flag for check break */
	oldLevel = intLock();
	gbBrkPressed = FALSE;
	intUnlock(oldLevel);

	/*	Turn off canonical/echo mode to process escape sequence	*/
	setTerminalMode(~ECHO);

	while (TRUE)
	{
		oldLevel = intLock();
		kpc = keyPressed;
		intUnlock(oldLevel);

		key = UartGetChar(fp);

		if (key == PC_KEY_F10)
		{
			if (gnEditMemory)
			{
				rprint1n("");
				strcpy(buf, ".\n");
			}
			else
			{
				gnDebugLevel = 0;
				rprint1n(" Exiting debug menu");
				strcpy(buf, "\n");
			}
			return(buf);
		}
		else if (key >= 0x100)
		{
			/* Process Cursor up/down key when editing memory	*/
			if (gnEditMemory)
			{
				buf[0] = '\0';
				if (key == PC_KEY_UP  ) strcpy(buf, "k\n");
				if (key == PC_KEY_DOWN) strcpy(buf, "j\n");

				if (buf[0] != '\0')
				{
					rprint1n("");
					return(buf);
				}
			}

			/* Process Escape Sequence							*/
			(void)CheckSpecialFunctions(key);
		}
		else if ( (gnEditMemory && ((key == 'j') || (key == 'k'))) || (key == 0x04) )
		{
			sprintf(buf, "%c\n", key);
			rprint1n("");
			return(buf);
		}
		else
		{
			/* Process Normal Command String					*/
			UINT32	tickBeg = (UINT32) readMsTicks();
			UINT32	tickEnd;

			/* Echo entered chararacters in raw mode			*/
			/* (To deal with input by 'PASTE' command			*/
			for (i = 0; (kpc != keyPressed) && (i < (n-1)); )
			{
				if ( (i > 0) && (key == '\b') )
				{
					i--;
					putchar(key);
				}
				else if (key != '\b')
				{
					buf[i++] = key;
					putchar(key);
				}

				tickEnd = (UINT32) readMsTicks();
				if ((tickEnd - tickBeg) > 30)
				{
					break;
				}
				tickBeg = tickEnd;

				kpc++;

				/*	Wait 5 ms to check more key pressed			*/
				for (j = 0; (kpc == keyPressed) && (j < 5); j++)
					delayMicroSecond(1000+j);

				if (kpc != keyPressed)
					key = getc(fp);
			}
			fflush(stdout);

			/*
			 *	While in interrupt mode, key press counter is not updated.
			 *	so, we must add it manually
			 */
			if (intContext())
			{
				buf[i++] = key;
				putchar(key);
			}

			setTerminalMode(ECHO);		/*	Turn on echo mode	*/

			for (; (key != '\n') && (i < (n-1)); )
			{
				key = getc(fp);
				if (key == 0x7f)
				{
					putchar('\b');
					putchar(' ');
					if (i > 0)
					{
						i--;
						putchar('\b');
					}
				}
				else if (key == '\n')
				{
					buf[i++] = key;
					break;
				}
				else
				{
					buf[i++] = key;

					if ( i >= n-1 ) putchar('\n');
				}
				fflush(stdout);
			}
			buf[i] = '\0';

			//setTerminalMode(ICANON);	/* Turn on canonical mode */
			setTerminalMode(~ECHO);
			return(buf);
		}
	}
}

void keyboardTask(void)
{
	SINT32	inKey;

	dbgprint("Keyboard Task Started ...");

	setTerminalMode(~ECHO);
	setTerminalExtISR( UartSetKeyPressed );

	while(1)
	{
		inKey = UartGetChar(stdin);

		if (inKey == 0)
			continue;
		if ((gDebugEnable == 0) && (inKey == 0x0a))
		{
			printf("Debug message has been suppressed\n");
		}
		dbgprint("0x%08x", inKey);
		if (CheckSpecialFunctions(inKey) == FALSE)
		{
			switch(inKey)
			{
				case 't':
				{
					static int toggle = 0;
					toggle = !toggle;
					if(toggle) postSem(lSMidTest01);
					else       waitSem(lSMidTest01, WAIT_FOREVER);
					break;
				}
			}
		}
	}
}
