/*
 * FFmpeg 垂习第4步：播放声音（配合 ALSA 与 libswresample）
 *
 * 简介	:
 * 本程序在第3步基础上，实现了最终的音频播放。
 * 1. 初始化 ALSA ，配置音频参数（采样率、格式、通道等。）
 * 2. 初始化 FFmpeg 的 swresample 库，用于将解码出的音频帧转换为
 *    ALSA 设备所支持的格式（例如，从 fltp 转换为 s16le ）。
 * 3. 在解码循环中，将解码后的 AVFrame 通过 swr_convert 进行重采样。
 * 4. 将重采样后的数据通过 snd_pcm_writei 写入 ALSA 设备，从而播放出声音。
 *
 * 编译命令	:
 * arm-buildroot-linux-gnueabihf-gcc -o step4_play_audio step4_play_audio.c \
 *     $(pkg-config --cflags libavformat libavcodec libswresample libavutil alsa) \
 *     $(pkg-config --libs libavformat libavcodec libswresample libavutil alsa)
 * 
 * (注意	:我们增加了 libswresample 和 alsa 两个库)
 * 
 * 使用方法	:
 * ./step4_play_audio /path/to/your/media.mp3
 * 
 * 预期行为	:
 * 程序运行后，不会再打印大量的 "Decoded a frame..." 信息，
 * 而是会直接开始播放指定的音频文件。播放结束后，程序会自动退出。
 */

#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libswresample/swresample.h>
#include <alsa/asoundlib.h>

// ALSA 相关参数
#define ALSA_DEVICE "default"
#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNELS 2
#define TARGET_FORMAT SND_PCM_FORMAT_S16_LE
#define TARGET_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO
#define TARGET_SAMPLE_FMT AV_SAMPLE_FMT_S16

static int decode_and_play(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, 
                           SwrContext *swr_ctx, snd_pcm_t *pcm_handle) {
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

        // --- 音频重采样并播放 ---
        uint8_t *out_buffer;
        int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
        av_samples_alloc(&out_buffer, NULL, TARGET_CHANNELS, out_samples, TARGET_SAMPLE_FMT, 0);

        int converted_samples = swr_convert(swr_ctx, &out_buffer, out_samples, 
                                            (const uint8_t **)frame->extended_data, frame->nb_samples);
        
        snd_pcm_writei(pcm_handle, out_buffer, converted_samples);
        
        av_freep(&out_buffer);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <media_file_path>\n", argv[0]);
        return -1;
    }
    const char *filepath = argv[1];

    // --- FFmpeg 初始化 ---
    AVFormatContext *pFormatContext = NULL;
    avformat_open_input(&pFormatContext, filepath, NULL, NULL);
    avformat_find_stream_info(pFormatContext, NULL);

    int audio_stream_index = -1;
    AVCodecParameters *pCodecParams = NULL;
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            pCodecParams = pFormatContext->streams[i]->codecpar;
            break;
        }
    }
    if (audio_stream_index == -1) return -1;

    const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecContext, pCodecParams);
    avcodec_open2(pCodecContext, pCodec, NULL);
    printf("FFmpeg components initialized.\n");

    // --- ALSA 初始化 ---
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

    // --- SwrContext (重采样) 初始化 ---
    SwrContext *swr_ctx = swr_alloc_set_opts(NULL,
                                           TARGET_CHANNEL_LAYOUT, TARGET_SAMPLE_FMT, TARGET_SAMPLE_RATE,
                                           pCodecContext->channel_layout, pCodecContext->sample_fmt, pCodecContext->sample_rate,
                                           0, NULL);
    swr_init(swr_ctx);
    printf("SWR context initialized.\n");

    // --- 解码与播放循环 ---
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    
    printf("--- Starting Playback ---\n");
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        if (pPacket->stream_index == audio_stream_index) {
            decode_and_play(pCodecContext, pPacket, pFrame, swr_ctx, pcm_handle);
        }
        av_packet_unref(pPacket);
    }

    // Flush
    decode_and_play(pCodecContext, NULL, pFrame, swr_ctx, pcm_handle);
    printf("--- Playback Finished ---\n");

    // --- 清理工作 ---
    snd_pcm_drain(pcm_handle); // 等待所有挂起的音频帧播完毕
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
