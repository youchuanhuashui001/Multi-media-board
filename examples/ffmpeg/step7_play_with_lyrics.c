/*
 * FFmpeg 学习第7步：同步歌词显示 (基于step4, ALSA时钟同步)
 *
 * 简介:
 * 本程序在第4步的基础上，实现了精确的歌词同步功能。
 * 1. 增加了LRC文件解析功能。
 * 2. **同步方案**: 放弃使用FFmpeg的PTS作为时钟源，因为它不考虑音频设备的缓冲延迟。
 *    改为使用ALSA的API作为时钟源，实现精准同步。
 * 3. **时钟实现**:
 *    a. 记录下所有通过 `snd_pcm_writei` 写入ALSA缓冲区的总帧数 (`total_frames_written`)。
 *    b. 通过 `snd_pcm_delay` 查询当前ALSA缓冲区中还剩多少帧未播放 (`delay_frames`)。
 *    c. `played_frames = total_frames_written - delay_frames` 即为已播放的帧数。
 *    d. `current_time_ms = (played_frames * 1000) / sample_rate` 得到精确的播放毫秒数。
 * 4. 使用新的FFmpeg API替换了已废弃的函数。
 *
 * 编译命令:
 * gcc -o step7_play_with_lyrics step7_play_with_lyrics.c \
 *     $(pkg-config --cflags libavformat libavcodec libswresample libavutil alsa) \
 *     $(pkg-config --libs libavformat libavcodec libswresample libavutil alsa)
 *
 * 使用方法:
 * ./step7_play_with_lyrics /path/to/your/media.mp3 /path/to/your/lyrics.lrc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h> // For new channel layout API
#include <libswresample/swresample.h>
#include <alsa/asoundlib.h>

// ALSA 相关参数
#define ALSA_DEVICE "default"
#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNELS 2
#define TARGET_FORMAT SND_PCM_FORMAT_S16_LE
#define TARGET_SAMPLE_FMT AV_SAMPLE_FMT_S16

// --- 歌词处理部分 ---

typedef struct {
    long time_ms;
    char text[256];
} LyricLine;

static int parse_lrc_file(const char *filepath, LyricLine **lyrics_out, int *count_out) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open LRC file: %s\n", filepath);
        return -1;
    }

    char line[512];
    int capacity = 10;
    int count = 0;
    LyricLine *lyrics = malloc(capacity * sizeof(LyricLine));
    if (!lyrics) {
        fclose(file);
        return -1;
    }

    while (fgets(line, sizeof(line), file)) {
        int min, sec, cs;
        if (sscanf(line, "[%d:%d.%d]", &min, &sec, &cs) == 3) {
            if (count >= capacity) {
                capacity *= 2;
                LyricLine *temp = realloc(lyrics, capacity * sizeof(LyricLine));
                if (!temp) {
                    free(lyrics);
                    fclose(file);
                    return -1;
                }
                lyrics = temp;
            }
            lyrics[count].time_ms = min * 60000 + sec * 1000 + cs * 10;
            char *lyric_text = strchr(line, ']');
            if (lyric_text && *(lyric_text + 1)) {
                lyric_text++;
                while (*lyric_text && isspace((unsigned char)*lyric_text)) lyric_text++;
                size_t len = strlen(lyric_text);
                while (len > 0 && isspace((unsigned char)lyric_text[len - 1])) len--;
                strncpy(lyrics[count].text, lyric_text, len);
                lyrics[count].text[len] = '\0';
            } else {
                lyrics[count].text[0] = '\0';
            }
            count++;
        }
    }

    fclose(file);
    *lyrics_out = lyrics;
    *count_out = count;
    printf("[LRC Parser] Loaded %d lines of lyrics.\n", count);
    return 0;
}

static int decode_and_play(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, 
                           SwrContext *swr_ctx, snd_pcm_t *pcm_handle,
                           LyricLine *lyrics, int lyric_count, int *current_lyric_index,
                           long long *total_frames_written) {
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding\n");
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error during decoding\n");
            return ret;
        }

        uint8_t *out_buffer;
        int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
        av_samples_alloc(&out_buffer, NULL, dec_ctx->ch_layout.nb_channels, out_samples, TARGET_SAMPLE_FMT, 0);

        int converted_samples = swr_convert(swr_ctx, &out_buffer, out_samples, 
                                            (const uint8_t **)frame->extended_data, frame->nb_samples);
        
        snd_pcm_sframes_t written_frames = snd_pcm_writei(pcm_handle, out_buffer, converted_samples);
        if (written_frames > 0) {
            *total_frames_written += written_frames;
        }
        
        av_freep(&out_buffer);

        // --- ALSA 时钟同步点 ---
        snd_pcm_sframes_t delay;
        long long alsa_time_ms = 0;
        if (snd_pcm_delay(pcm_handle, &delay) == 0) {
            long long played_frames = *total_frames_written - delay;
            if (played_frames < 0) played_frames = 0;
            alsa_time_ms = (played_frames * 1000) / TARGET_SAMPLE_RATE;
        }

        // 在合适的时间打印歌词内容
        if (lyrics && *current_lyric_index < lyric_count) {
            while (*current_lyric_index < lyric_count && alsa_time_ms >= lyrics[*current_lyric_index].time_ms) {
                printf(">>> %s\n", lyrics[*current_lyric_index].text);
                (*current_lyric_index)++;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <media_file_path> <lrc_file_path>\n", argv[0]);
        return -1;
    }
    const char *media_filepath = argv[1];
    const char *lrc_filepath = argv[2];

    LyricLine *lyrics = NULL;
    int lyric_count = 0;
    if (parse_lrc_file(lrc_filepath, &lyrics, &lyric_count) != 0) {
        fprintf(stderr, "Could not load lyrics, continuing without them.\n");
    }

    AVFormatContext *pFormatContext = NULL;
    avformat_open_input(&pFormatContext, media_filepath, NULL, NULL);
    avformat_find_stream_info(pFormatContext, NULL);

    int audio_stream_index = -1;
    AVStream *pAudioStream = NULL;
    AVCodecParameters *pCodecParams = NULL;
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            pAudioStream = pFormatContext->streams[i];
            pCodecParams = pAudioStream->codecpar;
            break;
        }
    }
    if (audio_stream_index == -1) {
        free(lyrics);
        return -1;
    }

    const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecContext, pCodecParams);
    avcodec_open2(pCodecContext, pCodec, NULL);
    printf("FFmpeg components initialized.\n");

    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, TARGET_FORMAT);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, TARGET_CHANNELS);
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, (unsigned int[]){TARGET_SAMPLE_RATE}, 0);
    snd_pcm_hw_params(pcm_handle, hw_params);
    printf("ALSA device initialized.\n");

    SwrContext *swr_ctx = NULL;
    AVChannelLayout in_ch_layout = {0}, out_ch_layout = {0};
    if (pCodecContext->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
        av_channel_layout_default(&in_ch_layout, pCodecContext->ch_layout.nb_channels);
    } else {
        av_channel_layout_copy(&in_ch_layout, &pCodecContext->ch_layout);
    }
    av_channel_layout_default(&out_ch_layout, TARGET_CHANNELS);
    int ret = swr_alloc_set_opts2(&swr_ctx,
                              &out_ch_layout, TARGET_SAMPLE_FMT, TARGET_SAMPLE_RATE,
                              &in_ch_layout, pCodecContext->sample_fmt, pCodecContext->sample_rate,
                              0, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate SWR context\n");
        return -1;
    }
    swr_init(swr_ctx);
    printf("SWR context initialized successfully.\n");

    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    int current_lyric_index = 0;
    long long total_frames_written = 0;
    
    printf("\n--- Starting Playback ---\n\n");
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        if (pPacket->stream_index == audio_stream_index) {
            decode_and_play(pCodecContext, pPacket, pFrame, swr_ctx, pcm_handle, 
                            lyrics, lyric_count, &current_lyric_index, &total_frames_written);
        }
        av_packet_unref(pPacket);
    }

    decode_and_play(pCodecContext, NULL, pFrame, swr_ctx, pcm_handle,
                    lyrics, lyric_count, &current_lyric_index, &total_frames_written);
    
    printf("\n\n--- Playback Finished ---\n");

    free(lyrics);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    printf("ALSA device closed.\n");

    swr_free(&swr_ctx);
    printf("SWR context freed.\n");

    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    avcodec_close(pCodecContext);
    avcodec_free_context(&pCodecContext);
    avformat_close_input(&pFormatContext);
    printf("FFmpeg components freed.\n");

    return 0;
}

