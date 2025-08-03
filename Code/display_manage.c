#include "display_manage.h"

PT_Display_Opr g_Display_Head = NULL;

int Display_Init(void)
{
	//TODO: Maybe other display oprs
	FB_Init();

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