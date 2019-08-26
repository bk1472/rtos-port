#ifndef __CMD_IO_H__
#define __CMD_IO_H__

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*------------------------------------------------------------------------------
	Extern 전역변수와 함수 prototype 선언
	(Extern Variables & Function Prototype Declarations)
------------------------------------------------------------------------------*/
extern UINT32 ReadKeyInput( char * comment );
extern UINT32 ReadHexKeyInput( char * comment );
extern UINT32 ReadDecimalKeyInput( char * comment );
extern UINT32 ReadCmdString(char *prompt, char *cmdStr, size_t cmdLen);
extern UINT32 ReadNumber(char *prompt, int digits, UINT32 val);

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif/*__CMD_IO_H__*/
