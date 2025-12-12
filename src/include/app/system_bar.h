/**
 * @file system_bar.h
 * @brief 全局控制中心模块
 *
 * 提供从任意界面顶部下滑呼出的控制中心功能，包括：
 * - 日期和时间显示
 * - 系统快捷方式（主页）
 */

#ifndef __SYSTEM_BAR_H_
#define __SYSTEM_BAR_H_

/**
 * @brief 初始化全局控制中心
 *
 * 在 lv_layer_top() 上创建控制中心 UI，默认隐藏。
 * 应在 view_manager_init() 之后调用。
 */
void system_bar_init(void);

/**
 * @brief 显示控制中心
 */
void system_bar_show(void);

/**
 * @brief 隐藏控制中心
 */
void system_bar_hide(void);

/**
 * @brief 更新时间显示
 *
 * 应由定时器每分钟调用一次。
 */
void system_bar_update_time(void);

/**
 * @brief 检查控制中心是否可见
 *
 * @return 1 如果可见，0 如果隐藏
 */
int system_bar_is_visible(void);

#endif /* __SYSTEM_BAR_H_ */
