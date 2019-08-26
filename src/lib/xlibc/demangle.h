#ifndef __DEMANGLE_H__
#define __DEMANGLE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *demangle_symbol (const char* mangled, char *dmgBuf, int size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif/*__DEMANGLE_H__*/
