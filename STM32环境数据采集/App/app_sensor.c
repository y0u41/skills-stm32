/**
 ****************************************************************************************************
 * @file        app_sensor.c
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       通用传感器注册表实现
 ****************************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include "app_config.h"
#include "app_sensor.h"
#include "app_sched.h"
#include "app_alarm.h"
#include "app_light.h"

/******************************************************************************************/
/* ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
 *                            传 感 器 注 册 表
 *
 *  每一行的字段顺序:
 *     英文短名, 界面标签, 单位, init函数, poll函数, 量程下限, 量程上限,
 *     当前值, 采集次数, 在线标志, 启用标志, 等级文字钩子, 颜色钩子
 *
 *  （当前值/采集次数/在线标志 这三个运行时字段填 0 即可, 程序会自己更新）
 * ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
 */
static app_sensor_t g_sensors[] =
{
#if APP_USE_LIGHT
    /* 光照强度: 板载光敏电阻 LS1 -> PF8 -> ADC3_IN6, 已在 lsens_init() 里配好模拟输入 */
    { "LIGHT", "LIGHT INTENSITY", "%",
      app_light_sensor_init, app_light_sensor_poll,
      0, 100, 0, 0, 0, 1,
      app_light_level_text, app_light_level_color },
#endif

    /* ---- 示例 1: 以后加 DHT11 温湿度, 打开 APP_USE_DHT11 并取消下面注释 ----
    { "TEMP", "TEMPERATURE", "C",
      dht11_sensor_init, dht11_sensor_poll,
      -20, 60, 0, 0, 0, 1,
      NULL, NULL },
    { "HUMI", "HUMIDITY", "%",
      dht11_sensor_init, dht11_sensor_poll,
      0, 100, 0, 0, 0, 1,
      NULL, NULL },
    */

    /* ---- 示例 2: 以后加 MQ-2 燃气浓度 ----
    { "GAS", "GAS LEVEL", "ppm",
      mq2_sensor_init, mq2_sensor_poll,
      0, 1000, 0, 0, 0, 1,
      mq2_level_text, mq2_level_color },
    */

    /* ★★★ 新传感器加在这里 ★★★ */
};

#define APP_SENSOR_TOTAL    (sizeof(g_sensors) / sizeof(g_sensors[0]))

/******************************************************************************************/

/**
 * @brief       初始化注册表里所有启用的传感器
 */
void app_sensor_init_all(void)
{
    uint8_t i;

    for (i = 0; i < APP_SENSOR_TOTAL; i++)
    {
        app_sensor_t *s = &g_sensors[i];

        if (!s->enable)
        {
            continue;
        }

        s->value    = 0;
        s->poll_cnt = 0;
        s->online   = 0;

        if (s->init != NULL)
        {
            s->online = (s->init() == 0) ? 1 : 0;
        }
    }
}

/**
 * @brief       采集一遍注册表里所有启用的传感器
 * @note        每次只做一次转换, 不做长延时, 保证调度器不被卡住
 */
void app_sensor_poll_all(void)
{
    uint8_t i;

    for (i = 0; i < APP_SENSOR_TOTAL; i++)
    {
        app_sensor_t *s = &g_sensors[i];
        int32_t v = 0;

        if (!s->enable || !s->online || s->poll == NULL)
        {
            continue;
        }

        if (s->poll(&v) == 0)
        {
            s->value = v;
            s->poll_cnt++;
        }
        else
        {
            s->online = 0;              /* 采集失败, 标记掉线 */
        }
    }
}

/**
 * @brief       调度器任务入口
 */
void app_sensor_task(void)
{
    app_sensor_poll_all();
}

/**
 * @brief       取启用的传感器个数
 */
uint8_t app_sensor_count(void)
{
    uint8_t i;
    uint8_t n = 0;

    for (i = 0; i < APP_SENSOR_TOTAL; i++)
    {
        if (g_sensors[i].enable)
        {
            n++;
        }
    }

    return n;
}

/**
 * @brief       取第 idx 个启用的传感器
 * @param       idx: 序号, 从 0 开始
 * @retval      传感器指针; 越界返回 NULL
 */
app_sensor_t *app_sensor_get(uint8_t idx)
{
    uint8_t i;
    uint8_t n = 0;

    for (i = 0; i < APP_SENSOR_TOTAL; i++)
    {
        if (!g_sensors[i].enable)
        {
            continue;
        }

        if (n == idx)
        {
            return &g_sensors[i];
        }

        n++;
    }

    return NULL;
}

/**
 * @brief       按英文短名查找传感器
 * @param       name: 如 "LIGHT"
 * @retval      传感器指针; 找不到返回 NULL
 */
app_sensor_t *app_sensor_find(const char *name)
{
    uint8_t i;

    for (i = 0; i < APP_SENSOR_TOTAL; i++)
    {
        if (strcmp(g_sensors[i].name, name) == 0)
        {
            return &g_sensors[i];
        }
    }

    return NULL;
}

/**
 * @brief       串口打印 CSV 表头, 列顺序与 print_row 严格对应
 */
void app_sensor_print_header(void)
{
    uint8_t i;

    printf("# tick_ms");
    printf(",alarm,th");

    for (i = 0; i < APP_SENSOR_TOTAL; i++)
    {
        if (g_sensors[i].enable)
        {
            printf(",%s", g_sensors[i].name);
        }
    }

    printf("\r\n");
}

/**
 * @brief       串口打印 CSV 数据行
 */
void app_sensor_print_row(void)
{
    uint8_t i;

    printf("%lu", (unsigned long)app_sched_get_tick());
    printf(",%u", (unsigned int)app_alarm_is_active());
    printf(",%u", (unsigned int)app_alarm_get_th());

    for (i = 0; i < APP_SENSOR_TOTAL; i++)
    {
        if (g_sensors[i].enable)
        {
            printf(",%ld", (long)g_sensors[i].value);
        }
    }

    printf("\r\n");
}
