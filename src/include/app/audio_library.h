#ifndef AUDIO_LIBRARY_H
#define AUDIO_LIBRARY_H

#include <stdint.h>

// 歌词行结构体
typedef struct {
	int64_t time_ms; // 时间戳 (毫秒)
	char *text;      // 歌词文本内容
} lyric_line_t;

// 歌词集合结构体
typedef struct {
	lyric_line_t *lines; // 歌词行数组
	int count;           // 歌词行数
} lyric_info_t;

// 音乐信息结构体 (单向链表节点)
typedef struct music_info {
	char *file_path;     // 文件绝对路径
	char *title;         // 歌曲标题 (ID3 Tag)
	char *artist;        // 歌手 (ID3 Tag)
	char *album;         // 专辑 (ID3 Tag)
	int64_t duration_ms; // 总时长 (毫秒)
	char *cover_path;    // 封面图片路径 (缓存的临时文件)

	lyric_info_t *lyrics; // 关联的歌词信息 (可为 NULL)

	struct music_info *next; // 指向下一首歌曲的指针
} music_info_t;

// 扫描目录，返回链表头
music_info_t *audio_library_scan_dir(const char *dir_path);

// 释放链表资源
void audio_library_free_list(music_info_t *head);

// 加载指定歌曲的歌词 (通常在扫描时或播放前调用)
lyric_info_t *audio_library_load_lyrics(const char *audio_path);
void audio_library_free_lyrics(lyric_info_t *lyrics);

#endif // AUDIO_LIBRARY_H
