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

/* 获取一个字节的从高位开始有几bit 1
 * 11001111 -->  2 bits
 * 11100001 -->  3 bits
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

/*
 * @brief: 从一个字节缓冲区（pucBufStart）的起始位置，解析出 一个 完整的UTF-8字符，并将其转换为对应的Unicode码点（一个整数）。
 *
 * @note: utf-8 编码规范：通过字节的头部几位来表示自身的作用
 *		码点起值	码点终值	字节序列	Byte1		Byte2		Byte3		Byte4		Byte5
 *		U+0000		U+007F		1		0xxxxxxx
 * 		U+0080		U+07FF		2		110xxxxx	10xxxxxx
 * 		U+0800		U+FFFF		3		1110xxxx	10xxxxxx	10xxxxxx
 * 		U+10000		U+1FFFFF	4		11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
 * 		U+200000	U+3FFFFFF	5		111110xx	10xxxxxx	10xxxxxx	10xxxxxx	10xxxxxx
 *		例如：0个1表示 ASCII 码
 *		      2个1表示 2 字节序列，其第一个字节有效数据为bit0~4, 第二个字节有效数据为 bit0~5
 * @return 字符在缓冲区中所占用的字节数
 */

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

	// 越界
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
		// 把头部的 iNum bit1(标志) 去掉
		ucVal = ucVal << iNum;
		ucVal = ucVal >> iNum;
		dwSum += ucVal;
		// 循环处理后面的 iNum - 1 个字节
		// 假如 iNum = 3：buf[0] & 0x1f << 12 | buf[1] & 0x3f << 6 | buf[2] & 0x3f
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

	// 将 utf-8 编码和 freetype 字体操作关联起来，说明可以用 freetype 来渲染 utf-8 编码格式的字符
	AddFontOprForEncoding(utf8_opr, GetFontOpr("freetype"));
	AddFontOprForEncoding(utf8_opr, GetFontOpr("ascii"));

	RegisterEncodingOpr(utf8_opr);

	return 0;
}

