/**
 ****************************************************************************************************
 * @file        app_key.c
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       按键模块实现
 ****************************************************************************************************
 * @attention
 *
 * 板载按键 (已核实):
 *   KEY0  -> PE4, 按下为低电平
 *   KEY1  -> PE3, 按下为低电平
 *   WK_UP -> PA0, 按下为高电平 (唯一一个高电平有效的按键)
 *
 * 功能分配:
 *   KEY0  阈值 -5%      (下限 LIGHT_ALARM_TH_MIN)
 *   KEY1  阈值 +5%      (上限 LIGHT_ALARM_TH_MAX)
 *   WK_UP 把当前阈值写入板载 24C02, 掉电不丢
 *
 * 说明:
 *   key_scan(0) 是官方提供的非阻塞扫描函数, 参数 0 表示"不支持连按",
 *   内部只在检测到按下时 delay_ms(10) 去抖, 不影响其它任务。
 ****************************************************************************************************
 */

#include <stdio.h>
#include "app_config.h"
#include "app_key.h"
#include "app_alarm.h"
#include "./BSP/KEY/key.h"

/**
 * @brief       初始化按键
 */
void app_key_init(void)
{
    key_init();
}

/**
 * @brief       按键任务
 */
void app_key_task(void)
{
    uint8_t key = key_scan(0);      /* 0 = 不支持连按 */

    if (key == 0)
    {
        return;
    }

    if (key == KEY0_PRES)           /* 阈值 - 5% */
    {
        uint8_t th = app_alarm_get_th();

        if (th > LIGHT_ALARM_TH_MIN)
        {
            app_alarm_set_th((uint8_t)(th - LIGHT_ALARM_TH_STEP));
        }

        printf("alarm threshold = %u%%\r\n", (unsigned int)app_alarm_get_th());
    }
    else if (key == KEY1_PRES)      /* 阈值 + 5% */
    {
        uint8_t th = app_alarm_get_th();

        if (th < LIGHT_ALARM_TH_MAX)
        {
            app_alarm_set_th((uint8_t)(th + LIGHT_ALARM_TH_STEP));
        }

        printf("alarm threshold = %u%%\r\n", (unsigned int)app_alarm_get_th());
    }
    else if (key == WKUP_PRES)      /* 保存到 24C02 */
    {
        app_alarm_save_th();
        printf("threshold %u%% saved to 24C02\r\n", (unsigned int)app_alarm_get_th());
    }
}
