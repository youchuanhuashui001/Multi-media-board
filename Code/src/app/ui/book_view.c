#include "src/display/lv_display.h"
#include "view_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lvgl/lvgl.h"

#define FONT_SIZE            24
#define BOOK_FILE_PATH       "./resources/book/jianlai.txt"
#define PAGE_BUFFER_SIZE     (4 * 1024) // 4KB 缓冲区

// Book 界面的UI元素
typedef struct {
    lv_obj_t *background;     // 背景图片
    lv_obj_t *label;                  // 显示内容的标签
    lv_obj_t *page_label;             // 页码显示
    lv_font_t *font;                  // LVGL FreeType字体

    // 文件和翻页逻辑所需的状态
    FILE *fp;                         // 文件指针
    char *page_buffer;                // 用于存放一页数据的缓冲区
    long current_pos;                 // 当前页的起始文件偏移量
    long history_pos[100];            // 位置历史记录, 用于后退
    int history_top;                  // 历史记录栈顶
} book_ui_t;

static book_ui_t g_book_ui;

// ------------------- 辅助函数: 从 UTF-8 字符串中根据字符索引获取字节偏移量 -------------------
static uint32_t utf8_get_byte_offset_from_char_index(const char *str, uint32_t char_index) {
    uint32_t byte_offset = 0;
    uint32_t current_char_count = 0;
    while (str[byte_offset] != '\0') {
        if (current_char_count == char_index) {
            return byte_offset;
        }
        // Move to the next UTF-8 character
        if ((str[byte_offset] & 0x80) == 0) { // 1-byte character (ASCII)
            byte_offset++;
        } else if ((str[byte_offset] & 0xE0) == 0xC0) { // 2-byte character
            byte_offset += 2;
        } else if ((str[byte_offset] & 0xF0) == 0xE0) { // 3-byte character
            byte_offset += 3;
        } else if ((str[byte_offset] & 0xF8) == 0xF0) { // 4-byte character
            byte_offset += 4;
        } else {
            // Invalid UTF-8 sequence, just advance by one byte
            byte_offset++;
        }
        current_char_count++;
    }
    // If char_index is out of bounds, return the total length
    return byte_offset;
}

// ------------------- 核心翻页函数 -------------------
static void show_page(void) {
    if (!g_book_ui.fp) return;

    // 定位到当前页的起始位置
    fseek(g_book_ui.fp, g_book_ui.current_pos, SEEK_SET);

    // 读取一块数据到缓冲区
    size_t bytes_read = fread(g_book_ui.page_buffer, 1, PAGE_BUFFER_SIZE - 1, g_book_ui.fp);
    if (bytes_read == 0 && feof(g_book_ui.fp)) {
        lv_label_set_text(g_book_ui.label, "--- 文件结束 ---");
        return;
    }
    g_book_ui.page_buffer[bytes_read] = '\0'; // 确保字符串结束

    // 将缓冲区文本设置到 Label
    lv_label_set_text(g_book_ui.label, g_book_ui.page_buffer);

    // --- 计算下一页的起始位置 (关键) ---
    uint32_t screen_w = lv_display_get_horizontal_resolution(NULL);
    uint32_t screen_h = lv_display_get_vertical_resolution(NULL);
    lv_point_t last_char_point = {screen_w - 40 - 1, screen_h - 80 - 1}; // 减去边距
    uint32_t last_char_index = lv_label_get_letter_on(g_book_ui.label, &last_char_point, false);

    if (last_char_index >= strlen(g_book_ui.page_buffer)) {
        last_char_index = strlen(g_book_ui.page_buffer) - 1;
    }

    uint32_t bytes_consumed = utf8_get_byte_offset_from_char_index(g_book_ui.page_buffer, last_char_index + 1);

    if (bytes_consumed == 0 && bytes_read > 0) {
        bytes_consumed = utf8_get_byte_offset_from_char_index(g_book_ui.page_buffer, 1);
    }

    long next_page_pos = g_book_ui.current_pos + bytes_consumed;

    // 将当前页的起始位置推入历史栈 (用于后退)
    if (g_book_ui.history_top < 100) {
        g_book_ui.history_pos[g_book_ui.history_top++] = g_book_ui.current_pos;
    }
    
    g_book_ui.current_pos = next_page_pos;

    // 更新页码 (这里只是一个简单的页数, 不是总页数)
    lv_label_set_text_fmt(g_book_ui.page_label, "%d", g_book_ui.history_top);
}

static void show_prev_page(void) {
    if (g_book_ui.history_top > 1) {
        // 弹出当前页和上一页的位置
        g_book_ui.history_top -= 2;
        g_book_ui.current_pos = g_book_ui.history_pos[g_book_ui.history_top];
        show_page();
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
			show_page();
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
    g_book_ui.font = lv_freetype_font_create("./resources/font/simsun.ttc",
					    LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                            FONT_SIZE,
                                            LV_FREETYPE_FONT_STYLE_NORMAL);
    if(!g_book_ui.font) {
        LV_LOG_ERROR("FreeType font create failed");
        return;
    }

    // 3. 创建文本样式
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, g_book_ui.font);
    lv_style_set_text_color(&style, lv_color_make(0x51, 0x44, 0x38));
    lv_style_set_text_line_space(&style, 8); // 设置行间距

    // 4. 创建主文本标签
    g_book_ui.label = lv_label_create(book_view.screen);
    lv_obj_add_style(g_book_ui.label, &style, 0);
    lv_label_set_text(g_book_ui.label, "Hi, this is test.!!!!");
    lv_obj_set_size(g_book_ui.label,
                    lv_display_get_horizontal_resolution(NULL) - 40,
                    lv_display_get_vertical_resolution(NULL) - 80);
    lv_label_set_long_mode(g_book_ui.label, LV_LABEL_LONG_WRAP);
    lv_obj_align(g_book_ui.label, LV_ALIGN_TOP_LEFT, 0, 20);

    // 5. 创建页码标签
    g_book_ui.page_label = lv_label_create(book_view.screen);
    lv_style_set_text_line_space(&style, 0); // 设置行间距
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);  // 文本居中对齐
    lv_obj_add_style(g_book_ui.page_label, &style, 0);
    lv_obj_set_size(g_book_ui.page_label,300,30);
    lv_obj_align(g_book_ui.page_label, LV_ALIGN_BOTTOM_MID, 0, 0);

    // 6. 添加触摸事件
    lv_obj_add_event_cb(book_view.screen, screen_event_handler,
                        LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(book_view.screen, LV_OBJ_FLAG_CLICKABLE);

    // 7. 打开文件并初始化阅读器状态
    g_book_ui.fp = fopen(BOOK_FILE_PATH, "r");
    if (!g_book_ui.fp) {
        LV_LOG_ERROR("Cannot open file: %s", BOOK_FILE_PATH);
        lv_label_set_text_fmt(g_book_ui.label, "错误: 无法打开文件\n%s", BOOK_FILE_PATH);
        return;
    }

    g_book_ui.page_buffer = (char *)malloc(PAGE_BUFFER_SIZE);
    if (!g_book_ui.page_buffer) {
        LV_LOG_ERROR("Failed to allocate page buffer");
        fclose(g_book_ui.fp);
        g_book_ui.fp = NULL;
        return;
    }

    // 8. 显示第一页
    g_book_ui.current_pos = 0;
    g_book_ui.history_top = 0;
    show_page();

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