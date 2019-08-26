/******************************************************************************

    LGE. DTV RESEARCH LABORATORY
    COPYRIGHT(c) LGE CO.,LTD. 2002. SEOUL, KOREA.
    All rights are reserved.
    No part of this work covered by the copyright hereon may be
    reproduced, stored in a retrieval system, in any form or
    by any means, electronic, mechanical, photocopying, recording
    or otherwise, without the prior permission of LG Electronics.

    FILE NAME   : osa_utils.h
    VERSION     : 1.0
    AUTHOR      : 이재길(jackee@lge.com)
    DATE        : 2003.01.08
    DESCRIPTION : O/S Adaptation Utilities.

*******************************************************************************/

#ifndef	_OSA_UTILS_H_
#define	_OSA_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif /* _cplusplus */

/*-----------------------------------------------------------------------------
    #include 파일들
    (File Inclusions)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	상수 정의
	(Constant Definitions)
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	매크로 함수 정의
	(Macro Definitions)
------------------------------------------------------------------------------*/

#define	LS_ISEMPTY(listp)													\
		(((lst_t *)(listp))->ls_next == (lst_t *)(listp))

#define	LS_INIT(listp) {													\
		((lst_t *)(&(listp)[0]))->ls_next = 								\
		((lst_t *)(&(listp)[0]))->ls_prev = ((lst_t *)(&(listp)[0]));		\
}

#define	LS_INS_BEFORE(oldp, newp) {											\
		((lst_t *)(newp))->ls_prev = ((lst_t *)(oldp))->ls_prev;			\
		((lst_t *)(newp))->ls_next = ((lst_t *)(oldp));						\
		((lst_t *)(newp))->ls_prev->ls_next = ((lst_t *)(newp));			\
		((lst_t *)(newp))->ls_next->ls_prev = ((lst_t *)(newp));			\
}

#define	LS_INS_AFTER(oldp, newp) {											\
		((lst_t *)(newp))->ls_next = ((lst_t *)(oldp))->ls_next;			\
		((lst_t *)(newp))->ls_prev = ((lst_t *)(oldp));						\
		((lst_t *)(newp))->ls_next->ls_prev = ((lst_t *)(newp));			\
		((lst_t *)(newp))->ls_prev->ls_next = ((lst_t *)(newp));			\
}

#define	LS_REMQUE(remp) {													\
	if (!LS_ISEMPTY(remp)) {												\
		((lst_t *)(remp))->ls_prev->ls_next = ((lst_t *)(remp))->ls_next;	\
		((lst_t *)(remp))->ls_next->ls_prev = ((lst_t *)(remp))->ls_prev;	\
		LS_INIT(remp);														\
	}																		\
}

#define LS_BASE(type, basep) (type *) &(basep)[0]

#ifdef	__COMMENT_OUT
/*---	Example of doubly linked list	---*/

		typedef struct LINKED_LIST_tag {
			struct LINKED_LIST_tag	*next;
			struct LINKED_LIST_tag	*prev;

			int	data;
		} LINKED_LIST;

		LINKED_LIST	*list[2];		//	declaration of header
		LS_INIT(list);			//	initialize

		LINKED_LIST *list_base = LS_BASE(LINKED_LIST, list);
		//LINKED_LIST *list_base = (LINKED_LIST *) &list[0]; //	type cast - (*)

		LINKED_LIST *list_data = list_base->next;

		while (list_data != list_base) {
			//	traverse
			list_data = list_data->next;
		}

	(*)		*list[0] --> *next, *list[1] --> *prev
			i.e. Header has only links without data.
#endif	/* __COMMENT_OUT */

/*----------------------------------------------------------------------------
    형 정의
    (Type Definitions)
------------------------------------------------------------------------------*/
//----------------------------------------------------------------------------
//	Macro for initializing doubly linked list header
//----------------------------------------------------------------------------
typedef	struct ls_elt {
	struct ls_elt	*ls_next;
	struct ls_elt	*ls_prev;
} lst_t;


/*-----------------------------------------------------------------------------
	Extern 전역변수와 함수 prototype 선언
	(Extern Variables & Function Prototype Declarations)
------------------------------------------------------------------------------*/

#if 0
char	*strdup(const char *);				 /* STRing DUPlicate			  */
#endif
char	*strdup2(const char *, size_t *sp);	 /* Extended style of strdup()	  */
char	*octsdup(const char *, size_t len);  /* OCTet string DUPlicate		  */
char	*strlwr(const char *);				 /* Convert string to lower case  */
char	*strupr(const char *);				 /* Convert string to upper case  */
#if 0
int		strcasecmp(char *, char *);			 /* Case insensitive strcmp()	  */
int		strncasecmp(char *, char *, size_t); /* Case insensitive strncmp()	  */
#endif
ULONG	strtoul2(char *, char **, int);		 /* Extended style of strtoul()	  */
char	*strtok2(char *, char *, char **);	 /* Extended style of strtok()	  */
char	*strtrim(char *);					 /* TRIM heading spaces			  */
int		str2argv(char *, int, char **);		 /* Make string to argv list	  */
int		str2indexInOpts(char **, char*, int *);	/* Find index in option list  */
ULONG	calccrc32(unsigned char *buf, int len);


#ifdef __cplusplus
}
#endif

#endif	/* _OSA_UTILS_H_ */
