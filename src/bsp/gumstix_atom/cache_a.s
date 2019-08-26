#include <board.h>
#include <bsp_arch.h>

	.extern cacheIntMask
	.extern cacheIndexMask
	.extern cacheSegMask


#define CACHELINESIZE		32
#define CACHESIZE			16384

	.globl cacheIdentify
	.type cacheIdentify, %function
cacheIdentify:
	MRC		CP_MMU, 0, r0, c0, c0, 1
	MOV		pc, lr
	.size cacheIdentify, .-cacheIdentify

	.globl cacheDClearDisable
	.type cacheDClearDisable, %function
cacheDClearDisable:
	STMFD SP!, {r1-r12}
	LDR	r1, L$_cacheIndexMask
	LDR r2, L$_cacheIntMask
	LDR r12, L$_cacheSegMask
	LDR	r1, [r1]
	LDR r2, [r2]
	LDR r12, [r12]
	MRS r3, cpsr
	ORR r2, r3, r2
	MSR cpsr, r2

	MOV	r5, #0
	SUB	r5, r5, r1
5:
	MOV r2, r12
6:
	#	Clean and invalidate DCache Entry using index
	ORR r0, r2, r1
	MCR CP_MMU, 0, r0, c7, c10, 1
	NOP
	NOP
	NOP
	NOP

	SUBS r2, r2, #(1<<5)
	BHS 6b
	SUBS r1, r1, r5
	BHS 5b

	#	Empty Write Buffer
	MOV r0, #0
	MCR CP_MMU, 0, r0, c7, c10, 4
	NOP
	NOP
	NOP
	NOP

	#	Invalidate all DCache Entries
	MOV r0, #0
	MCR CP_MMU, 0, r0, c7, c6, 0
	NOP
	NOP
	NOP
	NOP

	#	Disable Cacheable bit in MMU register
	MRC CP_MMU, 0, r2, c1, c0, 0
	BIC r2, r2, #MMUCR_C_ENABLE
	MCR CP_MMU, 0, r2, c1, c0, 0
	NOP
	NOP
	NOP
	NOP

	MSR cpsr, r3

	LDMFD SP!, {r1-r12}

	MOV pc, lr
	.size cacheDClearDisable, .-cacheDClearDisable

	.globl cacheIClearDisable
	.type cacheIClearDisable, %function
cacheIClearDisable:
	STMFD SP!, {r0-r12}
	MRS r3, cpsr
	ORR r2, r3, #I_BIT | F_BIT
	MSR cpsr, r2

	MRC CP_MMU, 0, r2, c1, c0, 0
	BIC r2, r2, #MMUCR_I_ENABLE
	MCR CP_MMU, 0, r2, c1, c0, 0
	NOP
	NOP
	NOP
	NOP

	# Invalidate all ICache entries
	MOV r0, #0
	MCR CP_MMU, 0, r0, c7, c5, 0
	NOP
	NOP
	NOP
	NOP

	MSR cpsr, r3

	LDMFD SP!, {r0-r12}
	MOV pc, lr
	.size cacheIClearDisable, .-cacheIClearDisable

	.globl cacheDFlush
	.type cacheDFlush, %function
cacheDFlush:
	MCR CP_MMU, 0, r0, c7, c10, 1
	MOV pc, lr
	.size cacheDFlush, .-cacheDFlush

	.globl cacheDFlushAll
	.type cacheDFlushAll, %function
cacheDFlushAll:
	STMFD SP!, {r0-r12}
	LDR r1, L$_cacheIndexMask
	LDR r3, L$_cacheSegMask
	LDR r1, [r1]
	LDR r3, [r3]

	MOV	r5, #0
	SUB	r5, r5, r1

5:
	MOV r2, r3
6:
	# Clean Dcache single entry using index
	ORR r0, r2, r1
	MCR CP_MMU, 0, r0, c7, c10, 2
	NOP
	NOP
	NOP
	NOP

	SUBS r2, r2, #(1<<5)
	BHS 6b
	SUBS r1, r1, r5
	BHS 5b
	NOP
	NOP
	NOP
	NOP

	# Empty Write buffer
	MOV r0, #0
	MCR CP_MMU, 0, r0, c7, c10, 4
	NOP
	NOP
	NOP
	NOP

	LDMFD SP!, {r0-r12}

	MOV pc, lr
	.size cacheDFlushAll, .-cacheDFlushAll

	.globl cacheDInvalidate
	.type cacheDInvalidate, %function
cacheDInvalidate:
	MCR CP_MMU, 0, r0, c7, c6, 1
	NOP
	NOP
	NOP
	NOP
	MOV pc, lr
	.size cacheDInvalidate, .-cacheDInvalidate

	.globl cacheDInvalidateAll
	.type cacheDInvalidateAll, %function
cacheDInvalidateAll:
	MOV r0, #0
	MCR CP_MMU, 0, r0, c7, c6, 0
	NOP
	NOP
	NOP
	NOP
	MOV pc, lr
	.size cacheDInvalidateAll, .-cacheDInvalidateAll

	.globl cacheIInvalidate
	.type cacheIInvalidate, %function
cacheIInvalidate:
	MCR CP_MMU, 0, r0, c7, c5, 1
	NOP
	NOP
	NOP
	NOP
	MOV pc, lr
	.size cacheIInvalidate, .-cacheIInvalidate

	.globl cacheIInvalidateAll
	.type cacheIInvalidateAll, %function
cacheIInvalidateAll:
	MOV r0, #0
	MCR CP_MMU, 0, r0, c7, c5, 0
	NOP
	NOP
	NOP
	NOP
	MOV pc, lr
	.size cacheIInvalidateAll, .-cacheIInvalidateAll

L$_cacheIndexMask:
	.long cacheIndexMask
L$_cacheIntMask:
	.long cacheIntMask
L$_cacheSegMask:
	.long cacheSegMask
