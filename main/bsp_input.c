#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "iot_button.h"
#include "iot_knob.h"

#include "bsp_input.h"

__unused static const char *TAG = "bsp_input";

QueueHandle_t bsp_input_queue = NULL;

/* ------------------------ 输入设备配置 ------------------------ */

static const knob_config_t bsp_knob_encoder_a_b_config = {
    .default_direction = 0,
    .gpio_encoder_a = BSP_KNOB_ENCODER_A,
    .gpio_encoder_b = BSP_KNOB_ENCODER_B,
};

static const button_config_t bsp_knob_btn_config = {
    .type = BUTTON_TYPE_GPIO,
    .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config =
        {
            .gpio_num = BSP_KNOB_BUTTON,
            .active_level = 0,
        },
};

static const button_config_t bsp_touch_button_left_config = {
    .type = BUTTON_TYPE_GPIO,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config =
        {
            .gpio_num = BSP_TOUCH_BUTTON_LEFT,
            .active_level = 1,
        },
};

static const button_config_t bsp_touch_button_right_config = {
    .type = BUTTON_TYPE_GPIO,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config =
        {
            .gpio_num = BSP_TOUCH_BUTTON_RIGHT,
            .active_level = 1,
        },
};

static const button_event_config_t button_multi_click_8_event = {
    .event = BUTTON_MULTIPLE_CLICK,
    .event_data = {.multiple_clicks = {.clicks = 8}},
};

/* ------------------------ 输入设备中断回调函数 ------------------------ */

static void knob_encoder_left_cb(void *arg, void *data) {
    const bsp_input_event_t event = BSP_KNOB_ENCODER_ACW;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void knob_encoder_right_cb(void *arg, void *data) {
    const bsp_input_event_t event = BSP_KNOB_ENCODER_CW;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void knob_button_long_press_cb(void *arg, void *usr_data) {
    const bsp_input_event_t event = BSP_KNOB_BUTTON_LONG_PRESS;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void knob_button_multiple_click_8_cb(void *arg, void *usr_data) {
    const bsp_input_event_t event = BSP_KNOB_BUTTON_MULTIPLE_CLICK_8;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void touch_button_left_click_cb(void *arg, void *data) {
    const bsp_input_event_t event = BSP_TOUCH_BUTTON_LEFT_CLICK;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void touch_button_right_click_cb(void *arg, void *data) {
    const bsp_input_event_t event = BSP_TOUCH_BUTTON_RIGHT_CLICK;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

/* ------------------------ 公开暴露函数 ------------------------ */

void bsp_input_init(void) {
    /* 初始化旋钮编码器 */
    knob_handle_t knob_handle = iot_knob_create(&bsp_knob_encoder_a_b_config);
    iot_knob_register_cb(knob_handle, KNOB_LEFT, knob_encoder_left_cb, NULL);
    iot_knob_register_cb(knob_handle, KNOB_RIGHT, knob_encoder_right_cb, NULL);

    /* 初始化旋钮按钮 */
    button_handle_t knob_btn_handle = iot_button_create(&bsp_knob_btn_config);
    iot_button_register_cb(knob_btn_handle, BUTTON_LONG_PRESS_START, knob_button_long_press_cb, NULL);
    iot_button_register_event_cb(knob_btn_handle, button_multi_click_8_event, knob_button_multiple_click_8_cb, NULL);

    /* 初始化触摸按键 */
    button_handle_t touch_btn_left_handle = iot_button_create(&bsp_touch_button_left_config);
    button_handle_t touch_btn_right_handle = iot_button_create(&bsp_touch_button_right_config);
    iot_button_register_cb(touch_btn_left_handle, BUTTON_SINGLE_CLICK, touch_button_left_click_cb, NULL);
    iot_button_register_cb(touch_btn_right_handle, BUTTON_SINGLE_CLICK, touch_button_right_click_cb, NULL);

    /* 创建输入设备输入事件队列 */
    bsp_input_queue = xQueueCreate(10, sizeof(bsp_input_event_t));
}

char *bsp_input_event_to_string(bsp_input_event_t event) {
    switch (event) {
        case BSP_KNOB_ENCODER_ACW: return "BSP_KNOB_ENCODER_ACW";
        case BSP_KNOB_ENCODER_CW: return "BSP_KNOB_ENCODER_CW";
        case BSP_KNOB_BUTTON_LONG_PRESS: return "BSP_KNOB_BUTTON_LONG_PRESS";
        case BSP_KNOB_BUTTON_MULTIPLE_CLICK_8: return "BSP_KNOB_BUTTON_MULTIPLE_CLICK_8";
        case BSP_TOUCH_BUTTON_LEFT_CLICK: return "BSP_TOUCH_BUTTON_LEFT_CLICK";
        case BSP_TOUCH_BUTTON_RIGHT_CLICK: return "BSP_TOUCH_BUTTON_RIGHT_CLICK";
        default: return "BSP_UNKNOWN_INPUT_EVENT";
    }
}
