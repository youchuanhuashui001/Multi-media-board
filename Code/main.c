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


extern int test_show_one_font(void);

int main(int argc, char **argv)
{
	char TextFile[128];
	char FontFile[128];

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

	test_show_one_font();


	while (1) {

	}

	return 0;
}
