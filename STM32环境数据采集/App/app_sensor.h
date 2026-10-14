/**
 ****************************************************************************************************
 * @file        app_sensor.h
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       通用传感器注册表 ---- 本工程"可扩展"的核心机制之二
 ****************************************************************************************************
 * @attention
 *
 * 设计目的:
 *   如果显示代码里写死 "画光照值", 以后加 DHT11 就得改显示函数; 再加 MQ-2 又要改一遍,
 *   越改越乱。本模块定义统一的传感器接口, 所有传感器都实现同一组函数并登记到一张表里;
 *   显示层和串口层只遍历这张表, 因此传感器数量变化时界面会自动跟着变。
 *
 * 新增一个传感器只需要三步:
 *   1) 底层驱动放到 Drivers/BSP/<模块>/;
 *   2) 在 App/ 下加一个 app_xxx.c, 实现 xxx_sensor_init() 与 xxx_sensor_poll();
 *   3) 到 app_sensor.c 的 g_sensors[] 表里加一行。
 *   -> 屏幕会自动多出一行数据, 串口 CSV 会自动多出一列, 显示模块一个字都不用改。
 ****************************************************************************************************
 */

#ifndef __APP_SENSOR_H
#define __APP_SENSOR_H

#include "./SYSTEM/sys/sys.h"

/* 通用传感器描述结构体 */
typedef struct
{
    const char *name;           /* 英文短名, 用于串口 CSV 表头, 如 "LIGHT" */
    const char *label;          /* 界面标签 (LCD 只支持 ASCII), 如 "LIGHT INTENSITY" */
    const char *unit;           /* 单位, 如 "%" */

    uint8_t   (*init)(void);    /* 初始化, 返回 0 = 成功 */
    uint8_t   (*poll)(int32_t *out);  /* 采集一次, 结果写到 *out, 返回 0 = 成功 */

    int32_t     vmin;           /* 量程下限, 用于画条形图 */
    int32_t     vmax;           /* 量程上限 */

    int32_t     value;          /* 当前值, 由 app_sensor_poll_all() 填写 */
    uint32_t    poll_cnt;       /* 累计采集次数 */
    uint8_t     online;         /* 1 = 在线, 0 = 掉线/初始化失败 */
    uint8_t     enable;         /* 1 = 启用, 0 = 停用 */

    /* 可选钩子, 不需要就填 NULL */
    const char *(*level_text)(int32_t value);   /* 把数值翻译成等级文字 (如 NORMAL) */
    uint16_t    (*color_of)(int32_t value);     /* 把数值翻译成显示颜色 */
} app_sensor_t;

void          app_sensor_init_all(void);        /* 初始化表里所有启用的传感器 */
void          app_sensor_poll_all(void);        /* 采集一遍表里所有启用的传感器 */
void          app_sensor_task(void);            /* 调度器入口, 内部调用 poll_all */
uint8_t       app_sensor_count(void);           /* 启用的传感器个数 */
app_sensor_t *app_sensor_get(uint8_t idx);      /* 取第 idx 个启用的传感器 (idx 从 0 开始) */
app_sensor_t *app_sensor_find(const char *name);/* 按英文短名查找 */
void          app_sensor_print_header(void);    /* 串口 CSV 表头 */
void          app_sensor_print_row(void);       /* 串口 CSV 数据行 */

#endif
