#pragma once

#include "driver/gpio.h"
#include "led_indicator.h"

#define BSP_LED_STRIP_GPIO (GPIO_NUM_8)
#define BSP_LED_STRIP_NUM (4)

#define BSP_LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

/**
 * @brief Define blinking type and priority.
 *
 */
enum {
    BLINK_RED_BREATHE = 0,
    BLINK_GREEN_BREATHE,
    BLINK_ORANGE,
    BLINK_BLUE,
    BLINK_FLOWING,
    BLINK_MAX,
};

/**
 * @brief Initialize LED strip.
 *
 */
void bsp_led_strip_init(void);
