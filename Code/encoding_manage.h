#ifndef _ENCODING_MANAGER_H
#define _ENCODING_MANAGER_H

#include "font_manage.h"
#include "display_manage.h"

typedef struct EncodingOpr {
	char *name;    // 编码名称
	int iHeadLen;  // 文件头部特征的长度。例如 UTF-8 的 Byte Order Mark 为 3 个字节
	PT_FontOpr ptFontOprSupportedHead; // 支持的字体渲染方式，encode 负责将文件中的原始字节流解码成标准的 Unicode 码 \
					   // 字体渲染负责根据 unicode 从字体文件中找到对应的字形，并把它画出来
	int (*isSupport)(unsigned char *pucBufHead); // 根据文件内容，判断是否为该编码格式
	int (*GetCodeFrmBuf)(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode);  // 从字节流中解析出一个字符的 Unicode 编码
	struct EncodingOpr *ptNext;
}T_EncodingOpr, *PT_EncodingOpr;

int RegisterEncodingOpr(PT_EncodingOpr ptEncodingOpr);
void ShowEncodingOpr(void);

PT_Display_Opr Get_Display_Opt(char *pcName);


int EncodingInit(void);
PT_EncodingOpr SelectEncodingOprForFile(unsigned char *pucFileBufHead);
int AddFontOprForEncoding(PT_EncodingOpr ptEncodingOpr, PT_FontOpr ptFontOpr);
int DelFontOprFrmEncoding(PT_EncodingOpr ptEncodingOpr, PT_FontOpr ptFontOpr);


//int AsciiEncodingInit(void);
//int Utf16beEncodingInit(void);
//int Utf16leEncodingInit(void);
//int Utf8EncodingInit(void);


int utf8_register(void);

#endif /* _ENCODING_MANAGER_H */
