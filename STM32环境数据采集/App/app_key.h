/**
 ****************************************************************************************************
 * @file        app_key.h
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       按键模块 ---- 调整报警阈值 / 保存到 24C02
 ****************************************************************************************************
 */

#ifndef __APP_KEY_H
#define __APP_KEY_H

#include "./SYSTEM/sys/sys.h"

void app_key_init(void);    /* 初始化按键 IO */
void app_key_task(void);    /* 扫描按键并处理 (由调度器周期调用) */

#endif
