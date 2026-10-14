/**
 ****************************************************************************************************
 * @file        app_sched.c
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       ʱ��Ƭ���������ʵ�� (1ms ���� + ����ʱ�������)
 ****************************************************************************************************
 * @attention
 *
 * ������Դ: TIM7 (������ʱ��, û���ⲿ����, �����κΰ���������ͻ)
 *   PSC = 71  -> 72MHz / 72   = 1MHz
 *   ARR = 999 -> 1MHz  / 1000 = 1kHz  -> ÿ 1ms �ж�һ��
 *
 * Ϊʲô���� SysTick ������:
 *   �ٷ� delay.c �� delay_us() �� SysTick ����һ����������װ����ͨ�ݼ���������,
 *   û�������������еĽ���, ������������ TIM7, ���߻������š�
 ****************************************************************************************************
 */

#include <stdio.h>
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "app_config.h"
#include "app_sched.h"
#include "app_sensor.h"
#include "app_display.h"
#include "app_alarm.h"
#include "app_console.h"
#include "app_key.h"
#include "app_io.h"

/******************************************************************************************/
/* 1ms ���� */

#define APP_TICK_TIM                TIM7
#define APP_TICK_IRQn               TIM7_IRQn
#define APP_TICK_IRQHandler         TIM7_IRQHandler

static volatile uint32_t g_tick_ms = 0;     /* ���������ĺ�����, 49.7 �����һ�� */

/**
 * @brief       TIM7 �����жϷ�����, ÿ 1ms ��һ��
 */
void APP_TICK_IRQHandler(void)
{
    if (APP_TICK_TIM->SR & TIM_SR_UIF)              /* update interrupt flag */
    {
        APP_TICK_TIM->SR &= ~TIM_SR_UIF;            /* clear flag */
        g_tick_ms++;
    }
}
}

/**
 * @brief       ��ʼ�� TIM7 Ϊ 1ms �����ж�
 */
static void app_tick_timer_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;        /* TIM7 clock enable */
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM7RST;     /* TIM7 reset */
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM7RST;    /* release reset */

    APP_TICK_TIM->PSC = 72 - 1;         /* 72MHz / 72   = 1MHz */
    APP_TICK_TIM->ARR = 1000 - 1;       /* 1MHz  / 1000 = 1kHz -> 1ms */
    APP_TICK_TIM->CR1 &= ~TIM_CR1_DIR;  /* upcounter */
    APP_TICK_TIM->CR1 |= TIM_CR1_ARPE;  /* ARR preload */
    APP_TICK_TIM->DIER |= TIM_DIER_UIE; /* update interrupt enable */
    APP_TICK_TIM->EGR |= TIM_EGR_UG;    /* generate update event to load PSC/ARR */
    APP_TICK_TIM->SR &= ~TIM_SR_UIF;    /* clear any pending update flag */

    /* NVIC priority same as official usart.c: preempt=3 / sub=3 / group=2 */
    sys_nvic_init(3, 3, APP_TICK_IRQn, 2);

    APP_TICK_TIM->CR1 |= TIM_CR1_CEN;   /* start timer */
}

/******************************************************************************************/
/* �����
 *
 * ��������������������������������������������������������������������
 *  ��������ֻ������������һ��, ���ø� main.c, ���ø��κ�����ģ�顣
 *  period_ms ����������, enable д 0 �����Ȳ��� (������ֽ׶ε���)��
 * ��������������������������������������������������������������������
 */
static app_task_t g_tasks[] =
{
    { "sensor",  APP_SENSOR_PERIOD_MS,  0, app_sensor_task,  1, 0 },    /* �ɼ����д����� */
    { "display", APP_DISPLAY_PERIOD_MS, 0, app_display_task, 1, 0 },    /* ˢ�½��� */

#if APP_USE_ALARM
    { "alarm",   APP_ALARM_PERIOD_MS,   0, app_alarm_task,   1, 0 },    /* ��ֵ�ж� + ���ⱨ�� */
#endif

#if APP_USE_KEY
    { "key",     APP_KEY_PERIOD_MS,     0, app_key_task,     1, 0 },    /* ��������ֵ */
#endif

#if APP_USE_CONSOLE
    { "console", APP_CONSOLE_PERIOD_MS, 0, app_console_task, 1, 0 },    /* ���� CSV ��� */
#endif

    { "io",      APP_IO_PERIOD_MS,      0, app_io_task,      0, 0 },    /* �� ��չ��, Ĭ�Ϲر� */

    /* ���� ������������� ���� */
};

#define APP_TASK_NUM    (sizeof(g_tasks) / sizeof(g_tasks[0]))

/******************************************************************************************/

/**
 * @brief       ��������ʼ�� (���� 1ms ����)
 * @note        ��������������� init ֮�����, ��������������軹û��ʼ��ʱ�Ϳ�ʼ��
 */
void app_sched_init(void)
{
    app_tick_timer_init();
}

/**
 * @brief       ��������ѭ����, �������� while(1) �ﷴ������
 * @note        ��������������, ÿ�ν���ֻ�ѵ����������һ��
 */
void app_sched_run(void)
{
    static uint32_t last_tick = 0;
    uint32_t now;
    uint32_t elapsed;
    uint8_t  i;

    now = g_tick_ms;

    if (now == last_tick)               /* ��һ���뻹û����, ֱ�ӷ��� */
    {
        return;
    }

    elapsed = now - last_tick;          /* �޷������, ��ʹ����Ҳ��ȷ */
    last_tick = now;

    for (i = 0; i < APP_TASK_NUM; i++)
    {
        app_task_t *t = &g_tasks[i];

        if (!t->enable)                 /* �ص�����������ʱ������ */
        {
            continue;
        }

        t->tick += (uint16_t)elapsed;

        if (t->tick >= t->period_ms)    /* due: run once */
        {
            t->tick -= t->period_ms;    /* subtract instead of zero to avoid accumulation drift */
            t->fn();
            t->run_cnt++;
        }
    }
}

/**
 * @brief       ȡϵͳ���к�����
 * @retval      ���������ĺ�����
 */
uint32_t app_sched_get_tick(void)
{
    return g_tick_ms;
}

/**
 * @brief       ��ӡ�����������, ����ȷ��������û������
 */
void app_sched_dump(void)
{
    uint8_t i;

    printf("\r\n---- task table ----\r\n");
    printf("name      period  enable  run\r\n");

    for (i = 0; i < APP_TASK_NUM; i++)
    {
        app_task_t *t = &g_tasks[i];
        printf("%-9s %5u   %5u   %lu\r\n",
               t->name, t->period_ms, t->enable, (unsigned long)t->run_cnt);
    }

    printf("--------------------\r\n");
}
