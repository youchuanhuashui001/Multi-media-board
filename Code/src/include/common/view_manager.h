#ifndef __VIEW_MANAGER_H_
#define __VIEW_MANAGER_H_

#include "common.h"

// 视图结构体
typedef struct {
    const char *name;                // 视图名称
    int initialized;           // 是否已初始化
    lv_obj_t *screen;          // 页面对象 TODO:
    void (*init)(void);        // 初始化回调
    void (*destroy)(void);     // 销毁回调
    void (*event_cb)(lv_event_t *e); // 事件处理回调
} view_t;

// 视图管理器
typedef struct {
    view_t *current_view;      // 当前视图
    view_t views[MAX_VIEWS];  // 视图列表
    unsigned char view_count;        // 视图数量
} view_manager_t;

void view_manager_init(void);
/*
 * @brief 注册视图，如果是第一个注册的视图则自动切换到该视图，并且执行初始化函数
 *
 * @param view 需要注册的视图指针
 *
 * @return 0表示成功，-1表示失败
 */
int view_manager_register(view_t *view);

/*
 * @brief 切换到指定名称的视图，如果要切换的视图未初始化则执行初始化函数
 *
 * @param name 目标视图的名称
 *
 * @return 0表示成功，-1表示失败
 */
int view_manager_switch_to(const char *name);



// 用于 register
extern view_t main_view;

#endif