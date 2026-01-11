# i.MX6ULL 芯片规格

## 基本信息

| 项目 | 规格 |
|------|------|
| 厂商 | NXP (原 Freescale) |
| 内核 | ARM Cortex-A7 |
| 主频 | 最高 900MHz (通常运行 792MHz) |
| 架构 | ARMv7-A |
| 工艺 | 40nm |

## 内存

| 项目 | 规格 |
|------|------|
| DDR3/DDR3L | 最高 16-bit, 400MHz |
| 内部 RAM | 128KB OCRAM |
| Boot ROM | 96KB |

## 外设接口

| 外设 | 数量 | 说明 |
|------|------|------|
| UART | 8 | 串口 |
| I2C | 4 | I2C 总线 |
| SPI | 4 | SPI 总线 (含 QSPI) |
| USB | 2 | USB 2.0 OTG |
| Ethernet | 2 | 10/100 Mbps |
| SD/MMC | 2 | SD 卡 / EMMC |
| LCD | 1 | 并行 RGB 接口 |
| SAI | 3 | 音频接口 (I2S) |
| PWM | 8 | 脉宽调制 |
| GPIO | 多个 | 通用 IO |

## 地址映射

| 区域 | 起始地址 | 结束地址 | 大小 |
|------|----------|----------|------|
| Boot ROM | 0x00000000 | 0x00017FFF | 96KB |
| OCRAM | 0x00900000 | 0x0091FFFF | 128KB |
| AIPS-1 | 0x02000000 | 0x020FFFFF | 1MB |
| AIPS-2 | 0x02100000 | 0x021FFFFF | 1MB |
| DDR | 0x80000000 | 0x9FFFFFFF | 512MB (最大) |

## 参考资料

- [i.MX6ULL Applications Processor Reference Manual](https://www.nxp.com/)
- [i.MX6ULL Datasheet](https://www.nxp.com/)

## 备注

<!-- 在此记录其他相关信息 -->
