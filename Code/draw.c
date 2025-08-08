#include <stdio.h>
#include "display_manage.h"

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
