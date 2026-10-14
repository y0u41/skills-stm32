# 📊 STM32 环境数据采集技能包

![License](https://img.shields.io/badge/license-MIT-green) ![Language](https://img.shields.io/badge/language-C-555555) ![Platform](https://img.shields.io/badge/platform-STM32F103-blue) ![Build](https://img.shields.io/badge/build-0%20Error-brightgreen)

> 基于 STM32 + 正点原子探索者开发板的环境数据采集系统，LCD 实时显示 + 串口 CSV 输出 📈

## 这是什么？

一个**模块化设计**的 STM32 环境采集系统，支持 SHT30/DHT11 温湿度、光敏电阻光照采集，采用 1ms 时间片调度器，代码结构清晰，适合作为 STM32 项目模板。

**适合场景**：
- 📚 学习 STM32 传感器采集
- 🛠️ 快速搭建环境监测项目的模板
- 📖 理解时间片调度与模块化设计

## ✨ 功能特性

### 采集功能
- 🌡 **温湿度**：SHT30（I2C）/ DHT11（单总线）双支持
- 💡 **光照**：光敏电阻 ADC 采集
- 📺 **LCD 实时显示**：温湿度 + 光照值

### 数据输出
- 📤 **串口 CSV**：115200-8-N-1，便于导入 Excel 分析
- 📅 格式：`日期,时间,温度,湿度,光照`

### 工程特性
- ⏱ **1ms 时间片调度器**：非阻塞架构
- 🎯 **模块化设计**：采集/显示/按键/串口解耦
- 🔧 **校准支持**：KEY0 进入校准模式

## 🛠️ 硬件要求

| 外设 | 型号/接口 | 说明 |
|------|-----------|------|
| MCU | STM32F103ZET6 | 正点原子探索者 |
| 温湿度 | SHT30 / DHT11 | 软件 I2C / 单总线 |
| 显示 | 3.5 寸 LCD | ALIENTEK |
| 光照 | 光敏电阻 | ADC1_CH1 (PA1) |

## 📁 目录结构

```
skills-stm32/
├── SKILL.md                    # 技能总览
├── LICENSE                     # MIT 许可证
└── STM32环境数据采集/           # Keil MDK 工程
    ├── App/                    # 应用层
    │   ├── app_main.c          # 主程序入口
    │   ├── app_sched.c         # 1ms 时间片调度器
    │   ├── app_io.c            # GPIO/ADC 输入
    │   ├── app_display.c       # LCD 显示
    │   └── app_uart.c          # 串口输出
    ├── Drivers/                # 驱动层
    └── Projects/               # Keil 工程文件
```

## 🚀 快速开始

```bash
# 1. 用 Keil 5.43a + AC6 打开工程
#    STM32环境数据采集/Projects/MDK-ARM/

# 2. F7 编译 → F8 下载

# 3. 上电查看 LCD 显示

# 4. 串口查看 CSV 输出（115200-8-N-1）
```

**串口输出示例**：
```
2026-01-15 12:30:45,25.3,60.2,45
```

## 📚 文档

| 文档 | 内容 |
|------|------|
| [SKILL.md](SKILL.md) | 技能总览与铁律 |
| [项目说明.md](STM32环境数据采集/项目说明.md) | 详细技术文档 |
| [扩展开发模板.md](STM32环境数据采集/扩展开发模板.md) | 如何添加新传感器 |

## 📄 许可证

[MIT License](LICENSE) © y0u41
