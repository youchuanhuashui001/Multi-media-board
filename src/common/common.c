#include "common.h"

#define FONT_FILE_PATH       "./resources/font/simsun.ttc"
#define DEFAULT_FONT_SIZE    24
#define MAX_FONT_SIZES       10  // 支持最多 10 种不同大小的字体

// 字体缓存结构
typedef struct {
	int size;
	lv_font_t *font;
} font_cache_t;

// 字体缓存数组
static font_cache_t g_font_cache[MAX_FONT_SIZES] = {0};
static int g_font_count = 0;

// 获取指定大小的 FreeType 字体
lv_font_t* font_manager_get_freetype_font(int font_size)
{
	// 检查是否已有该大小的字体
	for (int i = 0; i < g_font_count; i++) {
		if (g_font_cache[i].size == font_size && g_font_cache[i].font != NULL) {
			return g_font_cache[i].font;
		}
	}

	// 如果没有该大小的字体，创建新的
	if (g_font_count >= MAX_FONT_SIZES) {
		LV_LOG_WARN("Font cache full, max %d fonts supported", MAX_FONT_SIZES);
		return NULL;
	}

	// 创建 FreeType 字体
	lv_font_t *font = lv_freetype_font_create(FONT_FILE_PATH,
		LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
		font_size,
		LV_FREETYPE_FONT_STYLE_NORMAL);

	if (!font) {
		LV_LOG_ERROR("FreeType font create failed: %s (size: %d)", FONT_FILE_PATH, font_size);
		return NULL;
	}

	// 缓存新创建的字体
	g_font_cache[g_font_count].size = font_size;
	g_font_cache[g_font_count].font = font;
	g_font_count++;

	printf("FreeType font created: %s (size: %d)\n", FONT_FILE_PATH, font_size);
	return font;
}

// 获取默认大小的 FreeType 字体（24pt）
lv_font_t* font_manager_get_default_font(void)
{
	return font_manager_get_freetype_font(DEFAULT_FONT_SIZE);
}

// 释放所有 FreeType 字体资源
void font_manager_free_all_fonts(void)
{
	for (int i = 0; i < g_font_count; i++) {
		if (g_font_cache[i].font != NULL) {
			lv_freetype_font_delete(g_font_cache[i].font);
			g_font_cache[i].font = NULL;
			printf("FreeType font released (size: %d)\n", g_font_cache[i].size);
		}
	}
	g_font_count = 0;
}

