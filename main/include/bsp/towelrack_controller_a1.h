#pragma once

#include <stdbool.h>

#include "freertos/FreeRTOS.h"

/**************************************************************************************************
 * TowelRack-Controller-WiFi-A1 Pinout
 **************************************************************************************************/

/* 74HC595 IC & 7-Segment Display */
#define BSP_P_74HC595_DS (GPIO_NUM_10)
#define BSP_P_74HC595_SHCP (GPIO_NUM_18)
#define BSP_P_74HC595_STCP (GPIO_NUM_19)
#define BSP_P_DISP_S1_SW (GPIO_NUM_7)
#define BSP_P_DISP_S2_SW CONFIG_BSP_P_DISP_S2_SW

/* Button & Knob */
#define BSP_P_TOUCH_BUTTON_L CONFIG_BSP_P_TOUCH_BUTTON_L
#define BSP_P_TOUCH_BUTTON_R CONFIG_BSP_P_TOUCH_BUTTON_R
#define BSP_P_KNOB_ENCODER_A CONFIG_BSP_P_KNOB_ENCODER_A
#define BSP_P_KNOB_ENCODER_B CONFIG_BSP_P_KNOB_ENCODER_B
#define BSP_P_KNOB_BUTTON (GPIO_NUM_9)

/* SK6812 LED Strip */
#define BSP_P_LED_STRIP (GPIO_NUM_8)

/* NTC Temperature Sensor & Heating Control */
#define BSP_P_NTC_ADC_CHANNEL (ADC_CHANNEL_0)
#define BSP_P_NTC_ADC_UNIT (ADC_UNIT_1)
#define BSP_P_HEATING_CTRL CONFIG_BSP_P_HEATING_CTRL


/**************************************************************************************************
 *
 * 74HC595 IC & 7-Segment Display
 *
 * TowelRack-Controller-WiFi-A1 使用74HC595芯片驱动数码管显示
 **************************************************************************************************/

/**
 * @brief 初始化数码管
 */
void bsp_display_init(void);

/**
 * @brief 设置数码管显示字符串, 并立刻显示
 *
 * @param str 要显示的字符串
 */
void bsp_display_write_str(const char* str);

/**
 * @brief 设置数码管显示整数, 并立刻显示
 *
 * @param num 要显示的整数
 */
void bsp_display_write_int(int num);

/**
 * @brief 设置数码管是否显示C标志, 不会立刻显示
 */
void bsp_display_set_c_flag(bool flag);

/**
 * @brief 设置数码管是否显示H标志, 不会立刻显示
 */
void bsp_display_set_h_flag(bool flag);


/**************************************************************************************************
 *
 * Button & Knob
 *
 * TowelRack-Controller-WiFi-A1 拥有两个触摸按键，一个旋钮和一个按钮
 **************************************************************************************************/

typedef enum {
    BSP_KNOB_ENCODER_ACW,     // 旋钮逆时针旋转
    BSP_KNOB_ENCODER_CW,      // 旋钮顺时针旋转
    BSP_KNOB_LONG_PRESS,      // 旋钮长按
    BSP_KNOB_MT8_CLICK,       // 旋钮连续点击 8 次
    BSP_TOUCH_BUTTON_L_CLICK, // 左触摸按键点击
    BSP_TOUCH_BUTTON_R_CLICK, // 右触摸按键点击
} bsp_input_event_t;

void bsp_input_init(void);

QueueHandle_t bsp_input_get_queue(void);

char* bsp_input_event_to_string(bsp_input_event_t event);


/**************************************************************************************************
 *
 * SK6812 LED Strip
 *
 * TowelRack-Controller-WiFi-A1 使用SK6812 RGBW LED灯带
 **************************************************************************************************/

typedef enum {
    BSP_STRIP_OFF = 0,
    BSP_STRIP_WHITE,
    BSP_STRIP_ORANGE,
    BSP_STRIP_GREEN,
    BSP_STRIP_BLUE,
    BSP_STRIP_RED,
} bsp_led_strip_mode_t;

void bsp_led_strip_init(void);

void bsp_led_strip_write(bsp_led_strip_mode_t mode);


/**************************************************************************************************
 *
 * NTC Temperature Sensor + Heating Control
 *
 * TowelRack-Controller-WiFi-A1 使用NTC热敏电阻传感器测量温度, 并使用可控硅控制加热
 **************************************************************************************************/

void bsp_heating_init(void);

int bsp_heating_get_temp(void);

void bsp_heating_enable(void);

void bsp_heating_disable(void);


/**************************************************************************************************
 *
 * Initialize All Peripherals
 *
 * 初始化所有外设
 **************************************************************************************************/
void bsp_init_all(void);
