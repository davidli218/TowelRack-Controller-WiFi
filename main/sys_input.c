#include "sys_input.h"

__unused static const char *TAG = "TRC-W::SysIn";

static void button_single_click_cb(void *arg, void *usr_data) { ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK"); }

static void button_double_click_cb(void *arg, void *usr_data) { ESP_LOGI(TAG, "BUTTON_DOUBLE_CLICK"); }

static void button_long_press_cb(void *arg, void *usr_data) { ESP_LOGI(TAG, "BUTTON_LONG_PRESS"); }

static void button_press_repeat_cb(void *arg, void *usr_data) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    esp_restart();
}

static void knob_left_cb(void *arg, void *data) { ESP_LOGI(TAG, "[Knob] Left"); }

static void knob_right_cb(void *arg, void *data) { ESP_LOGI(TAG, "[Knob] Right"); }

static void system_knob_init(void) {
    knob_config_t knob_config = {
        .default_direction = 0,
        .gpio_encoder_a = KNOB_PIN_A,
        .gpio_encoder_b = KNOB_PIN_B,
    };

    knob_handle_t knob_handle = iot_knob_create(&knob_config);

    iot_knob_register_cb(knob_handle, KNOB_LEFT, knob_left_cb, NULL);
    iot_knob_register_cb(knob_handle, KNOB_RIGHT, knob_right_cb, NULL);
}

static void system_button_init(void) {
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
    iot_button_register_cb(btn_handle, BUTTON_PRESS_REPEAT, button_press_repeat_cb, NULL);
}

void system_input_init(void) {
    system_button_init();
    system_knob_init();
}
