#pragma once

/* 数码管引脚定义 */
#define BSP_DISP_IC_DS (GPIO_NUM_10)
#define BSP_DISP_IC_SHCP (GPIO_NUM_18)
#define BSP_DISP_IC_STCP (GPIO_NUM_19)

#define BSP_DISP_U1_CTRL (GPIO_NUM_7)
#define BSP_DISP_U2_CTRL (GPIO_NUM_6)

#define BSP_DISP_MAX_CHAR (2)

/**
 * @brief 初始化数码管
 */
void bsp_display_init(void);

/**
 * @brief 设置数码管显示字符串
 *
 * @param str 要显示的字符串
 */
void bsp_display_set_string(const char *str);

/**
 * @brief 设置数码管显示整数
 *
 * @param num 要显示的整数
 */
void bsp_display_set_int(int num);

/**
 * @brief 设置数码管是否显示C标志
 */
void bsp_display_set_c_flag(bool flag);

/**
 * @brief 设置数码管是否显示H标志
 */
void bsp_display_set_h_flag(bool flag);

/**
 * @brief 暂停数码管显示
 */
void bsp_display_pause(void);

/**
 * @brief 恢复数码管显示
 */
void bsp_display_resume(void);
