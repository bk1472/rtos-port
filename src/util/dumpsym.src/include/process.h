#ifndef __PROCESS_H__
#define __PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include	<stdio.h>
#include	<string.h>
#include	<ar.h>
#include	"symtypes.h"

extern ulong_t		process			(FILE *ifp, FILE *ofp);

#ifdef __cplusplus
}
#endif

#endif/*__PROCESS_H__*/
