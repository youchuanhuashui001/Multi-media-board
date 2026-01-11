# Control Center MQTT Status

## ADDED Requirements

### Requirement: MQTT 连接状态显示

控制中心 SHALL 显示 MQTT 连接状态。

#### Scenario: 显示已连接状态

- **GIVEN** 控制中心已打开
- **WHEN** MQTT 客户端已成功连接到 Broker
- **THEN** 控制中心 SHALL 显示绿色状态指示（如绿点或图标）
- **AND** 状态文字显示 "已连接" 或 "MQTT: 已连接"

#### Scenario: 显示未连接状态

- **GIVEN** 控制中心已打开
- **WHEN** MQTT 客户端未连接或断开连接
- **THEN** 控制中心 SHALL 显示红色/灰色状态指示（如红点或灰色图标）
- **AND** 状态文字显示 "未连接" 或 "MQTT: 断开"

#### Scenario: 显示连接中状态

- **GIVEN** 控制中心已打开
- **WHEN** MQTT 客户端正在异步连接中
- **THEN** 控制中心 SHALL 显示黄色/橙色状态指示（如黄点或加载图标）
- **AND** 状态文字显示 "连接中..." 或 "MQTT: 连接中"

#### Scenario: 实时更新状态

- **GIVEN** 控制中心正在显示
- **WHEN** MQTT 连接状态发生变化
- **THEN** 状态指示 SHALL 在 1 秒内更新

---

### Requirement: MQTT 开关控制

控制中心 SHALL 提供 MQTT 连接开关控制。

#### Scenario: 手动断开连接

- **GIVEN** 控制中心已打开
- **AND** MQTT 处于已连接状态
- **WHEN** 用户点击 MQTT 开关/按钮
- **THEN** 系统 SHALL 断开 MQTT 连接
- **AND** 状态指示更新为未连接

#### Scenario: 手动连接

- **GIVEN** 控制中心已打开
- **AND** MQTT 处于未连接状态
- **WHEN** 用户点击 MQTT 开关/按钮
- **THEN** 系统 SHALL 尝试连接到 MQTT Broker
- **AND** 连接成功后状态指示更新为已连接

#### Scenario: 连接失败反馈

- **GIVEN** 用户手动点击连接
- **WHEN** 连接失败（网络问题、认证失败等）
- **THEN** 状态指示保持未连接状态
- **AND** 可选：显示简短错误提示
