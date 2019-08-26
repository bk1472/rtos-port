/******************************************************************************

	LGE. DTV RESEARCH LABORATORY
	COPYRIGHT(c) LGE CO.,LTD. 2002. SEOUL, KOREA.
	All rights are reserved.
	No part of this work covered by the copyright hereon may be
	reproduced, stored in a retrieval system, in any form or
	by any means, electronic, mechanical, photocopying, recording
	or otherwise, without the prior permission of LG Electronics.

	FILE NAME   : cmd_io.c
	VERSION     :
	AUTHOR      : Choi Baekwon.
	DATE        : 2006.01.21
	DESCRIPTION :
					debugging 을 하기 위한
					공통 terminal input / oput 함수들의 집합.
*******************************************************************************/
/*------------------------------------------------------------------------------
	#include 파일들
	(File Inclusions)
------------------------------------------------------------------------------*/
#include <common.h>
#include <osa/osa_utils.h>

extern char *UartGetString(char *buf, int n, FILE *fp);

UINT32  ReadKeyInput( char * comment )
{
	char   inputStr[20]= "0x";

	rprint0n("%s: 0x", comment);

	(void)UartGetString(&inputStr[2], 17, stdin);

	return( strtol( inputStr, (char **) NULL, 16 ) );
}

UINT32  ReadHexKeyInput( char * comment )
{
	char   inputStr[20]= "0x";

	rprint0n("%s: 0x", comment);

	(void)UartGetString(&inputStr[2], 17, stdin);

	return( strtoul( inputStr, (char **) NULL, 16 ) );
}

UINT32  ReadDecimalKeyInput( char * comment )
{
	char   inputStr[20];

	rprint0n("%s: ", comment);

	(void)UartGetString(&inputStr[0], 19, stdin);

	return( strtol( inputStr, (char **) NULL, 10 ) );
}

UINT32	ReadCmdString(char *prompt, char *cmdStr, size_t cmdLen)
{
	size_t	inpLen;

	rprint0n("%s", prompt);
	(void)UartGetString(&cmdStr[0], cmdLen-1, stdin);
	inpLen = strlen(cmdStr);
	if ((inpLen > 0) && (cmdStr[inpLen-1] == '\n'))
	{
		inpLen--;
		cmdStr[inpLen] = '\x0';
	}
	return(inpLen);
}

UINT32 ReadNumber(char *prompt, int digits, UINT32 val)
{
	                    /* 0  1  2  3  4  5  6  7  8 */
	static char	dtag[] = { 1, 2, 3, 4, 5, 7, 8, 9, 9};
	char	inputStr[20];
	char	*cp = &inputStr[0];

	rprint0n("%s: 0x%0*x(%*d) ==> ", prompt, digits, val, dtag[digits], val);

	(void)UartGetString(cp, 19, stdin);
	cp = (char *)strtrim(cp);

	if ((*cp != '\r') && (*cp != '\n'))
		val = (UINT32) strtoul(cp, (char **) NULL, 0);

	return(val);
}


UINT32  ReadKeyInputWithSize( char * comment, UINT16 *strSize )
{
	char   inputStr[20]= "0x";

	rprint0n("%s: 0x", comment);

	(void)UartGetString(&inputStr[2], 17, stdin);

	/* 입력된 숫자 스트림의 bytes 크기 */
	*strSize = (strlen( &inputStr[2]) / 2 ) + (strlen(  &inputStr[2]) % 2 );
	if( * strSize == 0 ) * strSize = 1;

	return( strtol( inputStr, (char **) NULL, 16 ) );
}
