/**
 ****************************************************************************************************
 * @file        app_config.h
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       全局配置头文件 ---- 本工程所有可调参数、功能开关、预留引脚集中在这里
 * @license     Copyright (c) 2025
 ****************************************************************************************************
 * @attention
 *
 * 实验平台: 正点原子 精英版 V2 (STM32F103ZET6)
 * 项目名称: 光照强度实时采集与显示系统
 * 显示设备: 正点原子 3.5 寸 TFTLCD (NT35310, 480x320, FSMC 16位并口)
 * 编译环境: Keil MDK5 + Arm Compiler 6 (armclang)
 *
 * 修改说明
 * V1.0
 * 第一次发布
 *
 * 使用说明:
 *   1. 想增删功能  -> 只改本文件的 "一、功能开关" 段, 不用翻别的代码
 *   2. 想改采样/刷新速度 -> 只改本文件的 "二、时间参数" 段
 *   3. 想加新传感器 -> 见工程根目录的 扩展开发模板.md
 ****************************************************************************************************
 */

#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 版本信息 */

#define APP_PROJECT_NAME            "STM32 EnvDataCollect"
#define APP_VERSION_STR             "V1.0"

/******************************************************************************************/
/* 一、功能开关 (1=编译进来, 0=编译时裁剪掉) */

#define APP_USE_LIGHT               1       /* 光照采集 (核心功能, 必开) */
#define APP_USE_BARGRAPH            1       /* 界面条形图 */
#define APP_USE_ALARM               1       /* 阈值报警 (LED1 + 板载蜂鸣器) */
#define APP_USE_KEY                 1       /* 按键调阈值 */
#define APP_USE_EEPROM              1       /* 阈值掉电保存 (板载 24C02) */
#define APP_USE_CONSOLE             1       /* 串口 CSV 数据输出 */

/* ---- 以下为后续扩展预留的开关, 现在保持 0 ---- */
#define APP_USE_TOUCH               0       /* 触摸屏交互 (驱动已在 Drivers/BSP/TOUCH, 未加入工程) */
#define APP_USE_DHT11               0       /* DHT11 温湿度 (PG11 板载 U4 座) */
#define APP_USE_MQ2                 0       /* MQ-2 燃气 (PC1, 5V 输出需分压) */
#define APP_USE_WIFI                0       /* 4G / WiFi 上传 (PB10/PB11) */
#define APP_USE_SDCARD              0       /* TF 卡日志 (SDIO) */

/******************************************************************************************/
/* 二、时间参数 (单位: ms) */

#define APP_SENSOR_PERIOD_MS        100     /* 传感器采集周期, 10Hz */
#define APP_DISPLAY_PERIOD_MS       200     /* 刷屏周期, 5Hz (更快没必要, 且更费 CPU) */
#define APP_ALARM_PERIOD_MS         100     /* 报警判断周期 */
#define APP_CONSOLE_PERIOD_MS       500     /* 串口输出周期, 2Hz */
#define APP_KEY_PERIOD_MS           20      /* 按键扫描周期 */
#define APP_IO_PERIOD_MS            500     /* 扩展槽任务周期 (默认关闭) */

/******************************************************************************************/
/* 三、光照采集参数 */

#define LIGHT_ADC_CHANNEL           6       /* PF8 = ADC3_IN6, 板载光敏电阻 LS1, 不可改 */
#define LIGHT_FILTER_DEPTH          8       /* 滑动平均窗口点数, 必须是 2 的幂 */
#define LIGHT_IIR_SHIFT             2       /* 一阶 IIR: filt += (avg-filt)>>N, N 越大越平滑 */
#define LIGHT_RAW_FULL_SCALE        4095    /* 12 位 ADC 满量程 */
#define LIGHT_ADC_DIVISOR           40      /* 官方换算系数: 码值/40 -> 0~100 */

/* 光强分级门限 (0~100), 用于界面文字与配色 */
#define LIGHT_LV1_MAX               9       /* 0 ~ 9    很暗  V.DARK  */
#define LIGHT_LV2_MAX               39      /* 10 ~ 39  偏暗  DARK    */
#define LIGHT_LV3_MAX               69      /* 40 ~ 69  适中  NORMAL  */
#define LIGHT_LV4_MAX               89      /* 70 ~ 89  偏亮  BRIGHT  */
                                            /* 90 ~100  很亮  V.BRIGHT*/

/******************************************************************************************/
/* 四、报警参数 */

#define LIGHT_ALARM_TH_DEF          70      /* 默认报警阈值 */
#define LIGHT_ALARM_TH_MIN          10      /* 阈值可调下限 */
#define LIGHT_ALARM_TH_MAX          95      /* 阈值可调上限 */
#define LIGHT_ALARM_TH_STEP         5       /* 按键每次调整步进 */
#define LIGHT_ALARM_HYST            3       /* 滞回量, 防止在阈值附近反复通断 */

/* 蜂鸣器断续鸣叫节奏 (ms) */
#define ALARM_BEEP_ON_MS            200     /* 一个周期内响多久 */
#define ALARM_BEEP_CYCLE_MS         1000    /* 周期 */

/******************************************************************************************/
/* 五、显示参数 */

#define LCD_DIR                     1       /* 0=竖屏 320x480, 1=横屏 480x320 */
#define UI_MAX_SENSORS              8       /* 界面最多同时显示多少个传感器 */

#define UI_HEADER_H                 32      /* 标题栏高度 */
#define UI_FOOTER_H                 40      /* 状态栏高度 */
#define UI_MARGIN                   12      /* 卡片外边距 */

#define UI_HEADER_BG                0x01CF  /* 标题栏底色, 深蓝 */
#define UI_FOOTER_BG                0x8410  /* 状态栏底色, 深灰 */
#define UI_CARD_BG                  0xFFFF  /* 卡片底色, 白 */
#define UI_CARD_FRAME               0xC618  /* 卡片边框, 浅灰 */
#define UI_TITLE_FG                 0xFFFF  /* 标题文字, 白 */
#define UI_LABEL_FG                 0x0000  /* 标签文字, 黑 */
#define UI_UNIT_FG                  0x0000  /* 单位文字, 黑 */
#define UI_EXTRA_FG                 0x0000  /* 明细行文字, 黑 */
#define UI_BAR_BG                   0xC618  /* 进度条底色, 浅灰 */
#define UI_BAR_FRAME                0x8410  /* 进度条边框, 深灰 */
#define UI_HINT_FG                  0xFFFF  /* 状态栏文字, 白 */
#define UI_ONLINE_FG                0x07E0  /* 在线指示, 绿 */
#define UI_OFFLINE_FG               0xF800  /* 掉线指示, 红 */
#define UI_TH_MARK                  0xF800  /* 阈值刻线, 红 */

/* 未接传感器时的占位文字 (LCD 只支持 ASCII, 中文需实验36 的字库方案) */
#define UI_OFFLINE_TEXT             "OFFLINE"

/******************************************************************************************/
/* 六、掉电保存参数 (板载 24C02, PB6/PB7 软件 IIC) */
/* 注意: 地址 255 已被 at24cxx_check() 占用, 不要使用 */

#define EE_ADDR_TH                  0x00    /* 报警阈值存放地址 */
#define EE_ADDR_MAGIC               0x02    /* 参数有效标志存放地址 */
#define EE_MAGIC_VAL                0xA5    /* 标志值 */

/******************************************************************************************/
/* 七、扩展预留引脚表 (本阶段不驱动, 供后续接线参考, 详见 项目说明.md) */
/*
 *   +----------+------------------------+--------------------------------------+
 *   | 预留功能 | 引脚                   | 说明                                 |
 *   +----------+------------------------+--------------------------------------+
 *   | DHT11    | PG11                   | 板载 U4 单总线座, 直插免接线         |
 *   | DS18B20  | PG11                   | 同上, 与 DHT11 二选一                |
 *   | MQ-2     | PC1  (ADC12_IN11)      | 5V 输出必须 10k/20k 分压后再进 ADC   |
 *   | MQ-135   | PC2  (ADC12_IN12)      | 同上                                 |
 *   | 备用模拟 | PC3/PC4/PC5, PF3~PF7   | 插着 3.5 寸屏时依然干净              |
 *   | 继电器   | PD11                   | 必须三极管 + 续流二极管 + 光耦隔离   |
 *   | 串口模块 | PB10/PB11 (USART3)     | 板载 ATK-MODULE 座, 直插             |
 *   | PWM      | PC6/PC7 (TIM8_CH1/CH2) | 调光/调速首选, 无引脚冲突            |
 *   | TF 卡    | PC8~PC12 / PD2 (SDIO)  |                                      |
 *   | 蜂鸣器   | PB8  (已用)            | 板载, 报警用                         |
 *   | 心跳灯   | PB5  (已用)            | LED0                                 |
 *   | 报警灯   | PE5  (已用)            | LED1                                 |
 *   +----------+------------------------+--------------------------------------+
 */

#endif
