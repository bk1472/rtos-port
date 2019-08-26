#include <common.h>
#include <board.h>
#include <bsp_arch.h>

extern void			mmuDisable(void);
extern void			mmuEnable(UINT32);
extern void			mmuTtbrSet(UINT32);
extern UINT32		mmuTtbrGet(void);
extern void			mmuDacrSet(UINT32);
extern void			mmuTLBIDFlushAll(void);
extern UINT32		mmuCP15R1Get(void);

#undef	DEBUG

#define	LVL1_TLB_BASE			(CPU_MEM_BASE+0x4000)	/* Base of Level 1 TLB descriptors			  */
#define LVL2_TLB_BASE_000		(CPU_MEM_BASE+0x8000)	/* Base of Level 2 TLB 0x00000000..0x000fffff */
#define LVL2_TLB_BASE_fff		(CPU_MEM_BASE+0x8400)	/* Base of Level 2 TLB 0xfff00000..0xffffffff */

#define	LVL1_TLB_UNIT_SZ		0x00100000	/* Size of memory unit in level 1 TLB entry	  */
#define	LVL1_TLB_ENTRIES		0x00001000	/* Number of entries in Level 1 TLB			  */

#define LVL2_TLB_UNIT_SZ		0x00001000	/* Size of memory unit in Level 2 TLB entry	  */
#define LVL2_TLB_ENTRIES		0x00000100	/* Number of entries in Level 2 TLB			  */

#if (USE_HIGH_VECTOR > 0)
static int checkNullPtr = 1;
#endif

int MMU_IsMappedAddr(UINT32 addr);

void MMU_Enable(MEM_MAP_T *pMap)
{
	int		i;
	UINT32	mmuCr;
	UINT32	*tlb1 = (UINT32 *)LVL1_TLB_BASE;

	mmuDisable();

	for (i = 0 ; i < LVL1_TLB_ENTRIES ; i++)
	{
		tlb1[i] = (i<<20) | 0x00;
	}

	while (pMap->size != 0)
	{
		unsigned int index, fromIndex, toIndex;
		fromIndex	= (pMap->base)>>20;
		toIndex		= (pMap->base + pMap->size - 1) >> 20;
		if (toIndex < fromIndex)
			toIndex = fromIndex;
		index		= fromIndex;

		#if 0
		printf("Map from %08x - %08x (%-6s)(%04x:%04x)\n",
				fromIndex<<20,
				toIndex<<20,
				(pMap->flag & (DATA_CACHE|INST_CACHE))?"CACHED":"NONE",
				fromIndex,
				toIndex
			   );
		#endif

		while (index <= toIndex)
		{
			/*
			 * Level1 TLB Descriptor
			 *        +------------------------ 31..20  SB : section Base Address           |-> 11b => Read/Write access
			 *        |              +--------- 11..10  AP : Access Protection -------------|-> 00b => No access
			 *        |              |  +------   8..5  DS : Domain Selector                |-> 10b => Read Only
			 *        |              |  |   +--      3  C  : Cache Enable --------------------> 0/1 D-Cache Enable/Disable
			 *        |              |  |   |+-      2  B  : Write Buffer Enable -------------> 0/1 Buffer bit
			 *  [^^^^^^^^^^][ SBZ  ]^^0[^^]1||
			 *  10987654321098765432109876543210
			 *   3         2         1         0
			 */
			tlb1[index] = (index<<20) | 0xc02;  /* LGSI 9,Jan 2006, Changed AP value to enable Read/Write Access for memory region */

			if (pMap->flag & ( DATA_CACHE | INST_CACHE ) )
			{
				tlb1[index] |= 0x0C;
			}
			index++;
		}
		pMap++;
	}

	#if (USE_HIGH_VECTOR > 0)
	/*
	 *	Map All Page in [0x000000:0x1000000] as normal, cacheable
	 *	After Cache Initialized, page 0 will be invisible to user.
	 *	To prevent from accessing address zero by mistake, this
	 *	area must be protected from accessing.
	 */
	if (checkNullPtr)
	{
		UINT32	*tlb2, vSectBeg, pSectBeg, pSectBase;
		int		n;

		#if	0
		vSectBeg  = 0x080;
		pSectBeg  = 0x006;
		#else
		vSectBeg  = 0x000;
		pSectBeg  = 0x000;
		#endif
		tlb2      = (UINT32 *)LVL2_TLB_BASE_000;	/* Use 0x8000..0x83ff */
		pSectBase = (pSectBeg << 20);

		tlb1[vSectBeg] = ((UINT32)tlb2) | 0x11;

		for (n = 0; n < LVL2_TLB_ENTRIES; n++)
		{
			/*
			 * At first, map address 0 as accessble, It will be in visible
			 * just after cacheLibInit(), and excVecInit() called at the
			 * end of this function.
			 */
			tlb2[n] = pSectBase | (n<<12) | 0x0e; /* Map as normal  */
		}
		tlb2[0] = 0;	/* Map first 4K invalid */
	}

	/*
	 *	Map Vectored Interrupt Controller	: 0xffff0000
	 *	Map High Exception Vector			: 0xfffff000
	 */
	if(0)
	{
		UINT32	*tlb2, vSectBeg, pSectBeg, pSectBase;
		int		n;

		vSectBeg  = 0xfff;
		pSectBeg  = 0xfff;
		tlb2      = (UINT32 *)LVL2_TLB_BASE_fff;	/* Use 0x8400..0x87ff */
		pSectBase = (pSectBeg << 20);

		tlb1[vSectBeg] = ((UINT32)tlb2) | 0x11;

		for (n = 0; n < LVL2_TLB_ENTRIES; n++)
		{
			/* Disable all maps	*/
			tlb2[n] = pSectBase | (n<<12) | 0x00;	/* Map as invalid  */
		}

		tlb2[0xf0] = pSectBase | (0xf0<<12) | 0x02;	/* Map Reset Vector */
		tlb2[0xff] = pSectBase | (0xff<<12) | 0x02;	/* Map VIC			*/
	}
	#endif

	mmuTtbrSet((UINT32)tlb1);
	mmuDacrSet(0xffffffff);
	mmuTLBIDFlushAll();

	mmuCr = mmuCP15R1Get();

	printf("MMU CR : %08x (%08x)", mmuCr, mmuTtbrGet());

	/*
	 * CP15 R1 Register
	 * (Refer to Chapter 3. System Control Process on DDI0327A_1022E.pdf)
	 *
	 *      +--------------------------------  31:22 [SB] : Section Base Address
	 *      |     +--------------------------     21 [FI] : Fast Interrupt
	 *      |     |     +--------------------     15 [L4] : LDR uses Arm Architecture 4
	 *      |     |     |+-------------------     14 [RR] : Round Robin Replacement in Cache
	 *      |     |     ||+------------------     13 [V ] : Location of Exception Vector
	 *      |     |     |||+-----------------     12 [I ] : Enable Instruction Cache
	 *      |     |     ||||+----------------     11 [Z ] : Enable Branch Predection
	 *      |     |     ||||| +--------------      9 [R ] : Enable Rom-Protection
	 *      |     |     ||||| |+-------------      8 [S ] : Enable System Protection
	 *      |     |     ||||| ||+------------      7 [B ] : Big-Endian
	 *      |     |     ||||| |||   +--------      3 [W ] : Write Buffer Enable
	 *      |     |     ||||| |||   |+-------      2 [C ] : Data Cache Enable
	 *      |     |     ||||| |||   ||+------      1 [A ] : Enable Address Alignment fault.
	 *      |     |     ||||| |||   |||+-----      0 [M ] : Enable MMU
	 *      |     |     ||||| |||   ||||
	 *  [^^^^^^^^]^[SBZ]^^^^^0^^^111^^^^
	 *  10987654321098765432109876543210
	 *   3         2         1         0
	 */

	mmuCr |= MMUCR_M_ENABLE; /* Enabling MMU */
	mmuCr |= MMUCR_A_ENABLE; /* Enable Alignment and Fault checking */
	mmuCr |= MMUCR_C_ENABLE; /* Cache enable */
	mmuCr |= MMUCR_Z_ENABLE; /* Enable branch prediction */
	mmuCr |= MMUCR_W_ENABLE; /* Write buffer is enabled */
	mmuCr |= MMUCR_I_ENABLE; /* Instruction Cache is enabled */
#if (USE_HIGH_VECTOR > 0)
	mmuCr |= MMUCR_VECTOR;   /* Use High vector table */
#endif

	//mmuEnable(mmuCr);

	__asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
	__asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
	__asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
	__asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
	mmuCr = mmuCP15R1Get();
	printf("-> %08x\n", mmuCr);

	#ifdef	DEBUG
	MMU_IsMappedAddr(CPU_MEM_BASE);
	#endif
	return;
}

void MMU_Disable(void)
{
	mmuDisable();

	return;
}

int MMU_IsMappedAddr(UINT32 addr)
{
	UINT32	*tlb1 = (UINT32 *)LVL1_TLB_BASE, *tlb2;
	UINT32	sect, page;

	sect = addr >> 20;
	if ((tlb1[sect] & 0xc02) == 0xc02)
	{
		/* This address is level1(1M) mapped memory		*/
		#ifdef	DEBUG
		printf("1:: addr = 0x%08x, sect = 0x%03x, tlb1[sect] = 0x%08x\n", addr, sect, tlb1[sect]);
		#endif /* DEBUG */
		return 1;
	}
	else if ((tlb1[sect] & 0x011) != 0x011)
	{
		/* This address is not mapped in Level 1 TLB	*/
		#ifdef	DEBUG
		printf("2:: addr = 0x%08x, sect = 0x%03x, tlb1[sect] = 0x%08x\n", addr, sect, tlb1[sect]);
		#endif /* DEBUG */
		return 0;
	}
	else
	{
		tlb2 = (UINT32 *)(tlb1[sect] & ~0x3ff);
		page = ((addr - (sect << 20))) >> 12;
		#ifdef	DEBUG
		printf("3:: addr = 0x%08x, sect = 0x%03x, tlb1[sect] = 0x%08x, tlb2 = 0x%08x, tlb2[page] = 0x%08x\n",
					addr, sect, tlb1[sect], tlb2, tlb2[page]);
		#endif /* DEBUG */
		if (tlb2[page] & 0x2) return 1;
		else                  return 0;
	}
}
