#include <stdio.h>
#include <string.h>

#include "font_manage.h"

static PT_FontOpr g_ptFontOprHead = NULL;

int RegisterFontOpr(PT_FontOpr ptFontOpr)
{
	PT_FontOpr ptTmp;

	if (!g_ptFontOprHead)
	{
		g_ptFontOprHead   = ptFontOpr;
		ptFontOpr->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptFontOprHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext     = ptFontOpr;
		ptFontOpr->ptNext = NULL;
	}

	return 0;
}


void ShowFontOpr(void)
{
	int i = 0;
	PT_FontOpr ptTmp = g_ptFontOprHead;

	while (ptTmp)
	{
		printf("%02d %s\n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

PT_FontOpr GetFontOpr(char *pcName)
{
	PT_FontOpr ptTmp = g_ptFontOprHead;

	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			return ptTmp;
		}
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}


int FontsInit(void)
{
	PT_FontOpr font_list = g_ptFontOprHead;
	int ret;

	for (font_list = g_ptFontOprHead; font_list; font_list = font_list->ptNext) {
		if (font_list->Init) {
			ret = font_list->Init();
			if (ret < 0) {
				printf("Register %s failed.\n", font_list->name);
				return ret;
			}
			printf("Register %s suscess.\n", font_list->name);
		} else {
			printf("Register %s failed.\n", font_list->name);
		}
	}

	return 0;
}
