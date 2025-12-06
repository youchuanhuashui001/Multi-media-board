#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <stdint.h>

// 播放状态枚举
typedef enum {
	PLAYER_STATUS_STOPPED,
	PLAYER_STATUS_PLAYING,
	PLAYER_STATUS_PAUSED,
	PLAYER_STATUS_FINISHED,
	PLAYER_STATUS_ERROR
} player_status_t;

// 状态回调函数类型
typedef void (*engine_status_cb_t)(player_status_t status, void *user_data);

// 初始化
int audio_engine_init(void);

// 核心控制
int audio_engine_play(const char *file_path); // 启动播放线程
void audio_engine_pause(void);                // 暂停播放 (保留资源)
void audio_engine_resume(void);               // 恢复播放
void audio_engine_stop(void);                 // 停止播放 (释放解码资源)
void audio_engine_seek(int64_t time_ms);      // 触发 Seek 操作

// 状态查询
int64_t audio_engine_get_position(void);       // 获取当前进度 (ms)
int64_t audio_engine_get_duration(void);       // 获取总时长 (ms)
player_status_t audio_engine_get_status(void); // 获取播放状态

// 状态回调注册
void audio_engine_set_callback(engine_status_cb_t cb, void *user_data);

#endif // AUDIO_ENGINE_H
