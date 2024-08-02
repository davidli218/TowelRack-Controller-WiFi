#include "knob.h"

static const char *TAG = "TRC-W::KNOB";

static void knob_left_cb(void *arg, void *data) { ESP_LOGI(TAG, "[Knob] Left"); }

static void knob_right_cb(void *arg, void *data) { ESP_LOGI(TAG, "[Knob] Right"); }

void system_knob_init(void) {
    knob_config_t knob_config = {
        .default_direction = 0,
        .gpio_encoder_a = KNOB_PIN_A,
        .gpio_encoder_b = KNOB_PIN_B,
    };

    knob_handle_t knob_handle = iot_knob_create(&knob_config);

    iot_knob_register_cb(knob_handle, KNOB_LEFT, knob_left_cb, NULL);
    iot_knob_register_cb(knob_handle, KNOB_RIGHT, knob_right_cb, NULL);
}
