#include "view_manager.h"

view_manager_t g_view_manager;

void view_manager_init(void)
{
	int i;

	g_view_manager.current_view = NULL;

	for (i = 0; i < MAX_VIEWS; i++) {
		g_view_manager.views[i].name = NULL;
		g_view_manager.views[i].initialized = 0;
		g_view_manager.views[i].screen = NULL;
		g_view_manager.views[i].init = NULL;
		g_view_manager.views[i].destroy = NULL;
		g_view_manager.views[i].event_cb = NULL;
	}

	g_view_manager.view_count = 0;
}

int view_manager_register(view_t *view)
{
	int i;
	for (i = 0; i < MAX_VIEWS; i++) {
		if (g_view_manager.views[i].name == NULL) {
			break;
		}
	}
	if (i == MAX_VIEWS) {
		view_printf("视图注册失败，视图数量已达上限\n");
		return -1;
	}

	g_view_manager.views[i].name = view->name;
	g_view_manager.views[i].screen = view->screen;
	g_view_manager.views[i].init = view->init;
	g_view_manager.views[i].destroy = view->destroy;
	g_view_manager.views[i].event_cb = view->event_cb;

	g_view_manager.view_count++;

	if ((g_view_manager.view_count == 1) && (g_view_manager.current_view == NULL)) {
		g_view_manager.current_view = &g_view_manager.views[i];
		if (g_view_manager.current_view->init) {
			g_view_manager.current_view->init();
		}
		g_view_manager.current_view->initialized = 1;
	}

	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);
	view_printf("g_view_manager.current_view = %p\n", g_view_manager.current_view);
	view_printf("views[%d].name = %s\n", i, g_view_manager.views[i].name);
	view_printf("views[%d].screen = %p\n", i, g_view_manager.views[i].screen);
	view_printf("views[%d].init = %p\n", i, g_view_manager.views[i].init);
	view_printf("views[%d].destroy = %p\n", i, g_view_manager.views[i].destroy);
	view_printf("views[%d].event_cb = %p\n", i, g_view_manager.views[i].event_cb);
	view_printf("g_view_manager.view_count = %d\n", g_view_manager.view_count);
	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);

	return 0;
}

int view_manager_switch_to(const char *name)
{
	int i;

	for (i = 0; i < g_view_manager.view_count; i++) {
		if (strcmp(g_view_manager.views[i].name, name) == 0) {
			break;
		}
	}
	if (i == g_view_manager.view_count) {
		view_printf("视图切换失败，未找到名称为%s的视图\n", name);
		return -1;	
	}

	g_view_manager.current_view = &g_view_manager.views[i];
	if (g_view_manager.current_view->initialized == 0) {
		if (g_view_manager.current_view->init) {
			g_view_manager.current_view->init();
		}
		g_view_manager.current_view->initialized = 1;
	}

	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);
	view_printf("g_view_manager.current_view = %p\n", g_view_manager.current_view);
	view_printf("views[%d].name = %s\n", i, g_view_manager.views[i].name);
	view_printf("views[%d].screen = %p\n", i, g_view_manager.views[i].screen);
	view_printf("views[%d].init = %p\n", i, g_view_manager.views[i].init);
	view_printf("views[%d].destroy = %p\n", i, g_view_manager.views[i].destroy);
	view_printf("views[%d].event_cb = %p\n", i, g_view_manager.views[i].event_cb);
	view_printf("g_view_manager.view_count = %d\n", g_view_manager.view_count);
	view_printf("******* %d %s ********\n", __LINE__, __FUNCTION__);

	return 0;

}
