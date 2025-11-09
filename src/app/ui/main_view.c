#include "common.h"
#include "view_manager.h"

// 主界面的UI元素
typedef struct {
	lv_obj_t *background;     // 背景图片
	struct {
		lv_obj_t *music;      // 音乐按钮
		lv_obj_t *video;      // 视频按钮
		lv_obj_t *photo;      // 图片按钮
		lv_obj_t *book;       // 电子书按钮
		lv_obj_t *setup;      // 设置按钮
	} buttons;
	struct {
		lv_obj_t *time;       // 时间显示
		lv_obj_t *date;       // 日期显示
	} info;
} g_main_ui_t;

// UI元素实例
static g_main_ui_t g_main_ui;

// 主界面销毁函数
static void main_view_destroy(void)
{
	// 清理资源
	if (main_view.screen != NULL) {
		lv_obj_del(main_view.screen);
		main_view.screen = NULL;
	}
}

static void main_view_hide(void)
{
	if (main_view.screen != NULL) {
		main_view_printf("%d %s\n", __LINE__, __FUNCTION__);
		// 隐藏当前屏幕
		lv_obj_add_flag(main_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

static void main_view_show(void)
{
	if (main_view.screen != NULL) {
		main_view_printf("%d %s\n", __LINE__, __FUNCTION__);
		// 显示当前屏幕
		lv_obj_clear_flag(main_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

// 事件回调函数
static void main_view_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	switch(code) {
		case LV_EVENT_SCREEN_LOADED:
			// 屏幕加载完成后的处理
			break;
		case LV_EVENT_SCREEN_UNLOADED:
			// 屏幕卸载时的处理
			break;
		default:
			break;
	}
}

// 事件处理函数
static void button_event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * btn = lv_event_get_target(e);

	main_view_printf("(code=%d) btn=%p\n", code, btn);

	// 同时处理 CLICKED 和 SHORT_CLICKED 事件
	if(code == LV_EVENT_CLICKED || code == LV_EVENT_SHORT_CLICKED) {
		if(btn == g_main_ui.buttons.music) {
			main_view_printf("checkpoint music_view\n");
			view_manager_switch_to("music_view");
		}
		else if(btn == g_main_ui.buttons.video) {
			main_view_printf("checkpoint video_view\n");
			view_manager_switch_to("video_view");
		}
		else if(btn == g_main_ui.buttons.photo) {
			main_view_printf("checkpoint photo_view\n");
			view_manager_switch_to("photo_view");
		}
		else if(btn == g_main_ui.buttons.book) {
			main_view_printf("checkpoint book_view\n");
			view_manager_switch_to("book_view");
		}
		else if(btn == g_main_ui.buttons.setup) {
			main_view_printf("checkpoint setup_view\n");
			view_manager_switch_to("setup_view");
		}
	}
}

static lv_obj_t * create_app_button(lv_obj_t *parent, const char *icon_path, const char *label_text)
{
	// 创建一个容器作为按钮的根对象
	lv_obj_t * cont = lv_obj_create(parent);
	lv_obj_remove_style_all(cont);  // 移除容器的所有默认样式

	// 添加按钮属性和样式
	lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);  // 设置可点击
	lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);  // 透明背景
	lv_obj_set_style_bg_opa(cont, LV_OPA_50, LV_STATE_PRESSED);  // 按下时的背景透明度
	lv_obj_set_style_bg_color(cont, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 按下时的背景颜色

	// 创建图标按钮
	lv_obj_t *icon = lv_image_create(cont);
	lv_image_set_src(icon, icon_path);
	lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 0);

	// 设置容器大小
	lv_obj_set_size(cont, lv_image_get_src_width(icon), lv_image_get_src_height(icon) + 20);  // 增大容器尺寸，确保有足够空间

	// 创建标签
	lv_obj_t * label = lv_label_create(cont);
	lv_label_set_text(label, label_text);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
	lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

	// 释放按键时触发 event
	lv_obj_add_event_cb(cont, button_event_handler, LV_EVENT_CLICKED, NULL);

	return cont;
}

// 主界面初始化
static void main_view_init(void)
{
	// 创建主屏幕
	main_view.screen = lv_obj_create(lv_screen_active());
	lv_obj_set_size(main_view.screen, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));

	// 移除屏幕的默认样式（边框、背景等）
	lv_obj_set_style_border_width(main_view.screen, 0, 0);
	lv_obj_set_style_bg_opa(main_view.screen, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(main_view.screen, 0, 0);
	lv_obj_set_style_radius(main_view.screen, 0, 0);

	// 设置背景
	g_main_ui.background = lv_image_create(main_view.screen);
	lv_image_set_src(g_main_ui.background, "A:./resources/image/background.png");
	lv_obj_align(g_main_ui.background, LV_ALIGN_CENTER, 0, 0);
	lv_obj_move_background(g_main_ui.background);

	// 创建应用按钮
	// 50*50
	g_main_ui.buttons.music = create_app_button(main_view.screen, "A:./resources/image/icon/music_icon.png", "Music");
	g_main_ui.buttons.video = create_app_button(main_view.screen, "A:./resources/image/icon/video_icon.png", "Video");
	g_main_ui.buttons.photo = create_app_button(main_view.screen, "A:./resources/image/icon/photo_icon.png", "Photo");
	g_main_ui.buttons.book = create_app_button(main_view.screen, "A:./resources/image/icon/book_icon.png", "Book");
	g_main_ui.buttons.setup = create_app_button(main_view.screen, "A:./resources/image/icon/setup_icon.png", "Setup");

	// 布局应用按钮
	lv_obj_align(g_main_ui.buttons.music, LV_ALIGN_TOP_LEFT, 50, 50);
	lv_obj_align(g_main_ui.buttons.video, LV_ALIGN_TOP_LEFT, 200, 50);
	lv_obj_align(g_main_ui.buttons.photo, LV_ALIGN_TOP_LEFT, 350, 50);
	lv_obj_align(g_main_ui.buttons.book, LV_ALIGN_TOP_LEFT, 50, 200);
	lv_obj_align(g_main_ui.buttons.setup, LV_ALIGN_TOP_LEFT, 200, 200);

	// 创建时间日期显示
	g_main_ui.info.time = lv_label_create(main_view.screen);
	g_main_ui.info.date = lv_label_create(main_view.screen);

	lv_label_set_text(g_main_ui.info.time, "12:00");
	lv_obj_set_style_text_color(g_main_ui.info.time, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(g_main_ui.info.time, &lv_font_montserrat_16, 0);

	lv_label_set_text(g_main_ui.info.date, "2025-11-01");
	lv_obj_set_style_text_color(g_main_ui.info.date, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(g_main_ui.info.date, &lv_font_montserrat_16, 0);

	lv_obj_align(g_main_ui.info.time, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_align(g_main_ui.info.date, LV_ALIGN_TOP_MID, 0, 20);

}

view_t main_view = {
	.name = "main_view",
	.init = main_view_init,
	.hide = main_view_hide,
	.show = main_view_show,
	.destroy = main_view_destroy,
	.event_cb = main_view_event_cb,
	.screen = NULL,
	.initialized = 0
};
