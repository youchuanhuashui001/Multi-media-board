#include <stdio.h>
#include <string.h>
#include "display_manage.h"
#include "fb.h"

PT_Display_Opr g_Display_Head = NULL;

int Display_Init(void)
{
	PT_Display_Opr display_list = g_Display_Head;
	int ret;

	for (display_list = g_Display_Head; display_list; display_list = display_list->Next) {
		if (display_list->Init) {
			ret = display_list->Init();
			if (ret < 0) {
				printf("Register %s failed.\n", display_list->name);
				return ret;
			}
			printf("Register %s suscess.\n", display_list->name);
		} else {
			printf("Register %s failed\n", display_list->name);
		}
	}

	return 0;
}

int Register_Disp_Opr(PT_Display_Opr pDispOpr)
{
	PT_Display_Opr pTmp = NULL;

	if (pDispOpr == NULL || pDispOpr->name == NULL) {
		return -1;
	}

	if (g_Display_Head == NULL) {
		g_Display_Head = pDispOpr;
		pDispOpr->Next = NULL;
	} else {
		pTmp = g_Display_Head;
		while (pTmp->Next != NULL) {
			pTmp = pTmp->Next;
		}
		pTmp->Next = pDispOpr;
		pDispOpr->Next = NULL;
	}

	return 0;
}

int UnRegister_Disp_Opr(PT_Display_Opr pDispOpr)
{
	PT_Display_Opr pTmp = NULL;

	if (pDispOpr == NULL || pDispOpr->name == NULL) {
		return -1;
	}

	if (g_Display_Head == NULL) {
		printf("Bugs, g_Display_Head is NULL, can't unregister_disp_opr\n");
		return -2;
	}

	pTmp = g_Display_Head;
	while (pTmp != NULL) {
		if (pTmp->Next == pDispOpr) {
			pTmp->Next = pDispOpr->Next;
			return 0;
		}
		pTmp = pTmp->Next;
	}

	printf("Bugs, can't find %s in display_opr list\n", pDispOpr->name);

	return -1;
}

void Show_Display_Opr(void)
{
	PT_Display_Opr pDispOpr = g_Display_Head;

	while (pDispOpr != NULL) {
		printf("Display_Opr: %s\n", pDispOpr->name);
		pDispOpr = pDispOpr->Next;
	}

}

PT_Display_Opr Get_Display_Opt(char *name)
{
	PT_Display_Opr pDispOpr = g_Display_Head;

	while (pDispOpr != NULL) {
		if (strcmp(name, pDispOpr->name) == 0) {
			printf("Display_Opr: %s\n", pDispOpr->name);
			return pDispOpr;
		}
		pDispOpr = pDispOpr->Next;
	}

	return NULL;
}
