#pragma once

#define BSP_KNOB_ENCODER_A (GPIO_NUM_0)
#define BSP_KNOB_ENCODER_B (GPIO_NUM_1)
#define BSP_KNOB_BUTTON (GPIO_NUM_10)

typedef enum {
    BSP_KNOB_BUTTON_SINGLE_CLICK = 0,
    BSP_KNOB_BUTTON_DOUBLE_CLICK,
    BSP_KNOB_BUTTON_LONG_PRESS,
    BSP_KNOB_BUTTON_PRESS_REPEAT,
    BSP_KNOB_ENCODER_ACW,
    BSP_KNOB_ENCODER_CW,
} bsp_input_event_t;

void bsp_input_init(void);

char *bsp_input_event_to_string(bsp_input_event_t event);
