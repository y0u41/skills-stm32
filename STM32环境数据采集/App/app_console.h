/**
 ****************************************************************************************************
 * @file        app_console.h
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       串口控制台模块 ---- 输出 CSV 数据流, 为以后接 WiFi/4G 上传铺路
 ****************************************************************************************************
 */

#ifndef __APP_CONSOLE_H
#define __APP_CONSOLE_H

#include "./SYSTEM/sys/sys.h"

void app_console_init(void);    /* 打印开机信息 + CSV 表头 */
void app_console_task(void);    /* 周期输出一行 CSV */

#endif
