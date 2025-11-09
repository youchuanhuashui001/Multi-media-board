#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "display_manage.h"
#include "fb.h"
#include "draw.h"
#include "font_manage.h"
#include "encoding_manage.h"

#include <tslib.h>

extern int OpenTextFile(char *pcFileName);
extern int SetTextDetail(char *pcHZKFile, char *pcFileFreetype, unsigned int dwFontSize);
extern int SelectAndInitDisplay(char *pcName);

int show_next_page(void);
int show_pre_page(void);

#define INTERFACE_MODE_SERIAL 0
#define INTERFACE_MODE_TOUCH  1

int interface_mode = INTERFACE_MODE_TOUCH;

/* ./main <txt_file> <font_file> */
int main(int argc, char **argv)
{
	char TextFile[128];
	char FontFile[128];
	int ret;
	char cOpr;
	PT_Display_Opr fb = NULL; // 获取屏幕信息
	struct tsdev *ts = NULL;
	struct ts_sample samp; // 单点触摸
	struct ts_sample prev_samp;
	unsigned int slide_distance = 0; // 记录滑动时最近两次的距离
	int press_count = 0; // 扫描了 N 次都是按着屏幕的

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
	// 获取屏幕信息
	fb = Get_Display_Opt("fb");
	if (!fb) {
		printf("Get_Display_Opt err\n");
		return -1;
	}

	ts = ts_setup(NULL, 0);
	if (!ts) {
		printf("ts_setup err\n");
		return -1;
	}

	ret = show_next_page();
	if (ret) {
		printf("show first page error.\n");
		return -1;
	}

	while (1)
	{
		if (interface_mode == INTERFACE_MODE_SERIAL) {
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

		} else if (interface_mode == INTERFACE_MODE_TOUCH) {
			if (ts_read(ts, &samp, 1) > 0) {
				if (samp.pressure)
					press_count++;

				// click
				if ((prev_samp.pressure) && (!samp.pressure) && (press_count == 1)) {
					if ((prev_samp.x == samp.x) && (prev_samp.y == prev_samp.y)) {
						if ((prev_samp.x > 0) && (prev_samp.x < fb->Xres / 2)) {
							show_pre_page();
						} else if ((prev_samp.x > fb->Xres / 2) && (prev_samp.x < fb->Xres)) {
							show_next_page();
						}
					}

					press_count = 0;
					memset(&prev_samp, 0, sizeof(prev_samp));
					continue;
				}

				// slide
				if ((samp.pressure) && (prev_samp.pressure)) {
					slide_distance = samp.x + prev_samp.x;
				}

				if ((!samp.pressure) && (slide_distance) && (press_count > 3)) {
					// pre page
					if (slide_distance < 2 * samp.x) {
						show_pre_page();
					} else if (slide_distance > 2 * samp.x) {
						show_next_page();
					}

					slide_distance = 0;

					press_count = 0;
					memset(&prev_samp, 0, sizeof(prev_samp));
					continue;
				}

				prev_samp = samp;
			}
		}
	}

	return 0;
}
