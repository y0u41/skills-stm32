/**
 ****************************************************************************************************
 * @file        app_light.h
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       光照强度采集模块 (板载光敏电阻 LS1)
 ****************************************************************************************************
 * @attention
 *
 * 硬件通路 (已核对原理图):
 *
 *      VCC3.3 ----[ 固定电阻 ]----+---- PF8 (ADC3_IN6) ----> STM32 ADC3
 *                                 |
 *                            [ LS1 光敏电阻 ]
 *                                 |
 *                                GND
 *
 *   光越强 -> LS1 阻值越小 -> PF8 电压越低 -> ADC 码值越小。
 *   因此官方换算公式取反:  percent = 100 - (码值 / 40), 光越亮值越大 (0~100)。
 *
 * 已核实的两处官方实现细节 (容易踩坑):
 *   1) adc3.h 里的 ADC3_CHY = 1 / GPIOA / PIN1 是残留默认值, 和光敏无关。
 *      光敏真正的通道号在 lsens.h 的 LSENS_ADC3_CHX = 6, PF8 的 "模拟输入" 模式
 *      是 lsens_init() 自己配好的。切不可为了"改通道"去改 adc.h ---- 那是 ADC1/2 的定义。
 *   2) adc3_init() 内部会顺手把 PA1 也配成模拟输入 (无害的副作用)。若不使用 PA1,
 *      建议把 P7 跳线帽拔掉, 避免 TPAD/DAC 往 PA1 灌电流。
 *
 * 为什么不用官方的 lsens_get_val():
 *   它内部调用 adc3_get_result_average(ch, 10), 而该函数每采一次就 delay_ms(5),
 *   取 10 次 = 50ms 纯阻塞。在一个以 100ms 为周期的任务里这会吃掉一半 CPU 时间,
 *   与"实时显示"的目标相悖。本模块每个时间片只做一次转换 (约 21us), 再配环形缓冲
 *   加一阶 IIR 滤波, 滤波效果更好而 CPU 占用接近 0。
 ****************************************************************************************************
 */

#ifndef __APP_LIGHT_H
#define __APP_LIGHT_H

#include "./SYSTEM/sys/sys.h"

/* 光照数据快照 (供显示层取明细用) */
typedef struct
{
    uint16_t raw;           /* 最近一次 12 位 ADC 码值, 0 ~ 4095 */
    uint16_t avg;           /* 滑动平均后的码值 */
    uint16_t filt;          /* 一阶 IIR 滤波后的码值 */
    uint8_t  percent;       /* 相对光强百分比, 0 ~ 100, 越大越亮 */
    uint8_t  level;         /* 等级 0~4: 0很暗 1偏暗 2适中 3偏亮 4很亮 */
    uint32_t count;         /* 累计采样次数 */
    uint16_t fill;          /* 滤波缓冲已填充点数, 0 ~ LIGHT_FILTER_DEPTH */
} light_data_t;

uint8_t             app_light_sensor_init(void);        /* 注册表用: 初始化, 返回 0=成功 */
uint8_t             app_light_sensor_poll(int32_t *out);/* 注册表用: 采一次, 写 *out */
const light_data_t *app_light_get(void);                /* 取数据快照 */
const char         *app_light_level_text(int32_t value);/* 数值 -> 等级文字 (ASCII) */
uint16_t            app_light_level_color(int32_t value);/* 数值 -> 显示颜色 */

#endif
