#ifndef __BSP_ARCH_H__
#define __BSP_ARCH_H__

#ifndef __ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif
#endif/*__ASSEMBLY__*/

/*
 * PSR Register
 *
 *
 * +----------------------------------------- 31 [N] : regard as two's complement signed integer, 1= negative, 0= positive or zero
 * |+---------------------------------------- 30 [Z] : Is set to 1 if intruction result is zero , and to 0 otherwise
 * ||+--------------------------------------- 29 [C] : (1) add (2) sub (3) shift etc...
 * |||+-------------------------------------- 28 [V] : add / sub is signed overflow occurred
 * ||||                    +-----------------  7 [I] : Disable IRQ interrupts when it is set.
 * ||||                    |+----------------  6 [F] : Disable FIQ interrupts when it is set.
 * ||||                    ||+---------------  5 [T] : 0 : Arm execution 1: Thum execution
 * ||||                    |||  +------------  mode  : 0x10(0b10000)  User mode  =>  pc, r14 to r0, cpsr
 * ||||                    |||  |                      0x11(0b10001)  FIQ mode   =>  pc, r14_fiq to r8_fiq, r7 to r0, cpsr, spsr_fiq
 * ||||                    |||  |                      0x12(0b10010)  IRQ mode   =>  pc, r14_irq, r13_irq, r12 to r0, cpsr, spsr_irq
 * ||||                    |||  |                      0x13(0b10011)  Supervisor =>  pc, r14_svc, r13_svc, r12 to r0, cpsr, spsr_svc
 * ||||                    |||  |                      0x17(0b10111)  Abort mode =>  pc, r14_abt, r13_abt, r12 to r0, cpsr, spsr_abt
 * ||||                    |||[ ~ ]                    0x1B(0b11011)  Undefined  =>  pc, r14_und, r13_und, r12 to r0, cpsr, spsr_und
 * 10987654321098765432109876543210                    0x1F(0b11111)  system     =>  pc, r14 to r0, cpsr (ARM architecture v4 and above)
 *  3         2         1         0
 */

/* bits in the PSR */

#define V_BIT						(1<<28)
#define C_BIT						(1<<29)
#define Z_BIT						(1<<30)
#define N_BIT						(1<<31)
#define I_BIT						(1<<7)
#define F_BIT						(1<<6)
#define T_BIT						(1<<5)

/* mode bits */
#define MODE_SYSTEM32				0x1F
#define MODE_UNDEF32				0x1B
#define MODE_ABORT32				0x17
#define MODE_SVC32					0x13
#define MODE_IRQ32					0x12
#define MODE_FIQ32					0x11
#define MODE_USER32					0x10

/* masks for gettint bits from PSR */
#define MASK_MODE					0x0000003F
#define MASK32MODE					0x0000001F
#define MASK_SUBMODE				0x0000000F
#define MASK_INTMOE					0x000000C0
#define MASK_CC						0xF0000000

#define INT_MASK_SHIFT				6

/* ARM CPU co-processor number */
#define CP_MMU						15

#define MMUCR_M_ENABLE				0x0000001
#define MMUCR_A_ENABLE				0x0000002
#define MMUCR_C_ENABLE				0x0000004
#define MMUCR_W_ENABLE				0x0000008
#define MMUCR_BIGEND				0x0000080
#define MMUCR_SYSTEM				0x0000100
#define MMUCR_ROM					0x0000200
#define MMUCR_Z_ENABLE				0x0000800
#define MMUCR_I_ENABLE				0x0001000
#define MMUCR_VECTOR				0x0002000
#define MMUCR_RR_ENABLE				0x0004000
#define MMUCR_L4_ENABLE				0x0008000
#define MMUCR_FI_ENABLE				0x0200000

#define INST_CACHE					0x1
#define DATA_CACHE					0x2

#define INST_MMU					0x1
#define DATA_MMU					0x2

#define FLUSH_ALL					0xFFFFFFFF
#define INVALIDATE_ALL				0xFFFFFFFF

/* ARM CPU General purpose register count */
#define GREG_NUM					15

#ifndef __ASSEMBLY__
typedef struct
{
	char *			name;		/* Name of address space		*/
	unsigned int	base;		/* Base adddress of area		*/
	unsigned int	size;		/* Size of area					*/
	unsigned int	flag;		/* Optional flag for control	*/
} MEM_MAP_T;

/*---------------------------------------------------------------------*/
/* Some macro function to access byte, word, long words                */
/*---------------------------------------------------------------------*/
#define    outb(x,y)     (*((volatile unsigned char  *)(x))  = (y))
#define    outw(x,y)     (*((volatile unsigned short *)(x))  = (y))
#define    outl(x,y)     (*((volatile unsigned long  *)(x))  = (y))
#define    outl_OR(x,y)  (*((volatile unsigned long  *)(x)) |= (y))
#define    outl_AND(x,y) (*((volatile unsigned long  *)(x)) &= (y))

#define    inb(x)        (*((volatile unsigned char  *)(x)))
#define    inw(x)        (*((volatile unsigned short *)(x)))
#define    inl(x)        (*((volatile unsigned long  *)(x)))

/*---------------------------------------------------------------------*/
/* Some macro function to read some registers                          */
/*---------------------------------------------------------------------*/
#ifdef __MIPS__

#ifdef	__GNUC__
#define get_lr(_x) 	{ __asm__ ("SW $31, 0(%0)" : /* no output */ :"r" (_x)); }
#define get_ra(_x) 	{ __asm__ ("SW $31, 0(%0)" : /* no output */ :"r" (_x)); }
#define get_sp(_x) 	{ __asm__ ("SW $29, 0(%0)" : /* no output */ :"r" (_x)); }

#define str_lr(_x) 	{ __asm__ ("SW $31, 0(%0)" : /* no output */ :"r" (_x)); }
#define str_ra(_x) 	{ __asm__ ("SW $31, 0(%0)" : /* no output */ :"r" (_x)); }
#define str_sp(_x) 	{ __asm__ ("SW $29, 0(%0)" : /* no output */ :"r" (_x)); }

extern UINT32* get_pc(void);

#endif

#endif/*__MIPS__*/

#ifdef __ARM__

#ifdef	__DIAB__
#define get_lr(_x)  __asm{	MOV _x,  LR }
#define get_ra(_x)  __asm{	MOV _x,  LR }
#define get_sp(_x)  __asm{	MOV _x,  SP }
#define get_pc(_x)  __asm{	MOV _x,  PC }

#define str_lr(_x)  __asm{	STR	LR, [_x]}
#define str_ra(_x)  __asm{	STR	LR, [_x]}
#define str_sp(_x)  __asm{	STR	SP, [_x]}
#define str_pc(_x)  __asm{	STR	PC, [_x]}
#endif

#ifdef	__GNUC__
#define get_lr(_x) 	{ __asm__ ("MOV %0, LR"   : /* no output */ :"r" (_x)); }
#define get_ra(_x) 	{ __asm__ ("MOV %0, LR"   : /* no output */ :"r" (_x)); }
#define get_sp(_x) 	{ __asm__ ("MOV %0, SP"   : /* no output */ :"r" (_x)); }
#define get_pc(_x) 	{ __asm__ ("MOV %0, PC"   : /* no output */ :"r" (_x)); }

#define str_lr(_x) 	{ __asm__ ("STR LR, [%0]" : /* no output */ :"r" (_x)); }
#define str_ra(_x) 	{ __asm__ ("STR LR, [%0]" : /* no output */ :"r" (_x)); }
#define str_sp(_x) 	{ __asm__ ("STR SP, [%0]" : /* no output */ :"r" (_x)); }
#define str_pc(_x) 	{ __asm__ ("STR PC, [%0]" : /* no output */ :"r" (_x)); }
#endif

#define REG_PC		15
#define REG_LR		14
#define REG_SP		13

#endif/*__ARM__*/

#endif/*__ASSEMBLY__*/




#ifndef __ASSEMBLY__
#ifdef __cplusplus
}
#endif
#endif/*__ASSEMBLY__*/

#endif/*__BSP_ARCH_H__*/
