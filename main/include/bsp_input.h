#pragma once

#define BSP_KNOB_ENCODER_A (GPIO_NUM_2)
#define BSP_KNOB_ENCODER_B (GPIO_NUM_3)
#define BSP_KNOB_BUTTON (GPIO_NUM_1)

#define BSP_TOUCH_BUTTON_LEFT (GPIO_NUM_4)
#define BSP_TOUCH_BUTTON_RIGHT (GPIO_NUM_5)

typedef enum {
    BSP_KNOB_BUTTON_LONG_PRESS,
    BSP_KNOB_BUTTON_PRESS_REPEAT,
    BSP_KNOB_ENCODER_ACW,
    BSP_KNOB_ENCODER_CW,
    BSP_TOUCH_BUTTON_LEFT_PRESS,
    BSP_TOUCH_BUTTON_RIGHT_PRESS,
} bsp_input_event_t;

void bsp_input_init(void);

char *bsp_input_event_to_string(bsp_input_event_t event);
