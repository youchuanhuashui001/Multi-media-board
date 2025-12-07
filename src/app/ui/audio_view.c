#include "audio_view.h"
#include "audio_manager.h"
#include "common.h"
#include "src/core/lv_obj.h"
#include "view_manager.h"
#include <stdio.h>
#include <stdlib.h>

#define AUDIO_LIBRARY_DIR "./resources/audio/"
#define AUDIO_COVER_DEFAULT "./resources/image/audio/def_cover.png"

// ========== UI 元素定义 ==========
typedef struct {
	// 主屏幕在 view_management 中管理
//	lv_obj_t *screen;     // 主屏幕
	lv_obj_t *background; // 背景图片

	// 歌曲信息
	lv_obj_t *cover_img;    // 封面图
	lv_obj_t *title_label;  // 歌名
	lv_obj_t *artist_label; // 歌手

	// 歌词
	lv_obj_t *lyric_label; // 歌词显示

	// 进度控制
	lv_obj_t *progress_bar;       // 进度条
	lv_obj_t *time_current_label; // 当前时间
	lv_obj_t *time_total_label;   // 总时间

	// 控制按钮
	lv_obj_t *mode_btn;     // 模式按钮
	lv_obj_t *prev_btn;     // 上一首
	lv_obj_t *play_btn;     // 播放/暂停
	lv_obj_t *next_btn;     // 下一首
	lv_obj_t *playlist_btn; // 播放列表

	// 播放列表弹窗
	lv_obj_t *playlist_popup; // 弹窗容器
	lv_obj_t *playlist_table; // 列表

	// 定时器
	lv_timer_t *update_timer; // UI 更新定时器

	// 字体
	const lv_font_t *font_20;
	const lv_font_t *font_24;
	const lv_font_t *font_28;
} audio_view_t;

static audio_view_t g_audio_view = {0};

// ========== 辅助函数 ==========

// 格式化时间 (ms -> mm:ss)
static void format_time(int64_t ms, char *buf, int buf_size)
{
	int total_sec = ms / 1000;
	int min = total_sec / 60;
	int sec = total_sec % 60;
	snprintf(buf, buf_size, "%02d:%02d", min, sec);
}

// 更新播放按钮图标
static void update_play_button(void) {
	player_status_t status = audio_manager_get_status();
	lv_obj_t *label = lv_obj_get_child(g_audio_view.play_btn, 0);

	if (status == PLAYER_STATUS_PLAYING) {
		lv_label_set_text(label, "||");
	} else {
		lv_label_set_text(label, ">");
	}
}

// 更新歌曲信息显示
static void update_song_info(void)
{
	music_info_t *head = audio_manager_get_playlist_head();
	if (!head) {
		lv_label_set_text(g_audio_view.title_label, "无歌曲");
		lv_label_set_text(g_audio_view.artist_label, "");
		lv_label_set_text(g_audio_view.lyric_label, "");

		// 清除封面（显示默认背景）
		lv_obj_set_style_bg_img_src(g_audio_view.cover_img, AUDIO_COVER_DEFAULT, 0);
		lv_obj_set_style_bg_image_opa(g_audio_view.cover_img, LV_OPA_COVER, 0);
		return;
	}

	lv_label_set_text(g_audio_view.title_label, head->title ? head->title : "未知");
	lv_label_set_text(g_audio_view.artist_label, head->artist ? head->artist : "未知歌手");

	// 更新封面图
	if (head->cover_path) {
		// 使用文件路径加载封面图片
		lv_obj_set_style_bg_img_src(g_audio_view.cover_img, head->cover_path, 0);
		lv_obj_set_style_bg_image_opa(g_audio_view.cover_img, LV_OPA_COVER, 0);
		printf("Loading cover: %s\n", head->cover_path);
	} else {
		// 没有封面时显示默认颜色
		lv_obj_set_style_bg_img_src(g_audio_view.cover_img, AUDIO_COVER_DEFAULT, 0);
		lv_obj_set_style_bg_image_opa(g_audio_view.cover_img, LV_OPA_COVER, 0);
	}

	// 更新歌词显示 (显示7行)
	if (head->lyrics) {
		char lrc_buf[1024];
		snprintf(lrc_buf, sizeof(lrc_buf), "%s\n%s\n%s\n%s\n%s\n%s\n%s\n", head->lyrics->lines[0].text, \
			head->lyrics->lines[1].text, head->lyrics->lines[2].text, head->lyrics->lines[3].text, \
			head->lyrics->lines[4].text, head->lyrics->lines[5].text, head->lyrics->lines[6].text);
		lv_label_set_text(g_audio_view.lyric_label, lrc_buf);
	} else {
		lv_label_set_text(g_audio_view.lyric_label, "暂无歌词");
	}

	// 更新时间
	if (head->duration_ms > 0) {
		char time_buf[16];
		format_time(head->duration_ms, time_buf, sizeof(time_buf));
		lv_label_set_text(g_audio_view.time_total_label, time_buf);
	} else {
		lv_label_set_text(g_audio_view.time_total_label, "00:00");
	}
}

// 更新进度条和时间
static void update_progress(void)
{
	int64_t pos = audio_manager_get_position();
	int64_t dur = audio_manager_get_duration();

	char time_buf[16];

	// 更新当前时间
	format_time(pos, time_buf, sizeof(time_buf));
	lv_label_set_text(g_audio_view.time_current_label, time_buf);

	// 更新总时间
	format_time(dur, time_buf, sizeof(time_buf));
	lv_label_set_text(g_audio_view.time_total_label, time_buf);

	// 更新进度条 (slider)
	if (dur > 0) {
		int percentage = (int)((pos * 100) / dur);
		lv_slider_set_value(g_audio_view.progress_bar, percentage, LV_ANIM_OFF);
	}
}

// 更新歌词显示
static void update_lyrics(void)
{
	music_info_t *info = audio_manager_get_current_info();
	if (!info || !info->lyrics) {
		return;
	}

	int64_t pos = audio_manager_get_position();
	lyric_info_t *lyrics = info->lyrics;

	if (lyrics->count == 0) {
		return;
	}

	// 使用静态变量记住上次的索引，避免每次从头查找
	// TODO: 切歌时没有重置
	static int last_index = 0;
	int current_index = last_index;

	// 从上次位置开始查找当前歌词
	for (int i = last_index; i < lyrics->count; i++) {
		if (lyrics->lines[i].time_ms <= pos) {
			current_index = i;
		} else {
			break;
		}
	}
	last_index = current_index;

	// 计算显示窗口：前3行 + 当前行 + 后3行 = 7行
	int start = current_index - 3;
	int end = current_index + 3;

	// 简单的边界处理
	if (start < 0) start = 0;
	if (end >= lyrics->count) end = lyrics->count - 1;

	// 构建歌词文本
	char lrc_buf[1024] = "";
	char *p = lrc_buf;
	int remaining = sizeof(lrc_buf);

	for (int i = start; i <= end; i++) {
		if (lyrics->lines[i].text) {
			int written = snprintf(p, remaining, "%s%s", 
			                       lyrics->lines[i].text,
			                       (i < end) ? "\n" : "");
			if (written > 0 && written < remaining) {
				p += written;
				remaining -= written;
			} else {
				break;  // 缓冲区不足
			}
		}
	}

	lv_label_set_text(g_audio_view.lyric_label, lrc_buf);
}

// ========== 事件回调 ==========

// 播放/暂停按钮
static void play_btn_event_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		audio_manager_play();
	}
}

// 上一首按钮
static void prev_btn_event_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		audio_manager_play_prev();
	}
}

// 下一首按钮
static void next_btn_event_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		audio_manager_play_next();
	}
}

// 模式按钮
static void mode_btn_event_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		play_mode_t current_mode = audio_manager_get_mode();
		play_mode_t new_mode = (current_mode + 1) % 3;
		audio_manager_set_mode(new_mode);

		lv_obj_t *label = lv_obj_get_child(g_audio_view.mode_btn, 0);
		switch (new_mode) {
		case PLAY_MODE_LOOP_LIST:
			lv_label_set_text(label, "列表");
			break;
		case PLAY_MODE_LOOP_SINGLE:
			lv_label_set_text(label, "单曲");
			break;
		case PLAY_MODE_SHUFFLE:
			lv_label_set_text(label, "随机");
			break;
		}
	}
}

// 进度条滑动事件 (用户拖动 seek)
static void progress_slider_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	// 用户松手时触发 seek
	if (code == LV_EVENT_RELEASED) {
		lv_obj_t *slider = lv_event_get_target(e);
		int percent = lv_slider_get_value(slider);
		audio_manager_seek_percent(percent);
	}
}

// 播放列表按钮
static void playlist_btn_event_cb(lv_event_t *e) {
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		if (g_audio_view.playlist_popup) {
			lv_obj_remove_flag(g_audio_view.playlist_popup, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

// 播放列表项点击
static void playlist_item_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	// LVGL table 使用 VALUE_CHANGED 事件表示单元格被点击/选中
	if (code == LV_EVENT_VALUE_CHANGED) {
		lv_obj_t *table = lv_event_get_target(e);
		uint32_t row, col;
		lv_table_get_selected_cell(table, &row, &col);

		// 检查是否获取到有效的行号
		// 注意：第 0 行是表头，实际歌曲从第 1 行开始
		if (row != LV_TABLE_CELL_NONE && row > 0) {
			int song_index = row - 1;  // 转换为歌曲索引 (0-based)
			printf("playlist_item_event_cb: row = %u, song_index = %d\n", row, song_index);

			// TODO:播放选中的歌曲
			//audio_manager_play_at_index(song_index);

			// 关闭弹窗
			if (g_audio_view.playlist_popup) {
				lv_obj_add_flag(g_audio_view.playlist_popup, LV_OBJ_FLAG_HIDDEN);
			}
		}
	}
}

// 定时器回调
static void update_timer_cb(lv_timer_t *timer)
{
	player_status_t status = audio_manager_get_status();

	if (status == PLAYER_STATUS_PLAYING) {
		update_progress();
		update_lyrics();
	}

	update_play_button();
}

// ========== 创建播放列表弹窗 ==========
static void create_playlist_popup(void)
{
	// 创建遮罩层
	g_audio_view.playlist_popup = lv_obj_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.playlist_popup, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_color(g_audio_view.playlist_popup, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(g_audio_view.playlist_popup, LV_OPA_60, 0);
	lv_obj_set_style_border_width(g_audio_view.playlist_popup, 0, 0);
	lv_obj_add_flag(g_audio_view.playlist_popup, LV_OBJ_FLAG_HIDDEN);

	// 创建列表容器 (居中)
	lv_obj_t *list_cont = lv_obj_create(g_audio_view.playlist_popup);
	lv_obj_set_size(list_cont, 800, 500);
	lv_obj_center(list_cont);
	lv_obj_set_style_bg_color(list_cont, lv_color_make(30, 30, 30), 0);
	lv_obj_set_style_radius(list_cont, 10, 0);

	// 创建表格
	g_audio_view.playlist_table = lv_table_create(list_cont);
	// 中文字体
	lv_obj_set_style_text_font(g_audio_view.playlist_table, g_audio_view.font_20, 0);
	lv_obj_set_size(g_audio_view.playlist_table, 760, 460);
	lv_obj_center(g_audio_view.playlist_table);

	// 设置列宽
	lv_table_set_column_width(g_audio_view.playlist_table, 0, 100); // 序号
	lv_table_set_column_width(g_audio_view.playlist_table, 1, 350); // 标题
	lv_table_set_column_width(g_audio_view.playlist_table, 2, 200); // 歌手
	lv_table_set_column_width(g_audio_view.playlist_table, 3, 100); // 时长

	// 设置标题
	lv_table_set_cell_value(g_audio_view.playlist_table, 0, 0, "序号");
	lv_table_set_cell_value(g_audio_view.playlist_table, 0, 1, "歌曲");
	lv_table_set_cell_value(g_audio_view.playlist_table, 0, 2, "歌手");
	lv_table_set_cell_value(g_audio_view.playlist_table, 0, 3, "时长");

	// 填充数据
	music_info_t *current = audio_manager_get_playlist_head();
	int row = 1;
	char buf[32];

	while (current) {
		// 序号
		snprintf(buf, sizeof(buf), "%d", row);
		lv_table_set_cell_value(g_audio_view.playlist_table, row, 0, buf);

		// 标题
		lv_table_set_cell_value(g_audio_view.playlist_table, row, 1, current->title ? current->title : "未知");

		// 歌手
		lv_table_set_cell_value(g_audio_view.playlist_table, row, 2, current->artist ? current->artist : "未知");

		// 时长
		format_time(current->duration_ms, buf, sizeof(buf));
		lv_table_set_cell_value(g_audio_view.playlist_table, row, 3, buf);

		current = current->next;
		row++;
	}

	lv_obj_add_event_cb(g_audio_view.playlist_table, playlist_item_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// ========== 公开接口实现 ==========

void audio_view_init(void)
{
	// 创建主屏幕
	audio_view.screen = lv_obj_create(lv_screen_active());
	lv_obj_set_size(audio_view.screen, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_border_width(audio_view.screen, 0, 0);
	lv_obj_set_style_bg_opa(audio_view.screen, LV_OPA_0, 0);
	lv_obj_set_style_pad_all(audio_view.screen, 0, 0);

	// 加载字体
	g_audio_view.font_20 = font_manager_get_freetype_font(20);
	g_audio_view.font_24 = font_manager_get_freetype_font(24);
	g_audio_view.font_28 = font_manager_get_freetype_font(28);

	if (!g_audio_view.font_20 || !g_audio_view.font_24 || !g_audio_view.font_28) {
		printf("Error: Failed to load FreeType fonts\n");
		return ;
	}

	// ===== 左侧：封面和歌曲信息 =====

	// 封面图 (占位符)
	g_audio_view.cover_img = lv_obj_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.cover_img, 300, 300);
	lv_obj_set_pos(g_audio_view.cover_img, 50, 50);
	lv_obj_set_style_bg_color(g_audio_view.cover_img, lv_color_make(60, 60, 60), 0);

	// 歌名
	g_audio_view.title_label = lv_label_create(audio_view.screen);
	lv_obj_set_style_text_font(g_audio_view.title_label, g_audio_view.font_28, 0);
	lv_obj_set_pos(g_audio_view.title_label, 50, 370);
	lv_obj_set_width(g_audio_view.title_label, 300);
	lv_label_set_text(g_audio_view.title_label, "歌名");

	// 歌手
	g_audio_view.artist_label = lv_label_create(audio_view.screen);
	lv_obj_set_style_text_font(g_audio_view.artist_label, g_audio_view.font_20, 0);
	lv_obj_set_style_text_color(g_audio_view.artist_label, lv_color_make(180, 180, 180), 0);
	lv_obj_set_pos(g_audio_view.artist_label, 50, 415);
	lv_label_set_text(g_audio_view.artist_label, "歌手");

	// ===== 右侧：歌词和控制 =====

	// 歌词区域
	g_audio_view.lyric_label = lv_label_create(audio_view.screen);
	lv_obj_set_style_text_font(g_audio_view.lyric_label, g_audio_view.font_24, 0);
	lv_obj_set_size(g_audio_view.lyric_label, 500, 400);
	lv_obj_set_pos(g_audio_view.lyric_label, 450, 100);
	lv_obj_set_style_text_align(g_audio_view.lyric_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_long_mode(g_audio_view.lyric_label, LV_LABEL_LONG_WRAP);
	lv_label_set_text(g_audio_view.lyric_label, "");

	// 进度条 (使用 slider 支持用户拖动 seek)
	g_audio_view.progress_bar = lv_slider_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.progress_bar, 500, 10);
	lv_obj_set_pos(g_audio_view.progress_bar, 450, 460);
	lv_slider_set_range(g_audio_view.progress_bar, 0, 100);
	lv_slider_set_value(g_audio_view.progress_bar, 0, LV_ANIM_OFF);
	lv_obj_add_event_cb(g_audio_view.progress_bar, progress_slider_event_cb, LV_EVENT_RELEASED, NULL);

	// 时间标签
	g_audio_view.time_current_label = lv_label_create(audio_view.screen);
	lv_obj_set_pos(g_audio_view.time_current_label, 400, 460);
	lv_label_set_text(g_audio_view.time_current_label, "00:00");

	g_audio_view.time_total_label = lv_label_create(audio_view.screen);
	lv_obj_set_pos(g_audio_view.time_total_label, 960, 460);
	lv_label_set_text(g_audio_view.time_total_label, "00:00");

	// 控制按钮 (底部居中排列)
	int btn_y = 510;
	int btn_spacing = 60;
	int btn_start_x = 500;

	// 模式按钮
	g_audio_view.mode_btn = lv_button_create(audio_view.screen);
	// TODO: 换成 icon
	lv_obj_set_style_text_font(g_audio_view.mode_btn, g_audio_view.font_20, 0);
	lv_obj_set_size(g_audio_view.mode_btn, 50, 50);
	lv_obj_set_pos(g_audio_view.mode_btn, btn_start_x, btn_y);
	lv_obj_add_event_cb(g_audio_view.mode_btn, mode_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *mode_label = lv_label_create(g_audio_view.mode_btn);
	lv_label_set_text(mode_label, "list");
	lv_obj_center(mode_label);

	// 上一首
	g_audio_view.prev_btn = lv_button_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.prev_btn, 50, 50);
	lv_obj_set_pos(g_audio_view.prev_btn, btn_start_x + btn_spacing, btn_y);
	lv_obj_add_event_cb(g_audio_view.prev_btn, prev_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *prev_label = lv_label_create(g_audio_view.prev_btn);
	lv_label_set_text(prev_label, "<<");
	lv_obj_center(prev_label);

	// 播放/暂停
	g_audio_view.play_btn = lv_button_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.play_btn, 60, 60);
	lv_obj_set_pos(g_audio_view.play_btn, btn_start_x + btn_spacing * 2, btn_y - 5);
	lv_obj_add_event_cb(g_audio_view.play_btn, play_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *play_label = lv_label_create(g_audio_view.play_btn);
	lv_obj_set_style_text_font(play_label, g_audio_view.font_28, 0);
	lv_label_set_text(play_label, ">");
	lv_obj_center(play_label);

	// 下一首
	g_audio_view.next_btn = lv_button_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.next_btn, 50, 50);
	lv_obj_set_pos(g_audio_view.next_btn, btn_start_x + btn_spacing * 3 + 10, btn_y);
	lv_obj_add_event_cb(g_audio_view.next_btn, next_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *next_label = lv_label_create(g_audio_view.next_btn);
	lv_label_set_text(next_label, ">>");
	lv_obj_center(next_label);

	// 播放列表按钮
	g_audio_view.playlist_btn = lv_button_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.playlist_btn, 80, 40);
	lv_obj_set_pos(g_audio_view.playlist_btn, 880, 520);
	lv_obj_add_event_cb(g_audio_view.playlist_btn, playlist_btn_event_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *list_label = lv_label_create(g_audio_view.playlist_btn);
	lv_obj_set_style_text_font(list_label, g_audio_view.font_20, 0);
	lv_label_set_text(list_label, "播放列表");
	lv_obj_center(list_label);

	// 初始化 audio manager
	audio_manager_init();

	// 默认扫描目录
	audio_manager_scan_dir(AUDIO_LIBRARY_DIR);

	// 创建播放列表弹窗
	create_playlist_popup();

	// 更新初始显示(默认显示扫描到的第一首歌曲)
	update_song_info();

	// 创建定时器 (每 100ms 更新)
	g_audio_view.update_timer = lv_timer_create(update_timer_cb, 100, NULL);

	printf("Audio View initialized\n");
}

void audio_view_show(void) {
	if (audio_view.screen) {
		lv_obj_remove_flag(audio_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

void audio_view_hide(void) {
	if (audio_view.screen) {
		lv_obj_add_flag(audio_view.screen, LV_OBJ_FLAG_HIDDEN);
	}
}

void audio_view_destroy(void) {
	if (g_audio_view.update_timer) {
		lv_timer_delete(g_audio_view.update_timer);
		g_audio_view.update_timer = NULL;
	}

	if (audio_view.screen) {
		lv_obj_delete(audio_view.screen);
		audio_view.screen = NULL;
	}

	audio_manager_stop();
}

// ========== View 定义 ==========
view_t audio_view = {
	.name = "audio_view",
	.init = audio_view_init,
	.destroy = audio_view_destroy,
	.hide = audio_view_hide,
	.show = audio_view_show,
	.screen = NULL,
	.event_cb = NULL,
	.next = NULL,
	.initialized = 0
};
