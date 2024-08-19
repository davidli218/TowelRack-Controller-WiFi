#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "iot_button.h"
#include "iot_knob.h"

#include "bsp_input.h"

__unused static const char *TAG = "bsp_input";

QueueHandle_t bsp_input_queue = NULL;

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

/* 输入事件回调函数 */

static void knob_button_single_click_cb(void *arg, void *usr_data) {
    const bsp_input_event_t event = BSP_KNOB_BUTTON_SINGLE_CLICK;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void knob_button_double_click_cb(void *arg, void *usr_data) {
    const bsp_input_event_t event = BSP_KNOB_BUTTON_DOUBLE_CLICK;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void knob_button_long_press_cb(void *arg, void *usr_data) {
    const bsp_input_event_t event = BSP_KNOB_BUTTON_LONG_PRESS;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void knob_button_press_repeat_cb(void *arg, void *usr_data) {
    const bsp_input_event_t event = BSP_KNOB_BUTTON_PRESS_REPEAT;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void knob_encoder_left_cb(void *arg, void *data) {
    const bsp_input_event_t event = BSP_KNOB_ENCODER_ACW;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

static void knob_encoder_right_cb(void *arg, void *data) {
    const bsp_input_event_t event = BSP_KNOB_ENCODER_CW;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

/* 初始化输入设备 */

static void bsp_knob_init(void) {
    knob_handle_t knob_handle = iot_knob_create(&bsp_knob_encoder_a_b_config);
    iot_knob_register_cb(knob_handle, KNOB_LEFT, knob_encoder_left_cb, NULL);
    iot_knob_register_cb(knob_handle, KNOB_RIGHT, knob_encoder_right_cb, NULL);
}

static void bsp_button_init(void) {
    button_handle_t btn_handle = iot_button_create(&bsp_knob_btn_config);
    iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, knob_button_single_click_cb, NULL);
    iot_button_register_cb(btn_handle, BUTTON_DOUBLE_CLICK, knob_button_double_click_cb, NULL);
    iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_START, knob_button_long_press_cb, NULL);
    iot_button_register_cb(btn_handle, BUTTON_PRESS_REPEAT, knob_button_press_repeat_cb, NULL);
}

void bsp_input_init(void) {
    bsp_knob_init();
    bsp_button_init();

    bsp_input_queue = xQueueCreate(10, sizeof(bsp_input_event_t));
}
