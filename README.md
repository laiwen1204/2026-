# 2026 CIMC "西门子杯" 工业嵌入式系统开发

基于 GD32F470（Cortex-M4）的工业数采终端，含 Bootloader 与 APP 两个 Keil 工程。

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
