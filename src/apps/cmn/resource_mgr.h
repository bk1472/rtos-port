#ifndef __RESOURCE_MGR_H__
#define __RESOURCE_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <common.h>
#include <osa/osadap.h>

#ifdef __RESOURCE_CONF_C__

/* Global Variable Declarations in stb_conf.c							*/
#define	SYS_VAR(vType, vName, vValue)	vType vName = (vType) vValue
#define	SYS_EXT

#else

/* External Variable References from other sources						*/
#define	SYS_VAR(vType, vName, vValue)	extern vType vName
#define	SYS_EXT							extern

#endif

/*----------------------------------------------------------------------------+
| Global Semaphore ID variables.
+----------------------------------------------------------------------------*/
SYS_VAR(SID_TYPE, lSMidTest01, 0);
SYS_VAR(SID_TYPE, lSMidTest02, 0);

/*----------------------------------------------------------------------------+
| Global Queue ID variables.
+----------------------------------------------------------------------------*/
SYS_VAR(QID_TYPE, lQMidTest01, 0);
SYS_VAR(QID_TYPE, lQMidTest02, 0);

extern void createTasks(void);
extern void createSemas(void);
extern void createMsgQs(void);

#ifdef __cplusplus
}
#endif

#endif/*__RESOURCE_MGR_H__*/
