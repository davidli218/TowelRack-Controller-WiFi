#pragma once

/* 数码管引脚定义 */
#define BSP_DISP_PIN_1 (GPIO_NUM_4)
#define BSP_DISP_PIN_2 (GPIO_NUM_5)
#define BSP_DISP_PIN_3 (GPIO_NUM_6)
#define BSP_DISP_PIN_4 (GPIO_NUM_7)
#define BSP_DISP_PIN_5 (GPIO_NUM_18)
#define BSP_DISP_PIN_6 (GPIO_NUM_19)

#define BSP_DISP_MAX_CHAR (3)

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
 * @brief 暂停数码管显示
 */
void bsp_display_pause(void);

/**
 * @brief 恢复数码管显示
 */
void bsp_display_resume(void);
