#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "display_manage.h"
#include "fb.h"
#include "draw.h"

int main(int argc, char **argv)
{

	/* register */
	fb_register();

	/* init */
	Display_Init();

	while (1) {

		draw_point(0x33ff00ff);
		sleep(3);
		draw_point(0x330000ff);
		sleep(3);
		draw_point(0x3300ff00);
		sleep(3);
		draw_point(0x33ff0000);
		sleep(3);
		draw_point(0x33ff3333);
		sleep(3);
	}

	return 0;
}
