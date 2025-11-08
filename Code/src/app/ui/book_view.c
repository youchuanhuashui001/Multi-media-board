#include "common.h"
#include "src/core/lv_obj_pos.h"
#include "src/display/lv_display.h"
#include "src/font/lv_font.h"
#include "view_manager.h"
#include <stdint.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FONT_SIZE            24
#define BOOK_FILE_PATH       "A:/tmp/resources/book/"
#define PAGE_BUFFER          4096

typedef struct {
	lv_obj_t *background;             // 背景图片
	lv_obj_t *label;                  // 显示内容的标签
	lv_obj_t *page_label;             // 页码显示
	lv_font_t *font;                  // LVGL FreeType字体

	FILE *fp;                         // 文件指针
	char *page_buffer;                // 用于存放一页数据的缓冲区
	long current_pos;                 // 当前文件偏移量
	long history_pos[1024];           // 位置历史记录, 用于后退
	int history_top;                  // 历史记录的个数

	char *books_name[256];        // 所有的书籍名称
	int book_count;                   // 书籍数量

	lv_obj_t *book_shelf;            // 书架容器

	struct book_item {
		lv_obj_t *btn;        // 书籍按钮
		lv_obj_t *cover;      // 封面图片  w*h 200*240
		lv_obj_t *title;      // 书籍名称
	} *book_items;
} book_ui_t;

static book_ui_t g_book_ui;
const char *current_book_name = NULL;    // 当前打开的书籍信息

static int GetPreOneBits(unsigned char ucVal)
{
	int i;
	int j = 0;

	for (i = 7; i >= 0; i--)
	{
		if (!(ucVal & (1<<i)))
			break;
		else
			j++;
	}
	return j;
}

static void show_page(unsigned int file_offset)
{
	int row_max = lv_display_get_horizontal_resolution(NULL) / FONT_SIZE;
	// 需要根据字体的高度来设置，不能使用 FONT_SIZE
	int line_max = (lv_display_get_vertical_resolution(NULL) / lv_font_get_line_height(g_book_ui.font));
	int row = 0;
	int line = 0;
	char show_buffer[PAGE_BUFFER] = {0};
	int i = 0;

	fseek(g_book_ui.fp, file_offset, SEEK_SET);
	fread(g_book_ui.page_buffer, 1, PAGE_BUFFER, g_book_ui.fp);

	while (i < PAGE_BUFFER) {
		int pre_bit;
		// UTF-8 为变长编码，根据第一个字节判断长度，等到把整个字都读出来之后，认为是一个字，列 +1
		pre_bit = GetPreOneBits(g_book_ui.page_buffer[i]);

		//  lvgl label 会将 \r 也显示成换行
		if (g_book_ui.page_buffer[i] == '\r') {
			row++;
			show_buffer[i] = ' ';
			i++;
			continue;
		}
		if (g_book_ui.page_buffer[i] == '\n') {
			line++;
			row = 0;
			show_buffer[i] = g_book_ui.page_buffer[i];
			i++;
			if (line >= line_max) {
				break;
			}
			continue;
		}
		if (g_book_ui.page_buffer[i] == '\t') {
			show_buffer[i] = ' ';
			i++;
			row++;
			continue;
		}

		if (pre_bit == 0) {
			show_buffer[i] = g_book_ui.page_buffer[i];
		} else if (pre_bit == 1) {
			// 非法情况，跳过
			show_buffer[i] = g_book_ui.page_buffer[i];
			i++;
			continue;
		} else if (pre_bit == 2) {
			show_buffer[i] = g_book_ui.page_buffer[i];
			show_buffer[i+1] = g_book_ui.page_buffer[i+1];
			i += 1;
		} else if (pre_bit == 3) {
			show_buffer[i] = g_book_ui.page_buffer[i];
			show_buffer[i+1] = g_book_ui.page_buffer[i+1];
			show_buffer[i+2] = g_book_ui.page_buffer[i+2];
			i += 2;
		}

		row++;
		if (row >= row_max) {
			line++;
			row = 0;
			if (line >= line_max) {
				break;
			}
		}

 		i++;
	}

	show_buffer[i] = '\0';

	// 更新当前的文件偏移量
	g_book_ui.current_pos += i;
	// 只有显示下一页的时候才更新 top，显示上一页时不更新 top
	if (g_book_ui.current_pos > g_book_ui.history_pos[g_book_ui.history_top]) {
		g_book_ui.history_top++;
		g_book_ui.history_pos[g_book_ui.history_top] = g_book_ui.current_pos;
	}

	book_view_printf("page_buffer: %s\n", show_buffer);
	lv_label_set_text(g_book_ui.label, show_buffer);
}

static void show_prev_page(void) {
	if (g_book_ui.history_top >= 2) {
		// 如果回退到上个标记，那么去读的话就是当前显示的页
		// 因此需要回退到上上个标记，这样才是上一页内容的起始位置
		g_book_ui.history_top--;
		g_book_ui.current_pos = g_book_ui.history_pos[g_book_ui.history_top - 1];
		show_page(g_book_ui.current_pos);
	} else {
		book_view_printf("Already the first page.\n");
	}
}

// ------------------- View 框架函数 -------------------

// 主界面销毁函数
static void book_view_destroy(void)
{
	// 清理我们分配的资源
	if (g_book_ui.fp) {
		fclose(g_book_ui.fp);
		g_book_ui.fp = NULL;
	}
	if (g_book_ui.page_buffer) {
		free(g_book_ui.page_buffer);
		g_book_ui.page_buffer = NULL;
	}
	if (g_book_ui.font) {
		lv_freetype_font_delete(g_book_ui.font);
		g_book_ui.font = NULL;
	}

	// 清理LVGL对象
	if (book_view.screen != NULL) {
		lv_obj_del(book_view.screen);
		book_view.screen = NULL;
	}
}

static void book_view_hide(void)
{
	if (book_view.screen != NULL) {
		lv_obj_add_flag(book_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

static void book_view_show(void)
{
	if (book_view.screen != NULL) {
		lv_obj_clear_flag(book_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

static void screen_event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_indev_t * indev = lv_indev_get_act();
	if(indev == NULL) return;

	lv_point_t point;
	lv_indev_get_point(indev, &point);

	if (code == LV_EVENT_CLICKED) {
		if (point.x > lv_display_get_horizontal_resolution(NULL) / 2) {
			book_view_printf("next page.\n");
			show_page(g_book_ui.current_pos);
		} else {
			book_view_printf("pre page.\n");
			show_prev_page();
		}
	}
}

// 事件回调函数
static void book_view_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch(code) {
		case LV_EVENT_SCREEN_LOADED:
			break;
		case LV_EVENT_SCREEN_UNLOADED:
			break;
		default:
			break;
	}
}

// 书籍按钮点击事件处理函数
static void book_btn_event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * btn = lv_event_get_target(e);

	if(code == LV_EVENT_CLICKED) {
		// 获取按钮的用户数据(书籍索引)
		int book_index = (int)lv_obj_get_user_data(btn);
		book_view_printf("Selected book: %s\n", g_book_ui.books_name[book_index]);

		current_book_name = g_book_ui.books_name[book_index];

		// 打开文件
		char full_path[512];
		snprintf(full_path, sizeof(full_path), "./resources/book/%s", current_book_name);
		g_book_ui.fp = fopen(full_path, "r");
		if (!g_book_ui.fp) {
			book_view_printf("Cannot open file: %s\n", full_path);
			return;
		}

		lv_obj_add_flag(g_book_ui.book_shelf, LV_OBJ_FLAG_HIDDEN);

		g_book_ui.page_buffer = (char *)malloc(PAGE_BUFFER);
		if (!g_book_ui.page_buffer) {
			LV_LOG_ERROR("Failed to allocate page buffer");
			fclose(g_book_ui.fp);
			g_book_ui.fp = NULL;
			return;
		}

		// 初始化阅读状态
		g_book_ui.current_pos = 0;
		g_book_ui.history_top = 0;
		memset(g_book_ui.history_pos, 0, sizeof(g_book_ui.history_pos));

		// 显示第一页
		show_page(g_book_ui.current_pos);
	}
}

int scan_dir_resources(const char *path)
{
	lv_fs_dir_t dir;
	lv_fs_res_t res;
	res = lv_fs_dir_open(&dir, BOOK_FILE_PATH);
	if (res != LV_FS_RES_OK) {
		LV_LOG_USER("open dir failed %d.\n", res);
		return -1;
	}

	char fn[256];
	while(1) {
		res = lv_fs_dir_read(&dir, fn, sizeof(fn));
		if(res != LV_FS_RES_OK) {
			printf("read dir failed.\n");
			break;
		}

		/* fn is empty if there are no more files to read. */
		if(strlen(fn) == 0) {
			break;
		}

		g_book_ui.books_name[g_book_ui.book_count++] = strdup(fn);
		printf("%s\n", fn);
	}

	int i;
	for (i = 0; i < g_book_ui.book_count; i++) {
		printf("book %d: %s\n", i, g_book_ui.books_name[i]);
	}

	lv_fs_dir_close(&dir);

	g_book_ui.book_items = malloc(sizeof(struct book_item) * g_book_ui.book_count);
	if (!g_book_ui.book_items) {
		printf("malloc book items failed.\n");
		return -1;
	}

	g_book_ui.book_shelf = lv_obj_create(book_view.screen);
	lv_obj_set_size(g_book_ui.book_shelf, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
	// 移除屏幕的默认样式（边框、背景等）
	lv_obj_set_style_border_width(g_book_ui.book_shelf, 0, 0);
	lv_obj_set_style_bg_opa(g_book_ui.book_shelf, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(g_book_ui.book_shelf, 0, 0);
	lv_obj_set_style_radius(g_book_ui.book_shelf, 0, 0);

	lv_obj_set_style_bg_color(g_book_ui.book_shelf,
	                         lv_color_make(0xE7, 0xDB, 0xB5),
	                         LV_PART_MAIN);

	// 创建书籍目录界面
	for (i = 0; i < g_book_ui.book_count; i++) {
		printf("Create book button: %s\n", g_book_ui.books_name[i]);

		g_book_ui.book_items[i].btn = lv_btn_create(g_book_ui.book_shelf);
		lv_obj_set_size(g_book_ui.book_items[i].btn, 200, 240);

		// 设置按钮样式
		lv_obj_set_style_bg_color(g_book_ui.book_items[i].btn, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_bg_opa(g_book_ui.book_items[i].btn, LV_OPA_50, 0);
		lv_obj_set_style_border_width(g_book_ui.book_items[i].btn, 2, 0);
		lv_obj_set_style_border_color(g_book_ui.book_items[i].btn, lv_color_hex(0x808080), 0);
		lv_obj_set_style_radius(g_book_ui.book_items[i].btn, 10, 0);

		// 创建封面图片(如果有的话)
		g_book_ui.book_items[i].cover = lv_image_create(g_book_ui.book_items[i].btn);
		lv_image_set_src(g_book_ui.book_items[i].cover, "A:./resources/image/book/book.jpeg");
		lv_obj_align(g_book_ui.book_items[i].cover, LV_ALIGN_CENTER, 0, 0);

		// 创建书名标签
		static lv_style_t style;
		lv_style_init(&style);
		lv_style_set_text_font(&style, g_book_ui.font);
		lv_style_set_text_color(&style, lv_color_make(0x51, 0x44, 0x38));
		lv_style_set_text_line_space(&style, 0);  // 设置行间距为0

		g_book_ui.book_items[i].title = lv_label_create(g_book_ui.book_items[i].btn);
		lv_obj_add_style(g_book_ui.book_items[i].title, &style, 0);
		lv_label_set_text(g_book_ui.book_items[i].title, g_book_ui.books_name[i]);
		lv_obj_align(g_book_ui.book_items[i].title, LV_ALIGN_BOTTOM_MID, 0, -10);

		// 设置按钮位置
		if (i < 5) {
		    lv_obj_align(g_book_ui.book_items[i].btn, LV_ALIGN_TOP_LEFT, 10 + i*210, 20);
		} else if (i < 10) {
		    lv_obj_align(g_book_ui.book_items[i].btn, LV_ALIGN_TOP_LEFT, 10 + (i-5)*200, 280);
		} else {
			printf("more than 10 books not support yet.\n");
		}

		// 添加点击事件
		lv_obj_add_flag(g_book_ui.book_items[i].btn, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_user_data(g_book_ui.book_items[i].btn, (void*)i);  // 保存书籍索引
		lv_obj_add_event_cb(g_book_ui.book_items[i].btn, book_btn_event_handler, LV_EVENT_CLICKED, NULL);
	}

	return 0;

}

// 主界面初始化
static void book_view_init(void)
{
	// 创建主屏幕
	book_view.screen = lv_obj_create(lv_screen_active());
	lv_obj_set_size(book_view.screen, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));

	// 移除屏幕的默认样式（边框、背景等）
	lv_obj_set_style_border_width(book_view.screen, 0, 0);
	lv_obj_set_style_bg_opa(book_view.screen, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(book_view.screen, 0, 0);
	lv_obj_set_style_radius(book_view.screen, 0, 0);

	lv_obj_set_style_bg_color(book_view.screen,
	                         lv_color_make(0xE7, 0xDB, 0xB5),
	                         LV_PART_MAIN);
	lv_obj_move_background(book_view.screen);

	// 2. 创建 FreeType 字体
	//TODO: 考虑这里的字体文件路径
	g_book_ui.font = lv_freetype_font_create("./resources/font/simsun.ttc",
		LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
		FONT_SIZE,
		LV_FREETYPE_FONT_STYLE_NORMAL);
	if(!g_book_ui.font) {
		LV_LOG_ERROR("FreeType font create failed");
		return;
	}

	scan_dir_resources("./resources/book/");

	// 3. 创建文本样式
	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_text_font(&style, g_book_ui.font);
	lv_style_set_text_color(&style, lv_color_make(0x51, 0x44, 0x38));
	lv_style_set_text_line_space(&style, 0);  // 设置行间距为0

	// 4. 创建主文本标签
	g_book_ui.label = lv_label_create(book_view.screen);
	lv_obj_add_style(g_book_ui.label, &style, 0);
	lv_obj_set_size(g_book_ui.label, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
	lv_label_set_long_mode(g_book_ui.label, LV_LABEL_LONG_WRAP);
	lv_obj_align(g_book_ui.label, LV_ALIGN_TOP_LEFT, 0, 0);

	// 6. 添加触摸事件
	lv_obj_add_event_cb(book_view.screen, screen_event_handler,
		LV_EVENT_CLICKED, NULL);
	lv_obj_add_flag(book_view.screen, LV_OBJ_FLAG_CLICKABLE);

	printf("book view init finish.\n");
}

view_t book_view = {
	.name = "book_view",
	.init = book_view_init,
	.hide = book_view_hide,
	.show = book_view_show,
	.destroy = book_view_destroy,
	.event_cb = book_view_event_cb,
	.screen = NULL,
	.initialized = 0
};