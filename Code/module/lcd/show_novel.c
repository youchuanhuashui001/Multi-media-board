#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <sys/ioctl.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static int g_iFdFb;
static struct fb_var_screeninfo g_tVar;
static int g_iScreenSize;
static unsigned char *g_pucFbmem;
static unsigned int g_uiLineWidth;
static unsigned int g_uiPixelWidth;

static FT_Library    g_tLibrary;
static FT_Face       g_tFace;
static FT_GlyphSlot  g_tSlot;

/* color : 0x00RRGGBB */
static void LcdPutPixel(int x, int y, unsigned int color)
{
    if (x < 0 || x >= g_tVar.xres || y < 0 || y >= g_tVar.yres)
    {
        return;
    }

    unsigned char *pen_8 = g_pucFbmem + y * g_uiLineWidth + x * g_uiPixelWidth;
    unsigned short *pen_16;
    unsigned int *pen_32;

    unsigned int red, green, blue;

    pen_16 = (unsigned short *)pen_8;
    pen_32 = (unsigned int *)pen_8;

    switch (g_tVar.bits_per_pixel)
    {
        case 16:
        {
            /* 565 */
            red   = (color >> 16) & 0xff;
            green = (color >> 8) & 0xff;
            blue  = (color >> 0) & 0xff;
            color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
            *pen_16 = color;
            break;
        }
        case 32:
        {
            *pen_32 = color;
            break;
        }
        default:
        {
            printf("can't surport %dbpp\n", g_tVar.bits_per_pixel);
            break;
        }
    }
}

static void DrawBitmap( FT_Bitmap*  bitmap, FT_Int x, FT_Int y)
{
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;

    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
        for ( i = x, p = 0; i < x_max; i++, p++ )
        {
            if ( i < 0      || j < 0       ||
                i >= g_tVar.xres || j >= g_tVar.yres )
            continue;

            if (bitmap->buffer[q * bitmap->width + p])
                LcdPutPixel(i, j, 0xFFFFFF); // White
        }
    }
}


static int LcdInit(void)
{
    g_iFdFb = open("/dev/fb0", O_RDWR);
    if (g_iFdFb < 0)
    {
        printf("can't open /dev/fb0\n");
        return -1;
    }

    if (ioctl(g_iFdFb, FBIOGET_VSCREENINFO, &g_tVar))
    {
        printf("can't get var\n");
        return -1;
    }

    g_uiLineWidth  = g_tVar.xres * g_tVar.bits_per_pixel / 8;
    g_uiPixelWidth = g_tVar.bits_per_pixel / 8;
    g_iScreenSize = g_tVar.xres * g_tVar.yres * g_tVar.bits_per_pixel / 8;
    g_pucFbmem = (unsigned char *)mmap(NULL , g_iScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED, g_iFdFb, 0);
    if (g_pucFbmem == (unsigned char *)-1)
    {
        printf("can't mmap\n");
        return -1;
    }

    /* 清屏: 全部设为黑色 */
    memset(g_pucFbmem, 0, g_iScreenSize);
    return 0;
}

static void LcdExit(void)
{
    munmap(g_pucFbmem, g_iScreenSize);
    close(g_iFdFb);
}

static int FontInit(char *pcFontFile, int iFontSize)
{
    int error;
    error = FT_Init_FreeType( &g_tLibrary );
    if (error)
    {
        printf("FT_Init_FreeType error\n");
        return -1;
    }

    error = FT_New_Face( g_tLibrary, pcFontFile, 0, &g_tFace );
    if (error)
    {
        printf("FT_New_Face error\n");
        return -1;
    }

    g_tSlot = g_tFace->glyph;

    error = FT_Set_Pixel_Sizes(g_tFace, iFontSize, 0);
    if (error)
    {
        printf("FT_Set_Pixel_Sizes error\n");
        return -1;
    }
    return 0;
}

static void FontExit(void)
{
    FT_Done_Face(g_tFace);
    FT_Done_FreeType(g_tLibrary);
}


static void DisplayString(wchar_t *wstr, int x, int y)
{
    int i;
    int error;
    FT_Vector pen;

    pen.x = x * 64;
    pen.y = (g_tVar.yres - y) * 64;

    for (i = 0; i < wcslen(wstr); i++)
    {
        FT_Set_Transform(g_tFace, 0, &pen);

        error = FT_Load_Char(g_tFace, wstr[i], FT_LOAD_RENDER);
        if (error)
        {
            printf("FT_Load_Char error\n");
            continue;
        }

        DrawBitmap(&g_tSlot->bitmap, g_tSlot->bitmap_left, y - g_tSlot->bitmap_top);

        pen.x += g_tSlot->advance.x;
    }
}


int main(int argc, char **argv)
{
    wchar_t *wstr = L"你好, 这里是电子书阅读器！";
    int iFontSize;

    if (argc != 3)
    {
        printf("Usage : %s <font_file> <font_size>\n", argv[0]);
        return -1;
    }

    iFontSize = strtoul(argv[2], NULL, 0);

    if (LcdInit())
    {
        printf("LcdInit error\n");
        return -1;
    }

    if (FontInit(argv[1], iFontSize))
    {
        printf("FontInit error\n");
        LcdExit();
        return -1;
    }

//    DisplayString(wstr, 0, iFontSize);
    DisplayString(wstr, 100, 200);
    
    printf("Enter to exit\n");
    getchar();

    FontExit();
    LcdExit();
    
    return 0;   
}
