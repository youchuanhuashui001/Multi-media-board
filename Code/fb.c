#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/fb.h>

#include "display_manage.h"
#include "fb.h"

#define FB_FILE "/dev/fb0"

static struct fb_var_screeninfo g_FBVar;
static struct fb_fix_screeninfo g_FBFix;

static unsigned int g_fb_screen_size;
static unsigned char *g_fb_mem;

static unsigned int g_line_width;  // 每行占多少个字节数
static unsigned int g_pixel_width; // 每个像素占多少个字节数

static int fb_init(void)
{
	int fd = -1;
	int ret = -1;

	fd = open(FB_FILE, O_RDWR);
	if (fd < 0) {
		printf("Bugs, open %s failed\n", FB_FILE);
		return -1;
	}

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &g_FBVar);
	if (ret < 0)
	{
		printf("can't get fb's var\n");
		return -1;
	}

	ret = ioctl(fd, FBIOGET_FSCREENINFO, &g_FBFix);
	if (ret < 0)
	{
		printf("can't get fb's fix\n");
		return -1;
	}

	// 整个屏幕所占用的字节数:共有 x*y 个像素，每个像素用 N bit 表示
	g_fb_screen_size = g_FBVar.xres * g_FBVar.yres * g_FBVar.bits_per_pixel / 8;
	g_fb_mem = (unsigned char *)mmap(NULL, g_fb_screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (!g_fb_mem) {
		printf("Bugs, mmap failed\n");
		close(fd);
		return -1;
	}

	// 每行占多少个字节数
	g_line_width = g_FBVar.xres * g_FBVar.bits_per_pixel / 8;
	// 每个像素占几个字节
	g_pixel_width = g_FBVar.bits_per_pixel / 8;

	//TODO: delete
	printf("g_line_width:%d, g_pixel_width:%d\n", g_line_width, g_pixel_width);

	return 0;
}

// color：四个字节，ARGB8888 透明度、红色、绿色、蓝色
// 0xFF FF0000：不透明的纯红色
// 0xFF 00FF00：不透明的纯绿色
// 0xFF 0000FF：不透明的纯蓝色
// 本质上是一个有损压缩：将 ARGB8888 格式的色彩转换成 rgb565 格式的色彩
static int fb_show_pixel(int x, int y, int color)
{
	unsigned char *point_8;
	unsigned short *point_16;
	unsigned int *point_32;

	unsigned char red, green, blue;
	unsigned short color_565;

	if ((x > g_FBVar.xres) || (y > g_FBVar.yres)) {
		printf("x must be < %d, y must be < %d\n", g_FBVar.xres, g_FBVar.yres);
		return -1;
	}

	point_8 = g_fb_mem + g_line_width * y + g_pixel_width * x;
	point_16 = (unsigned short *)point_8;
	point_32 = (unsigned int*)point_8;

	switch (g_FBVar.bits_per_pixel) {
		case 8:
			*point_8 = (unsigned char)color;
			break;
		case 16:
			// 获取红色的高5位
			red = (color >> (16 + 3)) & 0x1f;
			// 获取绿色的高6位
			green = (color >> (8 + 2)) & 0x3f;
			// 获取蓝色的高5位
			blue = (color >> 3) & 0x1f;
			color_565 = (red << 11) | (green << 6) | (blue);
			*point_16 = color_565;
			break;
		case 32:
			*point_32 = color;
			break;
		default:
			printf("can't support %d bpp\n", g_FBVar.bits_per_pixel);
			return -1;
	}

	return 0;
}

static int fb_clean_screen(unsigned int back_color)
{
	unsigned char *point_8;
	unsigned short *point_16;
	unsigned int *point_32;

	int red, green, blue;
	int i;

	unsigned short color_565;


	point_8 = g_fb_mem;
	point_16 = (unsigned short *)point_8;
	point_32 = (unsigned int *)point_8;

	switch (g_FBVar.bits_per_pixel) {
		case 8:
			memset(g_fb_mem, back_color, g_fb_screen_size);
			break;
		case 16:
			red = (back_color >> (16 + 3)) & 0x1f;
			green = (back_color >> (8 + 2)) & 0x3f;
			red = (back_color >>  3) & 0x1f;
			color_565 = (red << 11) | (red << 5) | blue;
			while (i < g_fb_screen_size) {
				*point_16 = color_565;
				point_16++;
				i += 2;
			}
			break;
		case 32:
			while (i < g_fb_screen_size) {
				*point_32 = back_color;
				point_32++;
				i += 4;
			}
			break;
		default:
			printf("can't support %d bpp\n", g_FBVar.bits_per_pixel);
			return -1;
	}
}

int fb_register(void)
{
	PT_Display_Opr fb_opr = NULL;

	fb_opr = (PT_Display_Opr)malloc(sizeof(T_Display_Opr));
	if (fb_opr == NULL) {
		printf("Bugs, malloc fb_opr failed\n");
		return -1;
	}

	fb_opr->name = "fb";
	fb_opr->Init = fb_init;
	fb_opr->Show_Pixel = fb_show_pixel;
	fb_opr->Clean_Screen = fb_clean_screen;

	Register_Disp_Opr(fb_opr);

	return 0;
}
