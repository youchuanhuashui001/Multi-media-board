# I2S 驱动概述

## 简介

I2S (Inter-IC Sound) 是一种用于数字音频设备之间传输数据的串行总线协议。
i.MX6ULL 使用 SAI (Synchronous Audio Interface) 模块，兼容 I2S 协议。

## 硬件框图

```
┌─────────────┐     ┌─────────────┐     ┌──────────┐
│   CPU       │ <-> │    SAI      │ <-> │  Codec   │
│             │     │  (I2S)      │     │  (WM8960)│
└─────────────┘     └─────────────┘     └──────────┘
       │                  │
       │            ┌─────┴─────┐
    AHB Bus         │           │
                   BCLK        LRCLK
                    │           │
                   DATA ────────┘
```

## I2S 信号

| 信号 | 说明 |
|------|------|
| MCLK | 主时钟 (Master Clock) |
| BCLK | 位时钟 (Bit Clock) |
| LRCLK | 左右声道选择 (Frame Sync) |
| DATA | 数据线 (TX/RX) |

## 设备树节点

```dts
/* SAI 节点示例 */
&sai2 {
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_sai2>;
	assigned-clocks = <&clks IMX6UL_CLK_SAI2_SEL>,
			  <&clks IMX6UL_CLK_SAI2>;
	assigned-clock-parents = <&clks IMX6UL_CLK_PLL4_AUDIO_DIV>;
	assigned-clock-rates = <0>, <12288000>;
	status = "okay";
};
```

## 内核配置

```
Device Drivers  --->
    <*> Sound card support  --->
        <*> Advanced Linux Sound Architecture  --->
            <*> ALSA for SoC audio support  --->
                <*> SoC Audio for Freescale i.MX CPUs
```

## 相关源码

| 文件 | 说明 |
|------|------|
| `sound/soc/fsl/fsl_sai.c` | SAI 驱动 |
| `sound/soc/fsl/imx-audmux.c` | 音频多路复用器 |
| `sound/soc/codecs/wm8960.c` | WM8960 编解码器驱动 |

## 学习目标

- [ ] 理解 I2S 协议和时序
- [ ] 分析 SAI 驱动框架
- [ ] 理解 ALSA 音频子系统
- [ ] 掌握音频调试技巧

## 备注

<!-- 在此记录其他相关信息 -->
