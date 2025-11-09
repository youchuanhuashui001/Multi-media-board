#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "font_manage.h"


static int g_iFdHZK;
static unsigned char *g_pucHZKMem;
static unsigned char *g_pucHZKMemEnd;

static int GBKFontInit(char *pcFontFile, unsigned int dwFontSize)
{
	struct stat tStat;

	if (16 != dwFontSize)
	{
		printf("GBK can't support %d fontsize\n", dwFontSize);
		return -1;
	}

	g_iFdHZK = open(pcFontFile, O_RDONLY);
	if (g_iFdHZK < 0)
	{
		printf("can't open %s\n", pcFontFile);
		return -1;
	}
	if(fstat(g_iFdHZK, &tStat))
	{
		printf("can't get fstat\n");
		return -1;
	}
	g_pucHZKMem = (unsigned char *)mmap(NULL , tStat.st_size, PROT_READ, MAP_SHARED, g_iFdHZK, 0);
	if (g_pucHZKMem == (unsigned char *)-1)
	{
		printf("can't mmap for hzk16\n");
		return -1;
	}
	g_pucHZKMemEnd = g_pucHZKMem + tStat.st_size;
	return 0;
}

static int GBKGetFontBitmap(unsigned int dwCode, PT_FontBitMap ptFontBitMap)
{
	int iArea;
	int iWhere;

	int iPenX = ptFontBitMap->iCurOriginX;
	int iPenY = ptFontBitMap->iCurOriginY;

	printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	if (dwCode & 0xffff0000)
	{
		printf("don't support this code : 0x%x\n", dwCode);
		return -1;
	}

	iArea  = (int)(dwCode & 0xff) - 0xA1;
	iWhere = (int)((dwCode >> 8) & 0xff) - 0xA1;

	if ((iArea < 0) || (iWhere < 0))
	{
		printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	ptFontBitMap->iXLeft    = iPenX;
	ptFontBitMap->iYTop     = iPenY - 16;
	ptFontBitMap->iXMax     = iPenX + 16;
	ptFontBitMap->iYMax     = iPenY;
	ptFontBitMap->iBpp      = 1;
	ptFontBitMap->iPitch    = 2;
	ptFontBitMap->pucBuffer = g_pucHZKMem + (iArea * 94 + iWhere)*32;;

	if (ptFontBitMap->pucBuffer >= g_pucHZKMemEnd)
	{
		return -1;
	}

	ptFontBitMap->iNextOriginX = iPenX + 16;
	ptFontBitMap->iNextOriginY = iPenY;

	printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

int GBKInit(void)
{
	printf("gbk init.\n");
	return 0;
}

int gbk_register(void)
{
	PT_FontOpr gbk_opr = NULL;

	gbk_opr = (PT_FontOpr)malloc(sizeof(T_FontOpr));
	if (gbk_opr == NULL) {
		printf("Bugs, malloc gbk_opr failed\n");
		return -1;
	}

	gbk_opr->name = "gbk";
	gbk_opr->Init = GBKInit;
	gbk_opr->FontInit = GBKFontInit;
	gbk_opr->GetFontBitmap = GBKGetFontBitmap;

	RegisterFontOpr(gbk_opr);

	return 0;
}

