# STM32 环境数据采集技能包

![License](https://img.shields.io/badge/license-MIT-green) ![Language](https://img.shields.io/badge/language-C-555555) ![Platform](https://img.shields.io/badge/platform-STM32F103-blue)

> 基于 STM32F103 + 正点原子探索者开发板的环境数据采集系统，支持 SHT30/DHT11 温湿度、光敏电阻光照采集，LCD 实时显示 + 串口 CSV 输出。

## 项目结构

```
skills_stm32/
├── SKILL.md                    # 技能说明文档
├── LICENSE                     # MIT 许可证
└── STM32环境数据采集/           # Keil MDK 工程
    ├── App/                    # 应用层代码
    │   ├── app_main.c          # 主程序入口
    │   ├── app_config.h        # 配置文件
    │   ├── app_sched.c         # 1ms 时间片调度器
    │   ├── app_io.c            # GPIO/ADC 输入
    │   ├── app_display.c       # LCD 显示
    │   ├── app_key.c           # 按键处理
    │   └── app_uart.c          # 串口输出
    ├── Drivers/                # 驱动层（BSP + SYSTEM）
    ├── Projects/               # Keil 工程文件
    ├── Output/                 # 编译产物（已 gitignore）
    ├── 项目说明.md              # 详细说明文档
    ├── 扩展开发模板.md          # 扩展开发指南
    └── readme.txt              # 接线与排错说明
```

## 硬件需求

| 外设 | 型号/接口 | 说明 |
|------|-----------|------|
| MCU | STM32F103ZET6 | 正点原子探索者 |
| 温湿度 | SHT30 / DHT11 | 软件 I2C / 单总线 |
| 显示 | 3.5 寸 LCD | 正点原子 ALIENTEK |
| 存储 | 24C02 | I2C EEPROM，存储校准数据 |
| 光照 | 光敏电阻 | ADC1_CH1 (PA1) |

## 快速开始

1. 用 Keil 5.43a + AC6 打开 `STM32环境数据采集/Projects/MDK-ARM/` 下的工程
2. 编译并下载到开发板
3. 上电后 LCD 显示温湿度 + 光照值
4. KEY0 进入校准模式，KEY_UP 退出

串口输出格式（115200-8-N-1）：
```
2026-01-15 12:30:45,25.3,60.2,45
```

## 文档

- [SKILL.md](SKILL.md) - 技能总览与铁律
- [项目说明.md](STM32环境数据采集/项目说明.md) - 详细技术文档
- [扩展开发模板.md](STM32环境数据采集/扩展开发模板.md) - 如何添加新传感器

## 许可证

MIT License - 详见 [LICENSE](LICENSE)
