# Change: 在 audio_view 中添加音量控制滑动条

## Why

当前音频播放器界面缺少音量调节功能，用户无法在播放界面直接调整音量大小。虽然 `audio_engine` 已提供音量控制 API，但 UI 层尚未暴露此功能，影响用户体验。

## What Changes

- 在 `audio_view` 控制区域添加音量滑动条 (Volume Slider)
- 添加音量图标/标签指示当前音量状态
- 滑动条与 `audio_engine_set_volume()` / `audio_engine_get_volume()` API 对接
- 支持实时音量调节，拖动滑动条即时生效

## Impact

- Affected specs: 新增 `audio-view` spec
- Affected code:
  - `src/app/ui/audio_view.c` - 添加音量滑动条 UI 组件
  - `src/include/app/audio_view.h` - 无需修改（内部实现）
