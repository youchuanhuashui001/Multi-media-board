# SDHCI 驱动概述

## 简介

SDHCI (Secure Digital Host Controller Interface) 是 SD 卡控制器的标准接口规范。
i.MX6ULL 使用的是 uSDHC (Ultra Secured Digital Host Controller)，与 SDHCI 兼容。

## 硬件框图

```
┌─────────────┐     ┌─────────────┐     ┌──────────┐
│   CPU       │ <-> │   uSDHC     │ <-> │  SD Card │
│             │     │ Controller  │     │  / EMMC  │
└─────────────┘     └─────────────┘     └──────────┘
       │                  │
       │                  │
    AHB Bus          SD/MMC Bus
```

## 设备树节点

```dts
/* i.MX6ULL 的 uSDHC 节点示例 */
&usdhc1 {
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_usdhc1>;
	cd-gpios = <&gpio1 19 GPIO_ACTIVE_LOW>;
	bus-width = <4>;
	status = "okay";
};
```

## 内核配置

```
Device Drivers  --->
    <*> MMC/SD/SDIO card support  --->
        <*> Secure Digital Host Controller Interface support
        <*> SDHCI platform and target drivers  --->
            <*> SDHCI support for the Freescale eSDHC/uSDHC i.MX controller
```

## 相关源码

| 文件 | 说明 |
|------|------|
| `drivers/mmc/host/sdhci.c` | SDHCI 核心驱动 |
| `drivers/mmc/host/sdhci-esdhc-imx.c` | i.MX 平台驱动 |
| `drivers/mmc/core/` | MMC 核心子系统 |

## 学习目标

- [ ] 理解 SDHCI 硬件工作原理
- [ ] 分析 SDHCI 驱动框架
- [ ] 理解 DMA 传输机制
- [ ] 掌握调试技巧

## 备注

<!-- 在此记录其他相关信息 -->
