/**
 ****************************************************************************************************
 * @file        app_light.c
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       光照强度采集模块实现 (ADC3_IN6 / PF8 / 板载光敏电阻 LS1)
 ****************************************************************************************************
 */

#include <stddef.h>
#include "app_config.h"
#include "app_light.h"
#include "./BSP/ADC/adc3.h"
#include "./BSP/LSENS/lsens.h"

/******************************************************************************************/
/* 内部状态 */

static light_data_t g_light;                                    /* 对外快照 */
static uint16_t     s_buf[LIGHT_FILTER_DEPTH];                  /* 滑动平均环形缓冲 */
static uint8_t      s_idx;                                      /* 环形缓冲写指针 */
static uint32_t     s_count;                                    /* 累计采样次数 */
static uint16_t     s_fill;                                     /* 已填充点数 */
static int32_t      s_filt;                                     /* 一阶 IIR 状态 */

/**
 * @brief       初始化光照采集
 * @retval      0, 成功
 * @note        lsens_init() 会把 PF8 配成模拟输入, 并调用 adc3_init() 完成
 *              ADC3 的复位校准 (ADC 时钟 = PCLK2/6 = 12MHz, 低于 14MHz 上限)
 */
uint8_t app_light_sensor_init(void)
{
    uint8_t i;

    for (i = 0; i < LIGHT_FILTER_DEPTH; i++)
    {
        s_buf[i] = 0;
    }

    s_idx   = 0;
    s_count = 0;
    s_fill  = 0;
    s_filt  = 0;

    g_light.raw     = 0;
    g_light.avg     = 0;
    g_light.filt    = 0;
    g_light.percent = 0;
    g_light.level   = 0;
    g_light.count   = 0;
    g_light.fill    = 0;

    lsens_init();               /* 配 PF8 为模拟输入 + 初始化 ADC3 */

    return 0;
}

/**
 * @brief       采集一次光照, 做滤波与标定
 * @param       out: 输出相对光强百分比 0~100
 * @retval      0, 成功
 * @note        本函数只做一次 ADC 转换 (约 21us), 绝不阻塞
 */
uint8_t app_light_sensor_poll(int32_t *out)
{
    uint32_t sum = 0;
    uint32_t avg;
    uint32_t t;
    uint16_t i;
    int32_t  diff;

    /* ---- 第 1 步: 单次 ADC 采样, 采样时间 239.5 周期 (21us) ---- */
    g_light.raw = (uint16_t)adc3_get_result(LIGHT_ADC_CHANNEL);

    /* ---- 第 2 步: 写入环形缓冲, 算滑动平均 (去脉冲干扰) ---- */
    s_buf[s_idx] = g_light.raw;
    s_idx = (uint8_t)((s_idx + 1) & (LIGHT_FILTER_DEPTH - 1));

    if (s_fill < LIGHT_FILTER_DEPTH)
    {
        s_fill++;               /* 还没填满时, 有效点数是 s_fill */
    }

    for (i = 0; i < LIGHT_FILTER_DEPTH; i++)
    {
        sum += s_buf[i];        /* 未填充的位置是 0, 对求和没有影响 */
    }

    avg = sum / s_fill;         /* 只除以有效点数, 所以开机第一帧就是正确的 */

    /* ---- 第 3 步: 一阶 IIR 低通, 进一步平滑抖动 ---- */
    diff = (int32_t)avg - s_filt;
    s_filt += (diff >> LIGHT_IIR_SHIFT);

    if (s_filt < 0)
    {
        s_filt = 0;
    }
    else if (s_filt > LIGHT_RAW_FULL_SCALE)
    {
        s_filt = LIGHT_RAW_FULL_SCALE;
    }

    /* ---- 第 4 步: 换算成 0~100 相对光强 (官方实验20 的公式) ---- */
    t = (uint32_t)s_filt / LIGHT_ADC_DIVISOR;

    if (t > 100)
    {
        t = 100;
    }

    g_light.percent = (uint8_t)(100 - t);

    /* ---- 第 5 步: 分级 ---- */
    if (g_light.percent <= LIGHT_LV1_MAX)
    {
        g_light.level = 0;      /* 很暗 */
    }
    else if (g_light.percent <= LIGHT_LV2_MAX)
    {
        g_light.level = 1;      /* 偏暗 */
    }
    else if (g_light.percent <= LIGHT_LV3_MAX)
    {
        g_light.level = 2;      /* 适中 */
    }
    else if (g_light.percent <= LIGHT_LV4_MAX)
    {
        g_light.level = 3;      /* 偏亮 */
    }
    else
    {
        g_light.level = 4;      /* 很亮 */
    }

    /* ---- 第 6 步: 更新对外快照 ---- */
    g_light.avg   = (uint16_t)avg;
    g_light.filt  = (uint16_t)s_filt;
    g_light.fill  = s_fill;
    s_count++;
    g_light.count = s_count;

    if (out != NULL)
    {
        *out = (int32_t)g_light.percent;
    }

    return 0;
}

/**
 * @brief       取光照数据快照
 * @retval      指向内部快照的指针 (只读, 不要修改)
 */
const light_data_t *app_light_get(void)
{
    return &g_light;
}

/**
 * @brief       把光强数值翻译成等级文字
 * @param       value: 0~100
 * @retval      等级文字 (纯 ASCII, 因为官方 lcd.c 的字库只覆盖 32~126)
 */
const char *app_light_level_text(int32_t value)
{
    if (value <= LIGHT_LV1_MAX)
    {
        return "V.DARK";
    }
    else if (value <= LIGHT_LV2_MAX)
    {
        return "DARK";
    }
    else if (value <= LIGHT_LV3_MAX)
    {
        return "NORMAL";
    }
    else if (value <= LIGHT_LV4_MAX)
    {
        return "BRIGHT";
    }

    return "V.BRIGHT";
}

/**
 * @brief       把光强数值翻译成显示颜色 (RGB565)
 * @param       value: 0~100
 * @retval      颜色值
 */
uint16_t app_light_level_color(int32_t value)
{
    if (value <= LIGHT_LV1_MAX)
    {
        return 0x8410;      /* 很暗: 深灰 */
    }
    else if (value <= LIGHT_LV2_MAX)
    {
        return 0x001F;      /* 偏暗: 蓝  */
    }
    else if (value <= LIGHT_LV3_MAX)
    {
        return 0x07E0;      /* 适中: 绿  */
    }
    else if (value <= LIGHT_LV4_MAX)
    {
        return 0xFFE0;      /* 偏亮: 黄  */
    }

    return 0xF800;          /* 很亮: 红  */
}
