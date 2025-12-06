#ifndef __COMMON_H_
#define __COMMON_H_

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl.h"
#include "lvgl/demos/lv_demos.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

#include "view_manager.h"

#ifdef VIEW_DEBUG_ON
#define view_printf(fmt, args...)   printf("VIEW_DEBUG: " fmt, ## args)
#else
#define view_printf(fmt, args...) do {} while (0)
#endif

#ifdef MAIN_VIEW_DEBUG_ON
#define main_view_printf(fmt, args...)   printf("MAIN_VIEW_DEBUG: " fmt, ## args)
#else
#define main_view_printf(fmt, args...) do {} while (0)
#endif

#ifdef AUDIO_VIEW_DEBUG_ON
#define audio_view_printf(fmt, args...)   printf("AUDIO_VIEW_DEBUG: " fmt, ## args)
#else
#define audio_view_printf(fmt, args...) do {} while (0)
#endif

#ifdef BOOK_VIEW_DEBUG_ON
#define book_view_printf(fmt, args...)   printf("BOOK_VIEW_DEBUG: " fmt, ## args)
#else
#define book_view_printf(fmt, args...) do {} while (0)
#endif

// ===== 字体管理 =====
// 获取指定大小的 FreeType 字体
lv_font_t* font_manager_get_freetype_font(int font_size);

// 获取默认大小的 FreeType 字体（24pt）
lv_font_t* font_manager_get_default_font(void);

// 释放所有 FreeType 字体资源
void font_manager_free_all_fonts(void);

#endif