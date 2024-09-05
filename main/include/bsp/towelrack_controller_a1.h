#pragma once

#include <stdbool.h>

/**************************************************************************************************
 * TowelRack-Controller-WiFi-A1 Pinout
 **************************************************************************************************/

/* 74HC595 IC & 7-Segment Display */
#define BSP_DISP_IC_DS (GPIO_NUM_10)
#define BSP_DISP_IC_SHCP (GPIO_NUM_18)
#define BSP_DISP_IC_STCP (GPIO_NUM_19)
#define BSP_DISP_U1_CTRL (GPIO_NUM_7)
#define BSP_DISP_U2_CTRL (GPIO_NUM_6)

/* Button & Knob */
#define BSP_TOUCH_BUTTON_LEFT (GPIO_NUM_4)
#define BSP_TOUCH_BUTTON_RIGHT (GPIO_NUM_5)
#define BSP_KNOB_ENCODER_A (GPIO_NUM_2)
#define BSP_KNOB_ENCODER_B (GPIO_NUM_3)
#define BSP_KNOB_BUTTON (GPIO_NUM_9)

/* SK6812 LED Strip */
#define BSP_LED_STRIP_GPIO (GPIO_NUM_8)


/**************************************************************************************************
 *
 * 74HC595 IC & 7-Segment Display
 *
 * TowelRack-Controller-WiFi-A1 使用74HC595芯片驱动数码管显示
 *
 * 74HC595引脚定义:
 *                                     ----------------
 *                  #(数码管A段)  <--  | Q1         VCC |  -->  VCC_3V3
 *                  #(数码管B段)  <--  | Q2          Q0 |  -->  #(C/H指示灯)
 *                  #(数码管C段)  <--  | Q3          DS |  -->  @[[BSP_DISP_IC_DS]]
 *                  #(数码管D段)  <--  | Q4         OE# |  -->  GND
 *                  #(数码管E段)  <--  | Q5        STCP |  -->  @[[BSP_DISP_IC_STCP]]
 *                  #(数码管F段)  <--  | Q6        SHCP |  -->  @[[BSP_DISP_IC_SHCP]]
 *                  #(数码管G段)  <--  | Q7         MR# |  -->  VCC_3V3
 *                          GND  <--  | GND        Q7S |  -->  |
 *                                     ----------------
 * 7-Segment Display引脚定义:
 *                                         ---------
 *            @[[BSP_DISP_U2_CTRL]]  <--  | 2nd LSB |  -->  GND
 *            @[[BSP_DISP_U1_CTRL]]  <--  | 1st LSB |  -->  GND
 *                                         ---------
 *
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

char* bsp_input_event_to_string(bsp_input_event_t event);


/**************************************************************************************************
 *
 * SK6812 LED Strip
 *
 * TowelRack-Controller-WiFi-A1 使用SK6812 RGBW LED灯带
 **************************************************************************************************/

#define BSP_LED_STRIP_NUM (4)
#define BSP_LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

typedef enum {
    BSP_STRIP_OFF = 0,
    BSP_STRIP_ORANGE,
    BSP_STRIP_GREEN,
    BSP_STRIP_BLUE,
    BSP_STRIP_RED,
} bsp_led_strip_mode_t;

void bsp_led_strip_init(void);

void bsp_led_strip_write(bsp_led_strip_mode_t mode);
