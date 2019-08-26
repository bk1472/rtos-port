#ifndef __BOARD_HEADER__
#define __BOARD_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#include <pxa255.h>

#define INT_INVALID_ORDER			0xFFFFFFFF

/* Interrupt Check Order List */
#define INT_ORDER_LIST								\
	{												\
		INT_NUM_OS_TIMER1,							\
		INT_NUM_FFUART,								\
		INT_INVALID_ORDER							\
	}

#define CPU_MEM_BASE		(0xA0000000)
#define CPU_MEM_SIZE		(64*1024*1024)
#define SYSTEM_CLOCK		(3686400)

#ifdef __cplusplus
}
#endif

#endif/*__BOARD_HEADER__*/
