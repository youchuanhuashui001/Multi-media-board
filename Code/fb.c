#include "display_manage.h"
#include "fb.h"

#define FB_FILE "/dev/fb0"

static void *fb_mem = NULL;

static int fb_init(PT_Display_Opr pDispOpr)
{
	struct fb_var_screeninfo fb_var;
	int fd = -1;
	int ret = -1;

	fd = open(FB_FILE, O_RDWR);
	if (fd < 0) {
		printf("Bugs, open %s failed\n", FB_FILE);
		return -1;
	}

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
	if (ret < 0) {
		printf("Bugs, ioctl FBIOGET_VSCREENINFO failed\n");
		close(fd);
		return -1;
	}

	pDispOpr->Xres = fb_var.xres;
	pDispOpr->Yres = fb_var.yres;
	pDispOpr->Bpp = fb_var.bits_per_pixel / 8;

	fb_mem = mmap(NULL, fb_var.xres * fb_var.yres * pDispOpr->Bpp, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (!fb_mem) {
		printf("Bugs, mmap failed\n");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static void fb_show_pixel(int x, int y, int color)
{

}

static void fb_clean_screen(void)
{

}

int FB_Init(void)
{
	PT_Display_Opr fb_opr = NULL;

	fb_opr = (PT_Display_Opr)malloc(sizeof(T_Display_Opr));
	if (fb_opr == NULL) {
		printf("Bugs, malloc fb_opr failed\n");
		return -1;
	}

	fb_opr->name = "fb";
	//TODO: get from fb0
//	fb_opr->Xres = 800;
//	fb_opr->Yres = 600;
//	fb_opr->Bpp = 32;
	fb_opr->Init = fb_init;
	fb_opr->Show_Pixel = fb_show_pixel;
	fb_opr->Clean_Screen = fb_clean_screen;

	Register_Disp_Opr(fb_opr);

	return 0;
}