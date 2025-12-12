# Project Context

## Purpose
本项目是基于 100ask i.MX6ULL 开发板的多媒体应用平台。
主要功能包括：
- 电子书阅读器：支持文本显示、翻页、书架管理、字体渲染（FreeType）。
- 音乐播放器：支持音频播放（ALSA/FFmpeg）、歌词显示、播放控制（暂停/恢复/切歌）。
- 视频播放器：支持视频播放（FFmpeg/MPlayer）。
- 图形用户界面：使用 LVGL v9.x 构建现代化 UI。

## Tech Stack
- **Languages**: C (主要), C++ (部分)
- **Build System**: Makefile, Buildroot (交叉编译工具链 `arm-buildroot-linux-gnueabihf-`)
- **GUI Framework**: LVGL v9.x (LittlevGL)
- **Multimedia**:
    - **Audio**: ALSA (asound), FFmpeg (avcodec, avformat, etc.)
    - **Video**: FFmpeg, MPlayer
- **Libraries**:
    - FreeType (字体渲染)
    - tslib (触摸屏输入)
    - libpng, brotli (图像/压缩)
- **Hardware**: 100ask i.MX6ULL 开发板 (ARM Cortex-A7)
- **OS**: Embedded Linux (Buildroot generated)

## Project Conventions

### Code Style
- **Indentation**: 使用 TAB 缩进，TAB 长度为 8。
- **Language**: 注释和文档主要使用简体中文。
- **Compiler Flags**: `-Wall -Wextra` 等，保持代码整洁，减少警告。
- **Directory Structure**:
    - `src/`: 源代码
        - `app/`: 应用层逻辑 (ui, core)
        - `include/`: 头文件
    - `lvgl/`: LVGL 库
    - `doc/`: 项目文档
    - `build/`: 编译输出

### Architecture Patterns
- **Module Design**: 采用分层架构，主要分为 UI 层 (`src/app/ui`) 和 核心业务层 (`src/app/core`)。
- **Event Driven**: UI 与业务逻辑通过事件或回调进行交互（如 LVGL 回调）。
- **Separation of Concerns**: 音频播放、电子书渲染等核心功能封装在独立模块中。

### Testing Strategy
- **Manual Testing**: 在开发板上运行程序进行功能验证（如触摸响应、音频输出、屏幕显示）。
- **Integration Testing**: 集成各个模块（UI + Core + Drivers）进行系统测试。

### Git Workflow
- **Submodules**: 使用 Git Submodules 管理第三方库（如 LVGL）。
    - Clone: `git clone --recurse-submodules ...`
    - Update: `git submodule update --init --recursive`

## Domain Context
- **Embedded Linux**: 涉及 FrameBuffer 显示驱动、Input 子系统（触摸屏）、ALSA 音频驱动等底层交互。
- **Cross-Compilation**: 开发在 PC (Host) 上进行，运行在开发板 (Target) 上，需要处理交叉编译环境配置。

## Important Constraints
- **Hardware Resources**: i.MX6ULL 资源有限（CPU/RAM），需注意性能优化，避免内存泄漏。
- **Display**: 屏幕分辨率和色彩深度限制，UI 设计需适配。
- **Dependencies**: 依赖特定的交叉编译工具链和库版本（如 Buildroot 2020.02.x）。

## External Dependencies
- **100ask SDK**: 包含交叉编译工具链、内核源码、根文件系统等。
- **Third-party Libs**: FFmpeg, FreeType, tslib 等库文件需在 SDK 中正确配置路径。
