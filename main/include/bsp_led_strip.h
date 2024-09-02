#pragma once

#define BSP_LED_STRIP_GPIO (GPIO_NUM_8)
#define BSP_LED_STRIP_NUM (4)

#define BSP_LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

/**
 * @brief Initialize LED strip.
 *
 */
void bsp_led_strip_init(void);

void bsp_led_on(void);

void bsp_led_off(void);
