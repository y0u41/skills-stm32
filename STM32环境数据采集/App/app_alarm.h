/**
 ****************************************************************************************************
 * @file        app_alarm.h
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       阈值报警模块 (LED1 + 板载蜂鸣器, 预留执行器钩子)
 ****************************************************************************************************
 */

#ifndef __APP_ALARM_H
#define __APP_ALARM_H

#include "./SYSTEM/sys/sys.h"

void     app_alarm_init(void);          /* 初始化蜂鸣器, 并从 24C02 读回上次保存的阈值 */
void     app_alarm_task(void);          /* 阈值判断 + 声光报警 (由调度器周期调用) */
uint8_t  app_alarm_is_active(void);     /* 1 = 正在报警 */
uint8_t  app_alarm_get_th(void);        /* 取当前报警阈值 */
void     app_alarm_set_th(uint8_t th);  /* 设置报警阈值 (自动钳位到允许范围) */
void     app_alarm_save_th(void);       /* 把阈值写入 24C02 (掉电保存) */

#endif
