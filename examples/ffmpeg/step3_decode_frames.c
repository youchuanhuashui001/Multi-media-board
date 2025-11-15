/*
 * FFmpeg 学习第3步：解码音频（数据包 -> 数据帧）
 *
 * 简介:
 * 本程序在第2步基础上，增加了核心的解码循环。
 * 1. 使用 av_read_frame() 从媒体文件中循环读取数据包(AVPacket)。
 * 2. 找到属于音频流的 Packet。
 * 3. 使用 avcodec_send_packet() 将 Packet 发送给解码器。
 * 4. 使用 avcodec_receive_frame() 从解码器接收解码后的数据帧(AVFrame)。
 * 5. 打印出每个解码后数据帧的一些信息，以验证解码成功。
 * 6. 增加了 "flush" 解码器的操作，以确保获取所有被缓存的帧。
 *
 * 编译命令:
 * arm-buildroot-linux-gnueabihf-gcc -o step3_decode_frames step3_decode_frames.c \
 *     $(pkg-config --cflags libavformat libavcodec libavutil) \
 *     $(pkg-config --libs libavformat libavcodec libavutil)
 *
 * 使用方法:
 * ./step3_decode_frames /path/to/your/media.mp3
 *
 * 预期输出:
 * ... (前面的步骤输出)
 * Successfully opened decoder.
 * --- Decoding Loop Start ---
 * Decoded a frame with 1152 samples, format: fltp
 * Decoded a frame with 1152 samples, format: fltp
 * Decoded a frame with 1152 samples, format: fltp
 * ... (大量类似的输出)
 * --- Flushing Decoder ---
 * Decoded a frame with 1152 samples, format: fltp
 * --- Decoding Finished ---
 * ... (清理步骤输出)
 */

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>

// 封装一个函数用于解码，方便复用
static int decode_packet(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame) {
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding\n");
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // EAGAIN: 当前没有可用的输出帧，需要发送更多 packet
            // AVERROR_EOF: 解码器已完全刷新，不会再有输出帧
            return 0; 
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error during decoding\n");
            return ret;
        }

        // 成功解码一帧
        printf("Decoded a frame with %d samples, format: %s\n", 
               frame->nb_samples, 
               av_get_sample_fmt_name(frame->format));
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <media_file_path>\n", argv[0]);
        return -1;
    }
    const char *filepath = argv[1];

    AVFormatContext *pFormatContext = NULL;
    if (avformat_open_input(&pFormatContext, filepath, NULL, NULL) != 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input file '%s'\n", filepath);
        return -1;
    }

    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream information\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    int audio_stream_index = -1;
    AVCodecParameters *pCodecParams = NULL;
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            pCodecParams = pFormatContext->streams[i]->codecpar;
            break;
        }
    }

    if (audio_stream_index == -1) {
        av_log(NULL, AV_LOG_ERROR, "Could not find audio stream\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    if (pCodec == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find decoder\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate codec context\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    if (avcodec_parameters_to_context(pCodecContext, pCodecParams) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters to context\n");
        avcodec_free_context(&pCodecContext);
        avformat_close_input(&pFormatContext);
        return -1;
    }

    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open decoder\n");
        avcodec_free_context(&pCodecContext);
        avformat_close_input(&pFormatContext);
        return -1;
    }
    printf("Successfully opened decoder.\n");

    // --- 新增代码从这里开始 ---

    // 1. 分配 AVPacket 和 AVFrame
    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate packet\n");
        // ... (省略清理代码)
        return -1;
    }

    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate frame\n");
        // ... (省略清理代码)
        return -1;
    }

    printf("--- Decoding Loop Start ---\n");

    // 2. 循环读取数据帧
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        // 确保是我们关心的音频流
        if (pPacket->stream_index == audio_stream_index) {
            decode_packet(pCodecContext, pPacket, pFrame);
        }
        // 释放 packet 引用，准备下一次读取
        av_packet_unref(pPacket);
    }

    // 3. Flush 解码器
    // 发送一个 NULL packet 到解码器，以取出内部缓冲区中剩余的帧
    printf("--- Flushing Decoder ---\n");
    decode_packet(pCodecContext, NULL, pFrame);

    printf("--- Decoding Finished ---\n");

    // --- 清理工作 ---
    av_frame_free(&pFrame);
    printf("Frame freed.\n");
    av_packet_free(&pPacket);
    printf("Packet freed.\n");

    avcodec_close(pCodecContext);
    avcodec_free_context(&pCodecContext);
    printf("Decoder closed.\n");

    avformat_close_input(&pFormatContext);
    printf("Input file closed.\n");

    printf("Program exit.\n");

    return 0;
}
