#ifndef __DISPLAY_MANAGE_H__
#define __DISPLAY_MANAGE_H__

typedef struct Display_Opr {
    char *name;
    int Xres;
    int Yres;
    int Bpp;
    int (*Init)(void);
    int (*Show_Pixel)(int x, int y, int color);
    int (*Clean_Screen)(void);
    struct Display_Opr *Next;
} T_Display_Opr, *PT_Display_Opr;

// 显示器初始化
int Display_Init(void);
// 注册显示器
int Register_Disp_Opr(PT_Display_Opr pDispOpr);
// 注销显示器
// return 0: success, -1: 参数错误, -2: bugs
int UnRegister_Disp_Opr(PT_Display_Opr pDispOpr);
// 显示所有的显示器
void Show_Display_Opr(void);

#endif
