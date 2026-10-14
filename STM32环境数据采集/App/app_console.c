/**
 ****************************************************************************************************
 * @file        app_console.c
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       串口控制台模块实现
 ****************************************************************************************************
 * @attention
 *
 * 串口重定向:
 *   官方 Drivers/SYSTEM/usart/usart.c 里已经把 fputc() 重定向到 USART1, 并且用
 *   #if (__ARMCC_VERSION >= 6010050) 分别处理了 AC5 / AC6 两种编译器的半主机模式,
 *   所以本文件可以直接使用标准库的 printf()。
 *   波特率 115200。请用 XCOM / SSCOM 打开 ---- 官方 readme 提醒: 其它串口助手可能
 *   误动 DTR/RTS, 导致 MCU 复位或程序不运行。
 *
 * ★ 为什么这里的输出全是英文 (重要, 别改成中文):
 *   printf 的格式串是"字符串字面量", 如果源码用 GBK 编码, 中文字节里有可能出现
 *   0x5C (反斜杠), 编译器会把它当成转义字符或续行符, 导致字符串被截断甚至编译报错。
 *   注释里写中文没有这个问题(不会被当代码解析), 但字符串字面量里一律用 ASCII 最稳。
 *   这也是界面只用 ASCII 的同一个理由 ---- 官方 lcd.c 的 lcd_show_string() 循环条件是
 *   (*p <= '~' && *p >= ' '), 中文字节 (>0x7E) 会让循环立刻退出, 根本画不出来。
 *
 * 输出格式:
 *   开机打印配置信息和 CSV 表头, 之后每 APP_CONSOLE_PERIOD_MS 输出一行 CSV。
 *   CSV 的列顺序由 app_sensor.c 的注册表决定 ---- 加一个传感器就自动多一列。
 *   以后接 ESP8266 / 4G 上传时, 直接把这一行当载荷发出去即可, 不用重写数据组织逻辑。
 *
 * 输出示例:
 *   # tick_ms,alarm,th,LIGHT
 *   12345,0,70,68
 *   12845,0,70,68
 ****************************************************************************************************
 */

#include <stdio.h>
#include "app_config.h"
#include "app_console.h"
#include "app_sensor.h"
#include "app_sched.h"
#include "app_alarm.h"
#include "./BSP/LCD/lcd.h"

static uint8_t s_dumped = 0;        /* 任务表是否已经打印过 */

/**
 * @brief       开机信息 (只调用一次)
 */
void app_console_init(void)
{
    printf("\r\n\r\n");
    printf("==========================================================\r\n");
    printf("  %s   %s\r\n", APP_PROJECT_NAME, APP_VERSION_STR);
    printf("  Light Intensity Real-Time Monitor\r\n");
    printf("  Board  : ALIENTEK Elite V2 (STM32F103ZET6) + 3.5\" TFTLCD\r\n");
    printf("  Sensor : onboard photoresistor LS1 -> PF8 -> ADC3_IN6\r\n");
    printf("  Filter : %u-pt moving average + 1st-order IIR (>>%u)\r\n",
           (unsigned int)LIGHT_FILTER_DEPTH, (unsigned int)LIGHT_IIR_SHIFT);
    printf("  Period : sensor %ums / display %ums / console %ums\r\n",
           (unsigned int)APP_SENSOR_PERIOD_MS,
           (unsigned int)APP_DISPLAY_PERIOD_MS,
           (unsigned int)APP_CONSOLE_PERIOD_MS);
    printf("  Alarm  : threshold %u%%   range %u~%u%%   step %u%%\r\n",
           (unsigned int)app_alarm_get_th(),
           (unsigned int)LIGHT_ALARM_TH_MIN,
           (unsigned int)LIGHT_ALARM_TH_MAX,
           (unsigned int)LIGHT_ALARM_TH_STEP);
    printf("  LCD    : %ux%u   dir=%u\r\n",
           (unsigned int)lcddev.width, (unsigned int)lcddev.height, (unsigned int)LCD_DIR);
    printf("  Keys   : KEY0(PE4)=TH-%u  KEY1(PE3)=TH+%u  WKUP(PA0)=save\r\n",
           (unsigned int)LIGHT_ALARM_TH_STEP, (unsigned int)LIGHT_ALARM_TH_STEP);
    printf("==========================================================\r\n");

    app_sensor_print_header();      /* CSV 表头 */
}

/**
 * @brief       周期输出 (由调度器按 APP_CONSOLE_PERIOD_MS 调用)
 */
void app_console_task(void)
{
    /* 开机 5 秒后打印一次任务表, 用来确认所有任务都在正常跑 */
    if ((!s_dumped) && (app_sched_get_tick() > 5000UL))
    {
        s_dumped = 1;
        app_sched_dump();
    }

    app_sensor_print_row();         /* 一行 CSV */
}
