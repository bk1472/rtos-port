/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 1998-2003. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : pc_key.h
    VERSION     : 1.0
    AUTHOR      : 이재길(jackee@lge.com)
    DATE        : 2003.01.27
    DESCRIPTION : PC KEY code definitions with LGTERM

*******************************************************************************/

#ifndef	_PC_KEY_H_
#define	_PC_KEY_H_

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*-----------------------------------------------------------------------------
	제어 상수
	(Control Constants)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
    #include 파일들
    (File Inclusions)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	상수 정의
	(Constant Definitions)
------------------------------------------------------------------------------*/

/*	PC Keyboard Escape Sequence Map for LGTERM keymap LGDTV.CNF				  */
#define		PC_KEY_INS		0x005B317E		/*	0	*/
#define		PC_KEY_DEL		0x005B347E		/*	.	*/
#define		PC_KEY_HOME		0x005B327E		/*	7	*/
#define		PC_KEY_END		0x005B357E		/*	1	*/
#define		PC_KEY_PGUP		0x005B337E		/*	9	*/
#define		PC_KEY_PGDN		0x005B367E		/*	3	*/

#define		PC_KEY_UP		0x00005B41		/*	8	*/
#define		PC_KEY_DOWN		0x00005B42		/*	2	*/
#define		PC_KEY_RIGHT	0x00005B43		/*	6	*/
#define		PC_KEY_LEFT		0x00005B44		/*	4	*/

#define		PC_KEY_NOUSE	0x00000000		/*	5	*/

#define		PC_KEY_F01		0x5B31317E		/* F1	*/
#define		PC_KEY_F02		0x5B31327E		/* F2	*/
#define		PC_KEY_F03		0x5B31337E		/* F3	*/
#define		PC_KEY_F04		0x5B31347E		/* F4	*/
#define		PC_KEY_F05		0x5B31357E		/* F5	*/
#define		PC_KEY_F06		0x5B31377E		/* F6	*/
#define		PC_KEY_F07		0x5B31387E		/* F7	*/
#define		PC_KEY_F08		0x5B31397E		/* F8	*/
#define		PC_KEY_F09		0x5B32307E		/* F9	*/
#define		PC_KEY_F10		0x5B32317E		/* F10	*/
#define		PC_KEY_F11		0x5B32337E		/* F11	*/
#define		PC_KEY_F12		0x5B32347E		/* F12	*/
#define		PC_KEY_EXT		0x5B39397E		/* Extension */

#define		PC_KEY_ESC		0x0000001B		/* ESC	*/
#define		PC_KEY_NAC		0xFFFFFFFF		/* Not A Char*/

/*-----------------------------------------------------------------------------
	매크로 함수 정의
	(Macro Definitions)
------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
    형 정의
    (Type Definitions)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Extern 전역변수와 함수 prototype 선언
	(Extern Variables & Function Prototype Declarations)
------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PC_KEY_H_ */
