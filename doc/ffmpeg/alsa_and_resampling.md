# ALSA 播放与 FFmpeg 重采样指南

本文档衔接 `ffmpeg_api_usage.md`，讲述在获取解码后的 `AVFrame` 之后，如何通过 FFmpeg 的 `libswresample` 库进行格式转换，并最终使用 ALSA (Advanced Linux Sound Architecture) API 将音频数据发送到声卡进行播放。

## 1. ALSA 音频播放基础

ALSA 是 Linux 内核中标准的音频驱动和 API。要播放声音，你需要与一个“PCM 设备”进行交互。

### 核心概念

-   **句柄 (`snd_pcm_t *handle`)**:
    代表一个打开的 PCM 设备，是后续所有操作的“遥控器”。

-   **硬件参数 (`snd_pcm_hw_params_t *params`)**:
    一个结构体，用于配置声卡的硬件参数，如采样率、数据格式、声道数等。你必须设置一套硬件能支持的参数。

-   **数据格式 (`SND_PCM_FORMAT_*`)**:
    定义 PCM 数据的格式，例如 `SND_PCM_FORMAT_S16_LE` 表示“带符号16位小端整数”，这是非常常用的一种格式。

-   **访问模式 (`SND_PCM_ACCESS_*`)**:
    定义如何向硬件缓冲区读写数据。`SND_PCM_ACCESS_RW_INTERLEAVED` (交错模式) 是最常用的播放模式。在此模式下，立体声数据按 `[左声道样本1, 右声道样本1, 左声道样本2, 右声道样本2, ...]` 的顺序排列。

### ALSA 播放流程

1.  **`snd_pcm_open()`**:
    -   **作用**: 打开一个 PCM 设备。
    -   **参数**: 句柄指针、设备名 (如 `"default"` 或 `"hw:0,0"`), 流方向 (`SND_PCM_STREAM_PLAYBACK`)。

2.  **分配并初始化参数结构体**:
    -   `snd_pcm_hw_params_alloca()`: 在栈上为 `snd_pcm_hw_params_t` 分配内存。
    -   `snd_pcm_hw_params_any()`: 使用声卡支持的所有配置来初始化该结构体。

3.  **`snd_pcm_hw_params_set_*()` 系列函数**:
    -   **作用**: 在所有可能性中，选择你想要的具体参数。这是配置的核心步骤。
    -   `snd_pcm_hw_params_set_access()`: 设置访问模式 (如 `SND_PCM_ACCESS_RW_INTERLEAVED`)。
    -   `snd_pcm_hw_params_set_format()`: 设置数据格式 (如 `SND_PCM_FORMAT_S16_LE`)。
    -   `snd_pcm_hw_params_set_channels()`: 设置声道数。
    -   `snd_pcm_hw_params_set_rate_near()`: 设置采样率 (它会自动选择一个最接近你期望值的硬件支持值)。

4.  **`snd_pcm_hw_params()`**:
    -   **作用**: 将你配置好的参数应用到硬件设备上。

5.  **`snd_pcm_writei()`**:
    -   **作用**: 将**交错模式 (interleaved)** 的 PCM 数据写入声卡。
    -   **过程**: 在你的主解码循环中，你会不断调用此函数，将解码并转换好的数据块喂给声卡。
    -   **参数**: 句柄、数据缓冲区指针、要写入的**帧数** (Frames)。注意：`帧数 = 总样本数 / 声道数`。

6.  **`snd_pcm_drain()` 和 `snd_pcm_close()`**:
    -   `snd_pcm_drain()`: 等待硬件缓冲区中所有剩余的帧都播放完毕。
    -   `snd_pcm_close()`: 关闭设备，释放资源。

## 2. FFmpeg 重采样 (`libswresample`)

当 FFmpeg 解码出的 `AVFrame` 的数据格式（如 `AV_SAMPLE_FMT_FLTP`，浮点、平面模式）与 ALSA 设备期望的格式（如 `SND_PCM_FORMAT_S16_LE`，16位整数、交错模式）不匹配时，就需要进行转换。

### 核心概念

-   **重采样上下文 (`SwrContext *`)**:
    进行重采样操作的句柄，包含了输入格式、输出格式以及转换所需的所有状态。

### 重采样流程

1.  **`swr_alloc_set_opts()`**:
    -   **作用**: 这是 `libswresample` 的核心函数。它能一步到位地分配 `SwrContext` 并设置好所有转换参数。
    -   **参数**: 需要提供**输入**和**输出**两套参数，包括：
        -   声道布局 (`av_get_default_channel_layout`)
        -   采样格式 (`AVSampleFormat`)
        -   采样率 (int)

2.  **`swr_init()`**:
    -   **作用**: 初始化 `SwrContext`。在调用 `swr_alloc_set_opts` 之后、开始转换之前，必须调用此函数。

3.  **准备输出缓冲区**:
    -   你需要自己分配一块内存，用于存放转换后的数据。
    -   `av_samples_get_buffer_size()` 可以帮助你计算出所需缓冲区的大小。
    -   `av_samples_alloc()` 是一个便捷函数，可以分配内存并设置好数据指针。

4.  **`swr_convert()`**:
    -   **作用**: 执行转换。
    -   **参数**: `SwrContext`、输出数据缓冲区 (`uint8_t **out_data`)、输出缓冲区大小、输入数据 (`const uint8_t **in_data`，可以直接用 `frame->extended_data`)、输入的帧数。
    -   **返回**: 成功转换的样本数。

5.  **`swr_free()`**:
    -   **作用**: 释放 `SwrContext`。

## 3. 完整的播放逻辑链

现在，我们可以将解码、重采样和 ALSA 播放串联起来：

1.  **初始化阶段**:
    -   完成 FFmpeg 解码器的初始化 (`ffmpeg_api_usage.md` 中所述)。
    -   根据你的硬件能力，确定一个 ALSA 的目标输出参数（如 44100Hz, S16_LE, Stereo）。
    -   完成 ALSA 设备的初始化和参数设置。
    -   使用解码出的 `AVFrame` 的参数作为**输入参数**，使用 ALSA 的目标参数作为**输出参数**，初始化 `libswresample` 的 `SwrContext`。

2.  **主循环阶段**:
    -   `av_read_frame()` -> `avcodec_send_packet()` -> `avcodec_receive_frame()` 得到解码后的 `AVFrame`。
    -   调用 `swr_convert()`，将 `AVFrame` 中的数据转换到你预先分配好的输出缓冲区中。
    -   调用 `snd_pcm_writei()`，将输出缓冲区中的数据写入 ALSA 设备。

3.  **结束阶段**:
    -   循环结束后，依次释放 FFmpeg、libswresample 和 ALSA 的所有资源。

这个流程构成了一个完整播放器的核心逻辑。
