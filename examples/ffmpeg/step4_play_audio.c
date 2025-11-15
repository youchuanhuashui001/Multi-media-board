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
#include <libavutil/opt.h>

// 重要名词/概念（简短定义）AVFormatContext：封装输入媒体文件/流的格式层，上面有流数组（streams）、时长、格式信息。用于打开文件并读取包（packet）。AVCodecParameters：存放某个流的编码参数（codec_id、sample_rate、channels、channel_layout 等）。
//
// AVCodec / AVCodecContext：解码器描述和解码器的运行上下文，AVCodecContext 用于解码（维护解码状态、采样率等）。
// AVPacket：封装压缩数据包（编码后），由 av_read_frame 获得，送入解码器。
// AVFrame：解码后的原始帧（音频 PCM 或视频像素），由 avcodec_receive_frame 返回。
// SwrContext（libswresample）：音频重采样/格式/通道布局转换上下文（例如 fltp -> s16）。
// channel_layout：声道布局掩码（AV_CH_LAYOUT_STEREO 等），channel_layout=0 时需用 av_get_default_channel_layout(channels) 补全。
// sample_fmt：样本格式（AV_SAMPLE_FMT_FLTP、AV_SAMPLE_FMT_S16 等）。
// swr_get_out_samples / swr_convert：计算/执行重采样并输出目标样本数。
// av_samples_alloc / av_samples_get_buffer_size：分配样本缓冲区并计算大小。
// ALSA snd_pcm_t / hw_params：PCM 设备句柄与硬件参数，snd_pcm_writei 写入帧数据到设备。
// XRUN（underrun/overrun）：ALSA 写入错误，会导致播放卡顿，需处理 -EPIPE 等返回值并重置设备。


// 代码总体流程（按执行顺序）
//
// 解析命令行得到文件路径。
// 使用 avformat_open_input 打开输入并用 avformat_find_stream_info 获取流信息。
// 遍历 streams 找到音频流 index，读取对应 AVCodecParameters。
// 找到解码器 avcodec_find_decoder，创建并填充 AVCodecContext（avcodec_alloc_context3 + avcodec_parameters_to_context），打开解码器 avcodec_open2。
// 初始化 ALSA：snd_pcm_open -> 分配 hw_params -> 设置访问模式、格式、通道、采样率 -> hw_params 生效。
// 初始化 SwrContext（swr_alloc_set_opts），传入输入/输出的 channel_layout/sample_fmt/sample_rate，然后 swr_init。
// 进入读取循环：av_read_frame 得到 AVPacket，若为音频流则送入 decode_and_play。
// decode_and_play 调用 avcodec_send_packet, 循环 avcodec_receive_frame 获取 AVFrame。
// 对每个 AVFrame：计算输出样本数（swr_get_out_samples 或根据 delay+nb_samples 计算），分配输出缓冲区 av_samples_alloc，调用 swr_convert 进行重采样，调用 snd_pcm_writei 写入 ALSA。
// 循环结束后 flush 解码器（发送 NULL 包），drain + 释放资源。

// ALSA 相关参数
#define ALSA_DEVICE "default"
#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNELS 2
#define TARGET_FORMAT SND_PCM_FORMAT_S16_LE
#define TARGET_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO
#define TARGET_SAMPLE_FMT AV_SAMPLE_FMT_S16

static int decode_and_play(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, 
                           SwrContext *swr_ctx, snd_pcm_t *pcm_handle) {
    // 传入解码的参数和压缩过的数据，返回处理的数据量
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding\n");
        return ret;
    }

    // 循环把所有的数据都处理掉
    while (ret >= 0) {
        // 根据解码的参数，应该是把解码过的数据读到了 frame
        ret = avcodec_receive_frame(dec_ctx, frame);
        // 没有可用的输出帧，需要继续解码
        // 解码器已刷新，不会再有输出帧
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            // 解码过程有错误
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
    // 探测文件格式，并读取文件头信息
    avformat_open_input(&pFormatContext, filepath, NULL, NULL);
    // 读取一部分数据来填充流信息，例如时长、码率等
    avformat_find_stream_info(pFormatContext, NULL);

    // 遍历文件中的所有流，找到音频流的位置，并使用 pCodeParams 记录音频流的 params
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

    // 根据编码器 id 查找对应的解码器
    const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    // 分配解码器上下文，也就是一片空间吧
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    // 将音频流的参数信息拷贝到解码器上下文中
    avcodec_parameters_to_context(pCodecContext, pCodecParams);
    // 根据上面找到的解码器和音频流的参数信息，打开解码器，
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
    if (pCodecContext->channel_layout == 0) {
        pCodecContext->channel_layout = av_get_default_channel_layout(pCodecContext->channels);
    }

    // 重新设置 SwrContext 的输入参数
    av_opt_set_int(swr_ctx, "in_channel_layout", pCodecContext->channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_fmt", pCodecContext->sample_fmt, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", pCodecContext->sample_rate, 0);
    
    int ret = swr_init(swr_ctx);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to initialize SWR context\n");
        return -1;
    }
    printf("SWR context initialized successfully.\n");


    // --- 解码与播放循环 ---
    // 编码后的数据读到 pPacket, 解码后的数据放到 pFrame
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    
    printf("--- Starting Playback ---\n");
    // 读的是编码过的帧，读到  pPacket
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
    // 为解码器分配了上下文，这里就需要释放和关闭
    avcodec_close(pCodecContext);
    avcodec_free_context(&pCodecContext);
    // 打开了媒体文件，这里就关闭媒体文件
    avformat_close_input(&pFormatContext);
    printf("FFmpeg components freed.\n");

    return 0;
}
