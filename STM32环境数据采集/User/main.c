/**
 ****************************************************************************************************
 * @file        main.c
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       光照强度实时采集与显示系统 ---- 主程序
 * @license     Copyright (c) 2025
 ****************************************************************************************************
 * @attention
 *
 * 实验平台: 正点原子 精英版 V2 (STM32F103ZET6, 512KB Flash / 64KB SRAM / 72MHz)
 * 显示设备: 正点原子 3.5 寸 TFTLCD (NT35310, 480x320, FSMC 16 位并口)
 * 编译环境: Keil MDK5 + Arm Compiler 6
 *
 * 硬件资源及引脚分配
 *   1) 光敏传感器  LS1 (板载)  -> PF8  -> ADC3_IN6   (12 位, 采样时间 239.5 周期)
 *   2) TFTLCD      FSMC 并口   -> PD0/1/4/5/8/9/10/14/15, PE7~PE15, PG0, PG12
 *                  LCD 背光   -> PB0
 *                  触摸屏     -> PB1/PB2/PF9/PF10/PF11 (本工程只做 IO 配置, 不做交互)
 *   3) LED         DS0 (红)    -> PB5   心跳灯 (程序在跑就闪)
 *                  DS1 (绿)    -> PE5   报警灯 (低电平亮)
 *   4) 蜂鸣器      BEEP        -> PB8   报警时断续鸣叫 (高电平响)
 *   5) 按键        KEY0 -> PE4 (阈值 -5%)   KEY1 -> PE3 (阈值 +5%)   WK_UP -> PA0 (保存)
 *   6) 24C02 EEPROM 板载        -> PB6(SCL) / PB7(SDA)   存报警阈值
 *   7) 调试串口    USART1      -> PA9/PA10 -> CH340C, 115200   (需要插上 P3 跳线帽)
 *   8) 下载/调试   SWD         -> PA13/PA14  (本工程绝不占用这两个脚)
 *
 * 跳线帽要求
 *   P3  必须插上  (否则串口没有输出)       P7  建议拔掉 (避免 TPAD/DAC 往 PA1 灌电流)
 *   P5/P6 无所谓  (本项目用 PF8, 与 RS485/CAN 无关)
 *
 * 实验现象
 *   1) 上电后 LCD 出现界面: 顶部标题栏, 中间光照卡片 (大字数值 + 百分比 + 等级 + 条形图
 *      + 原始 ADC 码值), 底部状态栏 (运行时间 / 报警阈值 / 按键提示);
 *   2) 数值随光照实时变化, 用手指遮住 LS1 数值降到 0~20, 用手电筒照上升到 80~100;
 *   3) LED0(PB5) 以约 2.5Hz 闪烁, 提示程序在运行;
 *   4) 串口每 500ms 输出一行 CSV 数据, 开机时打印配置信息和任务表;
 *   5) 光强超过报警阈值 (默认 70%) 时 LED1 常亮 + 蜂鸣器断续鸣叫, 降低后自动解除;
 *   6) KEY0/KEY1 改阈值, WK_UP 把阈值存进 24C02, 断电重启仍然记得。
 *
 * ★ 想加功能看哪里
 *   - 只改参数 (周期/阈值/开关)   -> App/app_config.h
 *   - 加一个传感器              -> 照抄 App/app_light.c, 再到 App/app_sensor.c 注册一行
 *   - 加一个后台任务            -> 写个 xxx_task(), 到 App/app_sched.c 的表里加一行
 *   - 先随便试代码              -> 写在 App/app_io.c 的扩展槽里, 并把它的 enable 改成 1
 *   - 接继电器 / 电磁阀         -> App/app_alarm.c 的 app_alarm_actuator() 钩子
 *   完整模板见工程根目录 扩展开发模板.md
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"

#include "app_config.h"
#include "app_sched.h"
#include "app_sensor.h"
#include "app_display.h"
#include "app_alarm.h"
#include "app_console.h"
#include "app_key.h"

int main(void)
{
    /* ---- 1. 系统底层初始化 ---- */
    sys_stm32_clock_init(9);        /* 系统时钟 72MHz (HSE 8M x 9) */
    delay_init(72);                 /* 延时函数初始化 */
    usart_init(72, 115200);         /* USART1, 115200-8-N-1 */

    /* ---- 2. 板载外设 ---- */
    led_init();                     /* LED0(PB5) / LED1(PE5) */
    lcd_init();                     /* 3.5 寸 TFTLCD, 内部自动识别屏 ID 并做 NT35310 初始化 */

    /* ---- 3. 应用层初始化 ----
     * 注意顺序: 传感器要先初始化, 因为标题栏要显示"在线传感器个数"。
     */
    app_sensor_init_all();          /* 注册表里所有传感器 (含 lsens_init + adc3_init) */
    app_display_init();             /* 画静态界面 (清屏 + 标题栏 + 卡片框 + 状态栏) */

#if APP_USE_ALARM
    app_alarm_init();               /* 蜂鸣器 + 报警灯 + 从 24C02 读回上次的阈值 */
#endif

#if APP_USE_KEY
    app_key_init();                 /* 按键 IO */
#endif

#if APP_USE_CONSOLE
    app_console_init();             /* 打印开机信息 + CSV 表头 */
#endif

    /* ---- 4. 最后启动 1ms 心跳, 调度器开始跑任务 ---- */
    app_sched_init();

    /* ---- 5. 主循环: 永远只有这一句, 永不阻塞 ----
     * 所有功能都是 app_sched.c 任务表里的任务, 到点自动执行。
     */
    while (1)
    {
        app_sched_run();
    }
}
