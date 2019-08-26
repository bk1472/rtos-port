#include <common.h>
#include <board.h>
#include <bsp_arch.h>

extern UINT32		cacheIdentify(void);
extern void			cacheDClearDisable(void);
extern void			cacheIClearDisable(void);
extern void			cacheDFlushAll(void);
extern void			cacheIInvalidateAll(void);
extern void			cacheIInvalidate(UINT32);
extern void			cacheDFlush(UINT32);
extern UINT32		mmuCrGet(void);
extern void			mmuCrSet(UINT32, UINT32);

int					cacheIntMask   = 0;
int					cacheIndexMask = 0;
int					cacheSegMask   = 0;

void				cacheIEnable(void);
void				cacheDEnable(void);

typedef struct CACHE_INFO
{
	UINT08 ctype;
	UINT08 s;
	UINT08 dsize;
	UINT08 dassoc;
	UINT08 dm;
	UINT08 dlen;
	UINT08 isize;
	UINT08 iassoc;
	UINT08 im;
	UINT08 ilen;
} CACHE_INFO_T;

CACHE_INFO_T cache_info;

void Cache_Init(UINT32 intMask)
{
	UINT32	cacheInfo;
	UINT32	indexMask;
	UINT32	segMask = 0;

	cacheInfo = cacheIdentify();
	printf("cacheID : %08x\n", cacheInfo);
	cache_info.ctype	= (cacheInfo>>25)&0xf;
	cache_info.s		= (cacheInfo>>24)&0x1;
	cache_info.dsize	= (cacheInfo>>18)&0xf;
	cache_info.dassoc	= (cacheInfo>>15)&0x7;
	cache_info.dm		= (cacheInfo>>14)&0x1;
	cache_info.dlen		= (cacheInfo>>12)&0x3;
	cache_info.isize	= (cacheInfo>> 6)&0xf;
	cache_info.iassoc	= (cacheInfo>> 3)&0x7;
	cache_info.im		= (cacheInfo>> 2)&0x1;
	cache_info.ilen		= (cacheInfo    )&0x3;
	switch (cache_info.ctype)
	{
		case 0xe:
			printf("write-back, register 7, format C\n");
			break;
		case 0x6:
			printf("cache-clean-step, cache-invalidstep, lock-down\n");
			break;
		default:
			printf("Uknown ctype 0x%x\n", cache_info.ctype);
			break;
	}
	switch (cache_info.s)
	{
		case 0x1:
			printf("cache separated\n");
			break;
		case 0x0:
			printf("cache not separated\n");
			break;
	}
	switch (cache_info.dsize)
	{
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			printf("D-cache %2dkbytes ", 1<<(cache_info.dsize-1));
			break;
		default:
			printf("D-cache --kbytes ");
			break;
	}
	{
		indexMask = 0xffffffff << (32 - cache_info.dassoc);
		printf("%d-way ", 1<<(cache_info.dassoc));
	}
	switch (cache_info.dm)
	{
		case 1:
			printf("mult ");
	}
	switch (cache_info.dlen)
	{
		case 2:
			printf("8 words (32bytes) ");
			segMask = ((1<<(cache_info.dsize + 9 - cache_info.dassoc - 5))-1) << 5;
			break;
	}
	printf("\n");

	switch (cache_info.isize)
	{
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			printf("I-cache %2dkbytes ", 4<<(cache_info.dsize-3));
			break;
		default:
			printf("I-cache --kbytes ");
			break;
	}
	{
		printf("%d-way ", 1<<(cache_info.iassoc));
	}
	switch (cache_info.im)
	{
		case 1:
			printf("mult ");
	}
	switch (cache_info.ilen)
	{
		case 2:
			printf("8 words (32bytes) ");
			break;
	}
	printf("\n");

	cacheIntMask = intMask;
	cacheIndexMask = indexMask;
	cacheSegMask = segMask;
	printf("Masks:: Index: %08x, Int: %08x, Seg: %08x\n", indexMask, intMask, segMask);

	return;
}

void Cache_Enable(void)
{
	cacheIClearDisable();
	cacheDClearDisable();

	cacheDEnable();
	cacheIEnable();

	return;
}

void Cache_Disable(void)
{
	cacheDClearDisable();
	cacheIClearDisable();

	return;
}

void cacheDEnable(void)
{
	int cacheStatus = MMUCR_C_ENABLE | MMUCR_W_ENABLE;

	if (mmuCrGet() & MMUCR_M_ENABLE)
	{
		mmuCrSet(cacheStatus, cacheStatus);
	}
	return;
}

void cacheIEnable(void)
{
	int cacheStatus = MMUCR_I_ENABLE;

	if (mmuCrGet() & MMUCR_M_ENABLE)
	{
		mmuCrSet(cacheStatus, cacheStatus);
	}
	return;
}

#define CACHE_ALIGN_SIZE	32

void cacheFlush(int mode, int addr, int size)
{
	size += addr & (CACHE_ALIGN_SIZE - 1);

	if (mode & DATA_CACHE)
	{
		if (addr == FLUSH_ALL)
		{
			cacheDFlushAll();
		}
		else
		{
			int temp;

			for ( temp = addr ; temp < (addr+size) ; temp+=CACHE_ALIGN_SIZE)
			{
				cacheDFlush(temp);
			}
		}
	}
	else if (mode & INST_CACHE)
	{
		if (addr == FLUSH_ALL)
		{
			cacheIInvalidateAll();
		}
		else
		{
			int temp;

			for ( temp = addr ; temp < (addr+size) ; temp+=CACHE_ALIGN_SIZE)
			{
				cacheIInvalidate(temp);
			}
		}
	}

	return;
}
