#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "display_manage.h"
#include "encoding_manage.h"
#include "font_manage.h"

#define COLOR_BACKGROUND   0xE7DBB5
#define COLOR_FOREGROUND   0x514438

typedef struct page_desc {
	int page;
	unsigned char *lcd_first_post_at_file;
	unsigned char *lcd_next_page_first_pos_at_file;
	struct page_desc *pt_pre_page;
	struct page_desc *pt_next_page;
} t_page_desc, *pt_page_desc;

static int g_iFdTextFile; // 小说文件句柄
static unsigned char *g_pucTextFileMem;  // 小说文件内存映射起点
static unsigned char *g_pucTextFileMemEnd; // 小说文件内存映射的结尾
static PT_EncodingOpr g_ptEncodingOprForFile; // 用于将解码小说文件

static PT_Display_Opr g_ptDispOpr; // 用于将解码后的 Unicode 渲染到 lcd

// TODO: delete
unsigned char *g_pucLcdFirstPosAtFile;
//static unsigned char *g_pucLcdFirstPosAtFile; // 文件第一个字符的位置
static unsigned char *g_pucLcdNextPosAtFile; // 文件中的下一页的第一个字符的位置

static int g_dwFontSize; // 字体大小

static pt_page_desc g_pt_pages   = NULL; // 记录所有的页
static pt_page_desc g_pt_cur_page = NULL; // 记录当前的页


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

int RelocateFontPos(PT_FontBitMap ptFontBitMap)
{
	int iLcdY;
	int iDeltaX;
	int iDeltaY;

	if (ptFontBitMap->iYMax > g_ptDispOpr->Yres)
	{
		/* Y over the range */
		return -1;
	}

	/* X 越界，换行 */
	if (ptFontBitMap->iXMax > g_ptDispOpr->Xres)
	{
		/* Y++ */
		iLcdY = IncLcdY(ptFontBitMap->iCurOriginY);
		if (0 == iLcdY)
		{
			/* screen is full */
			return -1;
		}
		else
		{
			/* update bitmap */
			iDeltaX = 0 - ptFontBitMap->iCurOriginX;
			iDeltaY = iLcdY - ptFontBitMap->iCurOriginY;

			ptFontBitMap->iCurOriginX  += iDeltaX;
			ptFontBitMap->iCurOriginY  += iDeltaY;

			ptFontBitMap->iNextOriginX += iDeltaX;
			ptFontBitMap->iNextOriginY += iDeltaY;

			ptFontBitMap->iXLeft += iDeltaX;
			ptFontBitMap->iXMax  += iDeltaX;

			ptFontBitMap->iYTop  += iDeltaY;
			ptFontBitMap->iYMax  += iDeltaY;;
			return 0;
		}
	}

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

			/* 更新一行 */
			tFontBitMap.iCurOriginX = 0;
			tFontBitMap.iCurOriginY = IncLcdY(tFontBitMap.iCurOriginY);
			if (0 == tFontBitMap.iCurOriginY)
			{
				/* 回到开头了，也就是说一页显示完了 */
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

		ptFontOpr = g_ptEncodingOprForFile->ptFontOprSupportedHead;
		while (ptFontOpr)
		{
			iError = ptFontOpr->GetFontBitmap(dwCode, &tFontBitMap);
			if (0 == iError)
			{
				// 重定位，防止 x、y 越界
				if (RelocateFontPos(&tFontBitMap))
				{
					/* Ê£ÏÂµÄLCD¿Õ¼ä²»ÄÜÂú×ãÏÔÊ¾Õâ¸ö×Ö·û */
					return 0;
				}

				if (bHasNotClrSceen)
				{
					/* Ê×ÏÈÇåÆÁ */
					g_ptDispOpr->Clean_Screen(COLOR_BACKGROUND);
					bHasNotClrSceen = 0;
				}
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

	for (test = 0; test < 10; test++) {
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

static void record_page(pt_page_desc new_page)
{
	pt_page_desc page;

	if (!g_pt_pages) {
		g_pt_pages = new_page;
	} else {
		page = g_pt_pages;
		while (page->pt_next_page) {
			page = page->pt_next_page;
		}
		page->pt_next_page = new_page;
		new_page->pt_pre_page = page;
	}
}

int show_next_page(void)
{
	int ret;
	pt_page_desc page;
	unsigned char *text_file_mem_cur_postion;

	// non_first_page
	if (g_pt_cur_page) {
		text_file_mem_cur_postion = g_pt_cur_page->lcd_next_page_first_pos_at_file;
	// first_page
	} else {
		text_file_mem_cur_postion = g_pucLcdFirstPosAtFile;
	}
	ret = ShowOnePage(text_file_mem_cur_postion);
	if (ret == 0) {
		// 当前页面显示完成，将 cur_page 指向下一页，下次调用时就会直接显示下一页了
		if (g_pt_cur_page && g_pt_cur_page->pt_next_page) {
			g_pt_cur_page = g_pt_cur_page->pt_next_page;
			return 0;
		}

		// 对于第一次，需要先申请空间记录 cur_page
		page = (pt_page_desc)malloc(sizeof(t_page_desc));
		if (page) {
			// 打开文件的时候会更新此地址
			page->lcd_first_post_at_file          = text_file_mem_cur_postion;
			// 画完一页的时候会更新此地址
			page->lcd_next_page_first_pos_at_file = g_pucLcdNextPosAtFile;
			page->pt_pre_page = NULL;
			page->pt_next_page = NULL;
			g_pt_cur_page = page;
			// 将申请的 page 加入到链表中，后面可以通过 pre/next 找到
			record_page(page);
		} else {
			printf("malloc cur page failed.\n");
			return -1;
		}
	}

	return ret;
}

int show_pre_page(void)
{

	int ret;

	// 当前 page 还没有，或者上一页是空的
	if (!g_pt_cur_page || !g_pt_cur_page->pt_pre_page) {
		return -1;
	}

	// 显示上一页
	ret = ShowOnePage(g_pt_cur_page->pt_pre_page->lcd_first_post_at_file);
	if (ret == 0) {
		// 更新当前页为上一页
		g_pt_cur_page = g_pt_cur_page->pt_pre_page;
	}
	return ret;
}

