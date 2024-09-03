#pragma once

#define BSP_LED_STRIP_GPIO (GPIO_NUM_8)
#define BSP_LED_STRIP_NUM (4)

#define BSP_LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

typedef enum {
    BSP_STRIP_OFF = 0,
    BSP_STRIP_ORANGE,
    BSP_STRIP_GREEN,
    BSP_STRIP_BLUE,
    BSP_STRIP_RED,
} bsp_led_strip_mode_t;

/**
 * @brief Initialize LED strip.
 *
 */
void bsp_led_strip_init(void);

void bsp_led_strip_write(bsp_led_strip_mode_t mode);
