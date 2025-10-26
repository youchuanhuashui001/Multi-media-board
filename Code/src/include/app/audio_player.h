#ifndef __AUDIO_PLAYER_H_
#define __AUDIO_PLAYER_H_

#include "common.h"

// 音乐播放器控制结构体
typedef struct {
    char *current_file;    // 当前播放文件
    bool is_playing;       // 播放状态
    FILE *pipe_fp;        // mplayer管道
} audio_player_t;

// API函数
void audio_player_init(void);
void audio_player_play(const char *file);
void audio_player_pause(void);
void audio_player_resume(void);
void audio_player_stop(void);

#endif