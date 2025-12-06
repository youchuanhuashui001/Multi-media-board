#ifndef AUDIO_VIEW_H
#define AUDIO_VIEW_H

#include "lvgl/lvgl.h"

// 初始化音频视图
void audio_view_init(void);

// 显示/隐藏
void audio_view_show(void);
void audio_view_hide(void);

// 销毁
void audio_view_destroy(void);

#endif // AUDIO_VIEW_H
