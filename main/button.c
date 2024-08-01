#include "button.h"

static const char *TAG = "TRC-W::BTN";

static void button_single_click_cb(void *arg, void *usr_data) { ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK"); }

static void button_double_click_cb(void *arg, void *usr_data) { ESP_LOGI(TAG, "BUTTON_DOUBLE_CLICK"); }

static void button_long_press_cb(void *arg, void *usr_data) { ESP_LOGI(TAG, "BUTTON_LONG_PRESS"); }

void system_button_init(void) {
    /* 创建按钮 */
    button_config_t gpio_btn_config = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config =
            {
                .gpio_num = BUTTON_PIN,
                .active_level = 0,
            },
    };
    button_handle_t btn_handle = iot_button_create(&gpio_btn_config);

    /* 注册按钮事件回调函数 */
    iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
    iot_button_register_cb(btn_handle, BUTTON_DOUBLE_CLICK, button_double_click_cb, NULL);
    iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_START, button_long_press_cb, NULL);
}
