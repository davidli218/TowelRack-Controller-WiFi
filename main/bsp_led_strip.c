#include <stdlib.h>

#include "led_strip.h"

#include "bsp/towelrack_controller_a1.h"

__unused static const char* TAG = "bsp_led_strip";

static led_strip_handle_t led_strip = NULL;

void bsp_led_strip_write(const bsp_led_strip_mode_t mode) {
    switch (mode) {
        case BSP_STRIP_OFF:
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            break;
        case BSP_STRIP_ORANGE:
            for (int i = 0; i < BSP_LED_STRIP_NUM; i++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255, 76, 10));
            }
            break;
        case BSP_STRIP_RED:
            for (int i = 0; i < BSP_LED_STRIP_NUM; i++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255, 0, 0));
            }
            break;
        case BSP_STRIP_GREEN:
            for (int i = 0; i < BSP_LED_STRIP_NUM; i++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 255, 0));
            }
            break;
        case BSP_STRIP_BLUE:
            for (int i = 0; i < BSP_LED_STRIP_NUM; i++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 0, 255));
            }
            break;
        default:
            break;
    }

    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void bsp_led_strip_init(void) {
    const led_strip_config_t strip_config = {
        .strip_gpio_num = BSP_LED_STRIP_GPIO,
        .max_leds = BSP_LED_STRIP_NUM,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_SK6812,
        .flags.invert_out = false,
    };

    const led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = BSP_LED_STRIP_RMT_RES_HZ,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
}
