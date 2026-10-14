/**
 ****************************************************************************************************
 * @file        app_display.c
 * @author      STM32 EnvDataCollect Project
 * @version     V1.0
 * @date        2025
 * @brief       显示模块实现 ---- 表驱动布局 + 局部刷新
 ****************************************************************************************************
 * @attention
 *
 * 三个关键设计:
 *
 * 1) 局部刷新, 绝不整屏重画
 *    官方例程常见的写法是每帧 lcd_clear() 再全部重画, 那样屏幕会明显闪烁。
 *    本模块为每个传感器缓存上一次的值/颜色/条形宽度, 只有变化时才重画对应的小矩形。
 *    另外 lcd_show_xnum(x, y, val, 3, size, 0, color) 的 mode=0 会用 g_back_color
 *    把高位补成空格, 所以数字区域天然是定宽自清除的。
 *
 * 2) 表驱动布局
 *    界面行数 = app_sensor_count(), 由 app_sensor.c 的注册表决定。
 *    往注册表里加一个传感器, 屏幕上就自动多出一张卡片, 本文件一个字都不用改。
 *    卡片高度还会自动选择大/中/小三种排版。
 *
 * 3) 只用 ASCII
 *    官方 lcd.c 的字库 (lcdfont.h) 只有 12/16/24/32 号 ASCII 点阵,
 *    lcd_show_string() 的循环条件是 (*p <= '~' && *p >= ' '), 中文字节 (>0x7E)
 *    会让循环立刻退出, 所以屏上不能直接显示中文。
 *    要显示中文需要移植官方 实验36 汉字显示实验 的字库方案 (需要字库文件)。
 *    中文只出现在代码注释、串口输出和文档里。
 ****************************************************************************************************
 */

#include <stdio.h>
#include "app_config.h"
#include "app_display.h"
#include "app_sensor.h"
#include "app_sched.h"
#include "app_alarm.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/LED/led.h"

#if APP_USE_LIGHT
#include "app_light.h"
#endif

/******************************************************************************************/
/* 界面缓存: 每个传感器一份, 用来判断哪些区域需要重画 */

typedef struct
{
    uint8_t  valid;         /* 缓存是否已初始化 */
    int32_t  value;         /* 上一次显示的值 */
    uint16_t bar_w;         /* 上一次画的条形填充宽度 (像素) */
    uint16_t color;         /* 上一次用的颜色 */
    uint8_t  online;        /* 上一次的在线状态 */
} ui_cache_t;

static ui_cache_t s_cache[UI_MAX_SENSORS];

/* 屏幕与布局尺寸, app_display_init() 里根据实际屏参算出来 */
static uint16_t s_w;            /* 屏宽 */
static uint16_t s_h;            /* 屏高 */
static uint16_t s_cx0;          /* 卡片左边界 */
static uint16_t s_cx1;          /* 卡片右边界 */
static uint16_t s_ry0;          /* 卡片区上边界 */
static uint16_t s_ry1;          /* 卡片区下边界 */
static uint16_t s_card_h;       /* 每张卡片高度 */

static uint32_t s_last_sec = 0xFFFFFFFFUL;   /* 状态栏上次刷新的秒数, 用来把状态栏降到 1Hz */

/******************************************************************************************/
/* 基础绘图辅助 */

/**
 * @brief       画一段 ASCII 文字
 * @param       bg: 该文字所在区域的底色 (mode=0 的字符会用 g_back_color 铺底)
 */
static void ui_text(uint16_t x, uint16_t y, uint8_t size, const char *s, uint16_t color, uint16_t bg)
{
    g_back_color = bg;
    lcd_show_string(x, y, s_w, s_h, size, (char *)s, color);
}

/**
 * @brief       画一个文字并先把该区域清成底色 (用于长度会变的字符串)
 */
static void ui_text_clear(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                          uint8_t size, const char *s, uint16_t color, uint16_t bg)
{
    lcd_fill(x, y, x + w - 1, y + h - 1, bg);
    ui_text(x, y, size, s, color, bg);
}

/**
 * @brief       画一个可能带负号的定宽数值, 自动清除旧值痕迹
 * @param       size: 字号 (12/16/24/32)
 */
static void ui_draw_value(uint16_t x, uint16_t y, uint8_t size, int32_t v, uint16_t color)
{
    uint16_t w = (uint16_t)(size / 2) * 4;      /* 固定 4 个字符位: 符号 + 3 位数字 */
    uint32_t mag;
    uint16_t cx = x;

    lcd_fill(x, y, x + w - 1, y + size - 1, UI_CARD_BG);

    if (v < 0)
    {
        g_back_color = UI_CARD_BG;
        lcd_show_char(cx, y, '-', size, 0, color);
        cx = (uint16_t)(cx + size / 2);
        mag = (uint32_t)(-v);
    }
    else
    {
        mag = (uint32_t)v;
    }

    if (mag > 999)
    {
        mag = 999;                              /* 界面只留 3 位, 免得撑破卡片 */
    }

    g_back_color = UI_CARD_BG;
    lcd_show_xnum(cx, y, mag, 3, size, 0, color);
}

/**
 * @brief       画进度条
 * @param       value: 当前值, vmin~vmax 映射到 0~整条宽度
 * @param       th_pct: 阈值位置 (0~100), 传 0xFFFF 表示不画刻线
 */
static void ui_draw_bar(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                        int32_t value, int32_t vmin, int32_t vmax, uint16_t color,
                        uint16_t th_pct)
{
    uint16_t full_w;
    uint16_t fill_w;
    int32_t  span;

    if (x1 <= x0 || y1 <= y0)
    {
        return;
    }

    lcd_fill(x0, y0, x1, y1, UI_BAR_BG);                        /* 先铺底色 */
    lcd_draw_rectangle(x0, y0, x1, y1, UI_BAR_FRAME);           /* 再画边框 */

    full_w = (uint16_t)(x1 - x0 - 1);                           /* 内部可用宽度 */
    span = vmax - vmin;

    if (span <= 0)
    {
        span = 1;
    }

    if (value < vmin)
    {
        value = vmin;
    }
    else if (value > vmax)
    {
        value = vmax;
    }

    fill_w = (uint16_t)(((uint32_t)(value - vmin) * full_w) / (uint32_t)span);

    if (fill_w > 0)
    {
        lcd_fill((uint16_t)(x0 + 1), (uint16_t)(y0 + 1),
                 (uint16_t)(x0 + fill_w), (uint16_t)(y1 - 1), color);
    }

    /* 阈值刻线: 让用户一眼看出离报警线还有多远 */
    if (th_pct != 0xFFFF)
    {
        uint16_t tx = (uint16_t)(x0 + 1 + ((uint32_t)th_pct * full_w) / 100);

        if (tx > (uint16_t)(x1 - 2))
        {
            tx = (uint16_t)(x1 - 2);
        }

        lcd_draw_line(tx, (uint16_t)(y0 + 1), tx, (uint16_t)(y1 - 1), UI_TH_MARK);
    }
}

/******************************************************************************************/
/* 静态界面 */

/**
 * @brief       画标题栏 (只画一次)
 */
static void ui_draw_header(void)
{
    char buf[24];
    uint8_t n = app_sensor_count();
    uint8_t on = 0;
    uint8_t i;

    for (i = 0; i < n; i++)
    {
        app_sensor_t *s = app_sensor_get(i);

        if (s != NULL && s->online)
        {
            on++;
        }
    }

    lcd_fill(0, 0, (uint16_t)(s_w - 1), UI_HEADER_H - 1, UI_HEADER_BG);

    ui_text(8, 8, 16, "STM32 ENV DATA COLLECT", UI_TITLE_FG, UI_HEADER_BG);

    sprintf(buf, "ONLINE %u/%u", (unsigned int)on, (unsigned int)n);
    ui_text((uint16_t)(s_w - 12 * 8), 8, 16, buf, UI_ONLINE_FG, UI_HEADER_BG);
}

/**
 * @brief       画卡片外框与标签 (属于静态部分, 只在初始化时画一次)
 */
static void ui_draw_card_frame(uint8_t idx, uint16_t y0, uint16_t h)
{
    app_sensor_t *s = app_sensor_get(idx);

    if (s == NULL)
    {
        return;
    }

    lcd_fill(s_cx0, y0, s_cx1, (uint16_t)(y0 + h - 1), UI_CARD_BG);
    lcd_draw_rectangle(s_cx0, y0, s_cx1, (uint16_t)(y0 + h - 1), UI_CARD_FRAME);

    if (h >= 150)
    {
        ui_text((uint16_t)(s_cx0 + 12), (uint16_t)(y0 + 8), 16, s->label, UI_LABEL_FG, UI_CARD_BG);
    }
    else if (h >= 100)
    {
        ui_text((uint16_t)(s_cx0 + 8), (uint16_t)(y0 + 4), 16, s->label, UI_LABEL_FG, UI_CARD_BG);
    }
    else
    {
        ui_text((uint16_t)(s_cx0 + 6), (uint16_t)(y0 + 2), 12, s->label, UI_LABEL_FG, UI_CARD_BG);
    }
}

/**
 * @brief       画状态栏 (只画一次底板, 文字由 ui_update_footer 刷新)
 */
static void ui_draw_footer_frame(void)
{
    uint16_t y0 = (uint16_t)(s_h - UI_FOOTER_H);

    lcd_fill(0, y0, (uint16_t)(s_w - 1), (uint16_t)(s_h - 1), UI_FOOTER_BG);
}

/**
 * @brief       刷新状态栏文字 (1Hz)
 */
static void ui_update_footer(void)
{
    char buf[80];
    uint32_t ms = app_sched_get_tick();
    uint16_t y0 = (uint16_t)(s_h - UI_FOOTER_H);

    sprintf(buf, "UP %02u:%02u:%02u    TH %u%%    KEY0/1 = TH -/+    WKUP = SAVE",
            (unsigned int)(ms / 3600000UL),
            (unsigned int)((ms / 60000UL) % 60UL),
            (unsigned int)((ms / 1000UL) % 60UL),
            (unsigned int)app_alarm_get_th());

    ui_text_clear(8, (uint16_t)(y0 + 12), (uint16_t)(s_w - 16), 16,
                  16, buf, UI_HINT_FG, UI_FOOTER_BG);
}

/******************************************************************************************/
/* 动态部分 */

#if APP_USE_LIGHT
/**
 * @brief       画光照传感器独有的明细行 (原始码值 / 滤波参数 / 采样次数)
 * @note        只有卡片足够高时才画; 其它传感器不需要这些信息
 */
static void ui_draw_light_extra(uint16_t y0, uint16_t h)
{
    const light_data_t *d = app_light_get();
    char buf[40];

    if (h < 200)
    {
        return;
    }

    sprintf(buf, "RAW ADC : %u / %u", (unsigned int)d->raw, (unsigned int)LIGHT_RAW_FULL_SCALE);
    ui_text_clear((uint16_t)(s_cx0 + 12), (uint16_t)(y0 + 186), 300, 16,
                  16, buf, UI_EXTRA_FG, UI_CARD_BG);

    sprintf(buf, "FILTER  : %u pts + IIR>>%u",
            (unsigned int)LIGHT_FILTER_DEPTH, (unsigned int)LIGHT_IIR_SHIFT);
    ui_text_clear((uint16_t)(s_cx0 + 12), (uint16_t)(y0 + 208), 300, 16,
                  16, buf, UI_EXTRA_FG, UI_CARD_BG);

    sprintf(buf, "SAMPLES : %lu", (unsigned long)d->count);
    ui_text_clear((uint16_t)(s_cx0 + 12), (uint16_t)(y0 + 230), 300, 16,
                  16, buf, UI_EXTRA_FG, UI_CARD_BG);
}
#endif

/**
 * @brief       刷新一张卡片的数据区
 */
static void ui_update_card(uint8_t idx, uint16_t y0, uint16_t h)
{
    app_sensor_t *s = app_sensor_get(idx);
    ui_cache_t   *c = &s_cache[idx];
    uint16_t      color;
    uint16_t      bar_x0;
    uint16_t      bar_x1;
    uint16_t      bar_y0;
    uint16_t      bar_h;
    uint16_t      bar_fill_w;
    uint8_t       size;
    uint16_t      vx, vy;
    uint16_t      lx, ly;
    const char   *lv;

    if (s == NULL)
    {
        return;
    }

    /* 颜色: 优先用传感器自己的钩子, 没有就用黑色 */
    color = (s->color_of != NULL) ? s->color_of(s->value) : UI_LABEL_FG;

    /* ---- 版式参数按卡片高度自动选择 ---- */
    if (h >= 150)
    {
        size   = 32;
        vx     = (uint16_t)(s_cx0 + 12);
        vy     = (uint16_t)(y0 + 38);
        lx     = (uint16_t)(s_cx0 + 220);
        ly     = (uint16_t)(y0 + 48);
        bar_x0 = (uint16_t)(s_cx0 + 12);
        bar_x1 = (uint16_t)(s_cx1 - 12);
        bar_y0 = (uint16_t)(y0 + 112);
        bar_h  = 34;
    }
    else if (h >= 100)
    {
        size   = 24;
        vx     = (uint16_t)(s_cx0 + 8);
        vy     = (uint16_t)(y0 + 24);
        lx     = (uint16_t)(s_cx0 + 200);
        ly     = (uint16_t)(y0 + 28);
        bar_x0 = (uint16_t)(s_cx0 + 8);
        bar_x1 = (uint16_t)(s_cx1 - 8);
        bar_y0 = (uint16_t)(y0 + 58);
        bar_h  = 22;
    }
    else
    {
        size   = 16;
        vx     = (uint16_t)(s_cx0 + 6);
        vy     = (uint16_t)(y0 + 20);
        lx     = (uint16_t)(s_cx0 + 160);
        ly     = (uint16_t)(y0 + 22);
        bar_x0 = (uint16_t)(s_cx0 + 6);
        bar_x1 = (uint16_t)(s_cx1 - 6);
        bar_y0 = (uint16_t)(y0 + 42);
        bar_h  = 12;
    }

    bar_fill_w = (uint16_t)(bar_x1 - bar_x0 - 1);

    /* ---- 掉线: 只画一次 OFFLINE ---- */
    if (!s->online)
    {
        if (!c->valid || c->online)
        {
            ui_draw_value(vx, vy, size, 0, UI_OFFLINE_FG);
            ui_text_clear(lx, ly, 8 * 8, 16, 16, UI_OFFLINE_TEXT, UI_OFFLINE_FG, UI_CARD_BG);
            ui_draw_bar(bar_x0, bar_y0, bar_x1, (uint16_t)(bar_y0 + bar_h),
                        0, s->vmin, s->vmax, UI_OFFLINE_FG, 0xFFFF);
            c->valid   = 1;
            c->online  = 0;
            c->value   = 0;
            c->bar_w   = 0;
            c->color   = UI_OFFLINE_FG;
        }

        return;
    }

    /* ---- 先判断"需要重画"的条件: 缓存失效 / 刚从掉线恢复 / 值变了 / 颜色(等级)变了
     *      注意必须在这里就把结论存下来, 因为下面会把 c->value / c->color 更新掉,
     *      之后再比较就永远相等了 ---- */
    {
        uint8_t changed = (uint8_t)((!c->valid) || (!c->online) ||
                                    (c->value != s->value) || (c->color != color));

        /* ---- 数值 + 单位 + 等级文字 ---- */
        if (changed)
        {
            ui_draw_value(vx, vy, size, s->value, color);

            if (size == 32)
            {
                ui_text((uint16_t)(vx + 4 * 16 + 6), (uint16_t)(vy + 6), 24, s->unit, UI_UNIT_FG, UI_CARD_BG);
            }
            else if (size == 24)
            {
                ui_text((uint16_t)(vx + 4 * 12 + 4), (uint16_t)(vy + 4), 16, s->unit, UI_UNIT_FG, UI_CARD_BG);
            }
            else
            {
                ui_text((uint16_t)(vx + 4 * 8 + 2), (uint16_t)(vy + 2), 12, s->unit, UI_UNIT_FG, UI_CARD_BG);
            }

            lv = (s->level_text != NULL) ? s->level_text(s->value) : NULL;

            if (lv != NULL)
            {
                ui_text_clear(lx, ly, 8 * 8, 16, 16, lv, color, UI_CARD_BG);
            }

            c->value = s->value;
            c->color = color;
        }

        /* ---- 条形图: 像素宽度变了才重画 (否则每帧都刷整条太浪费) ---- */
        {
            int32_t  v = s->value;
            int32_t  span = s->vmax - s->vmin;
            uint16_t want;

            if (span <= 0)
            {
                span = 1;
            }

            if (v < s->vmin)
            {
                v = s->vmin;
            }
            else if (v > s->vmax)
            {
                v = s->vmax;
            }

            want = (uint16_t)(((uint32_t)(v - s->vmin) * bar_fill_w) / (uint32_t)span);

            if (changed || c->bar_w != want)
            {
                /* 只有光照传感器有"报警阈值"这个概念, 其它传感器不画刻线 */
                uint16_t th = 0xFFFF;

                if (app_sensor_find("LIGHT") == s)
                {
                    th = (uint16_t)app_alarm_get_th();
                }

                ui_draw_bar(bar_x0, bar_y0, bar_x1, (uint16_t)(bar_y0 + bar_h),
                            s->value, s->vmin, s->vmax, color, th);
                c->bar_w = want;
            }
        }
    }

    c->online = 1;
    c->valid  = 1;
}

/******************************************************************************************/
/* 对外接口 */

/**
 * @brief       初始化界面: 清屏, 设置横竖屏, 画所有静态元素
 * @note        必须在 lcd_init() 之后调用
 */
void app_display_init(void)
{
    uint8_t n;
    uint8_t i;

    lcd_display_dir(LCD_DIR);           /* lcd_init() 内部默认是竖屏, 这里改成配置的方向 */
    lcd_clear(UI_CARD_BG);              /* 上电只整屏清一次, 之后全部局部刷新 */

    s_w = lcddev.width;
    s_h = lcddev.height;

    s_cx0 = UI_MARGIN;
    s_cx1 = (uint16_t)(s_w - UI_MARGIN);
    s_ry0 = UI_HEADER_H;
    s_ry1 = (uint16_t)(s_h - UI_FOOTER_H);

    n = app_sensor_count();

    if (n == 0)
    {
        n = 1;                          /* 防止除零; 注册表为空时界面上只有一句提示 */
    }

    s_card_h = (uint16_t)((s_ry1 - s_ry0) / n);

    /* 清缓存, 保证第一帧一定全画一遍 */
    for (i = 0; i < UI_MAX_SENSORS; i++)
    {
        s_cache[i].valid  = 0;
        s_cache[i].online = 0;
        s_cache[i].value  = 0;
        s_cache[i].bar_w  = 0;
        s_cache[i].color  = 0;
    }

    ui_draw_header();
    ui_draw_footer_frame();

    if (app_sensor_count() == 0)
    {
        ui_text(40, (uint16_t)(s_ry0 + 40), 24, "NO SENSOR REGISTERED", UI_LABEL_FG, UI_CARD_BG);
    }
    else
    {
        for (i = 0; i < app_sensor_count(); i++)
        {
            ui_draw_card_frame(i, (uint16_t)(s_ry0 + i * s_card_h), s_card_h);
        }
    }
}

/**
 * @brief       刷新界面 (由调度器按 APP_DISPLAY_PERIOD_MS 周期调用)
 */
void app_display_task(void)
{
    uint8_t n = app_sensor_count();
    uint8_t i;

    for (i = 0; i < n && i < UI_MAX_SENSORS; i++)
    {
        uint16_t y0 = (uint16_t)(s_ry0 + i * s_card_h);
        uint16_t h  = s_card_h;

        /* 最后一张卡片吃掉整除余数, 免得底部留缝 */
        if (i == (uint8_t)(n - 1))
        {
            h = (uint16_t)(s_ry1 - y0);
        }

        ui_update_card(i, y0, h);

#if APP_USE_LIGHT
        if (app_sensor_find("LIGHT") == app_sensor_get(i))
        {
            ui_draw_light_extra(y0, h);
        }
#endif
    }

    /* 状态栏降到 1Hz 刷新, 省下大量无谓的刷屏开销 */
    {
        uint32_t sec = app_sched_get_tick() / 1000UL;

        if (sec != s_last_sec)
        {
            s_last_sec = sec;
            ui_update_footer();
        }
    }

    LED0_TOGGLE();      /* 心跳灯, 提示程序还在跑 */
}
