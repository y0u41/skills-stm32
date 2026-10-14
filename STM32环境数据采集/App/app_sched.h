/**
 ****************************************************************************************************
 * @file        app_sched.h
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       时间片任务调度器 ---- 非阻塞软定时器
 ****************************************************************************************************
 * @attention
 *
 * 设计目的:
 *   新手常写 while(1){ 采集; delay_ms(500); 显示; delay_ms(500); } 这种阻塞式主循环,
 *   一旦想加第二个功能就会互相拖慢, 加到第五个功能就彻底失控。
 *
 *   本调度器把每个功能都写成 "进来干一点活立刻返回" 的任务函数, 由 1ms 硬件心跳统一驱动。
 *   因此主循环永远只有一句:  while (1) { app_sched_run(); }
 *
 * 新增一个任务的方法:
 *   1) 写一个 void xxx_task(void) 函数, 里面绝对不能有长延时;
 *   2) 到 app_sched.c 的 g_tasks[] 表里加一行;
 *   3) 完事。不用改 main.c, 不用改任何已有模块。
 ****************************************************************************************************
 */

#ifndef __APP_SCHED_H
#define __APP_SCHED_H

#include "./SYSTEM/sys/sys.h"

/* 任务描述结构体 */
typedef struct
{
    const char *name;           /* 任务名, 便于串口打印调试 */
    uint16_t    period_ms;      /* 运行周期, 单位 ms */
    uint16_t    tick;           /* 内部计时, 不要手动改 */
    void      (*fn)(void);      /* 任务函数, 必须快速返回 (无长延时) */
    uint8_t     enable;         /* 1=启用  0=停用 */
    uint32_t    run_cnt;        /* 累计执行次数, 调试用 */
} app_task_t;

void     app_sched_init(void);      /* 启动 1ms 心跳 (TIM7) */
void     app_sched_run(void);       /* 主循环里反复调用 */
uint32_t app_sched_get_tick(void);  /* 取系统运行毫秒数 */
void     app_sched_dump(void);      /* 打印任务表 (串口) */

#endif
