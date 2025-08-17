#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "display_manage.h"
#include "fb.h"
#include "draw.h"
#include "font_manage.h"
#include "encoding_manage.h"

extern int OpenTextFile(char *pcFileName);
extern int SetTextDetail(char *pcHZKFile, char *pcFileFreetype, unsigned int dwFontSize);
extern int SelectAndInitDisplay(char *pcName);

int show_next_page(void);
int show_pre_page(void);

/* ./main <txt_file> <font_file> */
int main(int argc, char **argv)
{
	char TextFile[128];
	char FontFile[128];
	int ret;
	char cOpr;

	strncpy(TextFile, argv[1], 128);
	TextFile[127] = '\0';

	strncpy(FontFile, argv[2], 128);
	FontFile[127] = '\0';

	/* ************************** register start ******************* */

	/* display */
	fb_register();

	/* font */
	ascii_register();
	gbk_register();
	freetype_register();

	/* encoding */
//	utf8_register();

	/* ************************** register end ******************* */



	/* ************************** init start ******************* */
	Display_Init();
	FontsInit();
	EncodingInit();
	/* ************************** init end ******************* */

	OpenTextFile(TextFile);
	SetTextDetail(NULL, FontFile, 24);
	SelectAndInitDisplay("fb");

	ret = show_next_page();
	if (ret) {
		printf("show first page error.\n");
		return -1;
	}

	while (1)
	{
		printf("Enter 'n' to show next page, 'u' to show previous page, 'q' to exit: ");

		do {
			cOpr = getchar();
		} while ((cOpr != 'n') && (cOpr != 'u') && (cOpr != 'q'));

		if (cOpr == 'n')
		{
			show_next_page();
		}
		else if (cOpr == 'u')
		{
			show_pre_page();
		}
		else
		{
			return 0;
		}
	}

	return 0;
}
