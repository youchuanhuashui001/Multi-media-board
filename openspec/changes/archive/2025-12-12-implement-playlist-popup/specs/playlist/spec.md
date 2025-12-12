# Spec: Audio Player Playlist

## ADDED Requirements

### Requirement: Playlist Popup Display
音频播放器 SHALL 提供一个播放列表弹窗，以显示可用歌曲列表。

#### Scenario: Open Playlist
Given 音频播放器正在运行
When 用户点击“播放列表”按钮
Then 播放列表弹窗应该出现
And 当前歌曲应该高亮显示

#### Scenario: Close Playlist
Given 播放列表弹窗已打开
When 用户点击背景
Then 播放列表弹窗应该关闭

#### Scenario: Playlist Content
Given 播放列表弹窗已打开
Then 列表应该显示当前目录中的所有歌曲
And 每一行应该显示歌曲标题和歌手

### Requirement: Playlist Interaction
用户 SHALL 能够从播放列表中选择一首歌曲进行播放。

#### Scenario: Play Song from Playlist
Given 播放列表弹窗已打开
When 用户点击列表中的一首歌曲
Then 选中的歌曲应该开始播放
And 弹窗应该关闭
And 主视图应该更新为新歌曲
