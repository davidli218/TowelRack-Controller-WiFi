#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "iot_button.h"
#include "iot_knob.h"

#include "bsp_input.h"

__unused static const char* TAG = "bsp_input";

QueueHandle_t bsp_input_queue = NULL;

/* ------------------------ 输入设备配置 ------------------------ */

static const button_config_t config_touch_button_left = {
    .type = BUTTON_TYPE_GPIO,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config = {
        .gpio_num = BSP_TOUCH_BUTTON_LEFT,
        .active_level = 1,
    },
};

static const button_config_t config_touch_button_right = {
    .type = BUTTON_TYPE_GPIO,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config = {
        .gpio_num = BSP_TOUCH_BUTTON_RIGHT,
        .active_level = 1,
    },
};

static const knob_config_t config_knob_encoder_a_b = {
    .default_direction = 0,
    .gpio_encoder_a = BSP_KNOB_ENCODER_A,
    .gpio_encoder_b = BSP_KNOB_ENCODER_B,
};

static const button_config_t config_knob_btn = {
    .type = BUTTON_TYPE_GPIO,
    .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config = {
        .gpio_num = BSP_KNOB_BUTTON,
        .active_level = 0,
    },
};

static const button_event_config_t btn_mt8_click_config = {
    .event = BUTTON_MULTIPLE_CLICK,
    .event_data = {
        .multiple_clicks = {.clicks = 8},
    },
};

/* ------------------------ 输入设备中断回调函数 ------------------------ */

static void bsp_input_event_cb(void* arg, void* data) {
    bsp_input_event_t event = (uintptr_t)arg;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

/* ------------------------ 公开暴露函数 ------------------------ */

void bsp_input_init(void) {
    /* 初始化旋钮编码器 */
    knob_handle_t kb_ec_handle = iot_knob_create(&config_knob_encoder_a_b);
    iot_knob_register_cb(kb_ec_handle, KNOB_LEFT, bsp_input_event_cb, (void*)BSP_KNOB_ENCODER_ACW);
    iot_knob_register_cb(kb_ec_handle, KNOB_RIGHT, bsp_input_event_cb, (void*)BSP_KNOB_ENCODER_CW);

    /* 初始化旋钮按钮 */
    button_handle_t kb_btn_handle = iot_button_create(&config_knob_btn);
    iot_button_register_cb(kb_btn_handle, BUTTON_LONG_PRESS_START, bsp_input_event_cb, (void*)BSP_KNOB_LONG_PRESS);
    iot_button_register_event_cb(kb_btn_handle, btn_mt8_click_config, bsp_input_event_cb, (void*)BSP_KNOB_MT8_CLICK);

    /* 初始化触摸按键 */
    button_handle_t tc_btn_l_handle = iot_button_create(&config_touch_button_left);
    button_handle_t tc_btn_r_handle = iot_button_create(&config_touch_button_right);
    iot_button_register_cb(tc_btn_l_handle, BUTTON_SINGLE_CLICK, bsp_input_event_cb, (void*)BSP_TOUCH_BUTTON_L_CLICK);
    iot_button_register_cb(tc_btn_r_handle, BUTTON_SINGLE_CLICK, bsp_input_event_cb, (void*)BSP_TOUCH_BUTTON_R_CLICK);

    /* 创建输入设备输入事件队列 */
    bsp_input_queue = xQueueCreate(10, sizeof(bsp_input_event_t));
}

char* bsp_input_event_to_string(bsp_input_event_t event) {
    switch (event) {
        case BSP_KNOB_ENCODER_ACW: return "BSP_KNOB_ENCODER_ACW";
        case BSP_KNOB_ENCODER_CW: return "BSP_KNOB_ENCODER_CW";
        case BSP_KNOB_LONG_PRESS: return "BSP_KNOB_LONG_PRESS";
        case BSP_KNOB_MT8_CLICK: return "BSP_KNOB_MT8_CLICK";
        case BSP_TOUCH_BUTTON_L_CLICK: return "BSP_TOUCH_BUTTON_L_CLICK";
        case BSP_TOUCH_BUTTON_R_CLICK: return "BSP_TOUCH_BUTTON_R_CLICK";
        default:
            return "BSP_UNKNOWN_INPUT_EVENT";
    }
}
