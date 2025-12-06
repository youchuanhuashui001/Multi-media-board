#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "audio_engine.h"
#include "audio_library.h"

// 播放模式枚举
typedef enum {
	PLAY_MODE_LOOP_LIST,   // 列表循环
	PLAY_MODE_LOOP_SINGLE, // 单曲循环
	PLAY_MODE_SHUFFLE      // 随机播放
} play_mode_t;

// 初始化
int audio_manager_init(void);

// 播放控制
int audio_manager_play(void);                 // 播放/暂停切换
int audio_manager_pause(void);                // 暂停
int audio_manager_stop(void);                 // 停止
int audio_manager_play_next(void);            // 下一首
int audio_manager_play_prev(void);            // 上一首
int audio_manager_play_at_index(int index);   // 播放列表指定索引
int audio_manager_seek(int64_t time_ms);      // 跳转到指定时间(ms)
int audio_manager_set_mode(play_mode_t mode); // 设置模式
play_mode_t audio_manager_get_mode(void);     // 获取模式

// 信息获取
music_info_t *audio_manager_get_current_info(void); // 获取当前歌曲信息
int64_t audio_manager_get_position(void);           // 获取当前进度 (ms)
int64_t audio_manager_get_duration(void);           // 获取总时长 (ms)
player_status_t audio_manager_get_status(void);     // 获取播放状态

// 播放列表管理
int audio_manager_scan_dir(const char *path);        // 扫描目录
int audio_manager_get_playlist_count(void);          // 获取歌曲总数
music_info_t *audio_manager_get_playlist_head(void); // 获取链表头

#endif // AUDIO_MANAGER_H
