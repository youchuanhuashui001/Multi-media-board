#include "view_manager.h"
#include "system_bar.h"
#include "src/lv_api_map_v8.h"
#include "src/misc/lv_area.h"

// 前向声明
static void view_gesture_event_cb(lv_event_t * e);

view_manager_t g_view_manager;

void view_manager_handler(lv_timer_t *timer)
{
	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);
}

void view_manager_init(void)
{
	g_view_manager.current_view = NULL;
	g_view_manager.view_count = 0;
//	lv_timer_create(view_manager_handler, 1000, NULL);

	// 创建全局指示条（总是在最上层）
	g_view_manager.indicator_bar = lv_obj_create(lv_layer_top());
	lv_obj_remove_style_all(g_view_manager.indicator_bar);
	lv_obj_set_size(g_view_manager.indicator_bar, 100, 4);  // 宽100，高4像素
	lv_obj_set_style_bg_color(g_view_manager.indicator_bar, lv_color_hex(0xFFFFFF), 0);  // 白色
	lv_obj_set_style_bg_opa(g_view_manager.indicator_bar, LV_OPA_50, 0);  // 半透明
	lv_obj_set_style_radius(g_view_manager.indicator_bar, 2, 0);  // 圆角
	lv_obj_align(g_view_manager.indicator_bar, LV_ALIGN_BOTTOM_MID, 0, -10);  // 底部中间，向上偏移10像素

	// 确保指示条始终在最上层
	lv_obj_add_flag(g_view_manager.indicator_bar, LV_OBJ_FLAG_FLOATING);
	lv_obj_clear_flag(g_view_manager.indicator_bar, LV_OBJ_FLAG_CLICKABLE); // 避免影响下层控件的触摸

	lv_obj_move_foreground(g_view_manager.indicator_bar);

	// 初始化全局控制中心
	system_bar_init();
}

int view_manager_register(view_t *view)
{
	if (g_view_manager.view_head == NULL) {
		g_view_manager.view_head = view;
	} else {
		view_t *temp = g_view_manager.view_head;
		while (temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = view;
	}

	g_view_manager.view_count++;

	if ((g_view_manager.view_count == 1) && (g_view_manager.current_view == NULL)) {
		g_view_manager.current_view = view;
		if (g_view_manager.current_view->init) {
			g_view_manager.current_view->init();
		}
		g_view_manager.current_view->initialized = 1;

		// 为第一个视图也添加手势检测（用于从顶部下滑呼出控制中心）
		if (g_view_manager.current_view->screen) {
			lv_obj_add_event_cb(g_view_manager.current_view->screen,
						  view_gesture_event_cb,
						  LV_EVENT_ALL, NULL);
		}
	}

	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);
	view_printf("g_view_manager.current_view = %p\n", g_view_manager.current_view);
	view_printf("views.name = %s\n", g_view_manager.current_view->name);
	view_printf("views.screen = %p\n", g_view_manager.current_view->screen);
	view_printf("views.init = %p\n", g_view_manager.current_view->init);
	view_printf("views.destroy = %p\n", g_view_manager.current_view->destroy);
	view_printf("views.event_cb = %p\n", g_view_manager.current_view->event_cb);
	view_printf("g_view_manager.view_count = %d\n", g_view_manager.view_count);
	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);

	return 0;
}

static void view_gesture_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);

	static lv_point_t scroll_start;
	static lv_point_t diff;
	static bool is_scrolling = false;

	if(code == LV_EVENT_PRESSED) {
		lv_indev_get_point(lv_indev_get_act(), &scroll_start);
		is_scrolling = true;
		diff.y = 0;  // 重置 diff
	}
	else if(code == LV_EVENT_PRESSING && is_scrolling) {
		lv_point_t curr;
		lv_indev_get_point(lv_indev_get_act(), &curr);
		diff.y = curr.y - scroll_start.y;  // 向下滑动为正值

		printf("[view_gesture] PRESSING: start_y=%d, curr_y=%d, diff_y=%d\n",
			scroll_start.y, curr.y, diff.y);

		// 如果在顶部区域且向下滑动超过50像素 -> 显示控制中心
		if(scroll_start.y < 100 && diff.y > 50) {
			printf("[view_gesture] >>> 触发控制中心显示！\n");
			is_scrolling = false;
			system_bar_show();
		}
	}
	else if(code == LV_EVENT_RELEASED) {
		printf("[view_gesture] RELEASED\n");
		diff.y = 0;
		is_scrolling = false;
	}
}

static view_t *view_get_by_name(const char *name)
{
	view_t *temp = g_view_manager.view_head;

	while (temp != NULL) {
	if (strcmp(temp->name, name) == 0) {
		return temp;
	}
	temp = temp->next;
	}

	return NULL;
}

int view_manager_switch_to(const char *name)
{
	view_t *old_view = g_view_manager.current_view;
	view_t *target_view = g_view_manager.view_head;

	// 1. 查找目标视图
	target_view = view_get_by_name(name);
	if (target_view == NULL) {
		view_printf("can't find view %s\n", name);
		return -1;
	}

	// 2. 检查是否切换到同一视图
	if (old_view == target_view) {
		view_printf("the current view and the target view are the same.\n");
		return 0;
	}

	// 3. 处理旧视图
	if (old_view != NULL) {
	//TODO: 什么时候隐藏，什么时候销毁，默认先隐藏
	if (old_view->hide) {
		view_printf("hide %s view\n", old_view->name);
		old_view->hide();
	}
//	// 如果有销毁函数则调用
//	if (old_view->destroy) {
//		old_view->destroy();
//	}
	}

	view_printf(" !!!!!!!!!!!!!! old_view !!!!!!!!!!!!!!!!!\n");
	view_printf("old_view->name = %s\n", old_view->name);
	view_printf("old_view->screen = %p\n", old_view->screen);
	view_printf("old_view->init = %p\n", old_view->init);
	view_printf("old_view->hide = %p\n", old_view->hide);
	view_printf("old_view->show = %p\n", old_view->show);
	view_printf(" !!!!!!!!!!!!!! old_view !!!!!!!!!!!!!!!!!\n");




	// 4. 切换到新视图
	g_view_manager.current_view = target_view;

	// 5. 初始化新视图（如果未初始化）
	if (g_view_manager.current_view->initialized == 0) {
		if (g_view_manager.current_view->init) {
		view_printf("init %s view\n", g_view_manager.current_view->name);
			g_view_manager.current_view->init();
		view_printf("g_view_manager.current_view->name = %s\n", g_view_manager.current_view->name);
		view_printf("g_view_manager.current_view->screen = %p\n", g_view_manager.current_view->screen);
		view_printf("g_view_manager.current_view->init = %p\n", g_view_manager.current_view->init);
		view_printf("g_view_manager.current_view->hide = %p\n", g_view_manager.current_view->hide);
		view_printf("g_view_manager.current_view->show = %p\n", g_view_manager.current_view->show);

			// 为所有视图添加手势检测（包括 main_view，用于从顶部下滑呼出控制中心）
			lv_obj_add_event_cb(g_view_manager.current_view->screen,
						  view_gesture_event_cb,
						  LV_EVENT_ALL, NULL);
		}
		g_view_manager.current_view->initialized = 1;
	} else {
		// 如果已经初始化过，显示该视图
	if (g_view_manager.current_view->show) {
		view_printf("show %s view\n", g_view_manager.current_view->name);
		g_view_manager.current_view->show();
	}
	}

	// 7. 打印调试信息
	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);
	view_printf("change view: %s -> %s\n", old_view->name,
				g_view_manager.current_view->name);
	view_printf("g_view_manager.current_view = %p\n", g_view_manager.current_view);
	view_printf("current_views->name = %s\n", g_view_manager.current_view->name);
	view_printf("current_views->screen = %p\n", g_view_manager.current_view->screen);
	view_printf("current_views->init = %p\n", g_view_manager.current_view->init);
	view_printf("current_views->destroy = %p\n", g_view_manager.current_view->destroy);
	view_printf("current_views->event_cb = %p\n", g_view_manager.current_view->event_cb);
	view_printf("g_view_manager.view_count = %d\n", g_view_manager.view_count);
	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);

	return 0;
}
