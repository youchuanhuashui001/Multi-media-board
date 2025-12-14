# audio-view Specification

## Purpose
TBD - created by archiving change add-volume-slider. Update Purpose after archive.
## Requirements
### Requirement: Volume Control Slider

音频播放界面 SHALL 提供音量控制滑动条，允许用户通过拖动滑动条实时调整播放音量。

#### Scenario: 用户通过滑动条调节音量

- **WHEN** 用户在音量滑动条上拖动或点击
- **THEN** 系统 SHALL 调用 `audio_engine_set_volume()` 设置对应的音量百分比
- **AND** 音频播放音量 SHALL 即时变化

#### Scenario: 界面显示当前音量

- **WHEN** 音频播放界面加载或刷新
- **THEN** 音量滑动条 SHALL 显示当前系统音量值（通过 `audio_engine_get_volume()` 获取）

#### Scenario: 音量范围限制

- **WHEN** 用户调节音量滑动条
- **THEN** 音量值 SHALL 在 0（静音）到 100（最大）范围内
- **AND** 滑动条位置 SHALL 反映实际音量百分比

### Requirement: Dynamic Volume Icon

音频播放界面 SHALL 显示动态音量图标，根据当前音量大小显示不同的图标状态。

#### Scenario: 静音状态图标

- **WHEN** 音量值为 0
- **THEN** 音量图标 SHALL 显示静音图标 (`volume_mute.png`)

#### Scenario: 低音量状态图标

- **WHEN** 音量值在 1 到 33 之间
- **THEN** 音量图标 SHALL 显示低音量图标 (`volume_low.png`)

#### Scenario: 中音量状态图标

- **WHEN** 音量值在 34 到 66 之间
- **THEN** 音量图标 SHALL 显示中音量图标 (`volume_mid.png`)

#### Scenario: 高音量状态图标

- **WHEN** 音量值在 67 到 100 之间
- **THEN** 音量图标 SHALL 显示高音量图标 (`volume_high.png`)

#### Scenario: 音量变化时图标更新

- **WHEN** 用户通过滑动条改变音量
- **THEN** 音量图标 SHALL 立即更新以反映新的音量级别

