#include "common.h"
#include "src/misc/lv_area.h"
#include "src/misc/lv_types.h"
#include "src/widgets/label/lv_label.h"
#include "view_manager.h"
#include <stdio.h>

extern int audio_player_init(const char *audio_path, const char *lrc_path, lv_obj_t *lrc_label);
extern void player_start(void);
extern void player_stop(void);
extern void player_destroy(void);

// Music 界面的UI元素
typedef struct {
	lv_obj_t *background;          // 背景图片
	lv_obj_t *cover_img;           // 封面图片
	lv_obj_t *song_name_label;     // 歌名标签
	lv_obj_t *artist_label;        // 歌手标签
	lv_obj_t *progress_bar;        // 进度条
	lv_obj_t *progress_time_label; // 当前进度时间
	lv_obj_t *total_time_label;    // 总时间标签
	lv_obj_t *prev_btn;            // 上一首按钮
	lv_obj_t *play_btn;            // 播放/暂停按钮
	lv_obj_t *next_btn;            // 下一首按钮
	lv_obj_t *playlist_btn;        // 播放列表按钮
	lv_obj_t *prev_btn_label;      // <<
	lv_obj_t *play_btn_label;      // >
	lv_obj_t *next_btn_label;      // >>
	lv_obj_t *playlist_label;      // 播放列表
	lv_obj_t *lrc_label;           // 歌词
	lv_font_t *font_18;            // freetype 字体
	lv_font_t *font_24;            // freetype 字体
	lv_font_t *font_48;            // freetype 字体
} music_ui_t;

// UI元素实例
static music_ui_t g_music_ui;

// 播放状态标志
static int is_playing = 0;

// 按钮事件处理函数
static void prev_btn_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED) {
		printf("Previous song\n");
		// TODO: 实现切换上一首的逻辑
	}
}

static void play_btn_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED) {
		is_playing = !is_playing;
		if (is_playing) {
			lv_label_set_text(g_music_ui.play_btn_label, "||");
			player_start();
			printf("Playing\n");
		} else {
			lv_label_set_text(g_music_ui.play_btn_label, ">");
			player_stop();
			printf("Paused\n");
		}
		// TODO: 实现播放/暂停逻辑
	}
}

static void next_btn_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED) {
		printf("Next song\n");
		// TODO: 实现切换下一首的逻辑
	}
}

static void playlist_btn_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED) {
		printf("Show playlist\n");
		// TODO: 实现显示播放列表的逻辑
	}
}

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
		lv_obj_add_flag(music_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

static void music_view_show(void)
{
	if (music_view.screen != NULL) {
		lv_obj_clear_flag(music_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

// 事件回调函数
static void music_view_event_cb(lv_event_t *e)
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
static void music_view_init(void)
{
	// 创建主屏幕
	music_view.screen = lv_obj_create(lv_screen_active());
	lv_obj_set_size(music_view.screen, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));

	// 移除屏幕的默认样式
	lv_obj_set_style_border_width(music_view.screen, 0, 0);
	lv_obj_set_style_bg_opa(music_view.screen, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(music_view.screen, 0, 0);
	lv_obj_set_style_radius(music_view.screen, 0, 0);

	// 设置背景
	g_music_ui.background = lv_image_create(music_view.screen);
	lv_image_set_src(g_music_ui.background, "A:./resources/image/audio/audio_background.png");
	lv_obj_align(g_music_ui.background, LV_ALIGN_CENTER, 0, 0);
	lv_obj_move_background(g_music_ui.background);

	g_music_ui.font_18 = font_manager_get_freetype_font(18);
	if (g_music_ui.font_18 == NULL) {
		LV_LOG_ERROR("Failed to get FreeType font\n");
		return;
	}

	g_music_ui.font_24 = font_manager_get_freetype_font(24);
	if (g_music_ui.font_24 == NULL) {
		LV_LOG_ERROR("Failed to get FreeType font\n");
		return;
	}

	g_music_ui.font_48 = font_manager_get_freetype_font(48);
	if (g_music_ui.font_48 == NULL) {
		LV_LOG_ERROR("Failed to get FreeType font\n");
		return;
	}

	// ===== 上半部分：封面、歌名、歌手 =====
	// 创建封面图片容器
//	g_music_ui.cover_img = lv_image_create(music_view.screen);
//	lv_image_set_src(g_music_ui.cover_img, "A:./resources/image/music/cover.png");
//	lv_obj_set_size(g_music_ui.cover_img, 120, 120);
//	lv_obj_align(g_music_ui.cover_img, LV_ALIGN_TOP_LEFT, 20, 40);

	static lv_style_t style_48;
	lv_style_init(&style_48);
	lv_style_set_text_font(&style_48, g_music_ui.font_48);

	static lv_style_t style_24;
	lv_style_init(&style_24);
	lv_style_set_text_font(&style_24, g_music_ui.font_24);
//	lv_style_set_text_color(&style_32, lv_color_make(0xff, 0xff, 0xff));

	static lv_style_t style_18;
	lv_style_init(&style_18);
	lv_style_set_text_font(&style_18, g_music_ui.font_18);
//	lv_style_set_text_color(&style_20, lv_color_make(0xff, 0xff, 0xff));


	// 歌名、歌手
	g_music_ui.song_name_label = lv_label_create(music_view.screen);
//	lv_obj_set_width(g_music_ui.song_name_label, 200);
	lv_obj_add_style(g_music_ui.song_name_label, &style_24, 0);
	lv_obj_set_size(g_music_ui.song_name_label, 120, 60);
	lv_obj_align(g_music_ui.song_name_label, LV_ALIGN_TOP_LEFT, 120, 520);
//	lv_label_set_long_mode(g_music_ui.song_name_label, LV_LABEL_LONG_WRAP);
	lv_label_set_text(g_music_ui.song_name_label, "找自己\n陶喆");

	// ==== 歌词部分 ====
	g_music_ui.lrc_label = lv_label_create(music_view.screen);
	lv_obj_add_style(g_music_ui.lrc_label, &style_24, 0);
	lv_obj_set_size(g_music_ui.lrc_label, 400, 450);
	lv_label_set_long_mode(g_music_ui.lrc_label, LV_LABEL_LONG_WRAP);
	lv_obj_align(g_music_ui.lrc_label, LV_ALIGN_TOP_LEFT, 310, 30);
	lv_obj_set_style_text_align(g_music_ui.lrc_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_text(g_music_ui.lrc_label, "");

	// ===== 中间部分：进度条 =====
	// 进度条
	g_music_ui.progress_bar = lv_bar_create(music_view.screen);
	lv_obj_set_size(g_music_ui.progress_bar, 500, 10);
	lv_obj_align(g_music_ui.progress_bar, LV_ALIGN_TOP_LEFT, 262, 560);
	// TODO:
	lv_bar_set_value(g_music_ui.progress_bar, 30, LV_ANIM_OFF);

	// 当前进度时间
	g_music_ui.progress_time_label = lv_label_create(music_view.screen);
	//TODO:
	lv_label_set_text(g_music_ui.progress_time_label, "1:30");
	lv_obj_align(g_music_ui.progress_time_label, LV_ALIGN_TOP_LEFT, 230, 560);

	// 总时间标签
	g_music_ui.total_time_label = lv_label_create(music_view.screen);
	lv_label_set_text(g_music_ui.total_time_label, "5:00");
	lv_obj_align(g_music_ui.total_time_label, LV_ALIGN_TOP_LEFT, 800, 560);

	// ===== 下半部分：控制按钮 =====
	// 上一首按钮
	g_music_ui.prev_btn = lv_button_create(music_view.screen);
	lv_obj_set_size(g_music_ui.prev_btn, 30, 30);
	lv_obj_align(g_music_ui.prev_btn, LV_ALIGN_TOP_LEFT, 460, 520);
	lv_obj_add_event_cb(g_music_ui.prev_btn, prev_btn_event_handler, LV_EVENT_CLICKED, NULL);

	g_music_ui.prev_btn_label = lv_label_create(g_music_ui.prev_btn);
	lv_label_set_text(g_music_ui.prev_btn_label, "<<");
	lv_obj_center(g_music_ui.prev_btn_label);

	// 播放/暂停按钮
	g_music_ui.play_btn = lv_button_create(music_view.screen);
	lv_obj_set_size(g_music_ui.play_btn, 30, 30);
	lv_obj_align(g_music_ui.play_btn, LV_ALIGN_TOP_LEFT,500, 520);
	lv_obj_add_event_cb(g_music_ui.play_btn, play_btn_event_handler, LV_EVENT_CLICKED, NULL);

	g_music_ui.play_btn_label = lv_label_create(g_music_ui.play_btn);
	lv_label_set_text(g_music_ui.play_btn_label, ">");
	lv_obj_center(g_music_ui.play_btn_label);

	// 下一首按钮
	g_music_ui.next_btn = lv_button_create(music_view.screen);
	lv_obj_set_size(g_music_ui.next_btn, 30, 30);
	lv_obj_align(g_music_ui.next_btn, LV_ALIGN_TOP_LEFT, 540, 520);
	lv_obj_add_event_cb(g_music_ui.next_btn, next_btn_event_handler, LV_EVENT_CLICKED, NULL);

	g_music_ui.next_btn_label = lv_label_create(g_music_ui.next_btn);
	lv_label_set_text(g_music_ui.next_btn_label, ">>");
	lv_obj_center(g_music_ui.next_btn_label);

	// 播放列表按钮（右下角）
	g_music_ui.playlist_btn = lv_button_create(music_view.screen);
	lv_obj_set_size(g_music_ui.playlist_btn, 100, 30);
	lv_obj_align(g_music_ui.playlist_btn, LV_ALIGN_TOP_LEFT, 850, 560);
	lv_obj_add_event_cb(g_music_ui.playlist_btn, playlist_btn_event_handler, LV_EVENT_CLICKED, NULL);

	g_music_ui.playlist_label = lv_label_create(g_music_ui.playlist_btn);
	lv_obj_add_style(g_music_ui.playlist_label, &style_24, 0);
	lv_label_set_text(g_music_ui.playlist_label, "播放列表");
	lv_obj_center(g_music_ui.playlist_label);


	audio_player_init("./resources/audio/zhaoziji.mp3", "./resources/audio/zhaoziji.lrc", g_music_ui.lrc_label);


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
