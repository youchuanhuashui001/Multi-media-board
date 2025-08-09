#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "display_manage.h"
#include "encoding_manage.h"
#include "font_manage.h"


#define COLOR_BACKGROUND   0xE7DBB5  /* ·º»ÆµÄÖ½ */
#define COLOR_FOREGROUND   0x514438  /* ºÖÉ«×ÖÌå */


int draw_point(unsigned int color)
{
	PT_Display_Opr fb;
	int i, j;
	int ret;

	fb = Get_Display_Opt("fb");
	if (!fb) {
		printf("can't get display opr\n");
		return -1;
	}

	fb->Clean_Screen(0);

	for (i = 0; i < 500; i++) {
		for (j = 0; j < 100; j++) {
			ret = fb->Show_Pixel(i, j, color);
			if (ret) {
				printf("draw failed.\n");
				return ret;
			}
		}
	}
}

static int g_iFdTextFile;
static unsigned char *g_pucTextFileMem;
static unsigned char *g_pucTextFileMemEnd;
static PT_EncodingOpr g_ptEncodingOprForFile;

static PT_Display_Opr g_ptDispOpr;

unsigned char *g_pucLcdFirstPosAtFile;
//static unsigned char *g_pucLcdFirstPosAtFile;
static unsigned char *g_pucLcdNextPosAtFile;

static int g_dwFontSize;

//static PT_PageDesc g_ptPages   = NULL;
//static PT_PageDesc g_ptCurPage = NULL;


int OpenTextFile(char *pcFileName)
{
	struct stat tStat;

	g_iFdTextFile = open(pcFileName, O_RDONLY);
	if (0 > g_iFdTextFile)
	{
		printf("can't open text file %s\n", pcFileName);
		return -1;
	}

	if(fstat(g_iFdTextFile, &tStat))
	{
		printf("can't get fstat\n");
		return -1;
	}
	g_pucTextFileMem = (unsigned char *)mmap(NULL , tStat.st_size, PROT_READ, MAP_SHARED, g_iFdTextFile, 0);
	if (g_pucTextFileMem == (unsigned char *)-1)
	{
		printf("can't mmap for text file\n");
		return -1;
	}

	g_pucTextFileMemEnd = g_pucTextFileMem + tStat.st_size;

	g_ptEncodingOprForFile = SelectEncodingOprForFile(g_pucTextFileMem);

	if (g_ptEncodingOprForFile)
	{
		g_pucLcdFirstPosAtFile = g_pucTextFileMem + g_ptEncodingOprForFile->iHeadLen;
		return 0;
	}
	else
	{
		return -1;
	}

}

int SetTextDetail(char *pcHZKFile, char *pcFileFreetype, unsigned int dwFontSize)
{
	int iError = 0;
	PT_FontOpr ptFontOpr;
	PT_FontOpr ptTmp;
	int iRet = -1;

	g_dwFontSize = dwFontSize;


	ptFontOpr = g_ptEncodingOprForFile->ptFontOprSupportedHead;
	while (ptFontOpr)
	{
		if (strcmp(ptFontOpr->name, "ascii") == 0)
		{
			iError = ptFontOpr->FontInit(NULL, dwFontSize);
		}
		else if (strcmp(ptFontOpr->name, "gbk") == 0)
		{
			iError = ptFontOpr->FontInit(pcHZKFile, dwFontSize);
		}
		else
		{
			iError = ptFontOpr->FontInit(pcFileFreetype, dwFontSize);
		}

		printf("%s, %d\n", ptFontOpr->name, iError);

		ptTmp = ptFontOpr->ptNext;

		if (iError == 0)
		{
			/* ±ÈÈç¶ÔÓÚascii±àÂëµÄÎÄ¼þ, ¿ÉÄÜÓÃascii×ÖÌåÒ²¿ÉÄÜÓÃgbk×ÖÌå,
			 * ËùÒÔÖ»ÒªÓÐÒ»¸öFontInit³É¹¦, SetTextDetail×îÖÕ¾Í·µ»Ø³É¹¦
			 */
			iRet = 0;
		}
		else
		{
			DelFontOprFrmEncoding(g_ptEncodingOprForFile, ptFontOpr);
		}
		ptFontOpr = ptTmp;
	}
	return iRet;
}

int SelectAndInitDisplay(char *pcName)
{
	int ret;
	int i, j;
	unsigned int color = 0x00ff00ff;

	g_ptDispOpr = Get_Display_Opt(pcName);
	if (!g_ptDispOpr)
	{
		return -1;
	}

	g_ptDispOpr->Clean_Screen(0);

//	for (i = 0; i < 500; i++) {
//		for (j = 0; j < 100; j++) {
//			ret = g_ptDispOpr->Show_Pixel(i, j, color);
//			if (ret) {
//				printf("draw failed.\n");
//				return ret;
//			}
//		}
//	}

	return ret;
}

int IncLcdX(int iX)
{
	if (iX + 1 < g_ptDispOpr->Xres)
		return (iX + 1);
	else
		return 0;
}

int IncLcdY(int iY)
{
	if (iY + g_dwFontSize < g_ptDispOpr->Yres)
		return (iY + g_dwFontSize);
	else
		return 0;
}

int ShowOneFont(PT_FontBitMap ptFontBitMap)
{
	int x;
	int y;
	unsigned char ucByte = 0;
	int i = 0;
	int bit;

	if (ptFontBitMap->iBpp == 1)
	{
		for (y = ptFontBitMap->iYTop; y < ptFontBitMap->iYMax; y++)
		{
			i = (y - ptFontBitMap->iYTop) * ptFontBitMap->iPitch;
			for (x = ptFontBitMap->iXLeft, bit = 7; x < ptFontBitMap->iXMax; x++)
			{
				if (bit == 7)
				{
					ucByte = ptFontBitMap->pucBuffer[i++];
				}

				if (ucByte & (1<<bit))
				{
					g_ptDispOpr->Show_Pixel(x, y, COLOR_FOREGROUND);
				}
				else
				{
					/* Ê¹ÓÃ±³¾°É«, ²»ÓÃÃè»­ */
					// g_ptDispOpr->ShowPixel(x, y, 0); /* ºÚ */
				}
				bit--;
				if (bit == -1)
				{
					bit = 7;
				}
			}
		}
	}
	else if (ptFontBitMap->iBpp == 8)
	{
		for (y = ptFontBitMap->iYTop; y < ptFontBitMap->iYMax; y++)
			for (x = ptFontBitMap->iXLeft; x < ptFontBitMap->iXMax; x++)
			{
				//g_ptDispOpr->ShowPixel(x, y, ptFontBitMap->pucBuffer[i++]);
				if (ptFontBitMap->pucBuffer[i++])
					g_ptDispOpr->Show_Pixel(x, y, COLOR_FOREGROUND);
			}
	}
	else
	{
		printf("ShowOneFont error, can't support %d bpp\n", ptFontBitMap->iBpp);
		return -1;
	}
	return 0;
}

int ShowOnePage(unsigned char *pucTextFileMemCurPos)
{
	int iLen;
	int iError;
	unsigned char *pucBufStart;
	unsigned int dwCode;
	PT_FontOpr ptFontOpr;
	T_FontBitMap tFontBitMap;
	
	int bHasNotClrSceen = 1;
	int bHasGetCode = 0;

	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = g_dwFontSize;
	pucBufStart = pucTextFileMemCurPos;

	
	while (1)
	{
		iLen = g_ptEncodingOprForFile->GetCodeFrmBuf(pucBufStart, g_pucTextFileMemEnd, &dwCode);
		if (0 == iLen)
		{
			/* ÎÄ¼þ½áÊø */
			if (!bHasGetCode)
			{
				return -1;
			}
			else
			{
				return 0;
			}
		}

		bHasGetCode = 1;
		
		pucBufStart += iLen;

		/* ÓÐÐ©ÎÄ±¾, \n\rÁ½¸öÒ»Æð²Å±íÊ¾»Ø³µ»»ÐÐ
		 * Åöµ½ÕâÖÖÁ¬ÐøµÄ\n\r, Ö»´¦ÀíÒ»´Î
		 */
		if (dwCode == '\n')
		{
			g_pucLcdNextPosAtFile = pucBufStart;
			
			/* »Ø³µ»»ÐÐ */
			tFontBitMap.iCurOriginX = 0;
			tFontBitMap.iCurOriginY = IncLcdY(tFontBitMap.iCurOriginY);
			if (0 == tFontBitMap.iCurOriginY)
			{
				/* ÏÔÊ¾Íêµ±Ç°Ò»ÆÁÁË */
				return 0;
			}
			else
			{
				continue;
			}
		}
		else if (dwCode == '\r')
		{
			continue;
		}
		else if (dwCode == '\t')
		{
			/* TAB¼üÓÃÒ»¸ö¿Õ¸ñ´úÌæ */
			dwCode = ' ';
		}

		printf("dwCode = 0x%x\n", dwCode);

		ptFontOpr = g_ptEncodingOprForFile->ptFontOprSupportedHead;
		while (ptFontOpr)
		{
			printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
			iError = ptFontOpr->GetFontBitmap(dwCode, &tFontBitMap);
			printf("%s %s %d, ptFontOpr->name = %s, %d\n", __FILE__, __FUNCTION__, __LINE__, ptFontOpr->name, iError);
			if (0 == iError)
			{
				printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
//				if (RelocateFontPos(&tFontBitMap))
//				{
//					/* Ê£ÏÂµÄLCD¿Õ¼ä²»ÄÜÂú×ãÏÔÊ¾Õâ¸ö×Ö·û */
//					return 0;
//				}
				printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

				if (bHasNotClrSceen)
				{
					/* Ê×ÏÈÇåÆÁ */
					g_ptDispOpr->Clean_Screen(COLOR_BACKGROUND);
					bHasNotClrSceen = 0;
				}
				printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
				/* ÏÔÊ¾Ò»¸ö×Ö·û */
				if (ShowOneFont(&tFontBitMap))
				{
					return -1;
				}

				tFontBitMap.iCurOriginX = tFontBitMap.iNextOriginX;
				tFontBitMap.iCurOriginY = tFontBitMap.iNextOriginY;
				g_pucLcdNextPosAtFile = pucBufStart;

				/* ¼ÌÐøÈ¡³öÏÂÒ»¸ö±àÂëÀ´ÏÔÊ¾ */
				break;
			}
			ptFontOpr = ptFontOpr->ptNext;
		}
	}

	return 0;
}

int test_show_one_font(void)
{
	T_FontBitMap tFontBitMap;
	unsigned char *pucBufStart;
	unsigned int code; // 字体对应的编码
	unsigned int len;



	int x;
	int y;
	unsigned char ucByte = 0;
	int bit;
	int i = 0;
	int ret;

	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = g_dwFontSize;

	pucBufStart = g_pucLcdFirstPosAtFile;

	int test = 0;

	for (test = 0; test < 1; i++) {
		len = g_ptEncodingOprForFile->GetCodeFrmBuf(pucBufStart, g_pucTextFileMemEnd, &code);
		printf("code:0x%x\n", code);
		pucBufStart += len;

		if (code == '\n') {
			tFontBitMap.iCurOriginX = 0;
			tFontBitMap.iCurOriginY = IncLcdY(tFontBitMap.iCurOriginY);
		}

	PT_FontOpr font_opr;

	font_opr = GetFontOpr("freetype");

	ret = font_opr->GetFontBitmap(code, &tFontBitMap);

	if (tFontBitMap.iBpp == 1)
	{
		for (y = tFontBitMap.iYTop; y < tFontBitMap.iYMax; y++)
		{
			i = (y - tFontBitMap.iYTop) * tFontBitMap.iPitch;
			for (x = tFontBitMap.iXLeft, bit = 7; x < tFontBitMap.iXMax; x++)
			{
				if (bit == 7)
				{
					ucByte = tFontBitMap.pucBuffer[i++];
				}

				if (ucByte & (1<<bit))
				{
//					g_ptDispOpr->Show_Pixel(x, y, COLOR_FOREGROUND);
					g_ptDispOpr->Show_Pixel(100+x, 200+y, COLOR_FOREGROUND);
					printf("x:0x%x, y:0x%x\n", 100 + x, 200 + y);
				}
				else
				{
					/* Ê¹ÓÃ±³¾°É«, ²»ÓÃÃè»­ */
					// g_ptDispOpr->ShowPixel(x, y, 0); /* ºÚ */
				}
				bit--;
				if (bit == -1)
				{
					bit = 7;
				}
			}
		}
	}
	else if (tFontBitMap.iBpp == 8)
	{
		printf("bitmap 8\n");
		for (y = tFontBitMap.iYTop; y < tFontBitMap.iYMax; y++)
			for (x = tFontBitMap.iXLeft; x < tFontBitMap.iXMax; x++)
			{
				//g_ptDispOpr->ShowPixel(x, y, ptFontBitMap->pucBuffer[i++]);
				if (tFontBitMap.pucBuffer[i++])
					g_ptDispOpr->Show_Pixel(x, y, COLOR_FOREGROUND);
			}
	}
	else
	{
		printf("ShowOneFont error, can't support %d bpp\n", tFontBitMap.iBpp);
		return -1;
	}


	tFontBitMap.iCurOriginX = tFontBitMap.iNextOriginX;
	tFontBitMap.iCurOriginY = tFontBitMap.iNextOriginY;

	}

}
