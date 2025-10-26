#ifndef __EVENT_HANDLER_H_
#define __EVENT_HANDLER_H_

#include "common.h"

// 自定义事件类型
typedef enum {
    EVENT_VIEW_SWITCH,     // 视图切换事件
    EVENT_MUSIC_PLAY,      // 音乐播放事件
    EVENT_MUSIC_PAUSE,     // 音乐暂停事件
    EVENT_BOOK_PAGE_NEXT,  // 下一页事件
    EVENT_BOOK_PAGE_PREV,  // 上一页事件
} app_event_t;


#endif