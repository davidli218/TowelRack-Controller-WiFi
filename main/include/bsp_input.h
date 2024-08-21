#pragma once

#define BSP_KNOB_ENCODER_A (GPIO_NUM_2)
#define BSP_KNOB_ENCODER_B (GPIO_NUM_3)
#define BSP_KNOB_BUTTON (GPIO_NUM_1)

#define BSP_TOUCH_BUTTON_LEFT (GPIO_NUM_4)
#define BSP_TOUCH_BUTTON_RIGHT (GPIO_NUM_5)

typedef enum {
    BSP_KNOB_ENCODER_ACW,             // 旋钮逆时针旋转
    BSP_KNOB_ENCODER_CW,              // 旋钮顺时针旋转
    BSP_KNOB_BUTTON_LONG_PRESS,       // 旋钮长按
    BSP_KNOB_BUTTON_MULTIPLE_CLICK_8, // 旋钮连续点击 8 次
    BSP_TOUCH_BUTTON_LEFT_CLICK,      // 左触摸按键点击
    BSP_TOUCH_BUTTON_RIGHT_CLICK,     // 右触摸按键点击
} bsp_input_event_t;

void bsp_input_init(void);

char *bsp_input_event_to_string(bsp_input_event_t event);
