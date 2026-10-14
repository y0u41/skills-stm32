/**
 ****************************************************************************************************
 * @file        app_alarm.c
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       阈值报警模块实现
 ****************************************************************************************************
 * @attention
 *
 * 板载器件的电平极性 (已从官方综合例程 beepplay.c / 实验38 核实):
 *   LED0(PB5) / LED1(PE5) : 低电平点亮   LED0(0) = 亮
 *   BEEP(PB8)             : 高电平鸣叫   BEEP(1) = 响   <- 注意与 LED 相反
 *
 * 滞回的作用:
 *   如果光强正好卡在阈值上 (例如 70%), 不做滞回的话报警会以毫秒级频率反复通断,
 *   蜂鸣器会发出刺耳的"塔塔"声, LED 也会闪。所以在阈值上下各留 LIGHT_ALARM_HYST
 *   的缓冲: 高于 阈值+滞回 才报警, 低于 阈值-滞回 才解除。
 ****************************************************************************************************
 */

#include <stddef.h>
#include "app_config.h"
#include "app_alarm.h"
#include "app_sched.h"
#include "app_sensor.h"
#include "./BSP/LED/led.h"
#include "./BSP/BEEP/beep.h"

#if APP_USE_EEPROM
#include "./BSP/24CXX/24cxx.h"
#endif

static uint8_t s_th     = LIGHT_ALARM_TH_DEF;   /* 当前报警阈值 */
static uint8_t s_active = 0;                    /* 是否正在报警 */

/**
 * @brief       执行器钩子 ---- 扩展预留位
 * @param       on: 1 = 进入报警, 0 = 解除报警
 * @note        本函数只在报警状态"发生变化"的那一刻被调用一次, 不是每周期都调。
 *
 *              现在什么都不做。将来要接继电器切断燃气阀 / 打开排风扇 / 驱动水泵,
 *              只要在这里加几行, 其它任何文件都不用改。
 *
 *              ★ 安全提醒 (血的教训):
 *                1) 继电器/电磁阀绝对不能直接挂到 GPIO 上, 必须
 *                   三极管(或 MOSFET) + 续流二极管 + 光耦隔离;
 *                2) 不要从开发板的 5V / 3.3V 排针给继电器取电, 请外接电源, 只共地;
 *                3) 板子 3.3V 和 5V 排针各限 1000mA (USB 供电时只有 500mA)。
 */
static void app_alarm_actuator(uint8_t on)
{
    /* 示例: 用 PD11 驱动继电器 (PD11 是板载未接任何器件的自由脚)
     *
     * sys_gpio_set(GPIOD, SYS_GPIO_PIN11, SYS_GPIO_MODE_OUT,
     *              SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_LOW, SYS_GPIO_PUPD_PD);
     * sys_gpio_pin_set(GPIOD, SYS_GPIO_PIN11, on ? 1 : 0);
     */
    (void)on;
}

/**
 * @brief       初始化报警模块
 */
void app_alarm_init(void)
{
    beep_init();                /* 内部已经把蜂鸣器关掉 */
    LED1(1);                    /* 关报警灯 */

#if APP_USE_EEPROM
    at24cxx_init();

    if (at24cxx_check() == 0)   /* 返回 0 表示 24C02 在位且可读写 */
    {
        if (at24cxx_read_one_byte(EE_ADDR_MAGIC) == EE_MAGIC_VAL)
        {
            uint8_t th = at24cxx_read_one_byte(EE_ADDR_TH);

            if ((th >= LIGHT_ALARM_TH_MIN) && (th <= LIGHT_ALARM_TH_MAX))
            {
                s_th = th;      /* 参数合法才采用, 防止空白 EEPROM 读出的 0xFF 被当阈值 */
            }
        }
    }
#endif
}

/**
 * @brief       报警任务
 */
void app_alarm_task(void)
{
    static uint8_t prev_active = 0;
    app_sensor_t  *s = app_sensor_find("LIGHT");
    int32_t        v;

    if (s == NULL)              /* 注册表里没有光照传感器就什么都不做 */
    {
        return;
    }

    v = s->value;

    /* ---- 带滞回的阈值判断 ---- */
    if (!s_active)
    {
        if (v >= ((int32_t)s_th + LIGHT_ALARM_HYST))
        {
            s_active = 1;
        }
    }
    else
    {
        if (v <= ((int32_t)s_th - LIGHT_ALARM_HYST))
        {
            s_active = 0;
        }
    }

    /* ---- 声光输出 ---- */
    if (s_active)
    {
        LED1(0);                /* 报警灯亮 (低电平点亮) */

        if ((app_sched_get_tick() % ALARM_BEEP_CYCLE_MS) < ALARM_BEEP_ON_MS)
        {
            BEEP(1);            /* 蜂鸣器响 (高电平鸣叫) */
        }
        else
        {
            BEEP(0);
        }
    }
    else
    {
        LED1(1);                /* 报警灯灭 */
        BEEP(0);
    }

    /* ---- 状态变化时通知执行器钩子 ---- */
    if (s_active != prev_active)
    {
        prev_active = s_active;
        app_alarm_actuator(s_active);
    }
}

/**
 * @brief       查询是否正在报警
 */
uint8_t app_alarm_is_active(void)
{
    return s_active;
}

/**
 * @brief       取当前报警阈值
 */
uint8_t app_alarm_get_th(void)
{
    return s_th;
}

/**
 * @brief       设置报警阈值 (自动钳位)
 */
void app_alarm_set_th(uint8_t th)
{
    if (th < LIGHT_ALARM_TH_MIN)
    {
        th = LIGHT_ALARM_TH_MIN;
    }
    else if (th > LIGHT_ALARM_TH_MAX)
    {
        th = LIGHT_ALARM_TH_MAX;
    }

    s_th = th;
}

/**
 * @brief       把阈值保存到板载 24C02
 * @note        24C02 地址 255 被 at24cxx_check() 占用了, 这里用 0x00 / 0x02, 不冲突
 */
void app_alarm_save_th(void)
{
#if APP_USE_EEPROM
    at24cxx_write_one_byte(EE_ADDR_TH, s_th);
    at24cxx_write_one_byte(EE_ADDR_MAGIC, EE_MAGIC_VAL);
#endif
}
