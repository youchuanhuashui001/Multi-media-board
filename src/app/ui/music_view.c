#include "view_manager.h"

// Music 界面的UI元素
typedef struct {
	lv_obj_t *background;     // 背景图片
	lv_obj_t *label;

} music_ui_t;

// UI元素实例
static music_ui_t g_music_ui;

// 主界面销毁函数
static void music_view_destroy(void)
{
	// 清理资源
	if (music_view.screen != NULL) {
		lv_obj_del(music_view.screen);
		music_view.screen = NULL;
	}
}

static void music_view_hide(void)
{
	if (music_view.screen != NULL) {
		// 隐藏当前屏幕
		lv_obj_add_flag(music_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

static void music_view_show(void)
{
	if (music_view.screen != NULL) {
		// 显示当前屏幕
		lv_obj_clear_flag(music_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

// 事件回调函数
static void music_view_event_cb(lv_event_t *e)
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
#if 0

// 事件处理函数
static void button_event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * btn = lv_event_get_target(e);

	if(code == LV_EVENT_CLICKED) {
		if(btn == music_ui.buttons.music) {
			view_manager_switch_to("music_view");
		}
		else if(btn == music_ui.buttons.video) {
			view_manager_switch_to("video_view");
		}
		else if(btn == music_ui.buttons.photo) {
			view_manager_switch_to("photo_view");
		}
		else if(btn == music_ui.buttons.book) {
			view_manager_switch_to("book_view");
		}
		else if(btn == music_ui.buttons.settings) {
			view_manager_switch_to("settings_view");
		}
	}
}
#endif

// 主界面初始化
static void music_view_init(void)
{
	// 创建主屏幕
	music_view.screen = lv_obj_create(lv_screen_active());
	lv_obj_set_size(music_view.screen, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));

	printf("music_view.screen:%p\n", music_view.screen);

	// 移除屏幕的默认样式（边框、背景等）
	lv_obj_set_style_border_width(music_view.screen, 0, 0);
	lv_obj_set_style_bg_opa(music_view.screen, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(music_view.screen, 0, 0);
	lv_obj_set_style_radius(music_view.screen, 0, 0);




	// 设置背景
	g_music_ui.background = lv_image_create(music_view.screen);
	lv_image_set_src(g_music_ui.background, "A:./resources/image/music/music_view.png");
	lv_obj_align(g_music_ui.background, LV_ALIGN_CENTER, 0, 0);
	lv_obj_move_background(g_music_ui.background);

//	// 创建应用按钮
//	music_ui.buttons.music = create_app_button(music_ui.screen, "A:./resources/images/music_icon.png", "音乐");
//	music_ui.buttons.video = create_app_button(music_ui.screen, "A:./resources/images/video_icon.png", "视频");
//	music_ui.buttons.photo = create_app_button(music_ui.screen, "A:./resources/images/photo_icon.png", "图片");
//	music_ui.buttons.book = create_app_button(music_ui.screen, "A:./resources/images/book_icon.png", "电子书");
//	music_ui.buttons.settings = create_app_button(music_ui.screen, "A:./resources/images/settings_icon.png", "设置");
//
//	// 布局应用按钮
//	lv_obj_align(music_ui.buttons.music, LV_ALIGN_TOP_LEFT, 50, 50);
//	lv_obj_align(music_ui.buttons.video, LV_ALIGN_TOP_LEFT, 200, 50);
//	lv_obj_align(music_ui.buttons.photo, LV_ALIGN_TOP_LEFT, 350, 50);
//	lv_obj_align(music_ui.buttons.book, LV_ALIGN_TOP_LEFT, 50, 200);
//	lv_obj_align(music_ui.buttons.settings, LV_ALIGN_TOP_LEFT, 200, 200);



//	lv_obj_align(music_ui.info.time, LV_ALIGN_TOP_RIGHT, -20, 20);
//	lv_obj_align(music_ui.info.date, LV_ALIGN_TOP_RIGHT, -20, 50);
	printf("music view init finish.\n");
}

view_t music_view = {
	.name = "music_view",
	.init = music_view_init,
	.hide = music_view_hide,
	.show = music_view_show,
	.destroy = music_view_destroy,
	.event_cb = music_view_event_cb,
	.screen = NULL,
	.initialized = 0
};
