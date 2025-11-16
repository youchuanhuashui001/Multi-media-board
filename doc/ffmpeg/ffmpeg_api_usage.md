# FFmpeg API 使用指南：音频播放流程

本文档将引导你完成使用 FFmpeg 的核心 C API 从一个媒体文件中提取音频、解码并获得原始 PCM 数据的完整流程。

## 0. 准备工作：核心数据结构

在开始之前，你需要了解几个 FFmpeg 中最核心的结构体：

-   `AVFormatContext`:
    贯穿全局的结构体，包含了媒体文件的封装格式信息，例如时长、码率、包含多少个流（音频、视频、字幕等）。它是解封装（Demuxing）操作的句柄。

-   `AVStream`:
    存在于 `AVFormatContext` 中，描述一个独立的流。例如，一个 `.mp4` 文件可能有一个视频 `AVStream` 和一个音频 `AVStream`。它包含了该流的详细编码信息（如编码器ID、分辨率、采样率等）。

-   `AVCodecContext`:
    编解码器的上下文。包含了编解码器工作所需的所有参数和状态。它是解码（Decoding）操作的句柄。

-   `AVPacket`:
    存储**解码前**的、经过压缩的数据包。在解封装阶段，你从 `AVFormatContext` 中读取到的就是一个个的 `AVPacket`。

-   `AVFrame`:
    存储**解码后**的原始数据帧。对于音频，它存储的就是 PCM 数据。你将 `AVPacket` 送入解码器后，得到的就是 `AVFrame`。

## 1. 流程一：打开媒体文件并找到音频流

这是解封装的第一步，目标是初始化 `AVFormatContext` 并定位到我们感兴趣的音频流。

-   **`avformat_open_input()`**:
    -   **作用**：打开一个媒体文件或 URL。
    -   **过程**：它会读取文件头，分析容器格式（如 MP4, FLAC），并填充 `AVFormatContext` 的大部分内容。但此时它还不知道各个流的详细信息。
    -   **输入**：`AVFormatContext **` (传地址的地址), `const char *url` (文件路径), `AVInputFormat *fmt` (通常传 NULL 让 FFmpeg 自动检测), `AVDictionary **options` (附加选项，可为 NULL)。

-   **`avformat_find_stream_info()`**:
    -   **作用**：读取媒体文件的一小部分数据，来获取所有流的详细信息。
    -   **过程**：调用此函数后，`AVFormatContext->streams` 数组中的每个 `AVStream` 都会被正确填充信息（如编码格式、采样率等）。
    -   **输入**：`AVFormatContext *`。

-   **遍历 `AVFormatContext->streams`**:
    -   **作用**：找到类型为音频的流 (`AVMEDIA_TYPE_AUDIO`)。
    -   **过程**：循环遍历 `ic->streams[i]->codecpar->codec_type`，找到音频流并记录其索引 `audio_stream_index`。

## 2. 流程二：查找并打开解码器

找到音频流后，需要根据其编码信息找到对应的解码器并初始化 `AVCodecContext`。

-   **`avcodec_find_decoder()`**:
    -   **作用**：根据指定的 `codec_id` 查找 FFmpeg 中已注册的解码器。
    -   **输入**：`enum AVCodecID id` (可以从 `ic->streams[audio_stream_index]->codecpar->codec_id` 获取)。
    -   **返回**：一个 `AVCodec *` 结构体，它代表了解码器本身，但不包含状态。

-   **`avcodec_alloc_context3()`**:
    -   **作用**：为解码器上下文 `AVCodecContext` 分配内存。
    -   **输入**：`const AVCodec *codec` (上一步找到的解码器)。

-   **`avcodec_parameters_to_context()`**:
    -   **作用**：将音频流中的编码参数 (`AVCodecParameters`) 拷贝到新创建的 `AVCodecContext` 中。
    -   **输入**：`AVCodecContext *` 和 `AVCodecParameters *` (来自 `ic->streams[audio_stream_index]->codecpar`)。

-   **`avcodec_open2()`**:
    -   **作用**：使用指定的 `AVCodec` 来初始化 `AVCodecContext`，使其进入可工作状态。
    -   **输入**：`AVCodecContext *`, `const AVCodec *`, `AVDictionary **options`。

## 3. 流程三：循环读取、解码、处理

这是播放器的核心循环。不断地从文件中读取压缩数据包 (`AVPacket`)，送给解码器，然后从解码器获取解码后的数据帧 (`AVFrame`)。

-   **`av_read_frame()`**:
    -   **作用**：从 `AVFormatContext` 中读取一个 `AVPacket`。
    -   **注意**：FFmpeg 会自动处理不同容器格式的细节。你只需要循环调用此函数，直到它返回文件末尾的错误码。

-   **`avcodec_send_packet()`**:
    -   **作用**：将一个 `AVPacket` 发送给解码器。
    -   **注意**：这是一个异步 API。你发送一个 packet 后，可能需要多次接收 frame 才能取完数据。如果函数返回 `EAGAIN`，表示解码器内部缓冲区已满，你需要先用 `avcodec_receive_frame` 读取数据，再重新发送这个 packet。

-   **`avcodec_receive_frame()`**:
    -   **作用**：从解码器接收一个解码后的 `AVFrame`。
    -   **注意**：循环调用此函数，直到它返回 `EAGAIN`，表示解码器已经没有可输出的 frame 了。此时，你应该去 `av_read_frame` 读取下一个 packet。

-   **处理 `AVFrame`**:
    -   解码成功得到的 `AVFrame` 中就包含了原始的 PCM 数据，存储在 `frame->data` 数组中。
    -   **此时，你就需要将 `frame->data` 指向的音频数据，通过 ALSA 的 API 写入声卡设备。**
    -   如果 `AVFrame` 的格式（如采样率、位深度）不被声卡支持，就需要先用 `libswresample` 库进行转换，然后再写入。

## 4. 流程四：收尾与资源释放

播放结束后，或程序退出时，必须按顺序释放所有分配的资源，防止内存泄漏。

-   `av_frame_free()`: 释放 `AVFrame`。
-   `av_packet_unref()`: 释放 `AVPacket` 的引用（注意不是 `free`）。
-   `avcodec_close()`: 关闭解码器。
-   `avcodec_free_context()`: 释放解码器上下文。
-   `avformat_close_input()`: 关闭媒体文件并释放 `AVFormatContext`。

## 伪代码示例

```c
// 伪代码，仅展示API调用流程

// 0. 初始化
av_register_all(); // 在旧版FFmpeg中需要，新版可省略
AVFormatContext *pFormatCtx = avformat_alloc_context();
AVCodecContext *pCodecCtx = NULL;
AVPacket packet;
AVFrame *pFrame = av_frame_alloc();

// 1. 打开文件 & 查找流
avformat_open_input(&pFormatCtx, "my_music.mp3", NULL, NULL);
avformat_find_stream_info(pFormatCtx);

int audio_stream_index = -1;
for (int i = 0; i < pFormatCtx->nb_streams; i++) {
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        audio_stream_index = i;
        break;
    }
}

// 2. 查找并打开解码器
AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[audio_stream_index]->codecpar->codec_id);
pCodecCtx = avcodec_alloc_context3(pCodec);
avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[audio_stream_index]->codecpar);
avcodec_open2(pCodecCtx, pCodec, NULL);

// 3. 循环解码
while (av_read_frame(pFormatCtx, &packet) >= 0) {
    if (packet.stream_index == audio_stream_index) {
        // 发送 packet 到解码器
        if (avcodec_send_packet(pCodecCtx, &packet) == 0) {
            // 循环接收 frame
            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                // 成功解码得到一个 AVFrame
                // 在这里，pFrame->data 里就是 PCM 数据
                // TODO: 将 pFrame->data 通过 ALSA API 写入声卡
            }
        }
    }
    av_packet_unref(&packet); // 释放 packet 引用
}

// 4. 清理
av_frame_free(&pFrame);
avcodec_close(pCodecCtx);
avformat_close_input(&pFormatCtx);
```
