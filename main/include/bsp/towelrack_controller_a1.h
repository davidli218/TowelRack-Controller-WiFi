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
