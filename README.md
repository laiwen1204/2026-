# 2026 CIMC "西门子杯" 工业嵌入式系统开发 —— 数采仪表终端

2026 年 CIMC"西门子杯"中国智能制造挑战赛 · 工业嵌入式系统开发方向（初赛）参赛工程。
基于 **GD32F470（Cortex-M4）** 的工业数据采集终端，包含 Bootloader 与 APP 两个独立 Keil 工程。

## 功能特性

### 软件（C / 裸机）
- 自研时间片调度器，管理协议解析、定时自动上报、告警检测、OLED 显示等任务
- 自定义串口通信协议栈：ASCII 组帧 / 帧解析 / CRC16-Modbus 校验 / 异常帧处理，覆盖 30+ 命令字
- Bootloader + OTA 在线升级：Flash 分区管理（Boot / 参数区 / App / App 备份 / 固件暂存）、固件切片接收、CRC32 校验、魔术字验证、失败自动回滚备份固件
- 参数持久化：内部 Flash 参数区（Magic Word + CRC32），断电保持设备 ID、波特率、变比、阈值、告警记录
- 低功耗：MCU 深睡眠（Deep-Sleep）+ RTC 闹钟 10 s 自动唤醒
- 外设驱动：UART（RS485）、ADC/DMA、DAC、SPI（外置 ADC / Flash）、I2C（OLED）、RTC、PMU

### 硬件（嘉立创 EDA 专业版）
- DC-DC 电源板：GD30DC1354 降压方案，18~36 V 宽压输入转 5 V，满载输出精度 5 V ± 0.15 V
- PT100 温度采样板：恒流源激励 + 差分放大 + GD30AD3344 高精度 ADC（SPI），-50~150 ℃ 全量程误差 ≤ ±8 ℃

## 目录结构

```
├── APP/                # 应用层工程（Keil）
│   ├── Driver/         # 硬件驱动层（UART/IIC/SPI/OLED/Flash/RTC...）
│   ├── Protocol/       # 通讯协议层（帧解析、应答组帧、CRC 校验）
│   ├── Function/       # 业务逻辑层（采样、参数管理、告警、睡眠、调度器）
│   └── User/           # main、中断、systick
├── BootLoader/         # Bootloader 工程（Keil）
│   ├── Driver/
│   ├── Function/       # 固件接收、校验、搬运、回滚、跳转
│   └── User/
```

## Flash 分区（GD32F470，512K）

| 区域 | 起始地址 | 大小 |
|---|---|---|
| Bootloader | 0x08000000 | 64K |
| 参数区 | 0x08010000 | 4K |
| App | 0x08011000 | 128K |
| App 备份 | 0x08031000 | 128K |
| 固件暂存 | 0x08051000 | 128K |

## 构建

使用 Keil MDK-ARM 分别打开 `APP/project` 与 `BootLoader/project` 下的 `.uvprojx` 工程编译下载（Bootloader 先烧，App 后烧）。默认串口参数：USART1 / RS485 / 19200-8-N-1。
