/**
 ****************************************************************************************************
 * @file        app_display.h
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       显示模块 ---- 表驱动布局 + 局部刷新 (绝不整屏重画)
 ****************************************************************************************************
 */

#ifndef __APP_DISPLAY_H
#define __APP_DISPLAY_H

#include "./SYSTEM/sys/sys.h"

void app_display_init(void);    /* 画静态界面 (标题栏 / 卡片框 / 状态栏底板) */
void app_display_task(void);    /* 刷新动态数据 (只重画变化的区域) */

#endif
