---
name: skills-stm32
description: >-
  正点原子 精英版 V2（STM32F103ZET6）开发速查技能。覆盖：112 个 IO 的板载占用与自由度、
  ADC/DAC 模拟通道、跳线帽陷阱、官方 BSP 驱动 API 全表、48 个标准例程索引、选脚决策流程、
  故障速查。当需要在这块板子上做接线、选引脚、写/改外设驱动、移植例程或排错时使用。
version: 1.2
board: 正点原子 精英版 V2 (Elite STM32F103 Board V2.6)
mcu: STM32F103ZET6 (LQFP144, 512KB Flash, 64KB SRAM, 72MHz)
source: D:\正点原子 官方资料盘（原理图 / IO 分配表 / 硬件参考手册 / 185 个官方例程）
---

# skills_stm32

> **配套深度资料**：同目录 `D:\正点原子\STM32F103ZET6精英版V2_开发实例与IO资源总报告.md`
> 本文件是**干活时看的速查卡**；需要出处、完整表、逐脚说明时查那份报告。

---

## 0. 30 秒定调

| 项 | 值 |
|---|---|
| 板子 | 正点原子 精英版 V2，STM32F103ZET6，LQFP144 |
| 资源 | 512KB Flash / 64KB SRAM / 72MHz / **110 个 IO 引出** |
| 外设 | 3×ADC12、1×DAC12、5×USART、3×SPI、2×IIC、1×USB、1×CAN、SDIO、FSMC、11×TIM、2×WWDG/IWDG |
| 供电 | USB Type-C(5V) 或 DC 6~15V（DC005）；板载 RY9125→5V，AMS1117-3.3→3.3V |
| 下载 | ① SWD：ST-Link 接 PA13/PA14　② ISP：CH340C + BOOT0（flymcu 一键下载） |
| 例程 | `4，程序源码\1，标准例程-寄存器版本\`（48 实验 / 185 工程）；同名 HAL 版在 zip 里 |

**工程骨架**：`Drivers/SYSTEM`（sys/delay/usart）+ `Drivers/BSP/<模块>` + `Drivers/CMSIS` + `User/main.c` + `Projects/MDK-ARM`。
新建工程时**直接复制一个官方例程改**，别从零搭。

---

## 1. 八条铁律（违反必踩坑）

1. **PA13/PA14 是 SWD 下载口。** 做普通 IO 必须先禁用 JTAG&SWD，禁用后**仿真就没了**。除非万不得已不要动。
2. **PA15 / PB3 / PB4 是 JTAG 脚。** 要当普通 IO 用，只禁用 JTAG、保留 SWD：
   `__HAL_AFIO_REMAP_SWJ_NOJTAG();`（HAL） / `sys_gpio_remap_set(24, 3, 2);`（寄存器）。
   全禁（连 SWD 一起）= `sys_gpio_remap_set(24, 3, 3);` ← 之后只能靠 ISP 救。
3. **PA2/PA3 默认接在 RS485 上。** 要用作 USART2 或 ADC，**必须先拔掉 P5 跳线帽**。同理 PA11/PA12 由 P6 在 CAN 与 USB 之间选择。
4. **MQ 等 5V 模拟传感器输出可达 5V，STM32 ADC 满量程 3.3V。** 一律 **10k/20k 分压**后再进 ADC，直连打坏引脚。
5. **ADC 干净通道有 14 个**：PA5/PA6/PA7（ADC12_IN5/6/7）、PC0~PC5（ADC12_IN10~15）、PF3~PF7（ADC3_IN9/IN14/IN15/IN4/IN5）。
   **插着 3.5 寸屏时 PC0~PC5 依然干净**（屏走 FSMC 的 PD/PE/PG，不碰 PC 口）。
6. **5V / 3.3V 排针各限 1000mA**（DC 供电时；USB 供电时 500mA）。3.5 寸屏背光约 120mA 且**吃 3.3V 轨**（模块 R13 默认 0Ω → LEDA 接 VCC3.3）。继电器/电磁阀**不要挂在板子排针上**。
7. **PA4 是 DAC_OUT1**，同时也是 ADC12_IN4；PA5 是 DAC_OUT2 也是 ADC12_IN5 —— 一个脚不能同时兼两职。
8. **PC14/PC15 被 32.768K 晶振占用，不可做 IO。** 其余 110 个 IO 全部引出。

---

## 2. 选脚三步法

```
第 1 步：这个脚板载接了什么？   → 查 §3 固定占用表
第 2 步：要拔跳线 / 不插模块吗？ → 查 §4 跳线表
第 3 步：外设能不能映射到这个脚？→ 查 §5 外设可用脚
```

**结论优先用这张"绝对方便"清单**（板载未接任何外设，官方标注"完全独立"）：

| 用途 | 首选脚 | 备选 |
|---|---|---|
| 数字 IO / 继电器 / 指示灯 | **PD11、PD12、PD13、PE0、PE1、PE2、PE6、PG9、PG10、PC13、PF12~PF15、PA8** | PF0~PF2、PG1~PG5 |
| 模拟输入（ADC12） | **PA5、PA6、PA7** | **PC0~PC5** |
| 模拟输入（ADC3） | **PF3、PF4、PF5、PF6、PF7** | PC0~PC3 |
| PWM 输出 | PC6(TIM8_CH1)、PC7(TIM8_CH2)、PB5(TIM3_CH2 但影响 LED0) | PE9/PE11/PE13/PE14(TIM1) |
| 外部中断 | PD11/PD12/PD13、PE0/PE1/PE2、PG9/PG10 | 任意自由 IO |
| 串口 | USART3→**PB10/PB11**；USART2→**PA2/PA3（先拔 P5）** | UART4→PC10/PC11、UART5→PC12/PD2 |
| I2C 挂传感器 | **PB6/PB7**（板载 24C02 同总线，已带 4.7K 上拉，地址不冲突即可共挂） | 任意自由 IO 软件模拟 |
| SPI | **SPI2：PB13/PB14/PB15**（与板载 W25Q128 共用，CS 分开即可） | SPI1 重映射到 PB3/4/5（需禁 JTAG） |

---

## 3. 板载固定占用速查

| 引脚 | 板载连接 | 能不能挪作他用 |
|---|---|---|
| PB5 / PE5 | LED0(红) / LED1(绿) | 可以，但 LED 会跟着亮灭 |
| PB8 | 蜂鸣器 BEEP | 可以，但蜂鸣器会响 |
| PB9 | 红外接收头 LF0038 | 不推荐（有 4.7K 上拉且受接收头驱动） |
| PE4 / PE3 / PA0 | KEY0 / KEY1 / WK_UP | 可以（不按即独立） |
| PA1 | TPAD 电容触摸 / STM_ADC（P7） | 拔 P7 跳线即独立 |
| PF8 | 光敏电阻 LS1（ADC3_IN6） | 已占用，可直接读光照 |
| PB6 / PB7 | 板载 24C02 EEPROM（I2C1，4.7K 上拉） | 可挂其他 I2C 从机，**别再加上拉** |
| PB12 | W25Q128 片选 | 不建议 |
| PB13~PB15 | W25Q128 + WIRELESS 的 SPI2 | 不用这两个外设时可当普通 IO |
| PB0 | LCD 背光 LCD_BL | 被屏占用 |
| PB1 / PB2 / PF9 / PF10 / PF11 | 屏触摸 T_SCK/T_MISO/T_MOSI/T_PEN/T_CS | 被屏占用（不插屏即独立） |
| PB2 | 兼 BOOT1 | 上电时决定启动模式 |
| PD0/1/4/5/8~10/14/15、PE7~PE15、PG0、PG12 | FSMC → LCD | 不插屏即独立 |
| PC0~PC7 | OLED / 摄像头接口 OV_D0~D7 | **不插摄像头/OLED 即独立**（且仍可做 ADC） |
| PA8 / PD3 / PD6 / PB3 / PB4 / PG13 / PG14 / PG15 | OLED/摄像头控制线 | 同上 |
| PC8~PC12 / PD2 | TF 卡 SDIO | 不用 TF 卡时可用 |
| PG6 / PG7 / PG8 | WIRELESS 接口 IRQ/CS/CE | 不用无线模块即独立 |
| PG11 | 单总线座 U4（DHT11/DS18B20，4.7K 上拉） | 不插传感器即独立 |
| PA2 / PA3 / PD7 | RS485（P5 选择 + RE） | 拔 P5 即独立 |
| PA9 / PA10 | USART1 → CH340C（P3 选择） | 拔 P3 即独立 |
| PA11 / PA12 | CAN 或 USB SLAVE（P6 选择） | 拔 P6 即独立 |
| PA13 / PA14 | **SWD 下载口** | 见铁律 1 |
| PA15 | JTAG JTDI + ATK-MODULE LED | 禁 JTAG 后可用 |
| PB10 / PB11 | ATK-MODULE 的 RXD/TXD（USART3） | 不插 ATK 模块即独立 |
| PA4 | STM_DAC / ATK-MODULE KEY | 拔跳线即独立 |
| PC14 / PC15 | RTC 32.768K 晶振 | ❌ 不可用 |

---

## 4. 跳线帽陷阱表

| 标号 | 控制什么 | 拔掉后 |
|---|---|---|
| **P3** | USART1 是否接 CH340 | PA9/PA10 独立（但失去板载串口打印） |
| **P5** | PA2/PA3 接 RS485 还是他用 | **PA2/PA3 独立** ← 用 USART2 接 4G/蓝牙时必拔 |
| **P6** | PA11/PA12 接 CAN 还是 USB | PA11/PA12 独立 |
| **P7** | DAC(PA4) / ADC(PA1) / TPAD / RV1 互联 | 见下 |
| **B0 / B1** | BOOT0 / BOOT1(PB2) | B0=1 进 ISP 下载 |

**P7 四种玩法**：0 跳线 = 各自对 GND 单独用；1 跳线 DAC↔ADC = DAC 自测；1 跳线 ADC↔TPAD = 触摸按键；1 跳线 DAC↔RV1 = 电位器调压给 ADC。

---

## 5. 外设可用脚速查

| 外设 | 可用引脚 | 精英版 V2 上推荐 |
|---|---|---|
| USART1 | PA9/PA10、PB6/PB7 | PA9/PA10（板载 CH340） |
| USART2 | PA2/PA3、PD5/PD6 | PA2/PA3（**先拔 P5**）或重映射 PD5/PD6 |
| USART3 | PB10/PB11、PC10/PC11、PD8/PD9 | **PB10/PB11**（ATK-MODULE） |
| UART4 | PC10/PC11 | 与 SDIO 冲突，慎用 |
| UART5 | PC12/PD2 | 与 TF 卡冲突，慎用 |
| SPI1 | PA5/PA6/PA7/PA4；重映射 PB3/PB4/PB5/PA15 | PA5/6/7 自由；重映射需禁 JTAG |
| SPI2 | PB13/PB14/PB15 | 板载 W25Q128 + 无线接口共用 |
| I2C1 | PB6/PB7（默认）、PB8/PB9（重映射） | **PB6/PB7**（板载 24C02 + 4.7K 上拉） |
| CAN | PA11/PA12（默认）、PB8/PB9、PD0/PD1 | PA11/PA12（P6 跳线） |
| TIM1 | CH1 PA8/PE9，CH2 PA9/PE11，CH3 PA10/PE13，CH4 PA11/PE14，CH1N PA7/PE8，CH2N PB0/PE10，CH3N PB1/PE12，BKIN PA6/PE15 | PE9/PE8 + 刹车 PE15（互补输出） |
| TIM2 | CH1 PA0/PA15，CH2 PA1/PB3，CH3 PA2/PB10，CH4 PA3/PB11 | 常与按键/串口冲突 |
| TIM3 | CH1 PA6/PB4/PC6，CH2 PA7/PB5/PC7，CH3 PB0/PC8，CH4 PB1/PC9 | **PC6/PC7**（无冲突） |
| TIM4 | CH1 PB6/PD12，CH2 PB7/PD13，CH3 PB8/PD14，CH4 PB9/PD15 | PB9 已被红外占用 |
| TIM5 | CH1 PA0，CH2 PA1，CH3 PA2，CH4 PA3 | 与按键/串口冲突 |
| TIM6/TIM7 | 无外部通道 | 纯定时中断，推荐 |
| TIM8 | CH1 PC6，CH2 PC7，CH3 PC8，CH4 PC9，CH1N PA7，CH2N PB0，CH3N PB1，BKIN PA6 | **PC6~PC9**（无冲突，做 PWM 首选） |
| ADC1/2 | IN0~IN15 → PA0~PA7、PB0/PB1、PC0~PC5 | PA5/6/7、PC0~PC5 |
| ADC3 | IN0~IN3 → PA0~PA3；IN4~IN8 → PF6~PF10；IN9 PF3；IN10~IN13 PC0~PC3；IN14 PF4；IN15 PF5 | PF3~PF7 |
| DAC | OUT1 PA4，OUT2 PA5 | PA4 |
| SDIO | D0 PC8，D1 PC9，D2 PC10，D3 PC11，CK PC12，CMD PD2 | 固定（TF 卡座） |

---

## 6. 官方 BSP API 全表（照抄即可）

### 6.1 SYSTEM 层

```c
/* sys.h —— 引脚操作与配置 */
void     sys_gpio_set(GPIO_TypeDef *p_gpiox, uint16_t pinx, uint32_t mode,
                      uint8_t otype, uint8_t speed, uint8_t pupd);   /* 一次配好一个脚 */
void     sys_gpio_pin_set(GPIO_TypeDef *p_gpiox, uint16_t pinx, uint8_t status); /* 写引脚 */
uint8_t  sys_gpio_pin_get(GPIO_TypeDef *p_gpiox, uint16_t pinx);        /* 读引脚 */
void     sys_gpio_remap_set(uint8_t pos, uint8_t bit, uint8_t val);     /* 重映射 / 禁 JTAG */
void     sys_nvic_init(uint8_t pprio, uint8_t sprio, uint8_t ch, uint8_t group);
void     sys_nvic_ex_config(GPIO_TypeDef *p_gpiox, uint16_t pinx, uint8_t tmode); /* EXTI 配置 */
void     sys_stm32_clock_init(uint32_t plln);                           /* 时钟，72MHz 传 9 */
void     sys_soft_reset(void);
void     sys_standby(void);
void     sys_wfi_set(void);

/* 参数宏 */
// mode : SYS_GPIO_MODE_IN / _OUT / _AF / _AIN(模拟输入)
// otype: SYS_GPIO_OTYPE_PP(推挽) / _OD(开漏)
// speed: SYS_GPIO_SPEED_LOW(2M) / _MID(10M) / _HIGH(50M)
// pupd : SYS_GPIO_PUPD_NONE / _PU(上拉) / _PD(下拉)
// pinx : SYS_GPIO_PIN0 .. SYS_GPIO_PIN15
// EXTI 触发: SYS_GPIO_FTIR(下降沿) / _RTIR(上升沿) / _BTIR(任意边沿)

/* delay.h */
void delay_init(uint16_t sysclk);
void delay_ms(uint16_t nms);
void delay_us(uint32_t nus);

/* usart.h */
void usart_init(uint32_t pclk2, uint32_t bound);   /* 板载串口 = USART1, pclk2=72000000 */
```

**典型写法**：
```c
sys_gpio_set(GPIOB, SYS_GPIO_PIN5, SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP,
             SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* PB5 推挽输出，上拉 */
```

### 6.2 板载器件

```c
/* LED   PB5/PE5 */
void led_init(void);
#define LED0(x)          /* x=0 亮(x 为 0 时点亮) */ 
#define LED1(x)
#define LED0_TOGGLE()    /* 翻转 */
#define LED1_TOGGLE()

/* BEEP  PB8 */
void beep_init(void);
#define BEEP(x)

/* KEY   PE4/PE3/PA0 */
void    key_init(void);
uint8_t key_scan(uint8_t mode);          /* 返回 KEY0_PRES / KEY1_PRES / WKUP_PRES，0 = 无 */

/* 光敏  PF8 (ADC3_IN6) */
void    lsens_init(void);
uint8_t lsens_get_val(void);             /* 0~100 相对光强 */

/* 24C02 EEPROM  PB6/PB7 (软件 I2C) */
void    at24cxx_init(void);
uint8_t at24cxx_check(void);
uint8_t at24cxx_read_one_byte(uint16_t addr);
void    at24cxx_write_one_byte(uint16_t addr, uint8_t data);
void    at24cxx_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen);
void    at24cxx_write(uint16_t addr, uint8_t *pbuf, uint16_t datalen);

/* W25Q128 16MB  SPI2 PB12~PB15 */
void     norflash_init(void);
uint16_t norflash_read_id(void);
void     norflash_erase_chip(void);
void     norflash_erase_sector(uint32_t saddr);
void     norflash_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen);
void     norflash_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/* 内部 Flash 模拟 EEPROM（掉电存参数，不占引脚） */
uint16_t stmflash_read_halfword(uint32_t faddr);
void     stmflash_read(uint32_t raddr, uint16_t *pbuf, uint16_t length);
void     stmflash_write(uint32_t waddr, uint16_t *pbuf, uint16_t length);  /* 自动擦除 */

/* 看门狗 */
void iwdg_init(uint8_t prer, uint16_t rlr);   void iwdg_feed(void);
void wwdg_init(uint8_t tr, uint8_t wr, uint8_t fprer);  void wwdg_set_counter(uint8_t cnt);

/* RTC（PC14/PC15 晶振 + VBAT 纽扣电池） */
uint8_t  rtc_init(void);
uint8_t  rtc_set_time(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec);
void     rtc_get_time(void);
uint8_t  rtc_set_alarm(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec);
uint16_t rtc_read_bkr(uint32_t bkrx);   void rtc_write_bkr(uint32_t bkrx, uint16_t data);

/* 低功耗 */
void pwr_pvd_init(uint8_t pls);   void pwr_wkup_key_init(void);
void pwr_enter_sleep(void);   void pwr_enter_stop(void);   void pwr_enter_standby(void);
```

### 6.3 模拟

```c
/* ADC1/2 —— 默认 ADC_ADCX = ADC1，通道宏在 adc.h（默认 ADC_ADCX_CHY = 1，即 PA1） */
void     adc_init(void);
void     adc_channel_set(ADC_TypeDef *adcx, uint8_t ch, uint8_t stime);  /* stime=7 → 239.5 周期 */
uint32_t adc_get_result(uint8_t ch);
uint32_t adc_get_result_average(uint8_t ch, uint8_t times);              /* 常用：取 20 次平均 */
void     adc_dma_init(uint32_t mar);
void     adc_dma_enable(uint16_t cndtr);
void     adc_nch_dma_init(uint32_t mar);       /* 多通道 DMA */
void     adc_temperature_init(void);           /* 内部温度传感器 ADC1_IN16 */

/* ADC3 —— 引脚宏 ADC3_CHY_GPIO_PORT/PIN + ADC3_CHY 在 adc3.h */
void     adc3_init(void);
void     adc3_channel_set(uint8_t ch, uint8_t stime);
uint32_t adc3_get_result(uint8_t ch);
uint32_t adc3_get_result_average(uint8_t ch, uint8_t times);

/* DAC  PA4(DAC1) / PA5(DAC2) */
void dac_init(uint8_t outx);
void dac_set_voltage(uint8_t outx, uint16_t vol);   /* 0~4095 → 0~3.3V */
void dac_triangular_wave(uint16_t maxval, uint16_t dt, uint16_t samples, uint16_t n);
void dac_dma_wave_init(uint8_t outx, uint32_t par, uint32_t mar);
void dac_dma_wave_enable(uint8_t outx, uint16_t cndtr, uint16_t arr, uint16_t psc);
```

**改 ADC 通道时改哪里**：`Drivers/BSP/ADC/adc.h` 里
`ADC_ADCX`（用哪个 ADC）、`ADC_ADCX_CHY`（通道号）、`ADC_ADCX_CHY_GPIO_PORT`、`ADC_ADCX_CHY_GPIO_PIN`。

### 6.4 定时器

```c
/* 基本定时器（TIM6/TIM7）—— 纯定时中断 */
void btim_timx_int_init(uint16_t arr, uint16_t psc);

/* 通用定时器（TIM2~TIM5）*/
void     gtim_timx_int_init(uint16_t arr, uint16_t psc);      /* 定时中断 */
void     gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc);  /* PWM 输出 */
void     gtim_timx_cap_chy_init(uint16_t arr, uint16_t psc);  /* 输入捕获 */
void     gtim_timx_cnt_chy_init(uint16_t psc);                /* 脉冲计数 */
uint32_t gtim_timx_cnt_chy_get_count(void);
void     gtim_timx_cnt_chy_restart(void);

/* 高级定时器（TIM1/TIM8）*/
void atim_timx_pwm_chy_init(uint16_t arr, uint16_t psc);
void atim_timx_npwm_chy_init(uint16_t arr, uint16_t psc);   /* 输出指定个数 PWM */
void atim_timx_npwm_chy_set(uint32_t npwm);
void atim_timx_comp_pwm_init(uint16_t arr, uint16_t psc);   /* 输出比较 */
void atim_timx_cplm_pwm_init(uint16_t arr, uint16_t psc);   /* 互补输出 */
void atim_timx_cplm_pwm_set(uint16_t ccr, uint8_t dtg);     /* 比较值 & 死区 */
void atim_timx_pwmin_chy_init(void);                        /* PWM 输入模式 */
void atim_timx_pwmin_chy_restart(void);
```
定时周期：`T = (arr+1) × (psc+1) / 72MHz`（例：arr=4999, psc=7199 → 500ms）。

### 6.5 通信

```c
/* 串口 */
void usart_init(uint32_t pclk2, uint32_t bound);       /* USART1 */
void usart3_init(uint32_t sclk, uint32_t baudrate);    /* USART3 (PB10/PB11) */
void u3_printf(char *fmt, ...);

/* DMA */
void dma_usart1_tx_config(uint32_t mar);
void dma_basic_config(DMA_Channel_TypeDef *dmax_chy, uint32_t par, uint32_t mar);
void dma_enable(DMA_Channel_TypeDef *dmax_chy, uint16_t cndtr);

/* 软件 I2C */
void    iic_init(void);
void    iic_start(void);  void iic_stop(void);  void iic_ack(void);  void iic_nack(void);
uint8_t iic_wait_ack(void);
void    iic_send_byte(uint8_t txd);
uint8_t iic_read_byte(unsigned char ack);

/* 硬件 SPI2 */
void    spi2_init(void);
void    spi2_set_speed(uint8_t speed);
uint8_t spi2_read_write_byte(uint8_t txdata);

/* RS485  PA2/PA3 + PD7(RE)，P5 跳线 */
void rs485_init(uint32_t sclk, uint32_t baudrate);
void rs485_send_data(uint8_t *buf, uint8_t len);
void rs485_receive_data(uint8_t *buf, uint8_t *len);

/* CAN  PA11/PA12，P6 跳线 */
uint8_t can_init(uint8_t tsjw, uint8_t tbs2, uint8_t tbs1, uint16_t brp, uint8_t mode);
uint8_t can_send_msg(uint32_t id, uint8_t *msg, uint8_t len);
uint8_t can_receive_msg(uint32_t id, uint8_t *buf);

/* SD 卡  SDIO */
uint8_t sd_read_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);
uint8_t sd_write_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);

/* 2.4G NRF24L01（SPI2 + PG6/7/8） */
void    nrf24l01_init(void);   void nrf24l01_rx_mode(void);   void nrf24l01_tx_mode(void);
uint8_t nrf24l01_check(void);
uint8_t nrf24l01_tx_packet(uint8_t *ptxb );
uint8_t nrf24l01_rx_packet(uint8_t *prxbuf);
```

### 6.6 显示与交互

```c
/* TFTLCD（FSMC，2.8/3.5/4.3/7 寸 MCU 屏） */
void     lcd_init(void);
void     lcd_display_dir(uint8_t dir);        /* 显示方向 */
void     lcd_scan_dir(uint8_t dir);
void     lcd_display_on(void);   void lcd_display_off(void);
void     lcd_clear(uint16_t color);
void     lcd_set_cursor(uint16_t x, uint16_t y);
void     lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void     lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);
void     lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void     lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void     lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void     lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
void     lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void     lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void     lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);
void     lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void     lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);
void     lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);
void     lcd_ssd_backlight_set(uint8_t pwm);   /* 仅 SSD1963 大屏 */

/* 触摸（电阻 XPT2046/HR2046 与电容 GT9xxx/FT5206 统一接口） */
uint8_t tp_init(void);
uint8_t tp_scan(uint8_t mode);
void    tp_adjust(void);                 /* 电阻屏校准（电容屏不需要） */
void    tp_save_adjust_data(void);       /* 存到板载 24C02 */
uint8_t tp_get_adjust_data(void);

/* OLED（与摄像头接口共用排座） */
void oled_init(void);   void oled_clear(void);   void oled_refresh_gram(void);
void oled_display_on(void);   void oled_display_off(void);
void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot);
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode);
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void oled_show_string(uint8_t x, uint8_t y, const char *p, uint8_t size);

/* TPAD 电容触摸（PA1，需 P7 跳线） */
uint8_t tpad_init(uint16_t psc);   uint8_t tpad_scan(uint8_t mode);
```

### 6.7 传感器（板载座直插）

```c
/* DHT11 温湿度  PG11（U4 座） */
uint8_t dht11_init(void);
uint8_t dht11_check(void);
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi);   /* temp/humi 为整数字节 */

/* DS18B20 温度  PG11（U4 座） */
uint8_t ds18b20_init(void);
uint8_t ds18b20_check(void);

/* 红外遥控  PB9（TIM4_CH4 输入捕获） */
void    remote_init(void);
uint8_t remote_scan(void);

/* 摄像头 OV7725（OLED/摄像头接口） */
uint8_t ov7725_init(void);
uint8_t ov7725_read_reg(uint16_t reg);   uint8_t ov7725_write_reg(uint8_t reg, uint8_t data);
void    ov7725_light_mode(uint8_t mode);  void ov7725_color_saturation(uint8_t sat);
void    ov7725_brightness(uint8_t bright);  void ov7725_contrast(uint8_t contrast);
void    ov7725_special_effects(uint8_t eft);
void    ov7725_window_set(uint16_t width, uint16_t height, uint8_t mode);

/* 外部 SRAM / GSM / 游戏手柄 等（综合例程里有） */
void sram_init(void);  void sram_write(uint8_t *pbuf, uint32_t addr, uint32_t datalen);
void sram_read(uint8_t *pbuf, uint32_t addr, uint32_t datalen);
uint8_t gsm_send_cmd(uint8_t *cmd, uint8_t *ack, uint16_t waittime);
void    joypad_init(void);   uint8_t joypad_read(void);
```

---

## 7. 官方例程索引（照抄起点）

**路径**：`D:\正点原子\【正点原子】精英STM32F103开发板 V2-资料盘(A盘)(1)\4，程序源码\4，程序源码\1，标准例程-寄存器版本\`
**HAL 版**：同目录 `2，标准例程-HAL库版本.zip`（同名 48 个实验，多一个"实验0 基础入门实验"）

| # | 实验 | 学什么 | 关键脚 |
|---|---|---|---|
| 1 | 跑马灯 | GPIO 输出 | PB5 / PE5 |
| 2 | 蜂鸣器 | GPIO 输出 | PB8 |
| 3 | 按键输入 | GPIO 输入 | PE4 / PE3 / PA0 |
| 4 | 外部中断 | EXTI | PE4 / PE3 / PA0 |
| 5 | 串口 | USART 收发 | PA9 / PA10 |
| 6 / 7 | 独立 / 窗口看门狗 | IWDG / WWDG | — |
| 8-1 | 基本定时器中断 | TIM6 | — |
| 9-1~9-4 | 通用定时器：中断 / PWM / 输入捕获 / 脉冲计数 | TIM3 CH2→PB5、TIM5 CH1→PA0、TIM2 CH1→PA0 | — |
| 10-1~10-4 | 高级定时器：指定个数 PWM / 输出比较 / 互补+死区 / PWM 输入 | TIM8 CH1→PC6、TIM1 PE9+PE8+BKIN PE15 | PC6 / PE8 / PE9 / PE15 |
| 11 | 电容触摸按键 | TIM5 CH2 输入捕获 | PA1 |
| 12 | OLED | 软件 SPI | OLED 接口 |
| **13** | **TFTLCD（MCU 屏）** | **FSMC 并口驱动彩屏** | 见 §3 FSMC 组 |
| 14 | USMART | 串口在线调函数 | PA9/PA10 |
| 15 | RTC | 实时时钟 | PC14/PC15 + VBAT |
| 16-1~16-4 | PVD / 睡眠 / 停止 / 待机 | 低功耗 | PA0 唤醒 |
| 17 | DMA | DMA1→USART1 | — |
| **18-1~18-4** | **ADC：单通道 / DMA / 多通道 DMA / 过采样 16 位** | **ADC + DMA** | PA1（改 adc.h 换脚） |
| 19 | 内部温度传感器 | ADC1_IN16 | — |
| 20 | 光敏传感器 | ADC3_IN6 | PF8 |
| 21-1~21-3 | DAC：定值 / 三角波 / 正弦波(DMA) | DAC | PA4 出，PA1 回读（P7 跳线） |
| 22 | IIC | 软件 I2C 驱动 24C02 | PB6 / PB7 |
| 23 | SPI | 硬件 SPI2 驱动 W25Q128 | PB13/14/15 |
| 24 | RS485 | 半双工总线 | PA2/PA3 + PD7 |
| 25 | CAN | 环回/正常模式 | PA11/PA12 |
| 26 | 触摸屏 | 电阻/电容自适应 | 触摸 5 线 |
| 27 | 红外遥控 | TIM4_CH4 输入捕获 | PB9 |
| 28 | DS18B20 | 单总线 | PG11 |
| 29 | DHT11 | 单总线 | PG11 |
| 30 | 无线通信 | NRF24L01 + SPI2 | PB13~15 + PG6/7/8 |
| 31 | FLASH 模拟 EEPROM | 内部 Flash 读写 | — |
| 32 | 摄像头 | OV7725 + FIFO | PC0~7 + PA8 + PD3/PD6 + PB3/PB4 + PG13~15 |
| 33 | 内存管理 | 动态内存 | — |
| 34 / 35 | SD 卡 / FATFS | SDIO + 文件系统 | PC8~12 + PD2 |
| 36 / 37 | 汉字显示 / 图片显示 | 字库、图片解码 | SD 卡 + W25Q128 |
| 38 | 照相机 | 摄像头 + BMP + FATFS | 同 32 |
| 39_1 / 39_2 | DSP：BasicMath / FFT | DSP 库 | — |
| 40 / 41 | 手写识别 / T9 拼音输入法 | ATKNCR 库 | LCD + 触摸 |
| 42 | 串口 IAP | Bootloader + APP（Flash / SRAM 两种） | PA9/PA10 |
| 43 / 44 | USB 读卡器 / USB 虚拟串口 | 原生 USB | PA11/PA12 |
| 45~47 | UCOSII 任务调度 / 信号量邮箱 / 消息队列 | RTOS | — |
| **48** | **综合测试** | **33 个 BSP 模块全用上，最佳模板** | — |

**扩展例程**：uC-OS3 21 个（已解压）、FreeRTOS 25 个、emWin 42 个、LVGL 42 个。

---

## 8. 故障速查

| 现象 | 十有八九是 | 怎么办 |
|---|---|---|
| 串口助手认不到端口 | Type-C 线是"只充电不传数据" | 换线；正确应出现 **CH340** 串口 |
| 串口能开但收不到打印 | P3 跳线帽没插 / 波特率不对 | 插上 P3；波特率 **115200** |
| 下载失败 / 找不到芯片 | PA13/PA14 被配成了普通 IO | 把 BOOT0 拉高走 **ISP** 救；或用 ST-Link 的 "connect under reset" |
| 屏白屏 / 花屏 | LCD 排座没插到底 / FSMC 初始化被打断 | 重插排座；用官方 `lcd.c`，别自己写 NT35310 序列 |
| 触摸坐标全乱 | 电阻屏没校准 | 按 KEY0 进校准，结果存板载 24C02 |
| ADC 读数一直是 4095 或 0 | 没分压（5V 打满）/ 通道号或 GPIO 宏没改 | 改 `adc.h` 的 `ADC_ADCX_CHY` 系列宏；5V 信号先 10k/20k 分压 |
| ADC 读数乱跳 | 采样时间太短 / 没加滤波 / 参考电压不稳 | `stime=7`（239.5 周期）；`adc_get_result_average(ch, 20)`；加 100nF |
| 5V 外设一开板子就重启 | 从板子排针取电超了 1000mA | 外设单独供电，只共地 |
| 4G/WiFi 模块一发射就死机 | 从 3.3V 取电，峰值拉崩 LDO | 独立 4.0V buck + 1000µF |
| RS485 收不到数据 | 没拔 P5 跳线 / 波特率 / A B 接反 | 拔 P5；A/B 对调试试 |
| 用 PA15/PB3/PB4 当普通 IO 没反应 | JTAG 没禁 | `sys_gpio_remap_set(24, 3, 2)`（保留 SWD） |
| 断电后参数丢了 | 存在 RAM 里 | 存 24C02（`at24cxx_*`）或内部 Flash（`stmflash_*`） |
| 新版 MDK 打不开老工程 | 器件包没装 | 装 Keil.STM32F1xx_DFP |

---

## 9. 开工检查清单

- [ ] 确认用的是 **SWD** 而不是 JTAG（PA13/PA14 不要乱动）
- [ ] 改 ADC 前先看 `adc.h` / `adc3.h` 的 `ADCx_CHY` 与 GPIO 宏，改完再编译
- [ ] 用 USART2(PA2/PA3) 前**拔 P5 跳线帽**
- [ ] 5V 模拟信号先分压；3.3V 逻辑与 5V 逻辑之间加电平转换
- [ ] 继电器/电磁阀一律三极管 + 续流二极管 + 光耦隔离，**不直接挂 GPIO**
- [ ] 外设总电流先算一遍：5V ≤1000mA、3.3V ≤1000mA（含屏背光 120mA）
- [ ] 上电前用万用表量一遍电源对地电阻，防短路

---

## 10. 关联文件

| 文件 | 内容 |
|---|---|
| `D:\正点原子\STM32F103ZET6精英版V2_开发实例与IO资源总报告.md` | 完整报告：112 脚全表、AF 全表、全部例程明细、可扩展功能矩阵、官方文档错误记录 |
| `D:\正点原子\元器件采购清单_STM32F103ZET6环境监测与燃气报警_V1.9.xlsx` | 该板上的项目物料清单 |
| `D:\正点原子\元器件选型报告_STM32F103ZET6环境监测与燃气报警.md` | 选型推理过程 |
| `D:\正点原子\【正点原子】精英STM32F103开发板 V2-资料盘(A盘)(1)\3，原理图.zip` | 原理图 + IO 分配表（**一切引脚问题的最终依据**） |
| `D:\正点原子\...\精英V2 硬件参考手册_V1.0.pdf` | §1.1.4 资源列表 / §2.1 逐项说明 / §2.2 使用注意 |
| `D:\正点原子\...\4，程序源码\...\实验48 综合测试实验` | 33 个 BSP 模块的完整用法模板 |
