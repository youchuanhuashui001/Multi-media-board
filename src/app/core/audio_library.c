#include "audio_library.h"
#include <dirent.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// ========== 内部辅助函数 ==========

// 检查是否为支持的音频文件
static int is_audio_file(const char *filename) {
	const char *ext = strrchr(filename, '.');
	if (!ext)
		return 0;

	// 支持的扩展名: mp3, wav, flac, m4a, aac, ogg
	if (strcasecmp(ext, ".mp3") == 0 || strcasecmp(ext, ".wav") == 0 ||
			strcasecmp(ext, ".flac") == 0 || strcasecmp(ext, ".m4a") == 0 ||
			strcasecmp(ext, ".aac") == 0 || strcasecmp(ext, ".ogg") == 0) {
		return 1;
	}
	return 0;
}

// 使用 FFmpeg 提取元数据
static void extract_metadata(const char *path, music_info_t *info) {
	AVFormatContext *fmt_ctx = NULL;

	// 抑制 ffmpeg 日志输出
	av_log_set_level(AV_LOG_QUIET);

	if (avformat_open_input(&fmt_ctx, path, NULL, NULL) != 0) {
		return;
	}

	// 获取流信息
	if (avformat_find_stream_info(fmt_ctx, NULL) >= 0) {
		// 提取总时长 (微秒 -> 毫秒)
		info->duration_ms = fmt_ctx->duration / 1000;
	}

	// 读取元数据标签
	AVDictionaryEntry *tag = NULL;

	tag = av_dict_get(fmt_ctx->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);
	if (tag && tag->value) {
		info->title = strdup(tag->value);
	}

	tag = av_dict_get(fmt_ctx->metadata, "artist", NULL, AV_DICT_IGNORE_SUFFIX);
	if (tag && tag->value) {
		info->artist = strdup(tag->value);
	}

	tag = av_dict_get(fmt_ctx->metadata, "album", NULL, AV_DICT_IGNORE_SUFFIX);
	if (tag && tag->value) {
		info->album = strdup(tag->value);
	}

	// 提取封面 (查找视频流，通常是嵌入的封面图片)
	for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			// 找到视频流（封面）
			AVCodecParameters *codecpar = fmt_ctx->streams[i]->codecpar;
			
			printf("Found embedded image: codec=%s, width=%d, height=%d\n",
				avcodec_get_name(codecpar->codec_id),
				codecpar->width, codecpar->height);
			
			// 尝试直接保存（适用于 MJPEG/PNG）
			AVPacket *pkt = av_packet_alloc();
			if (!pkt)
				break;
			
			// 读取第一个视频包
			int found = 0;
			while (av_read_frame(fmt_ctx, pkt) >= 0) {
				if (pkt->stream_index == i) {
					// 构造封面文件路径（与音频文件同目录）
					char cover_path[1024];
					const char *last_slash = strrchr(info->file_path, '/');
					const char *last_dot = strrchr(info->file_path, '.');
					
					if (last_slash && last_dot && last_dot > last_slash) {
						// 提取文件名（不含扩展名）
						int name_len = last_dot - info->file_path;
						// 根据编解码器选择扩展名
						const char *ext = (codecpar->codec_id == AV_CODEC_ID_PNG) ? "png" : "jpg";
						snprintf(cover_path, sizeof(cover_path), "%.*s_cover.%s",
							name_len, info->file_path, ext);
					} else {
						const char *ext = (codecpar->codec_id == AV_CODEC_ID_PNG) ? "png" : "jpg";
						snprintf(cover_path, sizeof(cover_path), "%s_cover.%s",
							info->file_path, ext);
					}
					
					// 保存封面数据到文件
					FILE *cover_file = fopen(cover_path, "wb");
					if (cover_file) {
						fwrite(pkt->data, 1, pkt->size, cover_file);
						fclose(cover_file);
						
						// 保存封面路径
						info->cover_path = strdup(cover_path);
						printf("Extracted cover: %s (%d bytes)\n", cover_path, pkt->size);
						found = 1;
					}
					
					av_packet_unref(pkt);
					break;
				}
				av_packet_unref(pkt);
			}
			
			av_packet_free(&pkt);
			
			if (found) {
				// 重置文件读取位置
				av_seek_frame(fmt_ctx, -1, 0, AVSEEK_FLAG_BACKWARD);
			}
			break;
		}
	}

	avformat_close_input(&fmt_ctx);
}

// 解析 LRC 歌词文件
static lyric_info_t *parse_lrc_file(const char *lrc_path) {
	FILE *fp = fopen(lrc_path, "r");
	if (!fp)
		return NULL;

	lyric_info_t *lyrics = (lyric_info_t *)malloc(sizeof(lyric_info_t));
	if (!lyrics) {
		fclose(fp);
		return NULL;
	}

	lyrics->lines = NULL;
	lyrics->count = 0;

	// 第一遍：统计行数
	char line_buf[512];
	int capacity = 0;

	while (fgets(line_buf, sizeof(line_buf), fp)) {
		// 简单匹配 [mm:ss.xx] 格式
		if (line_buf[0] == '[' && strchr(line_buf, ':')) {
			capacity++;
		}
	}

	if (capacity == 0) {
		fclose(fp);
		free(lyrics);
		return NULL;
	}

	// 分配内存
	lyrics->lines = (lyric_line_t *)malloc(capacity * sizeof(lyric_line_t));
	if (!lyrics->lines) {
		fclose(fp);
		free(lyrics);
		return NULL;
	}

	// 第二遍：解析内容
	rewind(fp);
	int idx = 0;

	while (fgets(line_buf, sizeof(line_buf), fp) && idx < capacity) {
		// 解析格式: [mm:ss.xx]text
		if (line_buf[0] != '[')
			continue;

		int min, sec, ms;
		char text[256] = {0};

		// 尝试解析时间戳
		if (sscanf(line_buf, "[%d:%d.%d]%[^\n]", &min, &sec, &ms, text) == 4 ||
				sscanf(line_buf, "[%d:%d]%[^\n]", &min, &sec, text) == 3) {

			// 规避歌词时间错误的情况
			if (ms > 1000) ms = 1000;
			if (sec > 60) sec = 60;
			if (min > 60) min = 60;

			lyrics->lines[idx].time_ms = (int64_t)min * 60000 + sec * 1000 + ms * 10;
			lyrics->lines[idx].text = strdup(text);
			idx++;
		}
	}

	lyrics->count = idx;
	fclose(fp);

	return lyrics;
}

// ========== 公开接口实现 ==========

// 递归扫描目录
music_info_t *audio_library_scan_dir(const char *dir_path) {
	DIR *dir = opendir(dir_path);
	if (!dir) {
		perror("opendir");
		return NULL;
	}

	music_info_t *head = NULL;
	music_info_t *tail = NULL;

	struct dirent *entry;
	char full_path[1024];

	while ((entry = readdir(dir)) != NULL) {
		// 跳过 . 和 ..
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

		struct stat statbuf;
		if (stat(full_path, &statbuf) != 0) {
			continue;
		}

		if (S_ISDIR(statbuf.st_mode)) {
			// 递归扫描子目录
			music_info_t *sub_list = audio_library_scan_dir(full_path);

			// 将子列表合并到主列表
			if (sub_list) {
				if (!head) {
					head = sub_list;
					tail = sub_list;
				} else {
					tail->next = sub_list;
				}

				// 移动 tail 到末尾
				while (tail->next) {
					tail = tail->next;
				}
			}
		} else if (S_ISREG(statbuf.st_mode)) {
			// 检查是否为音频文件
			if (!is_audio_file(entry->d_name)) {
				continue;
			}

			// 创建新节点
			music_info_t *info = (music_info_t *)calloc(1, sizeof(music_info_t));
			if (!info)
				continue;

			info->file_path = strdup(full_path);

			// 设置默认值 (使用文件名)
			info->title = strdup(entry->d_name);
			info->artist = strdup("Unknown Artist");
			info->album = strdup("Unknown Album");
			info->duration_ms = 0;
			info->cover_path = NULL;
			info->lyrics = NULL;
			info->next = NULL;

			// 提取元数据
			extract_metadata(full_path, info);

			// 尝试加载歌词
			char lrc_path[1024];
			snprintf(lrc_path, sizeof(lrc_path), "%s", full_path);
			char *ext_pos = strrchr(lrc_path, '.');
			if (ext_pos) {
				strcpy(ext_pos, ".lrc");
				info->lyrics = parse_lrc_file(lrc_path);
			}

			// 插入链表尾部
			if (!head) {
				head = info;
				tail = info;
			} else {
				tail->next = info;
				tail = info;
			}
		}
	}

	closedir(dir);
	return head;
}

// 释放链表
void audio_library_free_list(music_info_t *head) {
	music_info_t *current = head;
	while (current) {
		music_info_t *next = current->next;

		free(current->file_path);
		free(current->title);
		free(current->artist);
		free(current->album);
		if (current->cover_path)
			free(current->cover_path);
		if (current->lyrics)
			audio_library_free_lyrics(current->lyrics);

		free(current);
		current = next;
	}
}

// 加载歌词 (外部调用)
lyric_info_t *audio_library_load_lyrics(const char *audio_path) {
	char lrc_path[1024];
	snprintf(lrc_path, sizeof(lrc_path), "%s", audio_path);

	char *ext_pos = strrchr(lrc_path, '.');
	if (!ext_pos)
		return NULL;

	strcpy(ext_pos, ".lrc");
	return parse_lrc_file(lrc_path);
}

// 释放歌词
void audio_library_free_lyrics(lyric_info_t *lyrics) {
	if (!lyrics)
		return;

	for (int i = 0; i < lyrics->count; i++) {
		free(lyrics->lines[i].text);
	}

	free(lyrics->lines);
	free(lyrics);
}
