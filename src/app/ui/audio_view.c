#include "audio_view.h"
#include "audio_library.h"
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
	// --- 主屏幕 ---
	lv_obj_t *screen;
	lv_obj_t *background;

	// --- 左侧面板：信息 ---
	lv_obj_t *cover_img;         // 专辑封面
	lv_obj_t *title_label;       // 歌名
	lv_obj_t *artist_label;      // 歌手名

	// --- 右侧面板：歌词 ---
	lv_obj_t *lyric_cont;        // 歌词容器 (用于滚动/裁剪)
	lv_obj_t *lyric_label;       // 显示歌词文本的标签

	// --- 底部面板：控制 ---
	lv_obj_t *progress_bar;      // 进度条 (Slider)
	lv_obj_t *time_current_label;
	lv_obj_t *time_total_label;

	lv_obj_t *mode_btn;          // 模式切换
	lv_obj_t *prev_btn;
	lv_obj_t *play_btn;          // 播放/暂停切换
	lv_obj_t *next_btn;
	lv_obj_t *playlist_btn;      // 打开播放列表弹窗

	// --- 弹窗 ---
	lv_obj_t *playlist_popup;    // 容器 (模态)
	lv_obj_t *playlist_table;    // 歌曲列表表格
	// lv_obj_t *playlist_close_btn; // Step 4 will add this

	// --- 系统 ---
	lv_timer_t *update_timer;    // 100ms UI 更新
	
	// --- 字体 ---
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

// 更新模式按钮文本
static void update_mode_btn_text(void)
{
	play_mode_t mode = audio_manager_get_mode();
	lv_obj_t *label = lv_obj_get_child(g_audio_view.mode_btn, 0);

	switch (mode) {
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

// 初始化歌曲信息显示 (仅用于界面加载时)
static void init_song_info(void)
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
	} else {
		// 没有封面时显示默认颜色
		lv_obj_set_style_bg_img_src(g_audio_view.cover_img, AUDIO_COVER_DEFAULT, 0);
		lv_obj_set_style_bg_image_opa(g_audio_view.cover_img, LV_OPA_COVER, 0);
	}

	// 更新歌词显示 (显示前7行，第1行高亮)
	if (head->lyrics && head->lyrics->count > 0) {
		char lrc_buf[1024] = "";
		char *p = lrc_buf;
		int remaining = sizeof(lrc_buf);

		for (int i = 0; i < 7; i++) {
			if (i < head->lyrics->count && head->lyrics->lines[i].text) {
				// 第0行高亮 (#0000ff), 其他行黑色 (#000000)
				const char *color = (i == 0) ? "#0000ff " : "#000000 ";

				int written = snprintf(p, remaining, "%s%s%s#", 
				                       color,
				                       head->lyrics->lines[i].text,
				                       (i < 6) ? "\n" : "");

				if (written > 0 && written < remaining) {
					p += written;
					remaining -= written;
				} else {
					break;
				}
			} else {
				// 填充空行
				int written = snprintf(p, remaining, "\n");
				if (written > 0 && written < remaining) {
					p += written;
					remaining -= written;
				}
			}
		}
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
	if (!info || !info->lyrics || info->lyrics->count == 0) {
		lv_label_set_text(g_audio_view.lyric_label, "暂无歌词");
		return;
	}

	int64_t pos = audio_manager_get_position();
	lyric_info_t *lyrics = info->lyrics;

	// 状态管理
	static music_info_t *last_music_ptr = NULL;
	static int last_index = 0;
	static int64_t last_pos = 0;

	// 切歌或后退 seek 检测：重置索引
	if (info != last_music_ptr || pos < last_pos) {
		last_index = 0;
		last_music_ptr = info;
	}
	last_pos = pos;

	// 查找当前行 (从上次位置开始)
	int current_index = last_index;
	for (int i = last_index; i < lyrics->count; i++) {
		if (lyrics->lines[i].time_ms <= pos) {
			current_index = i;
		} else {
			break;
		}
	}
	last_index = current_index;

	// 构建显示窗口：前3行 + 当前行 + 后3行 = 7行
	int start = current_index - 3;
	int end = current_index + 3;

	char lrc_buf[2048] = "";
	char *p = lrc_buf;
	int remaining = sizeof(lrc_buf);

	for (int i = start; i <= end; i++) {
		if (i >= 0 && i < lyrics->count && lyrics->lines[i].text) {
			// 高亮当前行 (#0000ff), 其他行黑色 (#000000)
			const char *color = (i == current_index) ? "#0000ff " : "#000000 ";

			int written = snprintf(p, remaining, "%s%s%s#", 
			                       color,
			                       lyrics->lines[i].text,
			                       (i < end) ? "\n" : "");

			if (written > 0 && written < remaining) {
				p += written;
				remaining -= written;
			} else {
				break;
			}
		} else {
			// 填充空行以保持垂直居中
			int written = snprintf(p, remaining, "\n");
			if (written > 0 && written < remaining) {
				p += written;
				remaining -= written;
			}
		}
	}

	lv_label_set_text(g_audio_view.lyric_label, lrc_buf);
}

// 更新歌曲元信息:歌名、歌手、封面图
static void update_mete_data(void)
{
	static music_info_t *last_info = NULL;
	music_info_t *info = audio_manager_get_current_info();

	if (last_info != info) {

		last_info = info;

		if (!info) {
			lv_label_set_text(g_audio_view.title_label, "无歌曲");
			lv_label_set_text(g_audio_view.artist_label, "");
			lv_label_set_text(g_audio_view.lyric_label, "");

			// 清除封面（显示默认背景）
			lv_obj_set_style_bg_img_src(g_audio_view.cover_img, AUDIO_COVER_DEFAULT, 0);
			lv_obj_set_style_bg_image_opa(g_audio_view.cover_img, LV_OPA_COVER, 0);
			return;
		}

		lv_label_set_text(g_audio_view.title_label, info->title ? info->title : "未知");
		lv_label_set_text(g_audio_view.artist_label, info->artist ? info->artist : "未知歌手");

		// 更新封面图
		if (info->cover_path) {
			// 使用文件路径加载封面图片
			lv_obj_set_style_bg_img_src(g_audio_view.cover_img, info->cover_path, 0);
			lv_obj_set_style_bg_image_opa(g_audio_view.cover_img, LV_OPA_COVER, 0);
		} else {
			// 没有封面时显示默认颜色
			lv_obj_set_style_bg_img_src(g_audio_view.cover_img, AUDIO_COVER_DEFAULT, 0);
			lv_obj_set_style_bg_image_opa(g_audio_view.cover_img, LV_OPA_COVER, 0);
		}
	}


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
		update_mode_btn_text();
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
		update_mete_data();
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

// ========== 组件创建 (Step 1 Layout) ==========

static void create_left_panel(void)
{
	// 封面图 (300x300, pos: 50,50)
	g_audio_view.cover_img = lv_obj_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.cover_img, 300, 300);
	lv_obj_set_pos(g_audio_view.cover_img, 50, 50);
	lv_obj_set_style_bg_color(g_audio_view.cover_img, lv_color_make(60, 60, 60), 0);
	lv_obj_set_style_bg_img_src(g_audio_view.cover_img, AUDIO_COVER_DEFAULT, 0);

	// 歌名 (pos: 50,370)
	g_audio_view.title_label = lv_label_create(audio_view.screen);
	lv_obj_set_style_text_font(g_audio_view.title_label, g_audio_view.font_28, 0);
	lv_obj_set_pos(g_audio_view.title_label, 50, 370);
	lv_obj_set_width(g_audio_view.title_label, 300);
	lv_label_set_long_mode(g_audio_view.title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_label_set_text(g_audio_view.title_label, "歌名");

	// 歌手 (pos: 50,415)
	g_audio_view.artist_label = lv_label_create(audio_view.screen);
	lv_obj_set_style_text_font(g_audio_view.artist_label, g_audio_view.font_20, 0);
	lv_obj_set_style_text_color(g_audio_view.artist_label, lv_color_make(180, 180, 180), 0);
	lv_obj_set_pos(g_audio_view.artist_label, 50, 415);
	lv_label_set_text(g_audio_view.artist_label, "歌手");
}

static void create_right_panel(void)
{
	// 歌词容器 (500x430, pos: 450,0)
	g_audio_view.lyric_cont = lv_obj_create(audio_view.screen);
	lv_obj_set_size(g_audio_view.lyric_cont, 500, 450);
	lv_obj_set_pos(g_audio_view.lyric_cont, 450, 0);
	lv_obj_set_style_bg_opa(g_audio_view.lyric_cont, LV_OPA_0, 0); // 透明背景
	lv_obj_set_style_border_width(g_audio_view.lyric_cont, 0, 0);

	// 设置垂直滚动
	lv_obj_set_scroll_dir(g_audio_view.lyric_cont, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(g_audio_view.lyric_cont, LV_SCROLLBAR_MODE_OFF); // 隐藏滚动条
	// TODO: 后续可以通过上下滑动歌词 seek 到音乐对应的位置

	// 歌词标签 (Inside container)
	g_audio_view.lyric_label = lv_label_create(g_audio_view.lyric_cont);
	lv_obj_set_width(g_audio_view.lyric_label, 460); // 留出滚动条空间
	lv_obj_set_style_text_font(g_audio_view.lyric_label, g_audio_view.font_24, 0);
	lv_obj_set_style_text_align(g_audio_view.lyric_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_line_space(g_audio_view.lyric_label, 10, 0); // 增加行间距
	lv_label_set_long_mode(g_audio_view.lyric_label, LV_LABEL_LONG_WRAP);
	lv_label_set_recolor(g_audio_view.lyric_label, true); // 开启颜色解析
	lv_obj_center(g_audio_view.lyric_label);
	lv_label_set_text(g_audio_view.lyric_label, "暂无歌词");
}

static void create_controls(void)
{
	// 进度条 (pos: 450,460)
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

	// 按钮区域 (Bottom aligned)
	int btn_y = 510;
	int btn_spacing = 60; // Reduced spacing
	int btn_start_x = 550;

	// 模式按钮
	g_audio_view.mode_btn = lv_button_create(audio_view.screen);
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
}

void audio_view_init(void)
{
	// 创建主屏幕
	audio_view.screen = lv_obj_create(lv_screen_active());
	lv_obj_set_size(audio_view.screen, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_border_width(audio_view.screen, 0, 0);
	lv_obj_set_style_bg_opa(audio_view.screen, LV_OPA_0, 0);
	//TODO:考虑背景图片
//	lv_obj_set_style_bg_color(audio_view.screen, lv_color_black(), 0); // Black background
	lv_obj_set_style_pad_all(audio_view.screen, 0, 0);

	// 加载字体
	g_audio_view.font_20 = font_manager_get_freetype_font(20);
	g_audio_view.font_24 = font_manager_get_freetype_font(24);
	g_audio_view.font_28 = font_manager_get_freetype_font(28);

	if (!g_audio_view.font_20 || !g_audio_view.font_24 || !g_audio_view.font_28) {
		printf("Error: Failed to load FreeType fonts\n");
		return ;
	}

	// 初始化 audio manager
	audio_manager_init();
	audio_manager_scan_dir(AUDIO_LIBRARY_DIR);

	// 创建 UI 组件 (Split Layout)
	create_left_panel();
	create_right_panel();
	create_controls();

	// 创建播放列表弹窗 (Keep existing logic for now, just ensure it's created)
	create_playlist_popup();

	// 更新初始显示
	init_song_info();
	update_mode_btn_text();

	// 创建定时器
	g_audio_view.update_timer = lv_timer_create(update_timer_cb, 100, NULL);

	printf("Audio View initialized (Step 2 Icons)\n");
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
