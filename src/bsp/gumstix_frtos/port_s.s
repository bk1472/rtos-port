			.text
            .globl  port_SR_Save               /* Functions declared in this file */
            .globl  port_SR_Restore

#define	NO_INT      0xC0                         /* Mask used to disable interrupts (Both FIR and IRQ) */
#define SVC32_MODE	0x13
#define FIQ32_MODE	0x11
#define IRQ32_MODE	0x12


	.type port_SR_Save, %function
port_SR_Save:
        MRS     r0,CPSR                     /* Set IRQ and FIQ bits in CPSR to disable all interrupts */
        ORR     r1,r0,#NO_INT
        MSR     CPSR_c,r1
        MRS     r1,CPSR                     /* Confirm that CPSR contains the proper interrupt disable flags */
        AND     r1,r1,#NO_INT
        CMP     r1,#NO_INT
        BNE     port_SR_Save              /* Not properly disabled (try again)*/
        MOV		PC,      lr                          /* Disabled, return the original CPSR contents in r0 */
	.size port_SR_Save, .-port_SR_Save

	.type port_SR_Restore, %function
port_SR_Restore:
        MSR     CPSR_c,r0
        MOV     PC, lr
	.size port_SR_Restore, .-port_SR_Restore

