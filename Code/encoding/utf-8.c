#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "font_manage.h"
#include "encoding_manage.h"

static int isUtf8Coding(unsigned char *pucBufHead);
static int Utf8GetCodeFrmBuf(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode);



static int isUtf8Coding(unsigned char *pucBufHead)
{
	const char aStrUtf8[]    = {0xEF, 0xBB, 0xBF, 0};
	if (strncmp((const char*)pucBufHead, aStrUtf8, 3) == 0)
	{
		printf("utf-8 coding.\n");
		/* UTF-8 */
		return 1;
	}
	else
	{
		printf("not utf-8 coding.\n");
		return 0;
	}
}

/* the number of 1 bits
 * 11001111 1st 2 bits
 * 11100001 1st 3 bits
 */
static int GetPreOneBits(unsigned char ucVal)
{
	int i;
	int j = 0;
	
	for (i = 7; i >= 0; i--)
	{
		if (!(ucVal & (1<<i)))
			break;
		else
			j++;
	}
	return j;

}

static int Utf8GetCodeFrmBuf(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode)
{
#if 0
    The first bit of B is 0, B is ASCII, and B represents a character;
    The first bit of B is 1, the second bit is 0, B is a character, and the first byte of the character is represented by two bytes;
    The first bit of B is 1, the second bit is 0, B is a character, and the first byte of the character is represented by two bytes;

    So, in UTF-8, according to the first bit, determine whether it is an ASCII character;
    Determine whether the first byte of the character is a character;
    Determine whether the first byte of the character is a character;
#endif

	int i;	
	int iNum;
	unsigned char ucVal;
	unsigned int dwSum = 0;

	if (pucBufStart >= pucBufEnd)
	{
		/* file end */
		return 0;
	}

	ucVal = pucBufStart[0];
	iNum  = GetPreOneBits(pucBufStart[0]);

	if ((pucBufStart + iNum) > pucBufEnd)
	{
		/* file end */
		return 0;
	}

	if (iNum == 0)
	{
		/* ASCII */
		*pdwCode = pucBufStart[0];
		return 1;
	}
	else
	{
		ucVal = ucVal << iNum;
		ucVal = ucVal >> iNum;
		dwSum += ucVal;
		for (i = 1; i < iNum; i++)
		{
			ucVal = pucBufStart[i] & 0x3f;
			dwSum = dwSum << 6;
			dwSum += ucVal;			
		}
		*pdwCode = dwSum;
		return iNum;
	}
}

int utf8_register(void)
{
	PT_EncodingOpr utf8_opr = NULL;

	utf8_opr = (PT_EncodingOpr)malloc(sizeof(T_EncodingOpr));
	if (utf8_opr == NULL) {
		printf("Bugs, malloc utf8_opr failed\n");
		return -1;
	}

	utf8_opr->name = "utf-8";
	utf8_opr->iHeadLen = 3;
	utf8_opr->isSupport = isUtf8Coding;
	utf8_opr->GetCodeFrmBuf = Utf8GetCodeFrmBuf;

	AddFontOprForEncoding(utf8_opr, GetFontOpr("freetype"));
	AddFontOprForEncoding(utf8_opr, GetFontOpr("ascii"));

	RegisterEncodingOpr(utf8_opr);

	return 0;
}

