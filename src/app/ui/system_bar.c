/**
 * @file system_bar.c
 * @brief 全局控制中心实现
 */

#include "system_bar.h"
#include "view_manager.h"
#include "common.h"
#include <stdio.h>
#include <time.h>

// ========== UI 结构体 ==========
typedef struct {
	lv_obj_t *gesture_trigger; // 全局手势触发区（屏幕顶部透明条，始终可见）
	lv_obj_t *overlay;         // 遮罩层（全屏，用于点击外部关闭）
	lv_obj_t *panel;           // 控制中心面板
	lv_obj_t *time_label;      // 时间显示
	lv_obj_t *date_label;      // 日期显示

	// 系统控制
	lv_obj_t *home_btn;        // 主页按钮

	// 字体
	lv_font_t *font_time;      // 时间字体（大）
	lv_font_t *font_normal;    // 普通字体

	int is_visible;            // 是否可见
} system_bar_t;

static system_bar_t g_system_bar;

// ========== 内部函数声明 ==========
static void create_control_center(void);
static void create_gesture_trigger(void);  // 创建全局手势触发区
static void gesture_trigger_event_cb(lv_event_t *e);  // 顶部手势触发区事件
static void home_btn_event_cb(lv_event_t *e);
static void overlay_event_cb(lv_event_t *e);      // 遮罩层点击事件
static void panel_gesture_event_cb(lv_event_t *e); // 面板上滑手势
static void time_update_timer_cb(lv_timer_t *timer); // 定时器回调

// ========== 公共接口实现 ==========

void system_bar_init(void)
{
	memset(&g_system_bar, 0, sizeof(system_bar_t));

	// 获取字体（时间使用大字体）
	g_system_bar.font_time = font_manager_get_freetype_font(48);
	g_system_bar.font_normal = font_manager_get_freetype_font(20);

	create_control_center();   //  创建控制中心
	create_gesture_trigger();  // 创建全局手势触发区

	// 创建定时器：每分钟（60000ms）更新一次时间
	lv_timer_create(time_update_timer_cb, 60000, NULL);

	// 首次更新时间
	system_bar_update_time();

	printf("system_bar_init: 控制中心初始化完成\n");
}

void system_bar_show(void)
{
	if (!g_system_bar.overlay) return;

	// 更新信息
	system_bar_update_time();

	// 显示遮罩层和面板
	lv_obj_clear_flag(g_system_bar.overlay, LV_OBJ_FLAG_HIDDEN);
	g_system_bar.is_visible = 1;

	printf("system_bar_show: 控制中心已显示\n");
}

void system_bar_hide(void)
{
	if (!g_system_bar.overlay) return;

	lv_obj_add_flag(g_system_bar.overlay, LV_OBJ_FLAG_HIDDEN);
	g_system_bar.is_visible = 0;

	printf("system_bar_hide: 控制中心已隐藏\n");
}

void system_bar_update_time(void)
{
	if (!g_system_bar.time_label) return;

	// 标准的 C 库函数，用于获取当前时间，因此需要系统是准的
	// TODO:需要写一个脚本，在系统启动的时候，从网络获取时间，然后设置系统时间
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);

	char time_buf[16];
	char date_buf[32];

	strftime(time_buf, sizeof(time_buf), "%H:%M", tm_info);
	strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %A", tm_info);

	lv_label_set_text(g_system_bar.time_label, time_buf);
	lv_label_set_text(g_system_bar.date_label, date_buf);
}

int system_bar_is_visible(void)
{
	return g_system_bar.is_visible;
}

// ========== 内部函数实现 ==========

static void create_control_center(void)
{
	// 获取屏幕尺寸
	int screen_w = lv_display_get_horizontal_resolution(NULL);
	int screen_h = lv_display_get_vertical_resolution(NULL);

	// 1. 创建全屏控制中心面板（直接作为遮罩层）
	g_system_bar.overlay = lv_obj_create(lv_layer_top());
	lv_obj_remove_style_all(g_system_bar.overlay);
	lv_obj_set_size(g_system_bar.overlay, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_color(g_system_bar.overlay, lv_color_make(20, 25, 35), 0);
	lv_obj_set_style_bg_opa(g_system_bar.overlay, LV_OPA_COVER, 0);

	// 面板引用（全屏模式下 panel = overlay）
	g_system_bar.panel = g_system_bar.overlay;

	// 添加上滑关闭手势
	lv_obj_add_event_cb(g_system_bar.overlay, panel_gesture_event_cb, LV_EVENT_ALL, NULL);

	// ===== 左上角：时间显示 =====
	g_system_bar.time_label = lv_label_create(g_system_bar.overlay);
	lv_obj_set_style_text_font(g_system_bar.time_label, g_system_bar.font_time, 0);
	lv_obj_set_style_text_color(g_system_bar.time_label, lv_color_white(), 0);
	lv_label_set_text(g_system_bar.time_label, "00:00");
	lv_obj_set_pos(g_system_bar.time_label, 50, 40);

	// 日期显示（时间下方）
	g_system_bar.date_label = lv_label_create(g_system_bar.overlay);
	lv_obj_set_style_text_font(g_system_bar.date_label, g_system_bar.font_normal, 0);
	lv_obj_set_style_text_color(g_system_bar.date_label, lv_color_hex(0x888888), 0);
	lv_label_set_text(g_system_bar.date_label, "2025-01-01");
	lv_obj_set_pos(g_system_bar.date_label, 50, 110);

	// ===== 右下角：返回主页按钮 =====
	g_system_bar.home_btn = lv_button_create(g_system_bar.overlay);
	lv_obj_set_size(g_system_bar.home_btn, 180, 50);
	lv_obj_set_pos(g_system_bar.home_btn, screen_w - 200, screen_h - 70);
	lv_obj_set_style_bg_color(g_system_bar.home_btn, lv_color_make(60, 60, 70), 0);
	lv_obj_set_style_bg_opa(g_system_bar.home_btn, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(g_system_bar.home_btn, 25, 0);
	lv_obj_set_style_border_width(g_system_bar.home_btn, 0, 0);
	lv_obj_add_event_cb(g_system_bar.home_btn, home_btn_event_cb, LV_EVENT_CLICKED, NULL);

	// 按钮文字
	lv_obj_t *home_text = lv_label_create(g_system_bar.home_btn);
	lv_obj_set_style_text_font(home_text, g_system_bar.font_normal, 0);
	lv_label_set_text(home_text, "Return to Home");
	lv_obj_set_style_text_color(home_text, lv_color_white(), 0);
	lv_obj_center(home_text);

	// ===== 底部中间：上滑提示 =====
	lv_obj_t *swipe_hint = lv_label_create(g_system_bar.overlay);
	lv_label_set_text(swipe_hint, LV_SYMBOL_UP);
	lv_obj_set_style_text_color(swipe_hint, lv_color_hex(0x666666), 0);
	lv_obj_align(swipe_hint, LV_ALIGN_BOTTOM_MID, 0, -15);

	// 默认隐藏
	lv_obj_add_flag(g_system_bar.overlay, LV_OBJ_FLAG_HIDDEN);
}

// ===== 事件回调 =====

// 点击遮罩层（面板外部）关闭控制中心
static void overlay_event_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		// 检查点击位置是否在面板外部
		lv_indev_t *indev = lv_indev_get_act();
		if (indev) {
			lv_point_t point;
			lv_indev_get_point(indev, &point);

			// 如果点击在面板区域外（y > 300），则关闭
			if (point.y > 300) {
				printf("system_bar: 点击遮罩层关闭\n");
				system_bar_hide();
			}
		}
	}
}

// 面板上滑手势关闭控制中心
static void panel_gesture_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	static lv_point_t start;
	static bool is_dragging = false;

	if (code == LV_EVENT_PRESSED) {
		lv_indev_get_point(lv_indev_get_act(), &start);
		is_dragging = true;
	}
	else if (code == LV_EVENT_PRESSING && is_dragging) {
		lv_point_t curr;
		lv_indev_get_point(lv_indev_get_act(), &curr);
		int diff_y = curr.y - start.y;

		// 向上滑动超过 50 像素则关闭
		if (diff_y < -50) {
			is_dragging = false;
			printf("system_bar: 上滑关闭\n");
			system_bar_hide();
		}
	}
	else if (code == LV_EVENT_RELEASED) {
		is_dragging = false;
	}
}

static void home_btn_event_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
		system_bar_hide();
		view_manager_switch_to("main_view");
		printf("system_bar: 点击主页按钮\n");
	}
}

// ===== 全局手势触发区 =====

/**
 * @brief 创建全局手势触发区
 *
 * 在屏幕顶部创建一个透明的条形区域，始终在最上层。
 * 用于检测从顶部下滑的手势，无论当前在哪个视图。
 */
static void create_gesture_trigger(void)
{
	// 在 lv_layer_top 上创建手势触发区
	g_system_bar.gesture_trigger = lv_obj_create(lv_layer_top());
	lv_obj_remove_style_all(g_system_bar.gesture_trigger);

	// 设置为屏幕顶部的窄条区域（避免遮挡返回按钮等 UI 元素）
	lv_obj_set_size(g_system_bar.gesture_trigger, LV_PCT(100), 15);
	lv_obj_align(g_system_bar.gesture_trigger, LV_ALIGN_TOP_MID, 0, 0);

	// 完全透明，不影响视觉
	lv_obj_set_style_bg_opa(g_system_bar.gesture_trigger, LV_OPA_0, 0);

	// 注册事件回调
	lv_obj_add_event_cb(g_system_bar.gesture_trigger, gesture_trigger_event_cb, LV_EVENT_ALL, NULL);

	// 保持在最上层
	lv_obj_move_foreground(g_system_bar.gesture_trigger);
}

/**
 * @brief 手势触发区事件回调
 *
 * 检测从顶部向下滑动的手势，触发控制中心显示。
 */
static void gesture_trigger_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	static lv_point_t start;
	static bool is_dragging = false;

	if (code == LV_EVENT_PRESSED) {
		lv_indev_get_point(lv_indev_get_act(), &start);
		// 必须从顶部下滑
		if (start.y > 10) {
			return ;
		}
		is_dragging = true;
	}
	else if (code == LV_EVENT_PRESSING && is_dragging) {
		lv_point_t curr;
		lv_indev_get_point(lv_indev_get_act(), &curr);
		int diff_y = curr.y - start.y;

		// 向下滑动超过 50 像素则显示控制中心
		if (diff_y > 50) {
			is_dragging = false;
			system_bar_show();
		}
	}
	else if (code == LV_EVENT_RELEASED) {
		is_dragging = false;
	}
}

// ===== 定时器回调 =====
static void time_update_timer_cb(lv_timer_t *timer)
{
	(void)timer;  // 避免未使用警告
	system_bar_update_time();
}
