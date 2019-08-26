#include <board.h>
#include <bsp_arch.h>

	.globl mmuCrGet
	.type mmuCrGet, %function
mmuCrGet:
	MRC CP_MMU, 0, r0, c1, c0, 0
	MOV pc, lr
	.size mmuCrGet, .-mmuCrGet

	.globl mmuCrSet
	.type mmuCrSet, %function
mmuCrSet:
	MRS r3, cpsr
	ORR r2, r3, #I_BIT | F_BIT
	MSR cpsr, r2

	MRC CP_MMU, 0, r2, c1, c0, 0

	BIC r2, r2, r1
	AND r0, r0, r1
	ORR r2, r2, r0

	MCR CP_MMU, 0, r2, c1, c0, 0

	MSR cpsr, r3

	MOV pc, lr
	.size mmuCrSet, .-mmuCrSet

	.globl mmuEnable
	.type mmuEnable, %function
mmuEnable:
	MRC CP_MMU, 0, r1, c1, c0, 0
	ORR	r1, r1, #(MMUCR_M_ENABLE)
	ORR r1, r1, r0
	MCR CP_MMU, 0, r1, c1, c0, 0
	NOP
	NOP
	MOV pc, lr
	.size mmuEnable, .-mmuEnable

	.globl mmuDisable
	.type mmuDisable, %function
mmuDisable:
	MRC CP_MMU, 0, r0, c1, c0, 0
	BIC	r0, r0, #(MMUCR_M_ENABLE | MMUCR_C_ENABLE | MMUCR_W_ENABLE)
	MCR CP_MMU, 0, r0, c1, c0, 0
	NOP
	NOP
	MOV pc, lr
	.size mmuDisable, .-mmuDisable

	.globl mmuTtbrSet
	.type mmuTtbrSet, %function
mmuTtbrSet:
	MCR CP_MMU, 0, r0, c2, c0, 0
	NOP
	NOP
	MOV pc, lr
	.size mmuTtbrSet, .-mmuTtbrSet

	.globl mmuTtbrGet
	.type mmuTtbrGet, %function
mmuTtbrGet:
	MRC CP_MMU, 0, r0, c2, c0, 0
	NOP
	NOP
	MOV pc, lr
	.size mmuTtbrGet, .-mmuTtbrGet

	.globl mmuDacrSet
	.type mmuDacrSet, %function
mmuDacrSet:
	MCR CP_MMU, 0, r0, c3, c0, 0
	NOP
	NOP
	MOV pc, lr
	.size mmuDacrSet, .-mmuDacrSet

	.globl mmuDacrGet
	.type mmuDacrGet, %function
mmuDacrGet:
	MRC CP_MMU, 0, r0, c3, c0, 0
	NOP
	NOP
	MOV pc, lr
	.size mmuDacrGet, .-mmuDacrGet

	.globl mmuTLBIDFlushAll
	.type mmuTLBIDFlushAll, %function
mmuTLBIDFlushAll:
	MOV r0, #0
	MCR CP_MMU, 0, r0, c8, c7, 0
	NOP
	NOP
	MOV pc, lr
	.size mmuTLBIDFlushAll, .-mmuTLBIDFlushAll

	.globl mmuCP15R1Get
	.type mmuCP15R1Get, %function
mmuCP15R1Get:
	MRC CP_MMU, 0, r0, c1, c0, 0
	MOV pc, lr
	.size mmuCP15R1Get, .-mmuCP15R1Get

	.globl mmuCP15R5GetDFS
	.type mmuCP15R5GetDFS, %function
mmuCP15R5GetDFS:
	MRC CP_MMU, 0, r0, c5, c0, 0
	MOV pc, lr
	.size mmuCP15R5GetDFS, .-mmuCP15R5GetDFS

	.globl mmuCP15R5GetIFS
	.type mmuCP15R5GetIFS. %function
mmuCP15R5GetIFS:
	MRC CP_MMU, 0, r0, c5, c0, 1
	MOV pc, lr
	.size mmuCP15R5GetIFS, .-mmuCP15R5GetIFS

	.globl mmuCP15R6GetDFA
	.type mmuCP15R6GetDFA, %function
mmuCP15R6GetDFA:
	MRC CP_MMU, 0, r0, c6, c0, 0
	MOV pc, lr
	.size mmuCP15R6GetDFA, .-mmuCP15R6GetDFA
